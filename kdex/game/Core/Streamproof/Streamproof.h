#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <cstring>

class StreamProof {
public:
    bool Update() {
        const ULONGLONG now = GetTickCount64();
        if (m_LastScanMs && (now - m_LastScanMs) < kScanIntervalMs) return IsActive();
        m_LastScanMs = now;

        EnableDebugPrivilege();

        if (!m_FtsChecked) {
            if (ApplyFts()) m_FtsChecked = true;
        }

        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return IsActive();
        PROCESSENTRY32W pe{ sizeof(pe) };
        if (Process32FirstW(snap, &pe)) {
            do {
                if (pe.th32ProcessID <= 4) continue;
                if (!IsNvProc(pe.szExeFile) && !IsAmdProc(pe.szExeFile)) continue;
                HANDLE hp = OpenProcess(
                    PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                    FALSE, pe.th32ProcessID);
                if (!hp) continue;
                PatchAffinity(hp, pe.th32ProcessID);
                CloseHandle(hp);
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);

        return IsActive();
    }

    bool Cleanup() {
        EnableDebugPrivilege();
        for (auto& p : m_Patches) {
            HANDLE hp = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, p.pid);
            if (!hp) continue;
            WriteRemote(hp, (void*)p.addr, p.orig, p.len);
            CloseHandle(hp);
        }
        m_Patches.clear();
        m_EnumHits = 0;
        m_AffinityHits = 0;
        m_DidRecycle = false;
        m_LastScanMs = 0;
        m_RecycleAtMs = 0;
        RestoreFts();
        m_FtsErased = false;
        m_FtsChecked = false;
        VerifyAndPurgeFts();
        return true;
    }

    bool IsActive() const { return m_AffinityHits > 0; }

    bool VerifyAndPurgeFts() {
        DWORD cur = 0;
        if (!ReadFtsValue(cur)) return true;
        return EraseFtsValue();
    }

private:
    struct PatchRecord {
        DWORD pid = 0;
        uintptr_t addr = 0;
        unsigned char orig[16]{};
        SIZE_T len = 0;
        enum Kind : unsigned char { Enum = 1, Affinity = 2 } kind = Enum;
    };

    std::vector<PatchRecord> m_Patches;
    size_t m_EnumHits = 0;
    size_t m_AffinityHits = 0;
    ULONGLONG m_LastScanMs = 0;
    bool m_RegApplied = false;
    bool m_RegHadValue = false;
    DWORD m_RegOldValue = 0;
    bool m_DidRecycle = false;
    bool m_FtsErased = false;
    bool m_FtsChecked = false;
    ULONGLONG m_RecycleAtMs = 0;

    static constexpr ULONGLONG kScanIntervalMs = 1500;
    static constexpr ULONGLONG kFtsEraseDelayMs = 2500;
    static constexpr DWORD kFtsDxgiValue = 0x24;

    static constexpr unsigned char kEnumSig[] = {
        0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20,
        0x48,0x8B,0xFA,0xC7,0x44,0x24,0x38,0x00,0x00,0x00,0x00,
        0x48,0x8B,0xD9,0xFF,0x15
    };
    static constexpr unsigned char kRetTrue[] = { 0xB8,0x01,0x00,0x00,0x00,0xC3 };
    static constexpr unsigned char kAffinityRetFalse[] = { 0x31,0xC0,0xC3 };

    static bool EnableDebugPrivilege() {
        HANDLE tok = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok)) return false;
        TOKEN_PRIVILEGES tp{};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &tp.Privileges[0].Luid)) {
            CloseHandle(tok); return false;
        }
        AdjustTokenPrivileges(tok, FALSE, &tp, sizeof(tp), nullptr, nullptr);
        CloseHandle(tok);
        return true;
    }

    static bool IsNvProc(const wchar_t* exe) {
        return !_wcsicmp(exe, L"nvcontainer.exe")
            || !_wcsicmp(exe, L"NVIDIA Share.exe")
            || !_wcsicmp(exe, L"NVIDIA ShareSrv.exe")
            || !_wcsicmp(exe, L"nvsphelper64.exe");
    }

    static bool IsAmdProc(const wchar_t* exe) {
        return !_wcsicmp(exe, L"RadeonSoftware.exe")
            || !_wcsicmp(exe, L"AMDRSServ.exe")
            || !_wcsicmp(exe, L"AMDRSSrcExt.exe")
            || !_wcsicmp(exe, L"AMDRSSrcTab.exe")
            || !_wcsicmp(exe, L"amdow.exe")
            || !_wcsicmp(exe, L"atieclxx.exe");
    }

    static bool ModInfo(DWORD pid, const wchar_t* name, uintptr_t& base, size_t& size) {
        base = 0; size = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) return false;
        MODULEENTRY32W me{ sizeof(me) };
        bool ok = false;
        if (Module32FirstW(snap, &me)) {
            do {
                if (!_wcsicmp(me.szModule, name)) {
                    base = (uintptr_t)me.modBaseAddr;
                    size = (size_t)me.modBaseSize;
                    ok = true; break;
                }
            } while (Module32NextW(snap, &me));
        }
        CloseHandle(snap);
        return ok;
    }

    static uintptr_t FindRemote(HANDLE hp, uintptr_t base, size_t size, const unsigned char* sig, size_t sigLen) {
        if (!base || !size || !sig || !sigLen) return 0;
        const size_t chunk = 0x10000;
        std::vector<unsigned char> buf(chunk + sigLen);
        for (size_t off = 0; off < size; ) {
            const size_t n = (off + chunk > size) ? (size - off) : chunk;
            SIZE_T got = 0;
            if (!ReadProcessMemory(hp, (LPCVOID)(base + off), buf.data(), n, &got) || !got) { off += chunk; continue; }
            for (size_t i = 0; i + sigLen <= got; ++i)
                if (memcmp(buf.data() + i, sig, sigLen) == 0) return base + off + i;
            off += (got > sigLen) ? (got - sigLen + 1) : got;
        }
        return 0;
    }

    bool AlreadyTracked(DWORD pid, uintptr_t addr) const {
        for (const auto& p : m_Patches) if (p.pid == pid && p.addr == addr) return true;
        return false;
    }

    static bool WriteRemote(HANDLE hp, void* addr, const void* data, SIZE_T len) {
        DWORD old = 0;
        if (!VirtualProtectEx(hp, addr, len, PAGE_EXECUTE_READWRITE, &old)) return false;
        SIZE_T wr = 0;
        const bool ok = WriteProcessMemory(hp, addr, data, len, &wr) && wr == len;
        DWORD tmp = 0;
        VirtualProtectEx(hp, addr, len, old, &tmp);
        if (ok) FlushInstructionCache(hp, addr, len);
        return ok;
    }

    bool ApplyTracked(HANDLE hp, DWORD pid, uintptr_t addr, const unsigned char* patch, SIZE_T len, PatchRecord::Kind kind) {
        if (!len || len > sizeof(PatchRecord::orig)) return false;
        if (AlreadyTracked(pid, addr)) return true;
        unsigned char orig[16]{};
        SIZE_T got = 0;
        if (!ReadProcessMemory(hp, (LPCVOID)addr, orig, len, &got) || got != len) return false;
        if (memcmp(orig, patch, len) == 0) return true;
        if (!WriteRemote(hp, (void*)addr, patch, len)) return false;
        PatchRecord r{};
        r.pid = pid; r.addr = addr; r.len = len; r.kind = kind;
        memcpy(r.orig, orig, len);
        m_Patches.push_back(r);
        if (kind == PatchRecord::Enum) ++m_EnumHits; else ++m_AffinityHits;
        return true;
    }

    bool PatchAffinity(HANDLE hp, DWORD pid) {
        uintptr_t remoteU32 = 0; size_t sz = 0;
        if (!ModInfo(pid, L"user32.dll", remoteU32, sz) || !remoteU32) return false;
        HMODULE local = GetModuleHandleW(L"user32.dll");
        if (!local) return false;
        FARPROC fn = GetProcAddress(local, "GetWindowDisplayAffinity");
        if (!fn) return false;
        const uintptr_t remote = remoteU32 + ((uintptr_t)fn - (uintptr_t)local);
        return ApplyTracked(hp, pid, remote, kAffinityRetFalse, sizeof(kAffinityRetFalse), PatchRecord::Affinity);
    }

    bool PatchEnum(HANDLE hp, DWORD pid) {
        bool any = false;
        const wchar_t* mods[] = { L"nvspcap64.dll", L"_nvspcaps64.dll" };
        for (auto* name : mods) {
            uintptr_t base = 0; size_t sz = 0;
            if (!ModInfo(pid, name, base, sz) || !base) continue;
            const uintptr_t hit = FindRemote(hp, base, sz, kEnumSig, sizeof(kEnumSig));
            if (hit) any |= ApplyTracked(hp, pid, hit, kRetTrue, sizeof(kRetTrue), PatchRecord::Enum);
        }
        return any;
    }

    bool ApplyFts() {
        if (m_RegApplied) return true;
        HKEY hk = nullptr;
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\NVIDIA Corporation\\Global\\NvApp\\ShadowPlay\\FTS",
            0, nullptr, 0,
            KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, nullptr, &hk, nullptr) != ERROR_SUCCESS) return false;
        DWORD type = 0, old = 0, cb = sizeof(old);
        if (RegQueryValueExW(hk, L"{497B8458-4244-4EE6-BFEA-F3D2BA294F21}", nullptr, &type, (LPBYTE)&old, &cb) == ERROR_SUCCESS && type == REG_DWORD) {
            m_RegHadValue = true; m_RegOldValue = old;
        } else { m_RegHadValue = false; m_RegOldValue = 0; }
        DWORD val = kFtsDxgiValue;
        const LONG st = RegSetValueExW(hk, L"{497B8458-4244-4EE6-BFEA-F3D2BA294F21}", 0, REG_DWORD, (const BYTE*)&val, sizeof(val));
        RegCloseKey(hk);
        if (st != ERROR_SUCCESS) return false;
        m_RegApplied = true;
        return true;
    }

    bool RestoreFts() {
        if (!m_RegApplied) return true;
        HKEY hk = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\NVIDIA Corporation\\Global\\NvApp\\ShadowPlay\\FTS",
            0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hk) != ERROR_SUCCESS) {
            m_RegApplied = false; return true;
        }
        if (m_RegHadValue)
            RegSetValueExW(hk, L"{497B8458-4244-4EE6-BFEA-F3D2BA294F21}", 0, REG_DWORD, (const BYTE*)&m_RegOldValue, sizeof(m_RegOldValue));
        else
            RegDeleteValueW(hk, L"{497B8458-4244-4EE6-BFEA-F3D2BA294F21}");
        RegCloseKey(hk);
        m_RegApplied = false;
        m_RegHadValue = false;
        m_RegOldValue = 0;
        return true;
    }

    static bool ReadFtsValue(DWORD& out) {
        out = 0;
        HKEY hk = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\NVIDIA Corporation\\Global\\NvApp\\ShadowPlay\\FTS",
            0, KEY_READ | KEY_WOW64_64KEY, &hk) != ERROR_SUCCESS) return false;
        DWORD type = 0, cb = sizeof(out);
        const LONG st = RegQueryValueExW(hk, L"{497B8458-4244-4EE6-BFEA-F3D2BA294F21}", nullptr, &type, (LPBYTE)&out, &cb);
        RegCloseKey(hk);
        return st == ERROR_SUCCESS && type == REG_DWORD;
    }

    bool EraseFtsValue() {
        HKEY hk = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\NVIDIA Corporation\\Global\\NvApp\\ShadowPlay\\FTS",
            0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hk) != ERROR_SUCCESS) return false;
        const LONG st = RegDeleteValueW(hk, L"{497B8458-4244-4EE6-BFEA-F3D2BA294F21}");
        RegCloseKey(hk);
        if (st != ERROR_SUCCESS && st != ERROR_FILE_NOT_FOUND) return false;
        m_RegApplied = false;
        m_RegHadValue = false;
        m_RegOldValue = 0;
        m_FtsErased = true;
        return true;
    }

    void RecycleNvOnce() {
        if (m_DidRecycle) return;
        m_DidRecycle = true;
        m_RecycleAtMs = GetTickCount64();
        m_Patches.clear();
        m_EnumHits = 0;
        m_AffinityHits = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return;
        PROCESSENTRY32W pe{ sizeof(pe) };
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"nvcontainer.exe") != 0) continue;
                HANDLE hp = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hp) { TerminateProcess(hp, 0); CloseHandle(hp); }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
};

inline StreamProof streamProof;
