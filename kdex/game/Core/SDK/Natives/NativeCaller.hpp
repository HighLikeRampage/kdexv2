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

    inline std::string ResolveInvokerPath(const wchar_t* leaf) {
        wchar_t exePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        std::wstring dir = exePath;
        size_t slash = dir.find_last_of(xorstr(L"\\/"));
        if (slash != std::wstring::npos) dir.resize(slash);
        for (int i = 0; i < 8; ++i) {
            std::wstring c1 = dir + xorstr(L"\\") + leaf;
            if (GetFileAttributesW(c1.c_str()) != INVALID_FILE_ATTRIBUTES) {
                char n[MAX_PATH] = {};
                WideCharToMultiByte(CP_UTF8, 0, c1.c_str(), -1, n, MAX_PATH, nullptr, nullptr);
                return n;
            }
            std::wstring c2 = dir + xorstr(L"\\kdex\\") + leaf;
            if (GetFileAttributesW(c2.c_str()) != INVALID_FILE_ATTRIBUTES) {
                char n[MAX_PATH] = {};
                WideCharToMultiByte(CP_UTF8, 0, c2.c_str(), -1, n, MAX_PATH, nullptr, nullptr);
                return n;
            }
            size_t up = dir.find_last_of(xorstr(L"\\/"));
            if (up == std::wstring::npos) break;
            dir.resize(up);
        }
        return {};
    }

    inline std::atomic<bool> g_InvokerBootStarted{false};
    inline std::atomic<bool> g_InvokerBootDone{false};

    inline bool BootInvoker(uint32_t pid) {
        if (g_InvokerBootDone.load()) return true;
        bool expected = false;
        if (!g_InvokerBootStarted.compare_exchange_strong(expected, true)) {
            return g_InvokerBootDone.load();
        }
        std::string nativesPath = ResolveInvokerPath(xorstr(L"game\\Core\\SDK\\Natives\\Natives.hpp"));
        if (nativesPath.empty()) nativesPath = ResolveInvokerPath(xorstr(L"Natives.hpp"));
        std::string crossmapPath = ResolveInvokerPath(xorstr(L"game\\Core\\SDK\\Natives\\Crossmap.hpp"));
        if (crossmapPath.empty()) crossmapPath = ResolveInvokerPath(xorstr(L"Crossmap.hpp"));
        if (!nativesPath.empty()) Invoker::SetNativesHppPath(nativesPath);
        if (!crossmapPath.empty()) Invoker::SetCrossmapPath(crossmapPath);
        std::fprintf(stderr, xorstr("[Invoker] using natives=%s crossmap=%s\n"),
            nativesPath.empty() ? xorstr("<default>") : nativesPath.c_str(),
            crossmapPath.empty() ? xorstr("<default>") : crossmapPath.c_str());

        Invoker::EnableLogging(true);
        if (!Invoker::AttachExternal(pid)) {
            std::fprintf(stderr, xorstr("[Invoker] AttachExternal(%u) failed\n"), pid);
            g_InvokerBootStarted.store(false);
            return false;
        }
        if (!Invoker::Initialize()) {
            std::fprintf(stderr, xorstr("[Invoker] Initialize failed (missing Natives.hpp / Crossmap.hpp?)\n"));
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

        uint64_t Invoke(uint64_t hash, std::initializer_list<uint64_t> args, uint32_t /*timeoutMs*/ = 0) {
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
