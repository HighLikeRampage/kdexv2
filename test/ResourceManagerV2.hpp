#pragma once
// ResourceManagerV2.hpp — fixed resource start/stop via QueueUserAPC
//
// ROOT CAUSE of original crash:
//   CreateRemoteThread creates a raw OS thread with no FiveM fiber/TLS context.
//   Resource vtable methods access the FiveM fiber scheduler and Lua runtime
//   via thread-local storage that doesn't exist on a raw OS thread → crash.
//
// FIX: Queue an APC to an existing FiveM scripting thread.  The APC fires when
// that thread enters an alertable wait (which FiveM does regularly in its yield
// loop).  The vtable call now runs on a real FiveM thread with correct context.
//
// INCLUDE: copy this file into kdex/game/Core/Features/Exploits/ and include from
// gui_menu.cpp using the project include path.

#include <Core/SDK/Memory.hpp>
#include <Core/SDK/DebugLog.hpp>
#include <Security/xorstr.hpp>
#include <Core/Features/Exploits/ResourceList.hpp>

#include <Windows.h>
#include <tlhelp32.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace ResourceV2 {

    using namespace Core;

    struct ApcPayload {
        uint64_t resource_ptr;
        uint32_t vtable_slot;
        uint32_t done;
        uint64_t result;
    };

    // x64 APC shellcode: rcx = &ApcPayload (in remote process)
    // Calls resource_ptr->vtable[vtable_slot]() and sets done=1.
    inline std::vector<uint8_t> BuildApcShellcode() {
        return {
            // sub rsp, 0x28
            0x48, 0x83, 0xEC, 0x28,
            // mov r10, rcx                ; r10 = &payload
            0x49, 0x89, 0xCA,
            // mov rcx, [r10+0x00]         ; rcx = resource_ptr ("this")
            0x49, 0x8B, 0x0A,
            // test rcx, rcx
            0x48, 0x85, 0xC9,
            // jz done
            0x74, 0x22,
            // mov eax, [r10+0x08]         ; eax = vtable_slot
            0x41, 0x8B, 0x42, 0x08,
            // shl eax, 3                  ; eax = slot * 8 (byte offset)
            0xC1, 0xE0, 0x03,
            // movsxd rax, eax             ; sign-extend to 64-bit
            0x48, 0x63, 0xC0,
            // mov rdx, [rcx]              ; rdx = vtable ptr
            0x48, 0x8B, 0x11,
            // add rdx, rax               ; rdx = &vtable[slot]
            0x48, 0x03, 0xD0,
            // mov rax, [rdx]              ; rax = fn ptr
            0x48, 0x8B, 0x02,
            // call rax
            0xFF, 0xD0,
            // mov [r10+0x10], rax         ; result
            0x49, 0x89, 0x42, 0x10,
            // done: mov dword [r10+0x0C], 1
            0x41, 0xC7, 0x42, 0x0C, 0x01, 0x00, 0x00, 0x00,
            // add rsp, 0x28
            0x48, 0x83, 0xC4, 0x28,
            // ret
            0xC3,
        };
    }

    inline std::vector<DWORD> EnumProcessThreads(DWORD pid) {
        std::vector<DWORD> out;
        HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE) return out;
        THREADENTRY32 te{ sizeof(te) };
        if (::Thread32First(snap, &te)) {
            do {
                if (te.th32OwnerProcessID == pid)
                    out.push_back(te.th32ThreadID);
            } while (::Thread32Next(snap, &te));
        }
        ::CloseHandle(snap);
        return out;
    }

    class cResourceManagerV2 {
    public:
        bool Init() {
            if (m_ready) return true;
            if (!Mem.ProcHandle) return false;

            m_shellcode = BuildApcShellcode();

            m_codeVA = reinterpret_cast<uintptr_t>(
                ::VirtualAllocEx(Mem.ProcHandle, nullptr,
                    m_shellcode.size() + 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            if (!m_codeVA) return false;

            if (!Mem.WriteRaw(m_codeVA, m_shellcode.data(), m_shellcode.size())) {
                Cleanup(); return false;
            }

            m_payloadVA = reinterpret_cast<uintptr_t>(
                ::VirtualAllocEx(Mem.ProcHandle, nullptr,
                    sizeof(ApcPayload) + 64, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            if (!m_payloadVA) { Cleanup(); return false; }

            m_ready = true;
            return true;
        }

        void Cleanup() {
            if (Mem.ProcHandle) {
                if (m_codeVA)    ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_codeVA),    0, MEM_RELEASE);
                if (m_payloadVA) ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_payloadVA), 0, MEM_RELEASE);
            }
            m_codeVA = m_payloadVA = 0;
            m_ready = false;
        }

        void LogVtable(uintptr_t resource_ptr, const std::string& name) {
            uintptr_t vtable = Mem.Read<uintptr_t>(resource_ptr);
            DebugLog(xorstr("[RMv2] vtable for '%s' @ 0x%llX\n"), name.c_str(), (unsigned long long)vtable);
            for (int s = 0; s < 16; ++s) {
                uintptr_t fn = Mem.Read<uintptr_t>(vtable + s * 8);
                DebugLog(xorstr("[RMv2]   slot %2d -> 0x%llX\n"), s, (unsigned long long)fn);
            }
        }

        bool CallSlot(uintptr_t resource_ptr, uint32_t slot, int timeoutMs = 3000) {
            if (!m_ready && !Init()) return false;
            if (!resource_ptr || !Mem.ProcHandle) return false;

            std::lock_guard<std::mutex> lk(m_mtx);

            ApcPayload payload{};
            payload.resource_ptr = resource_ptr;
            payload.vtable_slot  = slot;
            payload.done         = 0;
            Mem.WriteRaw(m_payloadVA, &payload, sizeof(payload));

            bool queued = false;
            static auto ntAlert = reinterpret_cast<LONG(NTAPI*)(HANDLE)>(
                ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtAlertThread"));

            for (DWORD tid : EnumProcessThreads(Mem.ProcId)) {
                HANDLE hT = ::OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
                if (!hT) continue;
                if (::QueueUserAPC(reinterpret_cast<PAPCFUNC>(m_codeVA), hT,
                                     static_cast<ULONG_PTR>(m_payloadVA)))
                    queued = true;
                if (ntAlert) ntAlert(hT);
                ::CloseHandle(hT);
            }
            if (!queued) return false;

            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
            while (std::chrono::steady_clock::now() < deadline) {
                if (Mem.Read<uint32_t>(m_payloadVA + offsetof(ApcPayload, done)))
                    return true;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return false;
        }

        void stop(uintptr_t resource_ptr) {
            // Signal Stopping so the game's own scheduler cooperates
            Mem.Write<uint32_t>(resource_ptr + 0x118, static_cast<uint32_t>(Features::Exploits::eResourceState::Stopping));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            CallSlot(resource_ptr, 9);
        }

        void start(uintptr_t resource_ptr) {
            CallSlot(resource_ptr, 8);
        }

        void destroy(uintptr_t resource_ptr) {
            stop(resource_ptr);
        }

        void revive(uintptr_t resource_ptr) {
            Mem.Write<uint32_t>(resource_ptr + 0x118, static_cast<uint32_t>(Features::Exploits::eResourceState::Stopped));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            start(resource_ptr);
        }

    private:
        bool      m_ready     = false;
        uintptr_t m_codeVA    = 0;
        uintptr_t m_payloadVA = 0;
        std::vector<uint8_t> m_shellcode;
        std::mutex m_mtx;
    };

    inline cResourceManagerV2 g_ResourceManagerV2;

} // namespace ResourceV2
