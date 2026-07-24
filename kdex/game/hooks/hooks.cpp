#include "hooks.hpp"
#include <tlhelp32.h>
#include <vector>

namespace hooks {

static constexpr BYTE k_sig[] = {
    0x48, 0x89, 0x5C, 0x24, 0x08,
    0x57,
    0x48, 0x83, 0xEC, 0x20,
    0x48, 0x8B, 0xFA,
    0xC7, 0x44, 0x24, 0x38, 0x00, 0x00, 0x00, 0x00,
    0x48, 0x8B, 0xD9,
    0xFF, 0x15
};

static constexpr BYTE k_patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 };
static constexpr uintptr_t k_rva_fallback = 0x4DCE0;

struct PatchSite {
    DWORD     pid;
    uintptr_t addr;
    BYTE      orig[sizeof(k_patch)];
    bool      restored;
};

static std::vector<PatchSite> g_sites;

static std::vector<DWORD> FindTargetPids() {
    std::vector<DWORD> pids;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return pids;
    PROCESSENTRY32W pe = { sizeof(pe) };
    if (Process32FirstW(hSnap, &pe)) {
        do {
            HANDLE hMod = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pe.th32ProcessID);
            if (hMod == INVALID_HANDLE_VALUE) continue;
            MODULEENTRY32W me = { sizeof(me) };
            if (Module32FirstW(hMod, &me)) {
                do {
                    if (_wcsicmp(me.szModule, xorstr(L"nvspcap64.dll")) == 0) {
                        pids.push_back(pe.th32ProcessID);
                        break;
                    }
                } while (Module32NextW(hMod, &me));
            }
            CloseHandle(hMod);
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pids;
}

struct ModInfo { uintptr_t base; SIZE_T size; };

static ModInfo GetRemoteMod(HANDLE hProc, const wchar_t* name) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(hProc));
    if (hSnap == INVALID_HANDLE_VALUE) return {};
    MODULEENTRY32W me = { sizeof(me) };
    ModInfo info = {};
    if (Module32FirstW(hSnap, &me)) {
        do {
            if (_wcsicmp(me.szModule, name) == 0) {
                info.base = (uintptr_t)me.modBaseAddr;
                info.size = me.modBaseSize;
                break;
            }
        } while (Module32NextW(hSnap, &me));
    }
    CloseHandle(hSnap);
    return info;
}

static uintptr_t ScanSig(HANDLE hProc, uintptr_t base, SIZE_T size) {
    if (size < sizeof(k_sig)) return 0;
    std::vector<BYTE> buf(size);
    SIZE_T rd = 0;
    if (!ReadProcessMemory(hProc, (LPCVOID)base, buf.data(), size, &rd) || rd < sizeof(k_sig))
        return 0;
    for (SIZE_T i = 0; i <= rd - sizeof(k_sig); i++) {
        if (memcmp(buf.data() + i, k_sig, sizeof(k_sig)) == 0)
            return base + i;
    }
    return 0;
}

static bool PatchRemote(HANDLE hProc, uintptr_t addr, BYTE* orig_out) {
    SIZE_T rd = 0;
    if (!ReadProcessMemory(hProc, (LPCVOID)addr, orig_out, sizeof(k_patch), &rd) || rd != sizeof(k_patch))
        return false;
    DWORD old = 0;
    if (!VirtualProtectEx(hProc, (LPVOID)addr, sizeof(k_patch), PAGE_EXECUTE_READWRITE, &old))
        return false;
    SIZE_T wr = 0;
    bool ok = WriteProcessMemory(hProc, (LPVOID)addr, k_patch, sizeof(k_patch), &wr) && wr == sizeof(k_patch);
    VirtualProtectEx(hProc, (LPVOID)addr, sizeof(k_patch), old, &old);
    return ok;
}

static bool RestoreRemote(HANDLE hProc, uintptr_t addr, const BYTE* orig) {
    DWORD old = 0;
    VirtualProtectEx(hProc, (LPVOID)addr, sizeof(k_patch), PAGE_EXECUTE_READWRITE, &old);
    SIZE_T wr = 0;
    bool ok = WriteProcessMemory(hProc, (LPVOID)addr, orig, sizeof(k_patch), &wr) && wr == sizeof(k_patch);
    VirtualProtectEx(hProc, (LPVOID)addr, sizeof(k_patch), old, &old);
    return ok;
}

static void StubGetWDA(HANDLE hProc) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(hProc));
    if (hSnap == INVALID_HANDLE_VALUE) return;
    MODULEENTRY32W me = { sizeof(me) };
    uintptr_t u32_remote = 0;
    if (Module32FirstW(hSnap, &me)) {
        do {
            if (_wcsicmp(me.szModule, xorstr(L"user32.dll")) == 0) {
                u32_remote = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32NextW(hSnap, &me));
    }
    CloseHandle(hSnap);
    if (!u32_remote) return;

    HMODULE hU32 = GetModuleHandleW(xorstr(L"user32.dll"));
    if (!hU32) return;
    FARPROC fn = GetProcAddress(hU32, xorstr("GetWindowDisplayAffinity"));
    if (!fn) return;

    uintptr_t remote_fn = u32_remote + ((uintptr_t)fn - (uintptr_t)hU32);

    static constexpr BYTE stub[] = {
        0x48, 0x85, 0xD2,
        0x74, 0x06,
        0xC7, 0x02, 0x00, 0x00, 0x00, 0x00,
        0xB8, 0x01, 0x00, 0x00, 0x00,
        0xC3
    };
    DWORD old = 0;
    if (VirtualProtectEx(hProc, (LPVOID)remote_fn, sizeof(stub), PAGE_EXECUTE_READWRITE, &old)) {
        SIZE_T wr = 0;
        WriteProcessMemory(hProc, (LPVOID)remote_fn, stub, sizeof(stub), &wr);
        VirtualProtectEx(hProc, (LPVOID)remote_fn, sizeof(stub), old, &old);
    }
}

void Install() {
    if (!g_sites.empty()) return;

    std::vector<DWORD> pids = FindTargetPids();
    for (DWORD pid : pids) {
        HANDLE hProc = OpenProcess(
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
            FALSE, pid);
        if (!hProc) continue;

        ModInfo mod = GetRemoteMod(hProc, xorstr(L"nvspcap64.dll"));
        uintptr_t target = 0;
        if (mod.base && mod.size)
            target = ScanSig(hProc, mod.base, mod.size);
        if (!target && mod.base && mod.size > k_rva_fallback)
            target = mod.base + k_rva_fallback;

        if (target) {
            PatchSite site = { pid, target, {}, false };
            if (PatchRemote(hProc, target, site.orig)) {
                g_sites.push_back(site);
                StubGetWDA(hProc);
            }
        }
        CloseHandle(hProc);
    }
}

void Uninstall() {
    for (auto& site : g_sites) {
        if (site.restored) continue;
        HANDLE hProc = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, site.pid);
        if (hProc) {
            RestoreRemote(hProc, site.addr, site.orig);
            CloseHandle(hProc);
        }
        site.restored = true;
    }
    g_sites.clear();
}

}
