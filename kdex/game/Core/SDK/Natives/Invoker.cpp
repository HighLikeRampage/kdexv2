#include <Core/SDK/Natives/Invoker.hpp>
#include <Security/xorstr.hpp>

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <cctype>
#include <cstring>
#include <cstdarg>
#include <fstream>
#include <sstream>
#include <mutex>
#include <cstdio>
#include <string_view>

#ifndef _NTDEF_
    typedef LONG NTSTATUS, *PNTSTATUS;
    #define STATUS_SUCCESS 0
#endif
    typedef NTSTATUS(NTAPI* PFN_NT_QUERY_INFORMATION_THREAD)(
        HANDLE ThreadHandle, LONG ThreadInformationClass,
        PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength);
    typedef LONG(NTAPI* PFN_NT_ALERT_THREAD)(HANDLE ThreadHandle);

namespace {
    struct NativeEntry { const char* name; uint64_t hash; };
    static const NativeEntry kNativeEntries[] = {
        #include "NativesEmbedded.inc"
    };

    struct CrossmapEntry { uint64_t orig; uint64_t trans; };
    static const CrossmapEntry kCrossmapEntries[] = {
        #include "CrossmapEmbedded.inc"
    };
}

namespace Invoker {

    static std::mutex                                        s_mutex;
    static bool                                              s_initialized = false;
    static bool                                              s_logging = false;
    static uintptr_t                                         s_gameBase = 0;
    static size_t                                            s_gameSize = 0;
    static uint32_t                                          s_pid = 0;
    static std::string                                       s_processName;
    static std::string                                       s_buildTag;
    static std::vector<Section>                              s_sections;
    static std::unordered_map<std::string, uint64_t>         s_hashes;
    static std::unordered_map<uint64_t, uint64_t>            s_crossmap;
    static std::unordered_map<uint64_t, NativeInfo>          s_natives;
    static std::unordered_map<std::string, uint64_t>         s_nameLower;
    static uint32_t                                          s_scannedCount = 0;
    static HANDLE                                            s_hProc = nullptr;
    static std::vector<uint8_t>                              s_image;

    thread_local struct alignas(16) InvokeState {
        NativeContext ctx;
        uint64_t      argBuf[32];
        uint64_t      resBuf[8];
    } s_invoke = {};

    static void Log(const char* fmt, ...) {
        if (!s_logging) return;
        char buf[512];
        va_list ap; va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        OutputDebugStringA(buf);
        std::fprintf(stderr, xorstr("%s"), buf);
    }

    uint64_t Joaat(std::string_view name) {
        uint32_t h = 0;
        for (char c : name) {
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
            h += static_cast<uint8_t>(c);
            h += h << 10;
            h ^= h >> 6;
        }
        h += h << 3;
        h ^= h >> 11;
        h += h << 15;
        return static_cast<uint64_t>(h);
    }

    uint64_t ArgU32(uint32_t v) { return static_cast<uint64_t>(v); }
    uint64_t ArgU64(uint64_t v) { return v; }
    uint64_t ArgF32(float f) { uint32_t b = 0; std::memcpy(&b, &f, 4); return static_cast<uint64_t>(b); }
    uint64_t ArgBool(bool b) { return b ? 1ull : 0ull; }

    static std::string ToLower(std::string s) {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    uint64_t NativeHashFromName(const std::string& name) {
        auto it = s_hashes.find(name);
        if (it != s_hashes.end()) return it->second;
        auto lit = s_nameLower.find(ToLower(name));
        if (lit != s_nameLower.end()) return lit->second;
        return Joaat(name);
    }

    uint64_t ParseHash(const std::string& s) {
        if (s.size() > 2 && (s[0] == '0') && (s[1] == 'x' || s[1] == 'X'))
            return std::stoull(s.substr(2), nullptr, 16);
        if (!s.empty() && (std::isdigit(static_cast<unsigned char>(s[0])) || s[0] == '-'))
            return std::stoull(s);
        return NativeHashFromName(s);
    }

    uint64_t ParseArg(const std::string& s) {
        if (s.empty()) return 0;
        if (s[0] == 'f') return ArgF32(std::stof(s.substr(1)));
        if (s[0] == 'b') {
            std::string v = ToLower(s.substr(1));
            return ArgBool(v == xorstr("1") || v == xorstr("true"));
        }
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
            return std::stoull(s.substr(2), nullptr, 16);
        if (s.find('.') != std::string::npos ||
            (s[0] == '-' && s.find('.') != std::string::npos)) {
            return ArgF32(std::stof(s));
        }
        if (s[0] == '-') return static_cast<uint64_t>(std::stoll(s));
        return std::stoull(s);
    }

    static bool LoadNativesHpp() {
        for (const auto& e : kNativeEntries) {
            s_hashes[e.name] = e.hash;
            s_nameLower[ToLower(e.name)] = e.hash;
        }
        return !s_hashes.empty();
    }

    static bool LoadCrossmap() {
        for (const auto& e : kCrossmapEntries) {
            s_crossmap[e.orig] = e.trans;
        }
        return !s_crossmap.empty();
    }

    static bool ReadMemInto(uintptr_t src, void* dst, size_t len) {
        if (!s_hProc) {
            std::memcpy(dst, reinterpret_cast<void*>(src), len);
            return true;
        }
        SIZE_T got = 0;
        BOOL ok = ReadProcessMemory(s_hProc, reinterpret_cast<LPCVOID>(src), dst, len, &got);
        return ok && got == len;
    }

    static bool ReadImage() {
        s_image.resize(s_gameSize);
        if (!s_hProc) {
            std::memcpy(s_image.data(), reinterpret_cast<void*>(s_gameBase), s_gameSize);
            return true;
        }
        const size_t CHUNK = 4 * 1024 * 1024;
        size_t totalGot = 0;
        for (size_t off = 0; off < s_gameSize; off += CHUNK) {
            size_t c = s_gameSize - off;
            if (c > CHUNK) c = CHUNK;
            SIZE_T got = 0;
            BOOL ok = ReadProcessMemory(s_hProc, reinterpret_cast<LPCVOID>(s_gameBase + off), s_image.data() + off, c, &got);
            if (!ok || got < c) {
                const size_t PAGE = 4096;
                for (size_t poff = off + got; poff < off + c; poff += PAGE) {
                    size_t pc = off + c - poff;
                    if (pc > PAGE) pc = PAGE;
                    SIZE_T pgGot = 0;
                    ReadProcessMemory(s_hProc, reinterpret_cast<LPCVOID>(s_gameBase + poff), s_image.data() + poff, pc, &pgGot);
                    got += pgGot;
                }
            }
            totalGot += got;
        }
        Log(xorstr("[scan] read %zu / %zu bytes\n"), totalGot, s_gameSize);
        return totalGot > 0;
    }

    static void ParseSections() {
        s_sections.clear();
        if (s_image.size() < 0x40) return;
        auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(s_image.data());
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
        auto nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(s_image.data() + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return;
        auto sec = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
            char name[9] = {};
            std::memcpy(name, sec->Name, 8);
            s_sections.push_back({ name, sec->VirtualAddress, sec->Misc.VirtualSize,
                                   sec->SizeOfRawData, sec->Characteristics });
        }
    }

    static bool IsCodeRVA(uint32_t rva) {
        for (const auto& s : s_sections) {
            uint32_t size = s.vsize > s.rsize ? s.vsize : s.rsize;
            if (rva >= s.va && rva < s.va + size)
                return (s.characteristics & 0x20000000u) != 0;
        }
        return false;
    }

    static int32_t  Read32(const uint8_t* p) { int32_t  v; std::memcpy(&v, p, 4); return v; }
    static uint32_t ReadU32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
    static uint64_t ReadU64(const uint8_t* p) { uint64_t v; std::memcpy(&v, p, 8); return v; }
    static int8_t   Read8s(const uint8_t* p) { return static_cast<int8_t>(*p); }

    static int32_t UnwrapFivemStub(const uint8_t* buf, size_t bufLen, uint32_t rva) {
        if (rva + 30 > bufLen) return -1;
        const uint8_t* b = buf + rva;

        if (b[0] == 0x55 && b[1] == 0x48 && b[2] == 0x8D && b[3] == 0x2D &&
            b[8] == 0x48 && b[9] == 0x87 && b[10] == 0x2C && b[11] == 0x24 && b[12] == 0xC3)
            return static_cast<int32_t>(rva) + 8 + Read32(b + 4);

        if (b[0] == 0x54 && b[1] == 0x58 && b[2] == 0x48 && b[3] == 0x05 &&
            b[4] == 0xF8 && b[5] == 0xFF && b[6] == 0xFF && b[7] == 0xFF && b[8] == 0xE9)
            return static_cast<int32_t>(rva) + 13 + Read32(b + 9);

        if (b[0] == 0x50 && b[1] == 0x48 && b[2] == 0x8D && b[3] == 0x05 &&
            b[8] == 0x48 && b[9] == 0x87 && b[10] == 0x04 && b[11] == 0x24 && b[12] == 0xC3)
            return static_cast<int32_t>(rva) + 8 + Read32(b + 4);

        auto trailerXchgJmp = [&](int o) {
            const uint8_t* q = b + o;
            return q[0] == 0x48 && q[1] == 0x87 && q[2] == 0x2C && q[3] == 0x24 &&
                q[4] == 0x48 && q[5] == 0x8D && q[6] == 0x64 && q[7] == 0x24 && q[8] == 0x08 &&
                q[9] == 0xFF && q[10] == 0x64 && q[11] == 0x24 && q[12] == 0xF8;
            };
        auto resolveWithTrailer = [&](int leaOff, int dispOff) -> int32_t {
            uint32_t tAt = rva + leaOff + 7;
            if (tAt + 5 <= bufLen && buf[tAt] == 0xE9)
                return static_cast<int32_t>(tAt) + 5 + Read32(buf + tAt + 1);
            if (tAt + 9 <= bufLen && buf[tAt] == 0x48 && buf[tAt + 1] == 0x87 &&
                buf[tAt + 2] == 0x2C && buf[tAt + 3] == 0x24 && buf[tAt + 4] == 0xE9)
                return static_cast<int32_t>(tAt) + 9 + Read32(buf + tAt + 5);
            if (trailerXchgJmp(leaOff + 7))
                return static_cast<int32_t>(rva) + leaOff + 7 + Read32(b + dispOff);
            return -1;
            };

        if (b[0] == 0x48 && b[1] == 0x8D && b[2] == 0x64 && b[3] == 0x24 && b[4] == 0xF8 &&
            b[5] == 0x48 && b[6] == 0x89 && b[7] == 0x2C && b[8] == 0x24 &&
            b[9] == 0x48 && b[10] == 0x8D && b[11] == 0x2D) {
            int32_t t = resolveWithTrailer(9, 12);
            if (t != -1) return t;
        }

        if (b[0] == 0x48 && b[1] == 0x89 && b[2] == 0x6C && b[3] == 0x24 && b[4] == 0xF8 &&
            b[5] == 0x48 && b[6] == 0x8D && b[7] == 0x64 && b[8] == 0x24 && b[9] == 0xF8 &&
            b[10] == 0x48 && b[11] == 0x8D && b[12] == 0x2D) {
            int32_t t = resolveWithTrailer(10, 13);
            if (t != -1) return t;
        }
        return -1;
    }

    static uint32_t FollowStubs(const uint8_t* buf, size_t bufLen, uint32_t rva, uintptr_t moduleBase, uint32_t maxHops = 24) {
        std::unordered_map<uint32_t, bool> seen;
        for (uint32_t hop = 0; hop < maxHops; ++hop) {
            if (rva + 16 > bufLen || seen.count(rva)) return rva;
            seen[rva] = true;
            int32_t u = UnwrapFivemStub(buf, bufLen, rva);
            if (u > 0 && (uint32_t)u < bufLen && IsCodeRVA((uint32_t)u)) { rva = (uint32_t)u; continue; }
            uint8_t b0 = buf[rva];
            if (b0 == 0xE9) {
                uint32_t next = rva + 5 + Read32(buf + rva + 1);
                if (!IsCodeRVA(next)) return rva;
                rva = next; continue;
            }
            if (b0 == 0xEB) {
                uint32_t next = rva + 2 + Read8s(buf + rva + 1);
                if (!IsCodeRVA(next)) return rva;
                rva = next; continue;
            }
            if (b0 == 0xFF && buf[rva + 1] == 0x25) {
                int32_t rel = Read32(buf + rva + 2);
                uint32_t pAt = rva + 6 + rel;
                if (pAt + 8 > bufLen) return rva;
                uint64_t va = ReadU64(buf + pAt);
                uint32_t targetRva = static_cast<uint32_t>(va - moduleBase);
                if (targetRva >= bufLen || !IsCodeRVA(targetRva)) return rva;
                rva = targetRva; continue;
            }
            if (b0 == 0xE8 && buf[rva + 5] == 0xC3) {
                uint32_t next = rva + 5 + Read32(buf + rva + 1);
                if (!IsCodeRVA(next)) return rva;
                rva = next; continue;
            }
            break;
        }
        return rva;
    }

    static void ExtractHandlerRVAs(const uint8_t* buf, size_t bufLen, uint32_t hitOff, uintptr_t moduleBase, std::vector<uint32_t>& out) {
        auto push = [&](int64_t rva) {
            if (rva >= 0 && (uint32_t)rva < bufLen && IsCodeRVA((uint32_t)rva)) {
                for (auto v : out) if (v == (uint32_t)rva) return;
                out.push_back(static_cast<uint32_t>(rva));
            }
            };

        if (hitOff + 16 <= bufLen)
            push(static_cast<int64_t>(ReadU64(buf + hitOff + 8) - moduleBase));
        if (hitOff >= 8)
            push(static_cast<int64_t>(ReadU64(buf + hitOff - 8) - moduleBase));

        uint8_t p0 = hitOff >= 2 ? buf[hitOff - 2] : 0;
        uint8_t p1 = hitOff >= 2 ? buf[hitOff - 1] : 0;
        if ((p0 == 0x48 || p0 == 0x49) && p1 >= 0xB8 && p1 <= 0xBF) {
            uint32_t wStart = hitOff > 48 ? hitOff - 48 : 0;
            uint32_t wEnd = hitOff + 32 < bufLen - 7 ? hitOff + 32 : (uint32_t)bufLen - 7;
            for (uint32_t i = wStart; i < wEnd; ++i) {
                uint8_t b0 = buf[i];
                if (b0 != 0x48 && b0 != 0x4C) continue;
                if (buf[i + 1] != 0x8D) continue;
                uint8_t modrm = buf[i + 2];
                if ((modrm & 0xC7) != 0x05) continue;
                push(static_cast<int64_t>(i + 7 + Read32(buf + i + 3)));
            }
        }
    }

    static void ScanHashes() {
        const uint8_t* image = s_image.data();
        size_t imageLen = s_image.size();
        uintptr_t moduleBase = s_gameBase;

        struct Cand { uint32_t high; const std::string* name; };
        std::unordered_map<uint32_t, std::vector<Cand>> idx;
        idx.reserve(s_hashes.size());

        std::vector<const std::string*> nameRefs;
        nameRefs.reserve(s_hashes.size());
        for (const auto& kv : s_hashes) nameRefs.push_back(&kv.first);

        for (const auto* np : nameRefs) {
            auto orig = s_hashes[*np];
            auto it = s_crossmap.find(orig);
            if (it == s_crossmap.end()) continue;
            uint64_t trans = it->second;
            uint32_t low = static_cast<uint32_t>(trans & 0xFFFFFFFFu);
            uint32_t high = static_cast<uint32_t>((trans >> 32) & 0xFFFFFFFFu);
            idx[low].push_back({ high, np });
        }

        std::unordered_map<const std::string*, std::vector<uint32_t>> hits;
        for (const auto& s : s_sections) {
            uint32_t size = s.vsize > s.rsize ? s.vsize : s.rsize;
            uint32_t start = s.va;
            uint32_t end = start + size;
            if (end > imageLen - 8) end = static_cast<uint32_t>(imageLen - 8);
            for (uint32_t i = start; i < end; ++i) {
                uint32_t low = ReadU32(image + i);
                auto bit = idx.find(low);
                if (bit == idx.end()) continue;
                uint32_t high = ReadU32(image + i + 4);
                for (const auto& c : bit->second) {
                    if (c.high == high) hits[c.name].push_back(i);
                }
            }
        }

        s_natives.clear();
        s_natives.reserve(hits.size() * 2);
        std::vector<uint32_t> cands;
        for (const auto& [namePtr, hitList] : hits) {
            const std::string& name = *namePtr;
            auto hit = s_hashes.find(name);
            if (hit == s_hashes.end()) continue;
            uint64_t hash = hit->second;
            uint64_t trans = s_crossmap[hash];
            uint32_t bodyRVA = 0;
            bool resolved = false;
            for (uint32_t off : hitList) {
                cands.clear();
                ExtractHandlerRVAs(image, imageLen, off, moduleBase, cands);
                for (uint32_t c : cands) {
                    uint32_t r = FollowStubs(image, imageLen, c, moduleBase);
                    if (IsCodeRVA(r)) { bodyRVA = r; resolved = true; break; }
                }
                if (resolved) break;
            }
            if (!resolved) continue;
            NativeInfo ni;
            ni.hash = trans;
            ni.rva = bodyRVA;
            ni.handler = reinterpret_cast<NativeHandler>(moduleBase + bodyRVA);
            s_natives[trans] = ni;
            s_natives[hash] = ni;
        }
        s_scannedCount = static_cast<uint32_t>(hits.size());
    }

    static bool FindProcessByName(const std::string& hint, uint32_t* outPid, std::string* outName) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;
        PROCESSENTRY32W pe = {}; pe.dwSize = sizeof(pe);
        std::string lowerHint = ToLower(hint);
        std::vector<std::pair<uint32_t, std::string>> matches;
        if (Process32FirstW(snap, &pe)) {
            do {
                char nameBuf[MAX_PATH] = {};
                int wrote = WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, nameBuf, sizeof(nameBuf), nullptr, nullptr);
                std::string exe = wrote > 0 ? std::string(nameBuf) : std::string();
                std::string lower = ToLower(exe);
                if (lower.find(lowerHint) != std::string::npos) {
                    matches.push_back({ pe.th32ProcessID, exe });
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        if (matches.empty()) return false;
        std::vector<std::pair<uint32_t, std::string>> filtered;
        for (const auto& m : matches) {
            std::string lo = ToLower(m.second);
            if (lo.find(xorstr("dumpserver")) != std::string::npos) continue;
            if (lo.find(xorstr("chromebrowser")) != std::string::npos) continue;
            if (lo.find(xorstr("roslauncher")) != std::string::npos) continue;
            if (lo.find(xorstr("rosservice")) != std::string::npos) continue;
            filtered.push_back(m);
        }
        if (filtered.empty()) return false;
        for (const auto& m : filtered) {
            std::string lo = ToLower(m.second);
            if (lo.find(xorstr("gtaprocess")) != std::string::npos) {
                *outPid = m.first; *outName = m.second; return true;
            }
        }
        for (const auto& m : filtered) {
            std::string lo = ToLower(m.second);
            if (lo.find(xorstr("gameprocess")) != std::string::npos) {
                *outPid = m.first; *outName = m.second; return true;
            }
        }
        *outPid = filtered[0].first;
        *outName = filtered[0].second;
        return true;
    }

    static bool GetFirstModule(HANDLE hProc, uintptr_t* outBase, size_t* outSize, std::string* outName) {
        HMODULE mods[512];
        DWORD needed = 0;
        if (!EnumProcessModulesEx(hProc, mods, sizeof(mods), &needed, LIST_MODULES_64BIT)) return false;
        size_t count = needed / sizeof(HMODULE);
        if (count == 0) return false;
        char name[MAX_PATH] = {};
        GetModuleBaseNameA(hProc, mods[0], name, sizeof(name));
        MODULEINFO mi = {};
        if (!GetModuleInformation(hProc, mods[0], &mi, sizeof(mi))) return false;
        *outBase = reinterpret_cast<uintptr_t>(mods[0]);
        *outSize = mi.SizeOfImage;
        *outName = name;
        return true;
    }

    bool AttachExternal(uint32_t pid) {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_hProc) { CloseHandle(s_hProc); s_hProc = nullptr; }
        DWORD access = PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION
            | PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD;
        HANDLE h = OpenProcess(access, FALSE, pid);
        if (!h) h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!h) return false;
        s_hProc = h;
        s_pid = pid;
        s_initialized = false;
        s_natives.clear();
        s_image.clear();
        return true;
    }

    bool AttachExternalByName(const std::string& hint) {
        uint32_t pid = 0; std::string name;
        if (!FindProcessByName(hint, &pid, &name)) return false;
        if (!AttachExternal(pid)) return false;
        s_processName = name;
        return true;
    }

    void DetachExternal() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_hProc) { CloseHandle(s_hProc); s_hProc = nullptr; }
        s_pid = 0;
        s_processName.clear();
        s_initialized = false;
        s_natives.clear();
        s_image.clear();
    }

    bool IsExternal() { return s_hProc != nullptr; }

    static void DetectBuild(const std::string& modName) {
        s_buildTag = xorstr("unknown");
        size_t p = modName.find(xorstr("_b"));
        if (p == std::string::npos) return;
        p += 2;
        size_t start = p;
        while (p < modName.size() && modName[p] >= '0' && modName[p] <= '9') ++p;
        if (p == start) return;
        if (p < modName.size() && modName[p] == '_')
            s_buildTag = xorstr("b") + modName.substr(start, p - start);
    }

    static uintptr_t ScanScrThreadInstance();
    static uintptr_t s_scrThreadInstVA = 0;

    bool Scan() {
        std::lock_guard<std::mutex> lock(s_mutex);
        Log(xorstr("[scan] begin\n"));

        if (s_hProc) {
            std::string modName;
            if (!GetFirstModule(s_hProc, &s_gameBase, &s_gameSize, &modName)) {
                Log(xorstr("[scan] EnumProcessModules failed err=%lu\n"), GetLastError());
                return false;
            }
            Log(xorstr("[scan] module: %s base=0x%llx size=%zu\n"),
                modName.c_str(), (unsigned long long)s_gameBase, s_gameSize);
            if (s_processName.empty()) s_processName = modName;
            DetectBuild(modName);
        }
        else {
            HMODULE hMod = GetModuleHandleA(nullptr);
            if (!hMod) return false;
            s_gameBase = reinterpret_cast<uintptr_t>(hMod);
            MODULEINFO mi = {};
            if (!GetModuleInformation(GetCurrentProcess(), hMod, &mi, sizeof(mi))) return false;
            s_gameSize = mi.SizeOfImage;
            char modPath[MAX_PATH] = {};
            GetModuleFileNameA(hMod, modPath, sizeof(modPath));
            std::string modName = modPath;
            size_t slash = modName.find_last_of(xorstr("\\/"));
            if (slash != std::string::npos) modName = modName.substr(slash + 1);
            s_processName = modName;
            DetectBuild(modName);
        }

        if (s_hashes.empty()) {
            Log(xorstr("[scan] loading embedded Natives.hpp\n"));
            if (!LoadNativesHpp()) {
                Log(xorstr("[scan] failed to load embedded Natives.hpp\n"));
                return false;
            }
            Log(xorstr("[scan] Natives.hpp = %zu entries\n"), s_hashes.size());
        }
        if (s_crossmap.empty()) {
            Log(xorstr("[scan] loading embedded Crossmap.hpp\n"));
            if (!LoadCrossmap()) {
                Log(xorstr("[scan] failed to load embedded Crossmap.hpp\n"));
                return false;
            }
            Log(xorstr("[scan] crossmap = %zu entries\n"), s_crossmap.size());
        }

        Log(xorstr("[scan] reading %zu MB image...\n"), s_gameSize / (1024 * 1024));
        if (!ReadImage()) { Log(xorstr("[scan] ReadImage failed\n")); return false; }
        Log(xorstr("[scan] parsing sections\n"));
        ParseSections();
        Log(xorstr("[scan] %zu sections, scanning hashes\n"), s_sections.size());
        ScanHashes();

        s_initialized = !s_natives.empty();
        s_scrThreadInstVA = ScanScrThreadInstance();
        Log(xorstr("[Invoker] scan: pid=%u %s base=0x%llx size=%zu sections=%u natives=%u handlers=%u\n"),
            s_pid, s_buildTag.c_str(),
            static_cast<unsigned long long>(s_gameBase), s_gameSize,
            static_cast<uint32_t>(s_sections.size()),
            s_scannedCount, static_cast<uint32_t>(s_natives.size()));
        return s_initialized;
    }

    bool Initialize() {
        if (s_initialized) return true;
        return Scan();
    }

    void Refresh() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_natives.clear();
        s_image.clear();
        s_initialized = false;
    }

    NativeHandler GetHandler(uint64_t hash) {
        if (!s_initialized && !Initialize()) return nullptr;
        auto it = s_natives.find(hash);
        if (it != s_natives.end()) return it->second.handler;
        uint64_t low32 = hash & 0xFFFFFFFFull;
        if (low32 != hash) {
            it = s_natives.find(low32);
            if (it != s_natives.end()) return it->second.handler;
        }
        return nullptr;
    }

    NativeHandler GetHandlerByName(const std::string& name) {
        return GetHandler(NativeHashFromName(name));
    }

    const NativeInfo* GetInfo(uint64_t hash) {
        if (!s_initialized && !Initialize()) return nullptr;
        auto it = s_natives.find(hash);
        if (it != s_natives.end()) return &it->second;
        uint64_t low32 = hash & 0xFFFFFFFFull;
        if (low32 != hash) {
            it = s_natives.find(low32);
            if (it != s_natives.end()) return &it->second;
        }
        return nullptr;
    }

    const NativeInfo* GetInfoByName(const std::string& name) {
        return GetInfo(NativeHashFromName(name));
    }

    static uintptr_t s_citizenCoreBase = 0;
    static size_t    s_citizenCoreSize = 0;
    static std::unordered_map<uintptr_t, bool> s_caveUsed;
    static std::vector<uintptr_t> s_caveFreelist;
    static uintptr_t s_apcCaveVA = 0;
    static uintptr_t s_apcDataVA = 0;
    static bool      s_citizenHookInstalled = false;

#pragma pack(push, 1)
    struct ApcNativeCall {
        uint64_t       handler;
        volatile LONG  done;
        NativeContext  ctx;
        uint64_t       args[32];
        uint64_t       results[8];
    };
#pragma pack(pop)

    static uintptr_t ScanScrThreadInstance() {
        if (s_image.empty()) return 0;
        std::unordered_map<uintptr_t, uint32_t> tally;
        for (const auto& sect : s_sections) {
            if (!(sect.characteristics & 0x20000000)) continue;
            const uint8_t* buf = s_image.data() + sect.va;
            uint32_t bufLen = sect.vsize;
            if ((size_t)(sect.va) + bufLen > s_image.size()) continue;
            if (bufLen < 20) continue;
            for (uint32_t i = 0; i + 20 <= bufLen; ++i) {
                if (buf[i]   != 0x48 || buf[i+1]  != 0x8B || buf[i+2]  != 0x05) continue;
                if (buf[i+7] != 0x48 || buf[i+8]  != 0x8B || buf[i+9]  != 0x88) continue;
                if (buf[i+14]!= 0x48 || buf[i+15] != 0x85 || buf[i+16] != 0xC9) continue;
                if (buf[i+17]!= 0x74) continue;
                int32_t disp = 0;
                std::memcpy(&disp, buf + i + 3, 4);
                uintptr_t ripRva = sect.va + i + 7;
                uintptr_t targetRva = (uintptr_t)((int64_t)ripRva + disp);
                if (targetRva >= s_image.size()) continue;
                tally[targetRva]++;
            }
        }
        if (tally.empty()) return 0;
        uintptr_t bestRva = 0; uint32_t bestCount = 0;
        for (const auto& kv : tally) {
            if (kv.second > bestCount) { bestCount = kv.second; bestRva = kv.first; }
        }
        Log(xorstr("[scan] scrThread::sm_Instance RVA=0x%llx (score=%u, candidates=%zu)\n"),
            (unsigned long long)bestRva, bestCount, tally.size());
        if (bestCount < 5) return 0;
        return s_gameBase + bestRva;
    }

    struct ModuleInfoEx { uintptr_t base; size_t size; std::string name; };

    static bool EnumRemoteModules(std::vector<ModuleInfoEx>& out) {
        if (!s_hProc) return false;
        HMODULE mods[2048];
        DWORD needed = 0;
        if (!EnumProcessModulesEx(s_hProc, mods, sizeof(mods), &needed, LIST_MODULES_64BIT))
            return false;
        size_t count = needed / sizeof(HMODULE);
        out.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            char nm[MAX_PATH] = {};
            GetModuleBaseNameA(s_hProc, mods[i], nm, sizeof(nm));
            MODULEINFO mi = {};
            if (!GetModuleInformation(s_hProc, mods[i], &mi, sizeof(mi))) continue;
            ModuleInfoEx m;
            m.base = (uintptr_t)mi.lpBaseOfDll;
            m.size = mi.SizeOfImage;
            m.name = nm;
            out.push_back(m);
        }
        return !out.empty();
    }

    static bool IsPreferrredCaveModule(const std::string& n) {
        std::string l = ToLower(n);
        if (l.find(xorstr("citizen-scripting-lua")) != std::string::npos) return true;
        if (l.find(xorstr("citizen-scripting-core")) != std::string::npos) return true;
        if (l.find(xorstr("citizen")) != std::string::npos) return true;
        if (l.find(xorstr("gtaprocess")) != std::string::npos) return true;
        if (l.find(xorstr("gameprocess")) != std::string::npos) return true;
        return false;
    }

    static uintptr_t HuntCodeCaveInModule(const ModuleInfoEx& mod, size_t minSize, size_t align = 16) {
        if (!s_hProc) return 0;
        std::vector<uint8_t> image(mod.size, 0);
        SIZE_T got = 0;
        if (!ReadProcessMemory(s_hProc, (LPCVOID)mod.base, image.data(), mod.size, &got) || got < 0x1000)
            return 0;
        auto dos = (IMAGE_DOS_HEADER*)image.data();
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
        auto nt = (IMAGE_NT_HEADERS64*)(image.data() + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
        auto sec = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++sec) {
            if (!(sec->Characteristics & 0x20000000u)) continue;
            uint32_t vsize = sec->Misc.VirtualSize;
            uint32_t rsize = sec->SizeOfRawData;
            uint32_t sz = vsize > rsize ? vsize : rsize;
            uint32_t va = sec->VirtualAddress;
            if (va + sz > image.size()) continue;
            const uint8_t* p = image.data() + va;
            size_t runStart = SIZE_MAX;
            size_t runLen = 0;
            for (uint32_t o = 0; o < sz; ++o) {
                uint8_t b = p[o];
                if (b == 0xCC || b == 0x00 || b == 0x90) {
                    if (runStart == SIZE_MAX) runStart = o;
                    ++runLen;
                } else {
                    if (runLen >= minSize) {
                        uintptr_t abs = mod.base + va + runStart;
                        if (align > 1) abs = (abs + align - 1) & ~(uintptr_t)(align - 1);
                        if (!s_caveUsed.count(abs)) return abs;
                    }
                    runStart = SIZE_MAX;
                    runLen = 0;
                }
            }
            if (runLen >= minSize) {
                uintptr_t abs = mod.base + va + runStart;
                if (align > 1) abs = (abs + align - 1) & ~(uintptr_t)(align - 1);
                if (!s_caveUsed.count(abs)) return abs;
            }
        }
        return 0;
    }

    static uintptr_t AllocCodeCave(size_t size, size_t align = 16) {
        if (!s_caveFreelist.empty()) {
            for (auto it = s_caveFreelist.begin(); it != s_caveFreelist.end(); ++it) {
                uintptr_t va = *it;
                if (!s_caveUsed.count(va)) {
                    s_caveUsed[va] = true;
                    s_caveFreelist.erase(it);
                    return va;
                }
            }
        }
        std::vector<ModuleInfoEx> mods;
        if (!EnumRemoteModules(mods)) return 0;
        std::stable_sort(mods.begin(), mods.end(), [](const ModuleInfoEx& a, const ModuleInfoEx& b) {
            int wa = IsPreferrredCaveModule(a.name) ? 0 : 1;
            int wb = IsPreferrredCaveModule(b.name) ? 0 : 1;
            return wa < wb;
        });
        for (const auto& m : mods) {
            uintptr_t va = HuntCodeCaveInModule(m, size, align);
            if (va) {
                s_caveUsed[va] = true;
                return va;
            }
        }
        for (const auto& m : mods) {
            uintptr_t va = HuntCodeCaveInModule(m, size, align);
            if (va) {
                s_caveUsed[va] = true;
                return va;
            }
        }
        return 0;
    }

    static void FreeCodeCave(uintptr_t va) {
        if (!va) return;
        s_caveUsed.erase(va);
        s_caveFreelist.push_back(va);
    }
    static std::unordered_map<uint64_t, bool> s_blacklist;

    static const char* const kBuiltinBlacklist[] = {
        xorstr("IS_HUD_HIDDEN"),
        xorstr("IS_CINEMATIC_CAM_RENDERING"),
        xorstr("RESET_ENTITY_ALPHA"),
        xorstr("GET_CLOCK_DAY_OF_WEEK"),
        xorstr("SET_CLOCK_TIME"),
        xorstr("GET_MINIMAP_FOW_DISCOVERY_RATIO"),
        nullptr,
    };

    static bool WriteMem(uintptr_t va, const void* src, size_t len) {
        if (!s_hProc) return false;
        SIZE_T got = 0;
        return WriteProcessMemory(s_hProc, (LPVOID)va, src, len, &got) && got == len;
    }

    static bool ReadMem(uintptr_t va, void* dst, size_t len) {
        if (!s_hProc) return false;
        SIZE_T got = 0;
        return ReadProcessMemory(s_hProc, (LPCVOID)va, dst, len, &got) && got == len;
    }

    static uintptr_t AllocRemoteData(size_t size, DWORD prot = PAGE_READWRITE) {
        return (uintptr_t)VirtualAllocEx(s_hProc, nullptr, size, MEM_COMMIT | MEM_RESERVE, prot);
    }

    static void FreeRemoteData(uintptr_t va) {
        if (s_hProc && va) VirtualFreeEx(s_hProc, (LPVOID)va, 0, MEM_RELEASE);
    }

    static std::vector<uint8_t> BuildApcStub() {
        std::vector<uint8_t> c;
        auto e = [&](std::initializer_list<uint8_t> bs) { for (auto b : bs) c.push_back(b); };
        auto imm64 = [&](uint64_t v) { for (int i = 0; i < 8; ++i) c.push_back(uint8_t(v >> (i * 8))); };

        e({0x50, 0x51, 0x52, 0x53, 0x55, 0x56, 0x57});
        e({0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57});
        e({0x48, 0x83, 0xEC, 0x20});

        e({0x48, 0x89, 0xCB});
        e({0x48, 0x81, 0xC3, 0x10, 0x00, 0x00, 0x00});
        e({0x48, 0x8D, 0x93, 0x80, 0x00, 0x00, 0x00});
        e({0x48, 0x89, 0x53, 0x08});
        e({0x48, 0x8D, 0x93, 0x18, 0x01, 0x00, 0x00});
        e({0x48, 0x89, 0x53, 0x00});
        e({0xC7, 0x43, 0x04, 0x01, 0x00, 0x00, 0x00});
        e({0x8B, 0x43, 0x00});
        e({0x83, 0xE0, 0x1F});
        e({0x89, 0x43, 0x00});
        e({0x48, 0x8B, 0x03});
        e({0xFF, 0xD0});
        e({0x48, 0x8D, 0x93, 0x18, 0x01, 0x00, 0x00});
        e({0x48, 0x8B, 0x03});
        e({0x48, 0x89, 0x43, 0xA0});
        e({0x48, 0x8B, 0x43, 0xA8});
        e({0x48, 0x89, 0x43, 0xA8});
        e({0x48, 0x8B, 0x43, 0xB0});
        e({0x48, 0x89, 0x43, 0xB0});
        e({0xB8, 0x01, 0x00, 0x00, 0x00});
        e({0x48, 0x87, 0x03});
        e({0x48, 0x83, 0xC4, 0x20});
        e({0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58});
        e({0x5F, 0x5E, 0x5D, 0x5B, 0x5A, 0x59, 0x58});
        e({0xC3});
        return c;
    }

    static uintptr_t GetScriptThreadStartHint() {
        HMODULE mods[2048];
        DWORD needed = 0;
        if (!EnumProcessModulesEx(s_hProc, mods, sizeof(mods), &needed, LIST_MODULES_64BIT)) return 0;
        size_t count = needed / sizeof(HMODULE);
        for (size_t i = 0; i < count; ++i) {
            char nm[MAX_PATH] = {};
            GetModuleBaseNameA(s_hProc, mods[i], nm, sizeof(nm));
            std::string l = ToLower(nm);
            if (l.find(xorstr("citizen-scripting-lua")) != std::string::npos)
                return (uintptr_t)mods[i];
        }
        for (size_t i = 0; i < count; ++i) {
            char nm[MAX_PATH] = {};
            GetModuleBaseNameA(s_hProc, mods[i], nm, sizeof(nm));
            std::string l = ToLower(nm);
            if (l.find(xorstr("citizen-scripting-core")) != std::string::npos)
                return (uintptr_t)mods[i];
        }
        return 0;
    }

    static HANDLE FindBestScriptThread(uintptr_t& outStart, size_t& outUserMs) {
        if (!s_hProc) return nullptr;
        uintptr_t prefBase = GetScriptThreadStartHint();
        uintptr_t prefEnd = prefBase + (32 * 1024 * 1024);

        auto ntqit = (PFN_NT_QUERY_INFORMATION_THREAD)GetProcAddress(
            GetModuleHandleA(xorstr("ntdll.dll")), xorstr("NtQueryInformationThread"));
        if (!ntqit) return nullptr;

        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snap == INVALID_HANDLE_VALUE) return nullptr;

        HANDLE best = nullptr;
        uintptr_t bestStart = 0;
        size_t bestUs = 0;

        THREADENTRY32 te; te.dwSize = sizeof(te);
        if (Thread32First(snap, &te)) {
            do {
                if (te.th32OwnerProcessID != s_pid) continue;
                HANDLE ht = OpenThread(THREAD_QUERY_INFORMATION | THREAD_SET_CONTEXT, FALSE, te.th32ThreadID);
                if (!ht) continue;
                uintptr_t start = 0;
                FILETIME c, e, k, u;
                bool okStart = (ntqit(ht, 9, &start, sizeof(start), nullptr) == 0);
                bool okTimes = !!GetThreadTimes(ht, &c, &e, &k, &u);
                ULARGE_INTEGER uli; uli.LowPart = u.dwLowDateTime; uli.HighPart = u.dwHighDateTime;
                size_t us = (size_t)(uli.QuadPart / 10000);
                if (okStart && okTimes) {
                    bool pref = prefBase && start >= prefBase && start < prefEnd;
                    bool better = (pref && !bestStart) ||
                        (pref == (bestStart >= prefBase && bestStart < prefEnd) && us > bestUs);
                    if (better) {
                        if (best) CloseHandle(best);
                        best = ht;
                        bestStart = start;
                        bestUs = us;
                        continue;
                    }
                }
                CloseHandle(ht);
            } while (Thread32Next(snap, &te));
        }
        CloseHandle(snap);
        outStart = bestStart;
        outUserMs = bestUs;
        return best;
    }

    static HANDLE s_apcThread = nullptr;
    static uintptr_t s_apcThreadStart = 0;

    static bool EnsureApcThread() {
        if (s_apcThread) {
            DWORD code = 0;
            if (GetExitCodeProcess(s_hProc, &code) && code == STILL_ACTIVE) return true;
            CloseHandle(s_apcThread); s_apcThread = nullptr;
        }
        size_t us = 0;
        s_apcThread = FindBestScriptThread(s_apcThreadStart, us);
        if (!s_apcThread) return false;
        Log(xorstr("[apc] chosen thread start=0x%llx user=%zums\n"),
            (unsigned long long)s_apcThreadStart, us);
        return true;
    }

    bool InstallCitizenHook() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_citizenHookInstalled) return true;
        if (!s_hProc) { Log(xorstr("[apc] not attached\n")); return false; }

        auto stub = BuildApcStub();
        s_apcCaveVA = AllocCodeCave(stub.size() + 64, 16);
        if (!s_apcCaveVA) {
            Log(xorstr("[apc] code cave hunt failed (fallback alloc)\n"));
            s_apcCaveVA = AllocRemoteData(stub.size() + 64, PAGE_EXECUTE_READ);
            if (!s_apcCaveVA) return false;
        }
        if (!WriteMem(s_apcCaveVA, stub.data(), stub.size())) {
            Log(xorstr("[apc] WPM stub failed gle=%lu\n"), GetLastError());
            return false;
        }

        s_apcDataVA = AllocRemoteData(sizeof(ApcNativeCall) + 64, PAGE_READWRITE);
        if (!s_apcDataVA) {
            Log(xorstr("[apc] data alloc failed gle=%lu\n"), GetLastError());
            return false;
        }

        if (!EnsureApcThread()) {
            Log(xorstr("[apc] no alertable candidate thread\n"));
            return false;
        }

        s_citizenHookInstalled = true;
        Log(xorstr("[apc] installed stub=0x%llx data=0x%llx\n"),
            (unsigned long long)s_apcCaveVA, (unsigned long long)s_apcDataVA);
        return true;
    }

    bool IsCitizenHookInstalled() { return s_citizenHookInstalled; }

    void UninstallCitizenHook() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_citizenHookInstalled) return;
        if (s_apcThread) { CloseHandle(s_apcThread); s_apcThread = nullptr; }
        FreeCodeCave(s_apcCaveVA);
        FreeRemoteData(s_apcDataVA);
        s_apcCaveVA = 0;
        s_apcDataVA = 0;
        s_citizenHookInstalled = false;
    }

    static bool ApcRunOnce(uintptr_t handlerVA, const uint64_t* args, uint32_t argCount,
                           uint64_t* outResult, uint32_t timeoutMs) {
        if (!s_citizenHookInstalled || !s_apcCaveVA || !s_apcDataVA) return false;

        uint8_t buf[sizeof(ApcNativeCall)];
        std::memset(buf, 0, sizeof(buf));
        ApcNativeCall* call = (ApcNativeCall*)buf;
        call->handler = handlerVA;
        call->done = 0;
        call->ctx.NumArgs = argCount;
        call->ctx.NumResults = 1;
        call->ctx.Args = (void*)(s_apcDataVA + offsetof(ApcNativeCall, args));
        call->ctx.Results = (void*)(s_apcDataVA + offsetof(ApcNativeCall, results));
        call->ctx._reserved[0] = 0;
        for (uint32_t i = 0; i < argCount && i < 32; ++i) call->args[i] = args ? args[i] : 0;

        if (!WriteMem(s_apcDataVA, buf, sizeof(buf))) return false;

        if (!EnsureApcThread()) return false;
        auto ntAlert = (PFN_NT_ALERT_THREAD)GetProcAddress(
            GetModuleHandleA(xorstr("ntdll.dll")), xorstr("NtAlertThread"));
        for (int attempt = 0; attempt < 4; ++attempt) {
            if (!QueueUserAPC((PAPCFUNC)s_apcCaveVA, s_apcThread, (ULONG_PTR)s_apcDataVA)) {
                DWORD gle = GetLastError();
                if (gle == 5) { CloseHandle(s_apcThread); s_apcThread = nullptr; EnsureApcThread(); continue; }
                return false;
            }
            if (ntAlert) ntAlert(s_apcThread);
            auto start = GetTickCount64();
            while (GetTickCount64() - start < timeoutMs) {
                LONG doneVal = 0;
                if (!ReadMem(s_apcDataVA + offsetof(ApcNativeCall, done), &doneVal, sizeof(doneVal))) return false;
                if (doneVal) {
                    if (outResult) {
                        uint64_t tmp[3] = {};
                        ReadMem(s_apcDataVA + offsetof(ApcNativeCall, results), tmp, sizeof(tmp));
                        std::memcpy(outResult, tmp, sizeof(uint64_t) * 3);
                    }
                    return true;
                }
                Sleep(1);
            }
        }
        return false;
    }

    void LoadBlacklist(const std::string&) {
        s_blacklist.clear();
        for (const char* const* p = kBuiltinBlacklist; *p; ++p) {
            uint64_t h = NativeHashFromName(*p);
            if (h) s_blacklist[h] = true;
        }
        Log(xorstr("[blacklist] %zu built-in entries\n"), s_blacklist.size());
    }

    void SaveBlacklist(const std::string&) {}

    void AddToBlacklist(uint64_t hash) {
        s_blacklist[hash] = true;
    }

    bool IsBlacklisted(uint64_t hash) {
        if (s_blacklist.count(hash)) return true;
        uint64_t low32 = hash & 0xFFFFFFFFull;
        return low32 != hash && s_blacklist.count(low32) > 0;
    }

    bool IsProcessAlive() {
        if (!s_hProc) return false;
        DWORD code = 0;
        return GetExitCodeProcess(s_hProc, &code) && code == STILL_ACTIVE;
    }

    uintptr_t AllocRemoteString(const std::string& s) {
        if (!s_hProc) return 0;
        uintptr_t va = AllocRemoteData(s.size() + 1, PAGE_READWRITE);
        if (!va) return 0;
        WriteMem(va, s.data(), s.size() + 1);
        return va;
    }

    void FreeRemote(uintptr_t va) {
        if (s_hProc && va) VirtualFreeEx(s_hProc, (LPVOID)va, 0, MEM_RELEASE);
    }

    static std::mutex s_invokeMutex;

    const uint64_t* Invoke(uint64_t hash, const uint64_t* args, uint32_t argCount) {
        if (argCount > 32) argCount = 32;

        if (s_hProc) {
            if (!s_citizenHookInstalled) return nullptr;
            if (IsBlacklisted(hash)) return nullptr;
            uintptr_t handlerVA = 0;
            const NativeInfo* ni = GetInfo(hash);
            if (ni) handlerVA = (uintptr_t)ni->handler;
            if (!handlerVA) {
                uint64_t low32 = hash & 0xFFFFFFFFull;
                if (low32 != hash) {
                    ni = GetInfo(low32);
                    if (ni) handlerVA = (uintptr_t)ni->handler;
                }
            }
            if (!handlerVA) return nullptr;
            uint64_t r[3] = {};
            {
                std::lock_guard<std::mutex> lock(s_invokeMutex);
                if (!ApcRunOnce(handlerVA, args, argCount, r, 500)) return nullptr;
            }
            std::memcpy(s_invoke.resBuf, r, sizeof(r));
            return s_invoke.resBuf;
        }

        NativeHandler fn = GetHandler(hash);
        if (!fn) return nullptr;
        std::memset(&s_invoke, 0, sizeof(s_invoke));
        s_invoke.ctx.NumArgs = argCount;
        s_invoke.ctx.NumResults = 1;
        s_invoke.ctx.Args = s_invoke.argBuf;
        s_invoke.ctx.Results = s_invoke.argBuf;
        if (args && argCount) std::memcpy(s_invoke.argBuf, args, argCount * sizeof(uint64_t));
        fn(&s_invoke.ctx);
        std::memcpy(s_invoke.resBuf, s_invoke.argBuf, sizeof(uint64_t) * 3);
        return s_invoke.resBuf;
    }

    const uint64_t* InvokeByName(const std::string& name, const uint64_t* args, uint32_t argCount) {
        return Invoke(NativeHashFromName(name), args, argCount);
    }

    uintptr_t   CitizenQueueBase() { return s_apcDataVA; }
    uintptr_t   GameBase() { return s_gameBase; }
    size_t      GameSize() { return s_gameSize; }
    uint32_t    Pid() { return s_pid; }
    uint32_t    NativeCount() { return static_cast<uint32_t>(s_natives.size()); }
    uint32_t    ScannedCount() { return s_scannedCount; }
    const std::string& BuildTag() { return s_buildTag; }
    const std::string& ProcessName() { return s_processName; }
    const std::unordered_map<std::string, uint64_t>& AllHashes() { return s_hashes; }
    const std::unordered_map<uint64_t, NativeInfo>& AllNatives() { return s_natives; }

    void SetNativesHppPath(std::string) {}
    void SetCrossmapPath(std::string) {}
    void EnableLogging(bool on) { s_logging = on; }

}
