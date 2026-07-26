#pragma once

#include "Invoker.hpp"
#include <Security/xorstr.hpp>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <windows.h>

namespace NativeCaller {

    inline std::atomic<bool> g_InvokerBootStarted{false};
    inline std::atomic<bool> g_InvokerBootDone{false};

    inline bool BootInvoker(uint32_t pid) {
        if (g_InvokerBootDone.load()) return true;
        bool expected = false;
        if (!g_InvokerBootStarted.compare_exchange_strong(expected, true)) {
            return g_InvokerBootDone.load();
        }

        Invoker::EnableLogging(true);
        if (!Invoker::AttachExternal(pid)) {
            std::fprintf(stderr, xorstr("[Invoker] AttachExternal(%u) failed\n"), pid);
            g_InvokerBootStarted.store(false);
            return false;
        }
        if (!Invoker::Initialize()) {
            std::fprintf(stderr, xorstr("[Invoker] Initialize failed\n"));
            g_InvokerBootStarted.store(false);
            return false;
        }
        if (!Invoker::Scan()) {
            std::fprintf(stderr, xorstr("[Invoker] Scan failed\n"));
            g_InvokerBootStarted.store(false);
            return false;
        }
        if (!Invoker::InstallCitizenHook()) {
            std::fprintf(stderr, xorstr("[Invoker] InstallCitizenHook failed\n"));
            g_InvokerBootStarted.store(false);
            return false;
        }
        std::fprintf(stderr, xorstr("[Invoker] ready: %u natives, queue @ 0x%llx\n"),
            Invoker::NativeCount(), (unsigned long long)Invoker::CitizenQueueBase());
        g_InvokerBootDone.store(true);
        return true;
    }

    struct NativeCallerShim {
        bool EnsureReady() {
            return g_InvokerBootDone.load() && Invoker::IsCitizenHookInstalled();
        }

        bool IsReady() const {
            return Invoker::IsCitizenHookInstalled();
        }

        uintptr_t GetQueueBase() const {
            return Invoker::CitizenQueueBase();
        }

        template <typename... Args>
        uint64_t Invoke(uint64_t hash, Args... args) {
            const uint64_t* r = Invoker::Call(hash, args...);
            return r ? r[0] : 0ull;
        }

        uint64_t Invoke(uint64_t hash, std::initializer_list<uint64_t> args, uint32_t = 0) {
            const uint64_t* r = Invoker::Invoke(hash, args.begin(), static_cast<uint32_t>(args.size()));
            return r ? r[0] : 0ull;
        }

        template <typename... Args>
        uint64_t InvokeByName(const std::string& name, Args... args) {
            const uint64_t* r = Invoker::CallByName(name, args...);
            return r ? r[0] : 0ull;
        }
    };

    inline NativeCallerShim g_NativeCaller;

}
