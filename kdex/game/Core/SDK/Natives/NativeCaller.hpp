#pragma once

#include <Core/SDK/Natives/B3751HandlerRvas.hpp>
#include <Core/SDK/Natives/CitizenNativeCore.hpp>
#include <Core/SDK/Natives/CrossmapNatives.hpp>
#include <Core/SDK/Natives/NativeHashNames.hpp>
#include <Core/SDK/Natives/Natives.hpp>
#include <Core/SDK/Natives/PatternResolver.hpp>
#include <Core/SDK/Natives/ShellcodeBuilder.hpp>
#include <Core/SDK/Memory.hpp>
#include <Core/SDK/DebugLog.hpp>
#include <Security/xorstr.hpp>

#include <cstdint>
#include <cstring>
#include <chrono>
#include <initializer_list>
#include <mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <Windows.h>

using namespace Core;

namespace NativeCaller {

    inline bool g_TraceInvoke = true;

    enum class Mode : int { Dead, Citizen, MainFn, Apc, Direct };

    constexpr size_t Q_TRIGGER  = 0x00;
    constexpr size_t Q_DONE     = 0x01;
    constexpr size_t Q_HANDLER  = 0x08;
    constexpr size_t Q_ARGCOUNT = 0x18;
    constexpr size_t Q_ARGS     = 0x20;
    constexpr size_t Q_RESULT   = 0xC8;

    constexpr size_t E_HASH0    = 0x00;
    constexpr size_t E_HASH1    = 0x08;
    constexpr size_t E_HANDLER  = 0x18;

    struct CitizenEntry {
        uint64_t  hash0;
        uint64_t  hash1;
        uint64_t  handler;
        uintptr_t slot;
    };

    struct SHVExports {
        uint64_t reg  = 0;
        uint64_t wait = 0;
        uint64_t init = 0;
        uint64_t push = 0;
        uint64_t call = 0;
        bool complete() const { return reg && wait && init && push && call; }
    };

    struct Range { uintptr_t begin; uintptr_t end; };

    const char* NameOf(uint64_t hash);

    class CNativeCaller {
    public:
        bool IsReady()  const { return m_ready; }
        Mode GetMode()  const { return m_mode; }

        HANDLE   GetHookProc()        const { return Core::Mem.ProcHandle; }
        DWORD    GetHookPid()         const { return Core::Mem.ProcId; }

        // Legacy API preserved. In citizen mode this is the citizen queue base;
        // in every other mode it's synthesised so `GetQueueBase() + Q_RESULT`
        // points at the last invocation's 24-byte result slot.
        uintptr_t GetQueueBase()      const;

        uintptr_t GetAnchorSlotAddr() const { return m_citSlotVA; }
        const CitizenNativeCore::SCitizenCore& GetCitizenCore() const { return m_core; }

        bool EnsureReady();
        void Shutdown();
        void Initialize();
        bool Probe(int timeoutMs = 2500);

        uint64_t Invoke(uint64_t hash,
                        std::initializer_list<uint64_t> args = {},
                        int timeoutMs = 400);

        template<typename First, typename... Rest>
        std::enable_if_t<!std::is_same_v<std::decay_t<First>, std::initializer_list<uint64_t>>, uint64_t>
        Invoke(uint64_t hash, First first, Rest... rest) {
            std::lock_guard<std::mutex> lk(m_mtx);
            if (!m_ready) return 0;
            std::vector<uint64_t> packed;
            packed.reserve(1 + sizeof...(Rest));
            packed.push_back(PackArg(first));
            (packed.push_back(PackArg(rest)), ...);
            return InvokeRaw(hash, packed, 400);
        }

        template<typename T>
        static uint64_t PackArg(T value) {
            using U = std::decay_t<T>;
            if constexpr (std::is_same_v<U, float>) {
                uint64_t bits = 0; std::memcpy(&bits, &value, sizeof(value)); return bits;
            } else if constexpr (std::is_same_v<U, double>) {
                uint64_t bits = 0; std::memcpy(&bits, &value, sizeof(value)); return bits;
            } else if constexpr (std::is_same_v<U, bool>) {
                return value ? 1ULL : 0ULL;
            } else if constexpr (std::is_same_v<U, uint64_t>) {
                return value;
            } else if constexpr (std::is_same_v<U, int64_t>) {
                return static_cast<uint64_t>(value);
            } else if constexpr (std::is_pointer_v<U>) {
                return reinterpret_cast<uint64_t>(value);
            } else if constexpr (std::is_integral_v<U>) {
                return static_cast<uint64_t>(static_cast<int64_t>(value));
            } else if constexpr (std::is_enum_v<U>) {
                return static_cast<uint64_t>(static_cast<std::underlying_type_t<U>>(value));
            } else {
                return static_cast<uint64_t>(value);
            }
        }

    private:
        Mode m_mode        = Mode::Dead;
        bool m_ready       = false;
        bool m_initFailed  = false;
        int  m_build       = 0;
        mutable std::mutex m_mtx;

        CitizenNativeCore::SCitizenCore                                       m_core{};
        std::vector<CitizenEntry>                                             m_citizen;
        std::unordered_map<uint64_t, uint64_t>                                m_citizenIndex;
        std::unordered_map<uint64_t, uint64_t>                                m_handlerCache;
        std::unordered_set<uint64_t>                                          m_unresolvedHash;
        std::unordered_map<uint64_t, std::chrono::steady_clock::time_point>   m_coolDownUntil;

        std::vector<uint8_t> m_gameImage;
        bool                 m_gameImageTried = false;

        // citizen-scripting-core.dll image — where FiveM registers natives
        // with BOTH the 1.68 hash and the current-build hash inline. Scanning
        // this catches natives that aren't in the citizen native table.
        std::vector<uint8_t> m_coreImage;
        bool                 m_coreImageTried = false;

        // Cached ranges of every module loaded in the target process — used
        // to validate stub-follow targets fall inside real executable code
        // (not heap allocations that VirtualAllocEx returns in the same VA
        // space).
        std::vector<Range>   m_modRanges;

        // ── citizen mode ────
        uintptr_t m_citQueueVA = 0;
        uintptr_t m_citCaveVA  = 0;
        uintptr_t m_citSlotVA  = 0;
        uintptr_t m_citOrigFn  = 0;

        // ── mainfn mode ─────
        uintptr_t                          m_mainBase   = 0;
        uintptr_t                          m_mainDataVA = 0;
        ShellcodeBuilder::MainFnShellcode  m_mainMeta{};
        uintptr_t                          m_mainResultVA = 0;

        // ── apc mode ────────
        uintptr_t                            m_apcCodeVA = 0;
        uintptr_t                            m_apcDataVA = 0;
        ShellcodeBuilder::ApcCallShellcode   m_apcMeta{};
        uintptr_t                            m_apcResultVA = 0;

        // ── direct mode ─────
        uintptr_t                              m_dcBase   = 0;
        uintptr_t                              m_dcDataVA = 0;
        ShellcodeBuilder::DirectCallShellcode  m_dcMeta{};
        std::vector<Range>                     m_dcScriptRanges;
        uintptr_t                              m_dcResultVA = 0;

        static constexpr std::chrono::milliseconds kCooldownAfterTimeout{ 1500 };

        // ── mode helpers ────
        bool     TryCitizenMode();
        bool     TryMainFnMode();
        bool     TryApcMode();
        bool     TryDirectMode();

        bool     ScanCitizenTable();
        void     RefreshModRanges();

        // On-demand scan: search m_gameImage for the 8-byte hash literal, then
        // look for a nearby LEA rax/rcx/r__ [rip+disp32] that references a
        // handler stub. Returns the resolved handler VA or 0.
        uint64_t ScanForNativeHandler(uint64_t hash);
        uint64_t CitizenLookup(uint64_t hash) const;
        uint64_t PatternResolve(uint64_t hash);
        uint64_t FollowStub(uintptr_t va) const;
        bool     EnsureGameImage();
        bool     EnsureCoreImage();
        int64_t  FindInGameImage(const PatternResolver::Pattern& p) const;
        int      DetectBuild() const;
        bool     IsNetworkNative(uint64_t hash) const;

        // ── dispatch ────────
        // The Invoke* mode drivers return true if the shellcode signalled DONE
        // in time (a legitimate 0 result is `outResult=0, return true`), false
        // on genuine timeout.
        uint64_t InvokeRaw(uint64_t hash, const std::vector<uint64_t>& args, int timeoutMs);
        bool     InvokeCitizen(uint64_t hash, uint64_t handler, const uint64_t* args, size_t nargs, int timeoutMs, uint64_t& outResult);
        bool     InvokeMainFn (uint64_t hash,                   const uint64_t* args, size_t nargs, int timeoutMs, uint64_t& outResult);
        bool     InvokeApc    (uint64_t handler,                const uint64_t* args, size_t nargs, int timeoutMs, uint64_t& outResult);
        bool     InvokeDirect (uint64_t handler,                const uint64_t* args, size_t nargs, int timeoutMs, uint64_t& outResult);
    };

    extern CNativeCaller g_NativeCaller;
}

using NativeCaller::g_NativeCaller;
