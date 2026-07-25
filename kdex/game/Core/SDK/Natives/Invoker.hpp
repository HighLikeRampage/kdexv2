#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <windows.h>

namespace Invoker {

    struct alignas(16) NativeContext {
        void* Results;
        uint32_t NumArgs;
        uint32_t NumResults;
        void* Args;
        uint32_t _reserved[8];
    };

    using NativeHandler = void(*)(NativeContext*);

    struct Section {
        std::string name;
        uint32_t    va;
        uint32_t    vsize;
        uint32_t    rsize;
        uint32_t    characteristics;
    };

    struct NativeInfo {
        uint64_t      hash;
        uint32_t      rva;
        NativeHandler handler;
    };

    bool AttachExternal(uint32_t pid);
    bool AttachExternalByName(const std::string& processHint);
    void DetachExternal();
    bool IsExternal();

    bool InstallCitizenHook();
    bool IsCitizenHookInstalled();
    void UninstallCitizenHook();

    void LoadBlacklist(const std::string& path);
    void SaveBlacklist(const std::string& path);
    void AddToBlacklist(uint64_t hash);
    bool IsBlacklisted(uint64_t hash);
    bool IsProcessAlive();

    uintptr_t AllocRemoteString(const std::string& s);
    void      FreeRemote(uintptr_t va);

    bool Initialize();
    bool Scan();
    void Refresh();

    uint64_t Joaat(std::string_view name);
    uint64_t NativeHashFromName(const std::string& name);
    uint64_t ParseHash(const std::string& s);
    uint64_t ParseArg(const std::string& s);

    uint64_t ArgU32(uint32_t v);
    uint64_t ArgU64(uint64_t v);
    uint64_t ArgF32(float f);
    uint64_t ArgBool(bool b);

    NativeHandler     GetHandler(uint64_t hash);
    NativeHandler     GetHandlerByName(const std::string& name);
    const NativeInfo* GetInfo(uint64_t hash);
    const NativeInfo* GetInfoByName(const std::string& name);

    const uint64_t* Invoke(uint64_t hash, const uint64_t* args, uint32_t argCount);
    const uint64_t* InvokeByName(const std::string& name, const uint64_t* args, uint32_t argCount);

    template<typename T>
    inline uint64_t CastArg(T v) {
        if constexpr (std::is_same_v<T, float>) {
            uint32_t bits = 0;
            std::memcpy(&bits, &v, sizeof(v));
            return static_cast<uint64_t>(bits);
        }
        else if constexpr (std::is_same_v<T, double>) {
            uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof(v));
            return bits;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return v ? 1ull : 0ull;
        }
        else if constexpr (std::is_pointer_v<T>) {
            return reinterpret_cast<uint64_t>(v);
        }
        else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
            return static_cast<uint64_t>(v);
        }
        else {
            uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof(v) > 8 ? 8 : sizeof(v));
            return bits;
        }
    }

    template<typename... Args>
    inline const uint64_t* Call(uint64_t hash, Args... args) {
        static_assert(sizeof...(Args) <= 32);
        uint64_t buf[sizeof...(Args) == 0 ? 1 : sizeof...(Args)] = {};
        int i = 0;
        ((buf[i++] = CastArg(args)), ...);
        return Invoke(hash, buf, sizeof...(Args));
    }

    template<typename... Args>
    inline const uint64_t* CallByName(const std::string& name, Args... args) {
        return Call(NativeHashFromName(name), args...);
    }

    template<typename T>
    inline T Cast(const uint64_t* result) {
        if (!result) return T{};
        if constexpr (std::is_same_v<T, float>) {
            uint32_t bits = static_cast<uint32_t>(result[0]);
            float f = 0.f;
            std::memcpy(&f, &bits, sizeof(f));
            return f;
        }
        else if constexpr (std::is_same_v<T, double>) {
            double d = 0.0;
            std::memcpy(&d, &result[0], sizeof(d));
            return d;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return result[0] != 0;
        }
        else if constexpr (std::is_pointer_v<T>) {
            return reinterpret_cast<T>(result[0]);
        }
        else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
            return static_cast<T>(result[0]);
        }
        else {
            T v{};
            std::memcpy(&v, result, sizeof(v) > 24 ? 24 : sizeof(v));
            return v;
        }
    }

    template<typename R, typename... Args>
    inline R CallAs(uint64_t hash, Args... args) {
        return Cast<R>(Call(hash, args...));
    }

    template<typename R, typename... Args>
    inline R CallByNameAs(const std::string& name, Args... args) {
        return Cast<R>(CallByName(name, args...));
    }

    uintptr_t CitizenQueueBase();

    uintptr_t GameBase();
    size_t    GameSize();
    uint32_t  NativeCount();
    uint32_t  ScannedCount();
    uint32_t  Pid();
    const std::string& BuildTag();
    const std::string& ProcessName();
    const std::unordered_map<std::string, uint64_t>& AllHashes();
    const std::unordered_map<uint64_t, NativeInfo>& AllNatives();

    void SetNativesHppPath(std::string path);
    void SetCrossmapPath(std::string path);
    void EnableLogging(bool on);

}
