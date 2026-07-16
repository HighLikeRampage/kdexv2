#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <intrin.h>
#include "../../xorstr.hpp"

#pragma comment(lib, "ntdll.lib")

#ifndef NT_SUCCESS
#define NT_SUCCESS(x) ((x) >= 0)
#endif
#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

namespace AntiCrack {
namespace AntiDebug {

    inline bool is_debugger_present_api() {
        return IsDebuggerPresent() != FALSE;
    }

    inline bool check_nt_global_flag() {
#if defined(_WIN64)
        PVOID pPeb = (PVOID)(void*)__readgsqword(0x60);
        const size_t offsetNtGlobalFlag = 0xBC;
#else
        PVOID pPeb = (PVOID)(void*)__readfsdword(0x30);
        const size_t offsetNtGlobalFlag = 0x68;
#endif
        if (!pPeb) return false;
        DWORD flg = *(DWORD*)((BYTE*)pPeb + offsetNtGlobalFlag);
        return (flg & 0x70) != 0;
    }

    inline bool check_remote_debugger() {
        BOOL present = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &present);
        return present != FALSE;
    }

    inline bool has_hardware_breakpoints() {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(GetCurrentThread(), &ctx))
            return false;
        return (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0);
    }

    inline bool rdtsc_timing_anomaly() {
        unsigned long long t1 = __rdtsc();
        unsigned long long t2 = __rdtsc();
        return (t2 - t1) > 1000;
    }

    inline bool query_debug_port() {
        using NtQueryInformationProcess_t = NTSTATUS(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
        HMODULE hNtdll = GetModuleHandleA(xorstr("ntdll.dll"));
        if (!hNtdll) return false;
        auto* NtQIP = (NtQueryInformationProcess_t)GetProcAddress(hNtdll, xorstr("NtQueryInformationProcess"));
        if (!NtQIP) return false;
        DWORD_PTR debugPort = 0;
        NTSTATUS st = NtQIP(GetCurrentProcess(), 7, &debugPort, sizeof(debugPort), nullptr);
        return NT_SUCCESS(st) && (debugPort != 0);
    }

    inline bool is_debugger_detected() {
        if (is_debugger_present_api()) return true;
        if (check_remote_debugger()) return true;
        if (query_debug_port()) return true;
        if (has_hardware_breakpoints()) return true;
        if (check_nt_global_flag()) return true;
        if (rdtsc_timing_anomaly()) return true;
        return false;
    }

}
}
