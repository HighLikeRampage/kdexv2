#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Psapi.h>
#include <string>
#include "../../xorstr.hpp"

#pragma comment(lib, "psapi.lib")

#ifndef NT_SUCCESS
#define NT_SUCCESS(x) ((x) >= 0)
#endif
#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif
#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
typedef NTSTATUS(NTAPI* _NtQueryVirtualMemory_t)(HANDLE, PVOID, int, PVOID, SIZE_T, PSIZE_T);

namespace AntiCrack {
namespace AntiTamper {

    inline bool check_module_integrity() {
        HMODULE hMod = GetModuleHandleW(nullptr);
        if (!hMod) return true;
        MODULEINFO modInfo = { 0 };
        if (!GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo)))
            return true;
        PIMAGE_DOS_HEADER pDos = reinterpret_cast<PIMAGE_DOS_HEADER>(hMod);
        if (pDos->e_magic != IMAGE_DOS_SIGNATURE) return false;
        PIMAGE_NT_HEADERS pNT = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<BYTE*>(hMod) + pDos->e_lfanew);
        if (pNT->Signature != IMAGE_NT_SIGNATURE) return false;
        PIMAGE_SECTION_HEADER pSec = IMAGE_FIRST_SECTION(pNT);
        for (WORD i = 0; i < pNT->FileHeader.NumberOfSections; i++, pSec++) {
            if (pSec->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
                BYTE* start = reinterpret_cast<BYTE*>(hMod) + pSec->VirtualAddress;
                SIZE_T size = pSec->Misc.VirtualSize;
                if (size > 0 && size < 0x10000000) {
                    volatile DWORD sum = 0;
                    for (SIZE_T j = 0; j < size; j += 4)
                        sum ^= *reinterpret_cast<DWORD*>(start + j);
                    (void)sum;
                }
                break;
            }
        }
        return true;
    }

    inline bool detect_external_read(void* address) {
        HMODULE hNtdll = GetModuleHandleA(xorstr("ntdll.dll"));
        if (!hNtdll) return false;
        _NtQueryVirtualMemory_t NtQueryVirtualMemory = (_NtQueryVirtualMemory_t)GetProcAddress(hNtdll, xorstr("NtQueryVirtualMemory"));
        if (!NtQueryVirtualMemory) return false;
        struct {
            PVOID VirtualAddress;
            ULONG_PTR VirtualAttributes;
        } info = { 0 };
        info.VirtualAddress = address;
        NTSTATUS st = NtQueryVirtualMemory(NtCurrentProcess(), nullptr, 4, &info, sizeof(info), nullptr);
        if (st != 0) return false;
        return (info.VirtualAttributes & 1) != 0;
    }

    inline void memory_read_detection_loop(void (*on_trigger)(), void* (*alloc)()) {
        void* address = alloc ? alloc() : VirtualAlloc(nullptr, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!address) return;
        HMODULE hNtdll = GetModuleHandleA(xorstr("ntdll.dll"));
        if (!hNtdll) return;
        _NtQueryVirtualMemory_t query_vm = (_NtQueryVirtualMemory_t)GetProcAddress(hNtdll, xorstr("NtQueryVirtualMemory"));
        if (!query_vm) return;
        while (true) {
            struct {
                PVOID VirtualAddress;
                ULONG_PTR VirtualAttributes;
            } info = { 0 };
            info.VirtualAddress = address;
            query_vm(NtCurrentProcess(), nullptr, 4, &info, sizeof(info), nullptr);
            if (info.VirtualAttributes & 1) {
                if (on_trigger) on_trigger();
                return;
            }
            Sleep(400);
        }
    }

}
}
