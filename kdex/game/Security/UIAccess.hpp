#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <cstdio>
#include "xorstr.hpp"
#pragma comment(lib, "shell32.lib")

namespace UIAccess {

    inline void Log(const char* fmt, ...) {
        char buf[512];
        va_list ap;
        va_start(ap, fmt);
        _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
        va_end(ap);
        printf("[UIAccess] %s\n", buf);
        fflush(stdout);
        char dbg[600];
        _snprintf_s(dbg, sizeof(dbg), _TRUNCATE, "[UIAccess] %s\n", buf);
        OutputDebugStringA(dbg);
    }

    inline bool HasUIAccess() {
        HANDLE hTok = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok)) return false;
        DWORD uia = 0, ret = 0;
        BOOL ok = GetTokenInformation(hTok, TokenUIAccess, &uia, sizeof(uia), &ret);
        CloseHandle(hTok);
        return ok && uia != 0;
    }

    inline bool EnablePrivilege(LPCWSTR name) {
        HANDLE hTok = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTok)) return false;
        TOKEN_PRIVILEGES tp{};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        BOOL ok = LookupPrivilegeValueW(nullptr, name, &tp.Privileges[0].Luid) &&
                  AdjustTokenPrivileges(hTok, FALSE, &tp, sizeof(tp), nullptr, nullptr) &&
                  GetLastError() == ERROR_SUCCESS;
        CloseHandle(hTok);
        return ok;
    }

    inline DWORD FindWinlogonInSession(DWORD sessionId) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) {
            Log("CreateToolhelp32Snapshot failed, err=%lu", GetLastError());
            return 0;
        }
        PROCESSENTRY32W pe{ sizeof(pe) };
        DWORD found = 0;
        int total = 0, wcount = 0;
        if (Process32FirstW(snap, &pe)) {
            do {
                total++;
                if (_wcsicmp(pe.szExeFile, L"winlogon.exe") == 0) {
                    wcount++;
                    DWORD sid = (DWORD)-1;
                    BOOL sok = ProcessIdToSessionId(pe.th32ProcessID, &sid);
                    Log("  winlogon pid=%lu session=%lu (sok=%d)", pe.th32ProcessID, sid, sok);
                    if (sok && sid == sessionId && !found) {
                        found = pe.th32ProcessID;
                    }
                }
            } while (Process32NextW(snap, &pe));
        }
        Log("enum: total=%d winlogon_matches=%d chosen_pid=%lu", total, wcount, found);
        CloseHandle(snap);
        return found;
    }

    inline bool IsElevatedToken() {
        HANDLE hTok = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok)) return false;
        TOKEN_ELEVATION te{};
        DWORD ret = 0;
        BOOL ok = GetTokenInformation(hTok, TokenElevation, &te, sizeof(te), &ret);
        CloseHandle(hTok);
        return ok && te.TokenIsElevated != 0;
    }

    inline DWORD GetIntegrityLevel() {
        HANDLE hTok = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok)) return 0;
        DWORD needed = 0;
        GetTokenInformation(hTok, TokenIntegrityLevel, nullptr, 0, &needed);
        if (!needed) { CloseHandle(hTok); return 0; }
        BYTE* buf = (BYTE*)_malloca(needed);
        DWORD rid = 0;
        if (buf && GetTokenInformation(hTok, TokenIntegrityLevel, buf, needed, &needed)) {
            TOKEN_MANDATORY_LABEL* tml = (TOKEN_MANDATORY_LABEL*)buf;
            rid = *GetSidSubAuthority(tml->Label.Sid, (DWORD)(UCHAR)(*GetSidSubAuthorityCount(tml->Label.Sid) - 1));
        }
        if (buf) _freea(buf);
        CloseHandle(hTok);
        return rid;
    }

    inline bool RelaunchElevated() {
        Log("RelaunchElevated() enter, pid=%lu", GetCurrentProcessId());
        if (IsElevatedToken()) {
            Log("already elevated, skip");
            return false;
        }
        LPCWSTR cmd = GetCommandLineW();
        if (cmd && wcsstr(cmd, L"--kdex-elev")) {
            Log("--kdex-elev marker present but still Medium -- UAC/policy/manifest is stripping elevation, cannot recover");
            return false;
        }

        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);

        SHELLEXECUTEINFOW sei{ sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = exePath;
        sei.lpParameters = L"--kdex-elev";
        sei.nShow = SW_NORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;

        Log("ShellExecuteExW runas exe=%ls", exePath);
        if (!ShellExecuteExW(&sei)) {
            Log("ShellExecuteExW failed, err=%lu (1223=user declined UAC)", GetLastError());
            return false;
        }
        Log("UAC accepted, elevated child spawned -- parent exiting");
        if (sei.hProcess) CloseHandle(sei.hProcess);
        return true;
    }

    inline HANDLE SpawnAndStealTokenFrom(const wchar_t* relative) {
        wchar_t path[MAX_PATH] = {};
        GetSystemDirectoryW(path, MAX_PATH);
        wcscat_s(path, L"\\");
        wcscat_s(path, relative);
        Log("SpawnAndStealTokenFrom: ShellExecuteExW %ls hidden", path);

        SHELLEXECUTEINFOW sei{ sizeof(sei) };
        sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
        sei.lpFile = path;
        sei.nShow = SW_HIDE;

        if (!ShellExecuteExW(&sei) || !sei.hProcess) {
            Log("ShellExecuteExW(%ls) failed, err=%lu hProcess=%p", relative, GetLastError(), sei.hProcess);
            if (sei.hProcess) CloseHandle(sei.hProcess);
            return nullptr;
        }
        DWORD pid = GetProcessId(sei.hProcess);
        Log("%ls spawned pid=%lu", relative, pid);

        HANDLE hTok = nullptr;
        BOOL ok = FALSE;
        for (int attempt = 0; attempt < 40 && !ok; attempt++) {
            Sleep(50);
            ok = OpenProcessToken(sei.hProcess,
                TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID,
                &hTok);
        }
        if (!ok) {
            Log("OpenProcessToken(%ls) failed after retries, err=%lu", relative, GetLastError());
            TerminateProcess(sei.hProcess, 0);
            CloseHandle(sei.hProcess);
            return nullptr;
        }
        Log("OpenProcessToken(%ls) ok", relative);

        HANDLE hDup = nullptr;
        ok = DuplicateTokenEx(hTok, MAXIMUM_ALLOWED, nullptr, SecurityImpersonation, TokenPrimary, &hDup);
        CloseHandle(hTok);
        TerminateProcess(sei.hProcess, 0);
        CloseHandle(sei.hProcess);

        if (!ok || !hDup) {
            Log("DuplicateTokenEx(%ls) failed, err=%lu", relative, GetLastError());
            return nullptr;
        }
        Log("DuplicateTokenEx(%ls) ok", relative);

        DWORD uia = 0, ret = 0;
        if (GetTokenInformation(hDup, TokenUIAccess, &uia, sizeof(uia), &ret)) {
            Log("stolen %ls token: UIAccess=%lu", relative, uia);
        }
        if (!uia) {
            DWORD one = 1;
            if (!SetTokenInformation(hDup, TokenUIAccess, &one, sizeof(one))) {
                Log("SetTokenInformation(TokenUIAccess=1) failed, err=%lu -- token unusable", GetLastError());
                CloseHandle(hDup);
                return nullptr;
            }
            Log("SetTokenInformation(TokenUIAccess=1) ok");
        }
        return hDup;
    }

    inline HANDLE StealUIAccessTokenFromOsk() {
        HANDLE h = SpawnAndStealTokenFrom(L"osk.exe");
        if (h) return h;
        Log("osk.exe failed, trying magnify.exe fallback");
        h = SpawnAndStealTokenFrom(L"magnify.exe");
        if (h) return h;
        Log("magnify.exe failed, trying narrator.exe fallback");
        return SpawnAndStealTokenFrom(L"narrator.exe");
    }

    inline bool RelaunchWithUIAccess() {
        Log("RelaunchWithUIAccess() enter, pid=%lu", GetCurrentProcessId());
        Log("token: elevated=%d integrity=0x%lx (0x2000=Medium 0x3000=High 0x4000=System)", (int)IsElevatedToken(), GetIntegrityLevel());

        if (HasUIAccess()) {
            Log("already have UIAccess, skipping relaunch");
            return false;
        }
        Log("current process does NOT have UIAccess");

        wchar_t marker[8] = {};
        if (GetEnvironmentVariableW(L"KDEX_UIACC", marker, 8) > 0) {
            Log("KDEX_UIACC marker present, skipping to avoid relaunch loop");
            return false;
        }
        SetEnvironmentVariableW(L"KDEX_UIACC", L"1");
        Log("set KDEX_UIACC=1");

        bool dbg = EnablePrivilege(SE_DEBUG_NAME);
        bool imp = EnablePrivilege(SE_IMPERSONATE_NAME);
        Log("EnablePrivilege SeDebug=%d SeImpersonate=%d", dbg, imp);
        if (!imp) {
            Log("SeImpersonatePrivilege not held -- CreateProcessWithTokenW will fail");
            return false;
        }

        HANDLE hDup = StealUIAccessTokenFromOsk();
        if (!hDup) {
            Log("StealUIAccessTokenFromOsk failed");
            return false;
        }

        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        Log("exe=%ls", exePath);

        wchar_t cmdBuf[32768] = {};
        LPCWSTR cmdSrc = GetCommandLineW();
        if (cmdSrc) wcsncpy_s(cmdBuf, cmdSrc, _TRUNCATE);

        STARTUPINFOW si{ sizeof(si) };
        PROCESS_INFORMATION pi{};
        BOOL ok = CreateProcessWithTokenW(hDup, 0, exePath, cmdBuf, 0, nullptr, nullptr, &si, &pi);
        CloseHandle(hDup);
        if (!ok) {
            Log("CreateProcessWithTokenW failed, err=%lu", GetLastError());
            return false;
        }
        Log("CreateProcessWithTokenW ok, child pid=%lu -- parent exiting", pi.dwProcessId);

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
}
