#include "NativeCaller.hpp"

#include <algorithm>
#include <functional>
#include <psapi.h>
#include <tlhelp32.h>
#include <thread>

using namespace Core;

namespace {

    // Alertable-thread hint (main.js uses ntdll's NtAlertThread).
    typedef LONG (NTAPI *NtAlertThread_t)(HANDLE ThreadHandle);
    NtAlertThread_t GetNtAlertThread() {
        static NtAlertThread_t fn = []() -> NtAlertThread_t {
            HMODULE nt = ::GetModuleHandleW(L"ntdll.dll");
            return nt ? reinterpret_cast<NtAlertThread_t>(::GetProcAddress(nt, "NtAlertThread")) : nullptr;
        }();
        return fn;
    }

    std::vector<DWORD> EnumThreadIds(DWORD pid) {
        std::vector<DWORD> out;
        HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE) return out;
        THREADENTRY32 te{ sizeof(te) };
        if (::Thread32First(snap, &te)) {
            do {
                if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID) &&
                    te.th32OwnerProcessID == pid) {
                    out.push_back(te.th32ThreadID);
                }
            } while (::Thread32Next(snap, &te));
        }
        ::CloseHandle(snap);
        return out;
    }

    struct ModuleRow {
        std::wstring        name;
        uintptr_t           base;
        uintptr_t           size;
    };

    std::vector<ModuleRow> EnumModules(DWORD pid) {
        std::vector<ModuleRow> out;
        HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) return out;
        MODULEENTRY32W me{ sizeof(me) };
        if (::Module32FirstW(snap, &me)) {
            do {
                ModuleRow r;
                r.name = me.szModule;
                r.base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                r.size = me.modBaseSize;
                out.push_back(std::move(r));
            } while (::Module32NextW(snap, &me));
        }
        ::CloseHandle(snap);
        return out;
    }

    bool StartsWithI(const std::wstring& s, const wchar_t* prefix) {
        const size_t n = std::wcslen(prefix);
        if (s.size() < n) return false;
        for (size_t i = 0; i < n; ++i) {
            const wchar_t a = s[i], b = prefix[i];
            const wchar_t la = (a >= L'A' && a <= L'Z') ? wchar_t(a + 32) : a;
            const wchar_t lb = (b >= L'A' && b <= L'Z') ? wchar_t(b + 32) : b;
            if (la != lb) return false;
        }
        return true;
    }

    // Any of the script/native module prefixes main.js uses to pick a script thread.
    bool IsScriptModule(const std::wstring& name) {
        static const wchar_t* const prefixes[] = {
            L"citizen-scripting-", L"gta-core-five", L"scripting-gta",
            L"rage-scripting-five", L"extra-natives-five", L"scripthookv",
            L"citizen-game-main", L"citizen-resources-",
        };
        for (auto p : prefixes) if (StartsWithI(name, p)) return true;
        return false;
    }

    bool ReadImageBytes(HANDLE hProc, uintptr_t base, size_t size, std::vector<uint8_t>& out) {
        out.assign(size, 0);
        const size_t CHUNK = 4 * 1024 * 1024;
        size_t off = 0, got = 0;
        while (off < size) {
            const size_t want = (size - off < CHUNK) ? (size - off) : CHUNK;
            SIZE_T r = 0;
            if (::ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(base + off),
                                     out.data() + off, want, &r) && r > 0) {
                got += r;
                off += r;
            } else {
                off += want;
            }
        }
        return got > 0;
    }

    NativeCaller::SHVExports FindShvExports(HANDLE hProc, DWORD pid) {
        NativeCaller::SHVExports out;
        auto mods = EnumModules(pid);
        auto shv = std::find_if(mods.begin(), mods.end(), [](const ModuleRow& r) {
            return StartsWithI(r.name, L"scripthookv");
        });
        if (shv == mods.end()) return out;

        std::vector<uint8_t> img;
        if (!ReadImageBytes(hProc, shv->base, shv->size, img) || img.size() < sizeof(IMAGE_DOS_HEADER))
            return out;

        auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(img.data());
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return out;
        if (dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > img.size()) return out;

        auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(img.data() + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return out;

        const auto& expDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (!expDir.VirtualAddress || !expDir.Size) return out;
        if (expDir.VirtualAddress + expDir.Size > img.size()) return out;

        auto* exp = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(img.data() + expDir.VirtualAddress);
        if (!exp->AddressOfFunctions || !exp->AddressOfNames || !exp->AddressOfNameOrdinals) return out;
        if (exp->AddressOfFunctions   >= img.size()) return out;
        if (exp->AddressOfNames       >= img.size()) return out;
        if (exp->AddressOfNameOrdinals >= img.size()) return out;

        const auto* funcs = reinterpret_cast<const DWORD*>(img.data() + exp->AddressOfFunctions);
        const auto* names = reinterpret_cast<const DWORD*>(img.data() + exp->AddressOfNames);
        const auto* ords  = reinterpret_cast<const WORD*>(img.data() + exp->AddressOfNameOrdinals);

        for (DWORD i = 0; i < exp->NumberOfNames; ++i) {
            if (names[i] >= img.size()) continue;
            const char* n = reinterpret_cast<const char*>(img.data() + names[i]);

            const DWORD fnRva = funcs[ords[i]];
            if (!fnRva) continue;
            const uint64_t va = shv->base + fnRva;

            if      (std::strcmp(n, xorstr("?scriptRegister@@YAXPEAUHINSTANCE__@@P6AXXZ@Z")) == 0) out.reg  = va;
            else if (std::strcmp(n, xorstr("?scriptWait@@YAXK@Z"))                          == 0) out.wait = va;
            else if (std::strcmp(n, xorstr("?nativeInit@@YAX_K@Z"))                         == 0) out.init = va;
            else if (std::strcmp(n, xorstr("?nativePush64@@YAX_K@Z"))                       == 0) out.push = va;
            else if (std::strcmp(n, xorstr("?nativeCall@@YAPEA_KXZ"))                       == 0) out.call = va;
        }
        return out;
    }

    // Returns the base of a committed PAGE_EXECUTE_READWRITE region whose first
    // 22 bytes match `sig` at every position where `mask` is nonzero.
    uintptr_t FindExistingMainFn(HANDLE hProc,
                                  const uint8_t* sig, const uint8_t* mask, size_t sigLen) {
        const uintptr_t START = 0x100000000ULL, END = 0x7FFF00000000ULL;
        MEMORY_BASIC_INFORMATION mbi{};
        uintptr_t addr = START;
        while (addr < END) {
            if (!::VirtualQueryEx(hProc, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) return 0;
            if (mbi.State == MEM_COMMIT &&
                (mbi.Protect & PAGE_EXECUTE_READWRITE) == PAGE_EXECUTE_READWRITE &&
                mbi.RegionSize >= 64) {
                uint8_t probe[64]{};
                SIZE_T got = 0;
                const size_t want = mbi.RegionSize < sizeof(probe) ? mbi.RegionSize : sizeof(probe);
                if (::ReadProcessMemory(hProc, mbi.BaseAddress, probe, want, &got) && got >= sigLen) {
                    bool match = true;
                    for (size_t i = 0; i < sigLen; ++i) {
                        if (mask[i] && probe[i] != sig[i]) { match = false; break; }
                    }
                    if (match) return reinterpret_cast<uintptr_t>(mbi.BaseAddress);
                }
            }
            addr = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            if (mbi.RegionSize == 0) break;
        }
        return 0;
    }

    // Suspend + read RIP + resume. Returns 0 on failure.
    uint64_t SafeGetThreadRip(HANDLE hT) {
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_FULL;
        if (::SuspendThread(hT) == static_cast<DWORD>(-1)) return 0;
        BOOL ok = ::GetThreadContext(hT, &ctx);
        ::ResumeThread(hT);
        return ok ? ctx.Rip : 0;
    }

    HANDLE PickThreadInRanges(DWORD pid,
                               const std::vector<NativeCaller::Range>& ranges,
                               DWORD* outTid, int tries, DWORD sleepMs) {
        for (int attempt = 0; attempt < tries; ++attempt) {
            for (DWORD tid : EnumThreadIds(pid)) {
                HANDLE hT = ::OpenThread(
                    THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT |
                    THREAD_QUERY_INFORMATION,
                    FALSE, tid);
                if (!hT) continue;
                const uint64_t rip = SafeGetThreadRip(hT);
                bool hit = false;
                for (const auto& r : ranges) {
                    if (rip >= r.begin && rip < r.end) { hit = true; break; }
                }
                if (hit) { if (outTid) *outTid = tid; return hT; }
                ::CloseHandle(hT);
            }
            ::Sleep(sleepMs);
        }
        return nullptr;
    }

    bool RunBootstrapOnce(HANDLE hProc, HANDLE hThread,
                           uintptr_t bootBase, const ShellcodeBuilder::BootstrapShellcode& boot,
                           uintptr_t dataVA) {
        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_FULL;
        if (::SuspendThread(hThread) == static_cast<DWORD>(-1)) return false;
        if (!::GetThreadContext(hThread, &ctx)) { ::ResumeThread(hThread); return false; }

        const uint64_t origRip = ctx.Rip;
        Mem.WriteRaw(dataVA + boot.D_SAVED_RIP, &origRip, sizeof(origRip));

        ctx.Rip = bootBase;
        BOOL ok = ::SetThreadContext(hThread, &ctx);
        ::ResumeThread(hThread);
        if (!ok) return false;
        ::Sleep(150);
        return true;
    }
}

namespace NativeCaller {

    CNativeCaller g_NativeCaller;

    const char* NameOf(uint64_t hash) {
        const auto& m = Natives::HashToName();
        auto it = m.find(hash);
        return it == m.end() ? xorstr("?") : it->second.data();
    }

    uintptr_t CNativeCaller::GetQueueBase() const {
        if (m_mode == Mode::Citizen) return m_citQueueVA;
        // For other modes: return a base such that base + Q_RESULT (0xC8)
        // lands on the last invocation's 24-byte result slot.
        switch (m_mode) {
            case Mode::MainFn: return m_mainResultVA ? m_mainResultVA - Q_RESULT : 0;
            case Mode::Apc:    return m_apcResultVA  ? m_apcResultVA  - Q_RESULT : 0;
            case Mode::Direct: return m_dcResultVA   ? m_dcResultVA   - Q_RESULT : 0;
            default:           return 0;
        }
    }

    bool CNativeCaller::EnsureReady() {
        if (m_ready) return true;
        if (m_initFailed) return false;
        Initialize();
        if (!m_ready) m_initFailed = true;
        return m_ready;
    }

    void CNativeCaller::Shutdown() {
        if (!m_ready && !m_initFailed) return;

        if (m_citSlotVA && m_citOrigFn && Mem.ProcHandle)
            Mem.WriteProtected<uintptr_t>(m_citSlotVA, m_citOrigFn);

        if (Mem.ProcHandle) {
            if (m_citCaveVA)  { ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_citCaveVA),  0, MEM_RELEASE); }
            if (m_citQueueVA) { ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_citQueueVA), 0, MEM_RELEASE); }
            if (m_mainBase)   { ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_mainBase),   0, MEM_RELEASE); }
            if (m_apcCodeVA)  { ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_apcCodeVA),  0, MEM_RELEASE); }
            if (m_apcDataVA)  { ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_apcDataVA),  0, MEM_RELEASE); }
            if (m_dcBase)     { ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(m_dcBase),     0, MEM_RELEASE); }
        }

        m_citQueueVA = m_citCaveVA = m_citSlotVA = m_citOrigFn = 0;
        m_mainBase = m_mainDataVA = m_mainResultVA = 0;
        m_apcCodeVA = m_apcDataVA = m_apcResultVA = 0;
        m_dcBase = m_dcDataVA = m_dcResultVA = 0;

        m_core = {};
        m_citizen.clear();
        m_citizenIndex.clear();
        m_handlerCache.clear();
        m_unresolvedHash.clear();
        m_coolDownUntil.clear();
        m_dcScriptRanges.clear();
        m_gameImage.clear();
        m_gameImage.shrink_to_fit();
        m_gameImageTried = false;
        m_ready = false;
        m_mode = Mode::Dead;
    }

    int CNativeCaller::DetectBuild() const {
        const std::wstring& n = Mem.ProcName;
        int build = 0;
        for (size_t i = 0; i + 2 < n.size(); ++i) {
            if (n[i] == L'_' && n[i + 1] == L'b' && n[i + 2] >= L'0' && n[i + 2] <= L'9') {
                for (size_t j = i + 2; j < n.size(); ++j) {
                    if (n[j] >= L'0' && n[j] <= L'9')      build = build * 10 + (n[j] - L'0');
                    else if (n[j] == L'_')                 return build;
                    else                                   return 0;
                }
                return build;
            }
        }
        return 0;
    }

    bool CNativeCaller::IsNetworkNative(uint64_t hash) const {
        const auto& hashMap = Natives::HashToName();
        const auto it = hashMap.find(hash);
        if (it == hashMap.end()) return false;
        std::string_view name = it->second;
        return name.size() >= 8 && std::memcmp(name.data(), xorstr("NETWORK_"), 8) == 0;
    }

    uint64_t CNativeCaller::CitizenLookup(uint64_t hash) const {
        auto it = m_citizenIndex.find(hash);
        if (it != m_citizenIndex.end()) return it->second;
        const uint32_t low32 = static_cast<uint32_t>(hash & 0xFFFFFFFFULL);
        it = m_citizenIndex.find(static_cast<uint64_t>(low32));
        if (it != m_citizenIndex.end()) return it->second;
        return 0;
    }

    bool CNativeCaller::EnsureGameImage() {
        if (m_gameImageTried) return !m_gameImage.empty();
        m_gameImageTried = true;
        if (!Mem.ProcHandle || !Mem.ModBase || !Mem.ModBaseSize) return false;

        m_gameImage.assign(Mem.ModBaseSize, 0);
        const size_t CHUNK = 4 * 1024 * 1024;
        size_t off = 0, readTotal = 0;
        while (off < Mem.ModBaseSize) {
            const size_t want = (Mem.ModBaseSize - off < CHUNK) ? (Mem.ModBaseSize - off) : CHUNK;
            SIZE_T got = 0;
            if (::ReadProcessMemory(Mem.ProcHandle, reinterpret_cast<LPCVOID>(Mem.ModBase + off),
                                     m_gameImage.data() + off, want, &got) && got > 0) {
                readTotal += got;
                off += got;
            } else {
                off += want;
            }
        }
        if (g_TraceInvoke)
            DebugLog(xorstr("[NC] game image: %.2f MB usable (of %.2f MB)\n"),
                readTotal / (1024.0 * 1024.0), Mem.ModBaseSize / (1024.0 * 1024.0));
        return readTotal > 0;
    }

    int64_t CNativeCaller::FindInGameImage(const PatternResolver::Pattern& p) const {
        if (p.bytes.empty() || m_gameImage.size() < p.bytes.size()) return -1;
        return PatternResolver::findPatternInBuf(m_gameImage.data(), m_gameImage.size(), p);
    }

    uint64_t CNativeCaller::FollowStub(uintptr_t va) const {
        // Loose check used during traversal — matches main.js `_isValidCodeAddress`.
        // We deliberately don't test manual-mapped-vs-image here because some
        // sibling modules (streaming, adhesive, custom-loaders) may not surface
        // as MEM_IMAGE and rejecting them stalls otherwise-valid stubs.
        const uintptr_t end = Mem.ModBase + Mem.ModBaseSize;
        auto isCodeLoose = [end](uint64_t a) { return a >= 0x10000ULL && a < end; };

        // Strict check applied ONLY to the returned handler VA. Accept
        // MEM_IMAGE outright (normal DLLs) and MEM_PRIVATE only if the
        // region is large enough to plausibly hold a full manual-mapped
        // module (>= 64KB). Excludes our own caveVA/apcCodeVA/dcBase.
        auto isRealCode = [this](uint64_t a) {
            if (a < 0x10000ULL) return false;
            if (m_citCaveVA && a >= m_citCaveVA && a < m_citCaveVA + 0x1000) return false;
            if (m_apcCodeVA && a >= m_apcCodeVA && a < m_apcCodeVA + 0x1000) return false;
            if (m_dcBase    && a >= m_dcBase    && a < m_dcBase    + 0x2000) return false;

            MEMORY_BASIC_INFORMATION mbi{};
            if (!::VirtualQueryEx(Mem.ProcHandle, reinterpret_cast<LPCVOID>(a),
                                    &mbi, sizeof(mbi)))
                return false;
            if (mbi.State != MEM_COMMIT) return false;
            constexpr DWORD kExec = PAGE_EXECUTE | PAGE_EXECUTE_READ |
                                     PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
            if (!(mbi.Protect & kExec)) return false;
            if (mbi.Type == MEM_IMAGE) return true;
            return mbi.RegionSize >= 0x10000;
        };

        // Strictly matches main.js's `_isValidHandlerAddress`: only these six
        // prologue bytes count as a "real handler". Accepting broader bytes
        // (like 40, 41, 53, 56, 57) treats functions with plausible-looking
        // prologues as native handlers and calls them with wrong args, which
        // crashes the game. Reject them.
        auto isHandler = [&](uint64_t a) {
            if (!isCodeLoose(a)) return false;
            const uint8_t b = Mem.Read<uint8_t>(a);
            return b == 0x55 || b == 0x48 || b == 0x4C || b == 0x49 ||
                   b == 0xE9 || b == 0xEB;
        };

        auto finalize = [&](uint64_t a) -> uint64_t {
            return isRealCode(a) ? a : 0;
        };

        if (!isCodeLoose(va)) return 0;

        uint64_t cur = va;
        constexpr int MAX_HOPS = 4;
        uint64_t seen[MAX_HOPS + 1]{};
        int nseen = 0;

        for (int hop = 0; hop < MAX_HOPS; ++hop) {
            for (int i = 0; i < nseen; ++i) if (seen[i] == cur) return finalize(cur);
            seen[nseen++] = cur;

            uint8_t b[32]{};
            for (size_t i = 0; i < 32; ++i) b[i] = Mem.Read<uint8_t>(cur + i);

            if (b[0] == 0x41 && b[1] == 0x51 && b[2] == 0x4C && b[3] == 0x8D && b[4] == 0x0D) {
                int32_t d = 0; std::memcpy(&d, b + 5, 4);
                const uint64_t r = cur + 9ULL + static_cast<int64_t>(d);
                if (isHandler(r)) return finalize(r);
                return finalize(cur);
            }
            if (b[0] == 0xFF && b[1] == 0x25) {
                int32_t d = 0; std::memcpy(&d, b + 2, 4);
                const uint64_t p = cur + 6ULL + static_cast<int64_t>(d);
                const uint64_t t = Mem.Read<uint64_t>(p);
                if (t && isHandler(t)) return finalize(t);
                return finalize(cur);
            }
            if (b[0] == 0xE9) {
                int32_t d = 0; std::memcpy(&d, b + 1, 4);
                const uint64_t n = cur + 5ULL + static_cast<int64_t>(d);
                if (!isCodeLoose(n)) return finalize(cur);
                cur = n;
                continue;
            }
            if (b[0] == 0x48 && b[1] == 0xB8 && b[10] == 0xFF && b[11] == 0xE0) {
                uint64_t t = 0; std::memcpy(&t, b + 2, 8);
                if (isHandler(t)) return finalize(t);
                return finalize(cur);
            }

            for (int off = 1; off <= 4; ++off) {
                if (b[off] == 0x4C && b[off + 1] == 0x8D && b[off + 2] == 0x0D) {
                    const uint8_t prev = b[off - 1];
                    const bool basicPush = (prev >= 0x50 && prev <= 0x57);
                    const bool rexPush   = (off >= 2 && b[off - 2] == 0x41 &&
                                             prev >= 0x50 && prev <= 0x57);
                    if (!basicPush && !rexPush) continue;
                    int32_t d = 0; std::memcpy(&d, b + off + 3, 4);
                    const uint64_t r = cur + static_cast<uint64_t>(off + 7) + static_cast<int64_t>(d);
                    if (isHandler(r)) return finalize(r);
                }
            }

            if (b[9] == 0xE9) {
                int32_t d = 0; std::memcpy(&d, b + 10, 4);
                const uint64_t n = cur + 14ULL + static_cast<int64_t>(d);
                if (isCodeLoose(n)) { cur = n; continue; }
            }

            // Fallback: cur is only a valid handler if its first byte is in
            // the strict prologue set — match main.js's `_isValidHandlerAddress`.
            // Landing on `40 53 ...` etc. means we can't confidently identify
            // this as a handler, so we return 0 (UNRESOLVED) instead of calling
            // an ambiguous function that would crash the game.
            if (isHandler(cur)) return finalize(cur);
            return 0;
        }
        return isHandler(cur) ? finalize(cur) : 0;
    }

    bool CNativeCaller::EnsureCoreImage() {
        if (m_coreImageTried) return !m_coreImage.empty();
        m_coreImageTried = true;
        if (!m_core.valid || !m_core.coreBase || !m_core.coreSize) return false;
        if (!Mem.ProcHandle) return false;

        m_coreImage.assign(m_core.coreSize, 0);
        const size_t CHUNK = 4 * 1024 * 1024;
        size_t off = 0, got = 0;
        while (off < m_core.coreSize) {
            const size_t want = (m_core.coreSize - off < CHUNK) ? (m_core.coreSize - off) : CHUNK;
            SIZE_T r = 0;
            if (::ReadProcessMemory(Mem.ProcHandle,
                                     reinterpret_cast<LPCVOID>(m_core.coreBase + off),
                                     m_coreImage.data() + off, want, &r) && r > 0) {
                got += r; off += r;
            } else {
                off += want;
            }
        }
        if (g_TraceInvoke)
            DebugLog(xorstr("[NC] core image: %.2f MB usable (of %.2f MB)\n"),
                got / (1024.0 * 1024.0), m_core.coreSize / (1024.0 * 1024.0));
        return got > 0;
    }

    static uint64_t ScanImageForHashAndFollow(
        const uint8_t* img, size_t n, uintptr_t moduleBase, uint64_t hash,
        std::function<uint64_t(uintptr_t)> follow) {
        if (n < 8) return 0;
        uint8_t hb[8];
        for (int i = 0; i < 8; ++i) hb[i] = uint8_t(hash >> (i * 8));
        const uint8_t first = hb[0];
        const size_t maxOff = n - 8;

        for (size_t i = 0; i <= maxOff; ++i) {
            if (img[i] != first) continue;
            if (std::memcmp(img + i, hb, 8) != 0) continue;

            const size_t lo = (i >= 48) ? i - 48 : 0;
            const size_t hi = (i + 48 + 6 < n) ? i + 48 : (n - 7);

            for (size_t j = lo; j <= hi; ++j) {
                const bool m48 = (img[j] == 0x48 && img[j + 1] == 0x8D &&
                                   (img[j + 2] == 0x05 || img[j + 2] == 0x0D ||
                                    img[j + 2] == 0x15 || img[j + 2] == 0x1D));
                const bool m4C = (img[j] == 0x4C && img[j + 1] == 0x8D &&
                                   (img[j + 2] == 0x05 || img[j + 2] == 0x0D ||
                                    img[j + 2] == 0x15 || img[j + 2] == 0x1D));
                if (!m48 && !m4C) continue;

                int32_t disp = 0; std::memcpy(&disp, img + j + 3, 4);
                const int64_t targetRva = int64_t(j) + 7 + int64_t(disp);
                if (targetRva < 0 || uint64_t(targetRva) >= n) continue;

                const uintptr_t targetVA = moduleBase + static_cast<uintptr_t>(targetRva);
                const uint64_t handler = follow(targetVA);
                if (handler > 0x10000ULL) return handler;
            }
        }
        return 0;
    }

    uint64_t CNativeCaller::ScanForNativeHandler(uint64_t hash) {
        auto follow = [this](uintptr_t va) { return FollowStub(va); };

        if (EnsureGameImage() && !m_gameImage.empty()) {
            const uint64_t h = ScanImageForHashAndFollow(
                m_gameImage.data(), m_gameImage.size(), Mem.ModBase, hash, follow);
            if (h) return h;
        }
        if (EnsureCoreImage() && !m_coreImage.empty()) {
            const uint64_t h = ScanImageForHashAndFollow(
                m_coreImage.data(), m_coreImage.size(), m_core.coreBase, hash, follow);
            if (h) return h;
        }
        return 0;
    }

    uint64_t CNativeCaller::PatternResolve(uint64_t hash) {
        auto c = m_handlerCache.find(hash);
        if (c != m_handlerCache.end()) return c->second;

        if (!m_build) m_build = DetectBuild();

        const auto& hashMap = Natives::HashToName();
        const auto n = hashMap.find(hash);
        if (n == hashMap.end()) {
            if (g_TraceInvoke) DebugLog(xorstr("[NC] pattern: 0x%llX unknown hash\n"), (unsigned long long)hash);
            m_handlerCache[hash] = 0; return 0;
        }

        if (!EnsureGameImage()) {
            if (g_TraceInvoke) DebugLog(xorstr("[NC] pattern: %s game image unavailable\n"), n->second.data());
            m_handlerCache[hash] = 0; return 0;
        }

        // Preferred: pre-scanned RVA table for b3751 embedded from
        // patterns_b3751.json. `rva` is the pattern-hit VA — we still need
        // FollowStub to unwrap through the stub chain (E9 tail-call, then
        // `48 B8 imm64 FF E0` movabs+jmp, etc.) to reach the real handler.
        // With strict isHandler (only 55/48/4C/49/E9/EB), FollowStub cleanly
        // returns 0 when the chain lands on an ambiguous prologue (40 53...),
        // so we never call a wrong function.
        {
            const auto& rvaMap = Natives::B3751HandlerRvas();
            auto it = rvaMap.find(std::string_view(n->second));
            if (it != rvaMap.end()) {
                const uintptr_t stubVA = Mem.ModBase + it->second;
                const uint64_t handler = FollowStub(stubVA);
                if (handler > 0x10000ULL) {
                    m_handlerCache[hash] = handler;
                    if (g_TraceInvoke)
                        DebugLog(xorstr("[NC] rva: %s stub 0x%llX -> handler=0x%llX\n"),
                            n->second.data(), (unsigned long long)stubVA, (unsigned long long)handler);
                    return handler;
                }
                if (g_TraceInvoke)
                    DebugLog(xorstr("[NC] rva: %s unknown stub @ 0x%llX (skipped)\n"),
                        n->second.data(), (unsigned long long)stubVA);
            }
        }

        // Fallback: on-demand scan for the 8-byte hash literal.
        const uint64_t byScan = ScanForNativeHandler(hash);
        m_handlerCache[hash] = byScan;

        if (g_TraceInvoke) {
            if (byScan)
                DebugLog(xorstr("[NC] scan: %s -> handler=0x%llX\n"),
                    n->second.data(), (unsigned long long)byScan);
            else
                DebugLog(xorstr("[NC] %s UNRESOLVED (no rva, no scan hit)\n"),
                    n->second.data());
        }
        return byScan;
    }

    void CNativeCaller::RefreshModRanges() {
        m_modRanges.clear();
        for (const auto& m : EnumModules(Mem.ProcId))
            m_modRanges.push_back({ m.base, m.base + m.size });
    }

    bool CNativeCaller::ScanCitizenTable() {
        m_core = CitizenNativeCore::Resolve(Mem.ProcHandle, Mem.ProcId);
        if (!m_core.valid) { DebugLog(xorstr("NativeCaller: citizen core resolve failed\n")); return false; }

        const uintptr_t entriesBegin = Mem.Read<uintptr_t>(m_core.nativeTable);
        const uintptr_t entriesEnd   = Mem.Read<uintptr_t>(m_core.nativeTable + 8);
        if (entriesEnd <= entriesBegin) return false;
        const size_t entryCount = (entriesEnd - entriesBegin) / 8;
        if (entryCount < 64) {
            DebugLog(xorstr("NativeCaller: entry count too small (%zu)\n"), entryCount);
            return false;
        }

        m_citizen.clear(); m_citizen.reserve(entryCount);
        m_citizenIndex.clear(); m_citizenIndex.reserve(entryCount * 2);

        for (size_t i = 0; i < entryCount; ++i) {
            const uintptr_t ent = Mem.Read<uintptr_t>(entriesBegin + i * 8);
            if (!ent || ent < 0x10000ULL) continue;
            if (!CitizenNativeCore::IsPlausibleNativeEntry(Mem.ProcHandle, ent)) continue;

            CitizenEntry ce{};
            ce.hash0   = Mem.Read<uint64_t>(ent + E_HASH0);
            ce.hash1   = Mem.Read<uint64_t>(ent + E_HASH1);
            ce.handler = Mem.Read<uint64_t>(ent + E_HANDLER);
            ce.slot    = ent + E_HANDLER;
            m_citizen.push_back(ce);

            if (ce.handler > 0x10000ULL) {
                m_citizenIndex.emplace(ce.hash0,                       ce.handler);
                m_citizenIndex.emplace(ce.hash1,                       ce.handler);
                m_citizenIndex.emplace(ce.hash0 & 0xFFFFFFFFULL,       ce.handler);
                m_citizenIndex.emplace(ce.hash1 & 0xFFFFFFFFULL,       ce.handler);
            }
        }
        return true;
    }

    bool CNativeCaller::TryCitizenMode() {
        if (!ScanCitizenTable()) return false;

        const uint64_t ggtHandler = CitizenLookup(Natives::GET_GAME_TIMER);
        if (!ggtHandler) {
            DebugLog(xorstr("NativeCaller: GET_GAME_TIMER not in citizen table\n"));
            return false;
        }

        const uintptr_t base = Mem.ModBase, end = Mem.ModBase + Mem.ModBaseSize;
        std::vector<std::pair<uintptr_t, uintptr_t>> anchors;
        for (const auto& ce : m_citizen)
            if (ce.handler >= base && ce.handler < end)
                anchors.emplace_back(ce.slot, static_cast<uintptr_t>(ce.handler));
        if (anchors.empty()) { DebugLog(xorstr("NativeCaller: no anchor candidates\n")); return false; }

        const auto sc = ShellcodeBuilder::buildCitizenShellcode();

        uintptr_t queueVA = 0, caveVA = 0, foundSlot = 0, foundOrig = 0;
        for (const auto& [slot, orig] : anchors) {
            if (queueVA) { ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(queueVA), 0, MEM_RELEASE); queueVA = 0; }
            if (caveVA)  { ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(caveVA),  0, MEM_RELEASE); caveVA  = 0; }

            queueVA = reinterpret_cast<uintptr_t>(
                ::VirtualAllocEx(Mem.ProcHandle, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            caveVA = reinterpret_cast<uintptr_t>(
                ::VirtualAllocEx(Mem.ProcHandle, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            if (!queueVA || !caveVA) continue;

            std::vector<uint8_t> code = sc.buffer;
            std::memcpy(code.data() + sc.PATCH_QUEUE,    &queueVA, 8);
            std::memcpy(code.data() + sc.PATCH_ORIGFUNC, &orig,    8);
            if (!Mem.WriteRaw(caveVA, code.data(), code.size())) continue;
            if (!Mem.WriteProtected<uintptr_t>(slot, caveVA))     continue;

            Mem.Write<uint8_t>(queueVA + Q_TRIGGER, 0);
            Mem.Write<uint8_t>(queueVA + Q_DONE,    0);
            Mem.Write<uint64_t>(queueVA + Q_HANDLER,  ggtHandler);
            Mem.Write<uint32_t>(queueVA + Q_ARGCOUNT, 0);
            Mem.Write<uint8_t>(queueVA + Q_TRIGGER, 1);

            bool ok = false;
            for (int w = 0; w < 10; ++w) {
                ::Sleep(10);
                if (Mem.Read<uint8_t>(queueVA + Q_DONE)) {
                    ok = Mem.Read<uint64_t>(queueVA + Q_RESULT) != 0;
                    break;
                }
            }
            Mem.WriteProtected<uintptr_t>(slot, orig);
            if (ok) { foundSlot = slot; foundOrig = orig; break; }
        }

        if (!foundSlot) {
            if (queueVA) ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(queueVA), 0, MEM_RELEASE);
            if (caveVA)  ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(caveVA),  0, MEM_RELEASE);
            DebugLog(xorstr("NativeCaller: no working citizen anchor\n"));
            return false;
        }

        std::vector<uint8_t> finalCode = sc.buffer;
        std::memcpy(finalCode.data() + sc.PATCH_QUEUE,    &queueVA,   8);
        std::memcpy(finalCode.data() + sc.PATCH_ORIGFUNC, &foundOrig, 8);
        Mem.WriteRaw(caveVA, finalCode.data(), finalCode.size());
        Mem.WriteProtected<uintptr_t>(foundSlot, caveVA);

        m_citQueueVA = queueVA;
        m_citCaveVA  = caveVA;
        m_citSlotVA  = foundSlot;
        m_citOrigFn  = foundOrig;
        m_mode       = Mode::Citizen;
        DebugLog(xorstr("NativeCaller: citizen anchor OK @ slot 0x%p\n"), (void*)foundSlot);
        return true;
    }

    bool CNativeCaller::TryMainFnMode() {
        const auto shv = FindShvExports(Mem.ProcHandle, Mem.ProcId);
        if (!shv.complete()) { DebugLog(xorstr("NativeCaller: scripthookv exports not found — skipping MainFn\n")); return false; }

        const auto main = ShellcodeBuilder::buildMainFn();

        // Signature: bytes[0..10) exact, [10..14) wild (RIP-rel disp32 of the FLAG read),
        // [14..22) exact.
        uint8_t sig[22]{}, mask[22]{};
        for (size_t i = 0; i < 22; ++i) {
            sig[i]  = main.buffer[i];
            mask[i] = 1;
        }
        for (size_t i = 10; i < 14; ++i) mask[i] = 0;

        uintptr_t mainBase = FindExistingMainFn(Mem.ProcHandle, sig, mask, sizeof(sig));
        bool reused = mainBase != 0;

        if (!reused) {
            mainBase = reinterpret_cast<uintptr_t>(::VirtualAllocEx(
                Mem.ProcHandle, nullptr, main.buffer.size(),
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            if (!mainBase) { DebugLog(xorstr("NativeCaller: MainFn VirtualAllocEx failed\n")); return false; }

            if (!Mem.WriteRaw(mainBase, main.buffer.data(), main.buffer.size())) {
                ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(mainBase), 0, MEM_RELEASE);
                return false;
            }

            const uintptr_t dataVA0 = mainBase + main.dataOff;
            Mem.Write<uint64_t>(dataVA0 + main.D_INIT, shv.init);
            Mem.Write<uint64_t>(dataVA0 + main.D_PUSH, shv.push);
            Mem.Write<uint64_t>(dataVA0 + main.D_CALL, shv.call);
            Mem.Write<uint64_t>(dataVA0 + main.D_WAIT, shv.wait);

            const auto boot = ShellcodeBuilder::buildBootstrap(shv.reg, mainBase, mainBase);
            constexpr size_t BOOT_STACK = 0x1000;
            const size_t bootTotal = boot.buffer.size() + BOOT_STACK;
            uintptr_t bootBase = reinterpret_cast<uintptr_t>(::VirtualAllocEx(
                Mem.ProcHandle, nullptr, bootTotal,
                MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            if (!bootBase) {
                DebugLog(xorstr("NativeCaller: bootstrap VirtualAllocEx failed\n"));
                ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(mainBase), 0, MEM_RELEASE);
                return false;
            }

            Mem.WriteRaw(bootBase, boot.buffer.data(), boot.buffer.size());
            const uintptr_t stackTop = bootBase + bootTotal;
            Mem.Write<uint64_t>(bootBase + boot.dataOff + boot.D_STACK_TOP, stackTop);

            const std::vector<Range> mainRange = {
                { Mem.ModBase, Mem.ModBase + Mem.ModBaseSize }
            };
            DWORD tid = 0;
            HANDLE hT = PickThreadInRanges(Mem.ProcId, mainRange, &tid, 20, 50);
            if (!hT) {
                DebugLog(xorstr("NativeCaller: no in-game thread found for bootstrap\n"));
                ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(bootBase), 0, MEM_RELEASE);
                ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(mainBase), 0, MEM_RELEASE);
                return false;
            }
            const bool ok = RunBootstrapOnce(Mem.ProcHandle, hT, bootBase, boot, bootBase + boot.dataOff);
            ::CloseHandle(hT);
            if (!ok) {
                DebugLog(xorstr("NativeCaller: bootstrap hijack failed\n"));
                ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(bootBase), 0, MEM_RELEASE);
                ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(mainBase), 0, MEM_RELEASE);
                return false;
            }
            ::Sleep(500);
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(bootBase), 0, MEM_RELEASE);
        }

        m_mainBase   = mainBase;
        m_mainDataVA = mainBase + main.dataOff;
        m_mainMeta   = main;
        m_mainMeta.buffer.clear();
        m_mainMeta.buffer.shrink_to_fit();

        // Probe with GET_GAME_TIMER twice (200ms + 300ms) — matches main.js.
        uint64_t rProbe1 = 0;
        bool probe1Ok = InvokeMainFn(Natives::GET_GAME_TIMER, nullptr, 0, 200, rProbe1);
        bool probe2Ok = probe1Ok;
        if (!probe1Ok) {
            uint64_t rProbe2 = 0;
            probe2Ok = InvokeMainFn(Natives::GET_GAME_TIMER, nullptr, 0, 300, rProbe2);
        }
        if (!probe1Ok && !probe2Ok) {
            DebugLog(xorstr("NativeCaller: MainFn silent (%s)\n"), reused ? xorstr("reused") : xorstr("fresh"));
            if (!reused) ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(mainBase), 0, MEM_RELEASE);
            m_mainBase = m_mainDataVA = 0;
            return false;
        }

        m_mode = Mode::MainFn;
        DebugLog(xorstr("NativeCaller: MainFn active @ 0x%p (%s)\n"),
                 (void*)mainBase, reused ? xorstr("reused") : xorstr("fresh"));
        return true;
    }

    bool CNativeCaller::TryApcMode() {
        const auto ac = ShellcodeBuilder::buildApcCall();

        const uintptr_t codeVA = reinterpret_cast<uintptr_t>(::VirtualAllocEx(
            Mem.ProcHandle, nullptr, ac.buffer.size() + 64,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!codeVA) return false;

        if (!Mem.WriteRaw(codeVA, ac.buffer.data(), ac.buffer.size())) {
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(codeVA), 0, MEM_RELEASE);
            return false;
        }
        std::vector<uint8_t> verify(ac.buffer.size(), 0);
        if (!Mem.ReadRaw(codeVA, verify.data(), verify.size()) ||
            std::memcmp(verify.data(), ac.buffer.data(), verify.size()) != 0) {
            DebugLog(xorstr("NativeCaller: APC shellcode write verification failed\n"));
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(codeVA), 0, MEM_RELEASE);
            return false;
        }

        const uintptr_t dataVA = reinterpret_cast<uintptr_t>(::VirtualAllocEx(
            Mem.ProcHandle, nullptr, ac.D_SIZE + 64,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (!dataVA) {
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(codeVA), 0, MEM_RELEASE);
            return false;
        }

        m_apcCodeVA = codeVA;
        m_apcDataVA = dataVA;
        m_apcMeta   = ac;
        m_apcMeta.buffer.clear();
        m_apcMeta.buffer.shrink_to_fit();

        const uint64_t ggtHandler = CitizenLookup(Natives::GET_GAME_TIMER);
        if (!ggtHandler) {
            DebugLog(xorstr("NativeCaller: APC probe skipped — GET_GAME_TIMER handler unknown\n"));
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(codeVA), 0, MEM_RELEASE);
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(dataVA), 0, MEM_RELEASE);
            m_apcCodeVA = m_apcDataVA = 0;
            return false;
        }

        uint64_t probe = 0;
        const bool probeOk = InvokeApc(ggtHandler, nullptr, 0, 5000, probe);
        if (!probeOk) {
            DebugLog(xorstr("NativeCaller: APC probe returned 0\n"));
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(codeVA), 0, MEM_RELEASE);
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(dataVA), 0, MEM_RELEASE);
            m_apcCodeVA = m_apcDataVA = 0;
            return false;
        }

        m_mode = Mode::Apc;
        DebugLog(xorstr("NativeCaller: APC active (code=0x%p data=0x%p)\n"), (void*)codeVA, (void*)dataVA);
        return true;
    }

    bool CNativeCaller::TryDirectMode() {
        const auto dc = ShellcodeBuilder::buildDirectCall();

        constexpr size_t DC_STACK = 0x1000;
        const size_t total = dc.buffer.size() + DC_STACK;
        const uintptr_t base = reinterpret_cast<uintptr_t>(::VirtualAllocEx(
            Mem.ProcHandle, nullptr, total,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!base) return false;

        if (!Mem.WriteRaw(base, dc.buffer.data(), dc.buffer.size())) {
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(base), 0, MEM_RELEASE);
            return false;
        }
        const uintptr_t dataVA = base + dc.dataOff;
        const uintptr_t stackTop = base + total;
        Mem.Write<uint64_t>(dataVA + dc.D_STACK_TOP, stackTop);

        // Collect script-thread ranges.
        m_dcScriptRanges.clear();
        for (const auto& m : EnumModules(Mem.ProcId)) {
            if (IsScriptModule(m.name))
                m_dcScriptRanges.push_back({ m.base, m.base + m.size });
        }
        if (m_dcScriptRanges.empty()) {
            DebugLog(xorstr("NativeCaller: no script-thread modules found\n"));
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(base), 0, MEM_RELEASE);
            return false;
        }

        m_dcBase   = base;
        m_dcDataVA = dataVA;
        m_dcMeta   = dc;
        m_dcMeta.buffer.clear();
        m_dcMeta.buffer.shrink_to_fit();

        const uint64_t ggtHandler = CitizenLookup(Natives::GET_GAME_TIMER);
        if (!ggtHandler) {
            DebugLog(xorstr("NativeCaller: Direct probe skipped — GET_GAME_TIMER handler unknown\n"));
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(base), 0, MEM_RELEASE);
            m_dcBase = m_dcDataVA = 0;
            m_dcScriptRanges.clear();
            return false;
        }

        uint64_t probe = 0;
        const bool probeOk = InvokeDirect(ggtHandler, nullptr, 0, 3000, probe);
        if (!probeOk) {
            DebugLog(xorstr("NativeCaller: Direct probe returned 0\n"));
            ::VirtualFreeEx(Mem.ProcHandle, reinterpret_cast<LPVOID>(base), 0, MEM_RELEASE);
            m_dcBase = m_dcDataVA = 0;
            m_dcScriptRanges.clear();
            return false;
        }

        m_mode = Mode::Direct;
        DebugLog(xorstr("NativeCaller: Direct-call active @ 0x%p\n"), (void*)base);
        return true;
    }

    void CNativeCaller::Initialize() {
        if (m_ready) return;
        if (!Mem.ProcId || !Mem.ProcHandle) {
            DebugLog(xorstr("NativeCaller: process not open\n"));
            return;
        }
        if (!m_build) m_build = DetectBuild();

        // Cache loaded-module ranges — FollowStub uses these to validate that
        // stub targets fall inside real executable code (not heap addresses
        // VirtualAllocEx returns in the same VA range).
        RefreshModRanges();

        // Every mode benefits from the citizen table (for GET_GAME_TIMER lookup
        // and pattern-scan-free handler resolution) — scan it once up front.
        ScanCitizenTable();

        if (TryCitizenMode())  { m_ready = true; DebugLog(xorstr("NativeCaller: mode=citizen build=%d\n"), m_build); return; }
        if (TryMainFnMode())   { m_ready = true; DebugLog(xorstr("NativeCaller: mode=mainfn  build=%d\n"), m_build); return; }
        if (TryApcMode())      { m_ready = true; DebugLog(xorstr("NativeCaller: mode=apc     build=%d\n"), m_build); return; }
        if (TryDirectMode())   { m_ready = true; DebugLog(xorstr("NativeCaller: mode=direct  build=%d\n"), m_build); return; }
        DebugLog(xorstr("NativeCaller: all four modes failed\n"));
    }

    bool CNativeCaller::Probe(int timeoutMs) {
        if (!m_ready) return false;
        std::lock_guard<std::mutex> lk(m_mtx);
        const std::vector<uint64_t> no;
        return InvokeRaw(Natives::PLAYER_PED_ID, no, timeoutMs) != 0;
    }

    uint64_t CNativeCaller::Invoke(uint64_t hash,
                                    std::initializer_list<uint64_t> args,
                                    int timeoutMs) {
        std::lock_guard<std::mutex> lk(m_mtx);
        if (!m_ready) return 0;
        const std::vector<uint64_t> v(args.begin(), args.end());
        return InvokeRaw(hash, v, timeoutMs);
    }

    uint64_t CNativeCaller::InvokeRaw(uint64_t hash, const std::vector<uint64_t>& args, int timeoutMs) {
        if (args.size() > 8) return 0;

        const char* name = NameOf(hash);
        const bool citizenMode = (m_mode == Mode::Citizen);

        uint64_t handler = 0;
        const char* src = xorstr("mainfn");
        if (citizenMode || m_mode == Mode::Apc || m_mode == Mode::Direct) {
            handler = CitizenLookup(hash);
            src = xorstr("citizen");
            if (!handler) {
                handler = PatternResolve(hash);
                src = xorstr("pattern");
            }
            if (!handler) {
                // main.js silently returns zeros for unresolved NETWORK_*
                // natives; do the same to keep vehicle-spawn flows going.
                if (IsNetworkNative(hash)) {
                    if (g_TraceInvoke) DebugLog(xorstr("[NC] skip unresolved networking native %s\n"), name);
                } else {
                    if (g_TraceInvoke) DebugLog(xorstr("[NC] %s(0x%llX) UNRESOLVED\n"), name, (unsigned long long)hash);
                }
                return 0;
            }
        }

        if (g_TraceInvoke) {
            char argStr[192]{};
            size_t off = 0;
            for (size_t i = 0; i < args.size() && off + 20 < sizeof(argStr); ++i) {
                int w = _snprintf_s(argStr + off, sizeof(argStr) - off, _TRUNCATE,
                    i ? xorstr(", 0x%llX") : xorstr("0x%llX"), (unsigned long long)args[i]);
                if (w <= 0) break;
                off += w;
            }
            DebugLog(xorstr("[NC] -> %s [%s] handler=0x%llX args=[%s]\n"),
                name, src, (unsigned long long)handler, argStr);
        }

        const uint64_t* argsPtr = args.empty() ? nullptr : args.data();
        const size_t    nargs   = args.size();
        const auto      t0      = std::chrono::steady_clock::now();

        uint64_t r = 0;
        bool ok = false;
        switch (m_mode) {
            case Mode::Citizen: ok = InvokeCitizen(hash, handler, argsPtr, nargs, timeoutMs, r); break;
            case Mode::MainFn:  ok = InvokeMainFn (hash,          argsPtr, nargs, timeoutMs, r); break;
            case Mode::Apc:     ok = InvokeApc    (handler,       argsPtr, nargs, timeoutMs, r); break;
            case Mode::Direct:  ok = InvokeDirect (handler,       argsPtr, nargs, timeoutMs, r); break;
            default: return 0;
        }

        if (ok) {
            if (g_TraceInvoke) {
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - t0).count();
                DebugLog(xorstr("[NC] <- %s = 0x%llX (%lldms)\n"),
                    name, (unsigned long long)r, (long long)ms);
            }
            return r;
        }

        if (g_TraceInvoke)
            DebugLog(xorstr("[NC] !! %s TIMEOUT (%dms)\n"), name, timeoutMs);
        return 0;
    }

    bool CNativeCaller::InvokeCitizen(uint64_t /*hash*/, uint64_t handler,
                                       const uint64_t* args, size_t nargs, int timeoutMs,
                                       uint64_t& outResult) {
        // Match main.js write order: HANDLER, ARGS, ARGCOUNT, RESULT clear,
        // DONE=0, TRIGGER=1 last.
        Mem.Write<uint64_t>(m_citQueueVA + Q_HANDLER, handler);

        uint64_t argBuf[8]{};
        for (size_t i = 0; i < nargs; ++i) argBuf[i] = args[i];
        Mem.WriteRaw(m_citQueueVA + Q_ARGS, argBuf, sizeof(argBuf));

        Mem.Write<uint32_t>(m_citQueueVA + Q_ARGCOUNT, static_cast<uint32_t>(nargs));

        uint8_t clr[24]{};
        Mem.WriteRaw(m_citQueueVA + Q_RESULT, clr, sizeof(clr));

        Mem.Write<uint8_t>(m_citQueueVA + Q_DONE,     0);
        Mem.Write<uint8_t>(m_citQueueVA + Q_TRIGGER,  1);

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (std::chrono::steady_clock::now() < deadline) {
            if (Mem.Read<uint8_t>(m_citQueueVA + Q_DONE)) {
                outResult = Mem.Read<uint64_t>(m_citQueueVA + Q_RESULT);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        outResult = 0;
        return false;
    }

    bool CNativeCaller::InvokeMainFn(uint64_t hash,
                                      const uint64_t* args, size_t nargs, int timeoutMs,
                                      uint64_t& outResult) {
        const uintptr_t data = m_mainDataVA;
        const auto&    m    = m_mainMeta;

        // header: [D_HASH]=hash u64, [D_ARGCOUNT]=nargs u64 — as one 16-byte block.
        uint8_t header[16]{};
        std::memcpy(header + 0, &hash,   8);
        const uint64_t argCountU64 = nargs;
        std::memcpy(header + 8, &argCountU64, 8);
        Mem.WriteRaw(data + m.D_HASH, header, sizeof(header));

        uint64_t argBuf[8]{};
        for (size_t i = 0; i < nargs; ++i) argBuf[i] = args[i];
        Mem.WriteRaw(data + m.D_ARG, argBuf, sizeof(argBuf));

        uint8_t clr[24]{};
        Mem.WriteRaw(data + m.D_RESULT, clr, sizeof(clr));

        const uintptr_t doneVA = data + m.D_DONE;
        const uintptr_t flagVA = data + m.D_FLAG;
        const uint64_t before  = Mem.Read<uint64_t>(doneVA);

        Mem.Write<uint8_t>(flagVA, 1);

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (std::chrono::steady_clock::now() < deadline) {
            const uint64_t now = Mem.Read<uint64_t>(doneVA);
            if (now != before) {
                m_mainResultVA = data + m.D_RESULT;
                outResult = Mem.Read<uint64_t>(data + m.D_RESULT);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        outResult = 0;
        return false;
    }

    bool CNativeCaller::InvokeApc(uint64_t handler,
                                   const uint64_t* args, size_t nargs, int timeoutMs,
                                   uint64_t& outResult) {
        const uintptr_t data = m_apcDataVA;
        const auto&    ac   = m_apcMeta;

        uint8_t header[16]{};
        std::memcpy(header + 0, &handler, 8);
        const uint64_t argCountU64 = nargs;
        std::memcpy(header + 8, &argCountU64, 8);
        Mem.WriteRaw(data + ac.D_HANDLER, header, sizeof(header));

        uint64_t argBuf[8]{};
        for (size_t i = 0; i < nargs; ++i) argBuf[i] = args[i];
        Mem.WriteRaw(data + ac.D_ARG, argBuf, sizeof(argBuf));

        uint8_t clr[24]{};
        Mem.WriteRaw(data + ac.D_RESULT, clr, sizeof(clr));

        const uintptr_t doneVA    = data + ac.D_DONE;
        const uintptr_t pendingVA = data + ac.D_PENDING;
        const uint64_t before     = Mem.Read<uint64_t>(doneVA);

        Mem.Write<uint32_t>(pendingVA, 1);

        NtAlertThread_t Alert = GetNtAlertThread();
        bool queued = false;
        for (DWORD tid : EnumThreadIds(Mem.ProcId)) {
            HANDLE hT = ::OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
            if (!hT) continue;
            if (::QueueUserAPC(reinterpret_cast<PAPCFUNC>(m_apcCodeVA), hT,
                                 static_cast<ULONG_PTR>(data))) queued = true;
            if (Alert) Alert(hT);
            ::CloseHandle(hT);
        }
        if (!queued) { outResult = 0; return false; }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (std::chrono::steady_clock::now() < deadline) {
            const uint64_t now = Mem.Read<uint64_t>(doneVA);
            if (now != before) {
                m_apcResultVA = data + ac.D_RESULT;
                outResult = Mem.Read<uint64_t>(data + ac.D_RESULT);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        outResult = 0;
        return false;
    }

    bool CNativeCaller::InvokeDirect(uint64_t handler,
                                      const uint64_t* args, size_t nargs, int timeoutMs,
                                      uint64_t& outResult) {
        const uintptr_t data = m_dcDataVA;
        const auto&    dc   = m_dcMeta;

        Mem.Write<uint64_t>(data + dc.D_HANDLER,  handler);
        Mem.Write<uint64_t>(data + dc.D_ARGCOUNT, static_cast<uint64_t>(nargs));

        uint64_t argBuf[8]{};
        for (size_t i = 0; i < nargs; ++i) argBuf[i] = args[i];
        Mem.WriteRaw(data + dc.D_ARG, argBuf, sizeof(argBuf));

        uint8_t clr[24]{};
        Mem.WriteRaw(data + dc.D_RESULT, clr, sizeof(clr));

        const uintptr_t doneVA = data + dc.D_DONE;
        const uint64_t before  = Mem.Read<uint64_t>(doneVA);

        DWORD tid = 0;
        HANDLE hT = PickThreadInRanges(Mem.ProcId, m_dcScriptRanges, &tid, 40, 25);
        if (!hT) { outResult = 0; return false; }

        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_FULL;
        if (::SuspendThread(hT) == static_cast<DWORD>(-1)) { ::CloseHandle(hT); outResult = 0; return false; }
        if (!::GetThreadContext(hT, &ctx)) { ::ResumeThread(hT); ::CloseHandle(hT); outResult = 0; return false; }

        const uint64_t origRip = ctx.Rip;
        Mem.Write<uint64_t>(data + dc.D_SAVED_RIP, origRip);
        ctx.Rip = m_dcBase;
        BOOL ok = ::SetThreadContext(hT, &ctx);
        ::ResumeThread(hT);
        ::CloseHandle(hT);
        if (!ok) { outResult = 0; return false; }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        while (std::chrono::steady_clock::now() < deadline) {
            const uint64_t now = Mem.Read<uint64_t>(doneVA);
            if (now != before) {
                m_dcResultVA = data + dc.D_RESULT;
                outResult = Mem.Read<uint64_t>(data + dc.D_RESULT);
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        outResult = 0;
        return false;
    }
}
