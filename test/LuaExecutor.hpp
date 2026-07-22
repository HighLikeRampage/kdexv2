#pragma once
// LuaExecutor.hpp — Execute arbitrary Lua 5.4 code inside FiveM
//
// STRATEGY:
//   1. Scan citizen-scripting-lua.dll exports for luaL_loadbuffer + lua_pcall.
//   2. Scan process private heap pages for valid lua_State* (Lua 5.4 header check).
//   3. Write Lua source + ExecFrame + APC shellcode into the target process.
//   4. Queue APC to FiveM scripting threads; shellcode calls luaL_loadbuffer then lua_pcall.
//   5. Wait for done flag; return status codes.

#include <Core/SDK/Memory.hpp>
#include <Core/SDK/DebugLog.hpp>
#include <Security/xorstr.hpp>

#include <Windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace LuaExec {

    using namespace Core;

    static constexpr int    LUA_OK      = 0;
    static constexpr int    LUA_MULTRET = -1;
    static constexpr int    LUA_TTHREAD = 8;

    // Written into the remote process; the APC shellcode reads/writes this.
    struct ExecFrame {
        uint64_t lua_state_ptr;     // lua_State* L
        uint64_t src_ptr;           // pointer to source string in remote memory
        uint64_t src_len;           // source length (bytes, no NUL)
        uint64_t loadbuffer_fn;     // luaL_loadbuffer VA
        uint64_t pcall_fn;          // lua_pcall VA
        uint32_t done;              // set to 1 when shellcode finishes
        int32_t  load_status;       // luaL_loadbuffer return code
        int32_t  pcall_status;      // lua_pcall return code
        uint32_t _pad;
    };

    // x64 APC shellcode.
    // rcx = param = VA of ExecFrame in the remote process.
    // The chunkname string "@kdex\0" is placed at remoteBase + chunkNameOff
    // and reached via a rip-relative lea.
    //
    // Layout in one remote allocation:
    //   [0 .. codeSize)         shellcode
    //   [chunkNameOff ..)       "@kdex\0"
    //   [srcOff ..)             Lua source string + NUL
    //   [frameOff ..)           ExecFrame
    inline std::vector<uint8_t> BuildExecShellcode(int32_t chunkNameDisp) {
        // chunkNameDisp = signed byte offset from END of the lea instruction to "@kdex\0"
        //
        // Encoding (x64):
        //   sub  rsp, 0x48
        //   mov  r15, rcx               ; save frame ptr
        //   mov  rcx, [r15+0x00]        ; L
        //   mov  rdx, [r15+0x08]        ; src
        //   mov  r8,  [r15+0x10]        ; src_len
        //   lea  r9, [rip + chunkNameDisp]
        //   call qword ptr [r15+0x18]   ; loadbuffer_fn(L,src,len,name)
        //   mov  dword ptr [r15+0x28], eax  ; load_status
        //   test eax, eax
        //   jnz  done
        //   mov  rcx, [r15+0x00]        ; L
        //   xor  edx, edx               ; nargs = 0
        //   mov  r8d, 0xFFFFFFFF        ; nresults = LUA_MULTRET
        //   xor  r9d, r9d               ; msgh = 0
        //   call qword ptr [r15+0x20]   ; pcall_fn(L,0,-1,0)
        //   mov  dword ptr [r15+0x2C], eax  ; pcall_status
        // done:
        //   mov  dword ptr [r15+0x24], 1    ; done = 1
        //   add  rsp, 0x48
        //   ret

        std::vector<uint8_t> c;
        auto e  = [&](std::initializer_list<uint8_t> bs) { for (auto b : bs) c.push_back(b); };
        auto e32 = [&](int32_t v) {
            c.push_back(uint8_t(v));       c.push_back(uint8_t(v >> 8));
            c.push_back(uint8_t(v >> 16)); c.push_back(uint8_t(v >> 24));
        };

        e({0x48, 0x83, 0xEC, 0x48});        // sub rsp, 0x48
        e({0x49, 0x89, 0xCF});              // mov r15, rcx
        e({0x49, 0x8B, 0x4F, 0x00});        // mov rcx, [r15+0x00]  L
        e({0x49, 0x8B, 0x57, 0x08});        // mov rdx, [r15+0x08]  src
        e({0x4D, 0x8B, 0x47, 0x10});        // mov r8,  [r15+0x10]  src_len
        e({0x4C, 0x8D, 0x0D});              // lea r9, [rip + disp32]
        e32(chunkNameDisp);                 // patched displacement
        e({0x41, 0xFF, 0x57, 0x18});        // call [r15+0x18]      loadbuffer_fn
        e({0x41, 0x89, 0x47, 0x28});        // mov [r15+0x28], eax  load_status
        e({0x85, 0xC0});                    // test eax, eax
        const size_t jnzAt = c.size();
        e({0x75, 0x00});                    // jnz done (placeholder)
        e({0x49, 0x8B, 0x4F, 0x00});        // mov rcx, [r15+0x00]  L
        e({0x31, 0xD2});                    // xor edx, edx         nargs=0
        e({0x41, 0xB8, 0xFF, 0xFF, 0xFF, 0xFF}); // mov r8d, -1     nresults
        e({0x45, 0x31, 0xC9});              // xor r9d, r9d         msgh=0
        e({0x41, 0xFF, 0x57, 0x20});        // call [r15+0x20]      pcall_fn
        e({0x41, 0x89, 0x47, 0x2C});        // mov [r15+0x2C], eax  pcall_status
        // done:
        const size_t doneAt = c.size();
        e({0x41, 0xC7, 0x47, 0x24, 0x01, 0x00, 0x00, 0x00}); // mov dword[r15+0x24],1
        e({0x48, 0x83, 0xC4, 0x48});        // add rsp, 0x48
        e({0xC3});                          // ret

        c[jnzAt + 1] = static_cast<uint8_t>(doneAt - (jnzAt + 2));
        return c;
    }

    // ---- Lua API locator -------------------------------------------------------

    struct LuaApi {
        uint64_t loadbuffer = 0;
        uint64_t pcall      = 0;
        bool complete() const { return loadbuffer && pcall; }
    };

    inline LuaApi FindLuaApi(HANDLE hProc, DWORD pid) {
        LuaApi api;
        HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) return api;

        uintptr_t luaBase = 0; size_t luaSize = 0;
        MODULEENTRY32W me{ sizeof(me) };
        if (::Module32FirstW(snap, &me)) {
            do {
                if (::_wcsnicmp(me.szModule, L"citizen-scripting-lua", 21) == 0) {
                    luaBase = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                    luaSize = me.modBaseSize;
                    break;
                }
            } while (::Module32NextW(snap, &me));
        }
        ::CloseHandle(snap);
        if (!luaBase) return api;

        std::vector<uint8_t> img(luaSize, 0);
        SIZE_T got = 0;
        if (!::ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(luaBase), img.data(), luaSize, &got) || got == 0)
            return api;

        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(img.data());
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return api;
        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(img.data() + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return api;

        const auto& ed = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (!ed.VirtualAddress || ed.VirtualAddress + ed.Size > img.size()) return api;
        auto* exp = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(img.data() + ed.VirtualAddress);

        const auto* funcs = reinterpret_cast<const DWORD*>(img.data() + exp->AddressOfFunctions);
        const auto* names = reinterpret_cast<const DWORD*>(img.data() + exp->AddressOfNames);
        const auto* ords  = reinterpret_cast<const WORD*>(img.data() + exp->AddressOfNameOrdinals);

        for (DWORD i = 0; i < exp->NumberOfNames; ++i) {
            if (names[i] >= img.size()) continue;
            const char* n = reinterpret_cast<const char*>(img.data() + names[i]);
            const uint64_t va = luaBase + funcs[ords[i]];
            if      (std::strcmp(n, xorstr("luaL_loadbuffer")) == 0) api.loadbuffer = va;
            else if (std::strcmp(n, xorstr("lua_pcall"))       == 0) api.pcall      = va;
            if (api.complete()) break;
        }

        // Pattern fallback for luaL_loadbuffer if not exported by name
        if (!api.loadbuffer) {
            // luaL_loadbuffer calls luaL_loadbufferx with a NULL mode arg.
            // luaL_loadbufferx prologue (Lua 5.4.x Windows x64 release build):
            static const uint8_t kPat[] = {
                0x48, 0x89, 0x5C, 0x24, 0x08,   // mov [rsp+8], rbx
                0x48, 0x89, 0x74, 0x24, 0x10,   // mov [rsp+16], rsi
                0x57,                            // push rdi
                0x48, 0x83, 0xEC, 0x30           // sub rsp, 0x30
            };
            for (size_t off = 0; off + sizeof(kPat) < img.size(); ++off) {
                if (std::memcmp(img.data() + off, kPat, sizeof(kPat)) == 0) {
                    api.loadbuffer = luaBase + off;
                    DebugLog(xorstr("[LuaExec] luaL_loadbuffer pattern @ RVA 0x%zX\n"), off);
                    break;
                }
            }
        }
        return api;
    }

    // ---- lua_State* scanner ----------------------------------------------------

    struct LuaStateCandidate {
        uintptr_t addr = 0;
        uint16_t  nci  = 0; // call depth; prefer small
    };

    inline std::vector<LuaStateCandidate> ScanForLuaStates(HANDLE hProc) {
        std::vector<LuaStateCandidate> found;
        const uintptr_t kEnd = 0x7FFF00000000ULL;
        MEMORY_BASIC_INFORMATION mbi{};
        uintptr_t addr = 0x10000ULL;

        while (addr < kEnd && found.size() < 8) {
            if (!::VirtualQueryEx(hProc, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) {
                addr += 0x1000; continue;
            }
            const uintptr_t regionEnd = addr + mbi.RegionSize;

            if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE &&
                (mbi.Protect == PAGE_READWRITE || mbi.Protect == PAGE_EXECUTE_READWRITE) &&
                mbi.RegionSize >= 0x200)
            {
                const size_t readSz = std::min(mbi.RegionSize, size_t(64 * 1024));
                std::vector<uint8_t> buf(readSz, 0);
                SIZE_T got = 0;
                if (::ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(addr), buf.data(), readSz, &got) && got > 0x30) {
                    for (size_t off = 0; off + 0x30 < got; off += 8) {
                        if (buf[off + 0x08] != LUA_TTHREAD) continue;
                        const uint8_t status = buf[off + 0x0A];
                        if (status > 3) continue;

                        uint64_t lG = 0; std::memcpy(&lG, buf.data() + off + 0x18, 8);
                        if (lG < 0x10000ULL || lG > kEnd) continue;

                        uint64_t mainthread = 0; SIZE_T r2 = 0;
                        if (!::ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(lG + 0x68), &mainthread, 8, &r2) || r2 != 8) continue;
                        if (mainthread < 0x10000ULL || mainthread > kEnd) continue;

                        uint64_t mt_lG = 0;
                        if (!::ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(mainthread + 0x18), &mt_lG, 8, &r2) || r2 != 8) continue;
                        if (mt_lG != lG) continue;

                        uint16_t nci = 0; std::memcpy(&nci, buf.data() + off + 0x0C, 2);
                        if (nci < 128) found.push_back({ addr + off, nci });
                    }
                }
            }
            addr = regionEnd;
            if (mbi.RegionSize == 0) break;
        }
        return found;
    }

    // ---- Main executor ---------------------------------------------------------

    class cLuaExecutor {
    public:
        struct Result {
            bool    ok          = false;
            int32_t loadStatus  = -1;
            int32_t pcallStatus = -1;
        };

        bool Init() {
            if (m_ready) return true;
            if (!Mem.ProcHandle || !Mem.ProcId) return false;

            // FindLuaApi reads ~8 MB from the target — throttle retries to avoid
            // triggering adhesive's "excessive external ReadProcessMemory" heuristic.
            const auto now = std::chrono::steady_clock::now();
            if (m_lastInitAttempt != std::chrono::steady_clock::time_point{} &&
                now - m_lastInitAttempt < std::chrono::seconds(30))
                return false;
            m_lastInitAttempt = now;

            m_api = FindLuaApi(Mem.ProcHandle, Mem.ProcId);
            if (!m_api.complete()) {
                DebugLog(xorstr("[LuaExec] Lua API not found\n"));
                return false;
            }
            DebugLog(xorstr("[LuaExec] loadbuffer=0x%llX  pcall=0x%llX\n"),
                     (unsigned long long)m_api.loadbuffer, (unsigned long long)m_api.pcall);
            m_ready = true;
            return true;
        }

        void Invalidate() { m_luaState = 0; }

        void Cleanup() {
            m_luaState = 0;
            m_api = {};
            m_ready = false;
            m_lastInitAttempt = {};
        }

        Result Execute(const std::string& code, int timeoutMs = 5000) {
            Result res;
            if (!m_ready && !Init()) return res;

            uintptr_t L = GetLuaState();
            if (!L) { DebugLog(xorstr("[LuaExec] no valid lua_State\n")); return res; }

            static const char kChunkName[] = "@kdex";

            // Layout:  [shellcode] [chunkname] [source\0] [ExecFrame]
            // We build the shellcode first pass to get its size.
            auto sc0 = BuildExecShellcode(0); // dummy disp to get size
            const size_t codeSize      = sc0.size();
            const size_t chunkNameOff  = (codeSize + 15) & ~size_t(15);
            const size_t srcOff        = chunkNameOff + sizeof(kChunkName);
            const size_t frameOff      = (srcOff + code.size() + 1 + 15) & ~size_t(15);
            const size_t totalSize     = frameOff + sizeof(ExecFrame) + 64;

            // lea r9, [rip + disp] where rip = base + leaEnd, target = base + chunkNameOff
            // leaEnd is at offset: 4(sub rsp) + 3(mov r15) + 4+4+4 (three movs) + 3 = 22, +4 disp = 26
            // Let me count the bytes up to the end of lea:
            // sub rsp,0x48 = 4; mov r15,rcx = 3; mov rcx,[r15+0] = 4; mov rdx,[r15+8] = 4;
            // mov r8,[r15+10] = 4; lea r9,[rip+disp] prefix 4C 8D 0D = 3 bytes + 4 disp = 7 total
            // leaEnd offset from start = 4+3+4+4+4+7 = 26
            const size_t leaEnd = 26; // bytes from start of shellcode to end of lea instruction
            const int32_t chunkNameDisp = static_cast<int32_t>(chunkNameOff) - static_cast<int32_t>(leaEnd);

            auto sc = BuildExecShellcode(chunkNameDisp);

            uintptr_t remoteBase = reinterpret_cast<uintptr_t>(
                ::VirtualAllocEx(Mem.ProcHandle, nullptr, totalSize,
                    MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            if (!remoteBase) return res;

            Mem.WriteRaw(remoteBase,                    sc.data(), sc.size());
            Mem.WriteRaw(remoteBase + chunkNameOff,     kChunkName, sizeof(kChunkName));
            Mem.WriteRaw(remoteBase + srcOff,           code.data(), code.size());
            Mem.Write<uint8_t>(remoteBase + srcOff + code.size(), 0);

            ExecFrame frame{};
            frame.lua_state_ptr = L;
            frame.src_ptr       = remoteBase + srcOff;
            frame.src_len       = code.size();
            frame.loadbuffer_fn = m_api.loadbuffer;
            frame.pcall_fn      = m_api.pcall;
            Mem.WriteRaw(remoteBase + frameOff, &frame, sizeof(frame));

            const uintptr_t paramVA = remoteBase + frameOff;
            bool queued = false;

            static auto ntAlert = reinterpret_cast<LONG(NTAPI*)(HANDLE)>(
                ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtAlertThread"));

            HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snap != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te{ sizeof(te) };
                if (::Thread32First(snap, &te)) {
                    do {
                        if (te.th32OwnerProcessID != Mem.ProcId) continue;
                        HANDLE hT = ::OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
                        if (!hT) continue;
                        if (::QueueUserAPC(reinterpret_cast<PAPCFUNC>(remoteBase), hT,
                                             static_cast<ULONG_PTR>(paramVA)))
                            queued = true;
                        if (ntAlert) ntAlert(hT);
                        ::CloseHandle(hT);
                    } while (::Thread32Next(snap, &te));
                }
                ::CloseHandle(snap);
            }

            if (queued) {
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
                while (std::chrono::steady_clock::now() < deadline) {
                    ExecFrame rf{};
                    Mem.ReadRaw(remoteBase + frameOff, &rf, sizeof(rf));
                    if (rf.done) {
                        res.ok          = true;
                        res.loadStatus  = rf.load_status;
                        res.pcallStatus = rf.pcall_status;
                        DebugLog(xorstr("[LuaExec] done load=%d pcall=%d\n"),
                                 rf.load_status, rf.pcall_status);
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }

            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(remoteBase), 0, MEM_RELEASE);
            return res;
        }

    private:
        bool      m_ready    = false;
        LuaApi    m_api{};
        uintptr_t m_luaState = 0;
        std::mutex m_mtx;
        std::chrono::steady_clock::time_point m_lastInitAttempt{};

        uintptr_t GetLuaState() {
            if (m_luaState && ValidateState(m_luaState)) return m_luaState;
            m_luaState = 0;
            auto candidates = ScanForLuaStates(Mem.ProcHandle);
            DebugLog(xorstr("[LuaExec] found %zu lua_State candidates\n"), candidates.size());
            // Prefer states with smaller nci (shallower = safer to push onto)
            uintptr_t best = 0; uint16_t bestNci = 0xFFFF;
            for (const auto& c : candidates) {
                if (c.nci < bestNci && ValidateState(c.addr)) {
                    best = c.addr; bestNci = c.nci;
                }
            }
            m_luaState = best;
            if (best) DebugLog(xorstr("[LuaExec] selected state @ 0x%llX nci=%u\n"),
                               (unsigned long long)best, (unsigned)bestNci);
            return m_luaState;
        }

        bool ValidateState(uintptr_t addr) {
            if (!addr || !Mem.ProcHandle) return false;
            if (Mem.Read<uint8_t>(addr + 0x08) != LUA_TTHREAD) return false;
            if (Mem.Read<uint8_t>(addr + 0x0A) > 3) return false;
            const uint64_t lG = Mem.Read<uint64_t>(addr + 0x18);
            if (lG < 0x10000ULL) return false;
            const uint64_t mt = Mem.Read<uint64_t>(lG + 0x68);
            if (mt < 0x10000ULL) return false;
            return Mem.Read<uint64_t>(mt + 0x18) == lG;
        }
    };

    inline cLuaExecutor g_LuaExecutor;

    inline bool ExecLua(const std::string& code, int timeoutMs = 5000) {
        auto r = g_LuaExecutor.Execute(code, timeoutMs);
        return r.ok && r.loadStatus == 0 && r.pcallStatus == 0;
    }

} // namespace LuaExec
