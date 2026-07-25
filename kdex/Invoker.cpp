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

namespace Invoker {

    static std::mutex                                        s_mutex;
    static bool                                              s_initialized = false;
    static bool                                              s_logging = false;
    static uintptr_t                                         s_gameBase = 0;
    static size_t                                            s_gameSize = 0;
    static uint32_t                                          s_pid = 0;
    static std::string                                       s_processName;
    static std::string                                       s_buildTag;
    static std::string                                       s_nativesHppPath = xorstr("Natives.hpp");
    static std::string                                       s_crossmapPath = xorstr("Crossmap.hpp");
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

    static std::string ReadFile(const std::string& path) {
        FILE* f = std::fopen(path.c_str(), xorstr("rb"));
        if (!f) return {};
        std::fseek(f, 0, SEEK_END);
        long sz = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        if (sz <= 0) { std::fclose(f); return {}; }
        std::string out(static_cast<size_t>(sz), '\0');
        size_t got = std::fread(out.data(), 1, static_cast<size_t>(sz), f);
        std::fclose(f);
        out.resize(got);
        return out;
    }

    static bool IsIdentChar(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_';
    }

    static bool IsHexChar(char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static bool LoadNativesHpp(const std::string& path) {
        std::string text = ReadFile(path);
        if (text.empty()) return false;
        const std::string kw = xorstr("constexpr");
        size_t pos = 0;
        while ((pos = text.find(kw, pos)) != std::string::npos) {
            size_t p = pos + kw.size();
            pos = p;
            while (p < text.size() && (text[p] == ' ' || text[p] == '\t')) ++p;
            if (text.compare(p, 8, xorstr("uint64_t")) != 0) continue;
            p += 8;
            while (p < text.size() && (text[p] == ' ' || text[p] == '\t')) ++p;
            size_t nameStart = p;
            while (p < text.size() && IsIdentChar(text[p])) ++p;
            if (p == nameStart) continue;
            std::string name = text.substr(nameStart, p - nameStart);
            while (p < text.size() && (text[p] == ' ' || text[p] == '\t' || text[p] == '=')) ++p;
            if (p + 2 > text.size() || text[p] != '0' || (text[p + 1] != 'x' && text[p + 1] != 'X')) continue;
            p += 2;
            size_t hexStart = p;
            while (p < text.size() && IsHexChar(text[p])) ++p;
            if (p == hexStart) continue;
            uint64_t hash = std::stoull(text.substr(hexStart, p - hexStart), nullptr, 16);
            s_hashes[name] = hash;
            s_nameLower[ToLower(name)] = hash;
        }
        return !s_hashes.empty();
    }

    static bool LoadCrossmap(const std::string& path) {
        std::string text = ReadFile(path);
        if (text.empty()) return false;
        size_t pos = 0;
        while (pos < text.size()) {
            size_t brace = text.find('{', pos);
            if (brace == std::string::npos) break;
            size_t p = brace + 1;
            while (p < text.size() && (text[p] == ' ' || text[p] == '\t')) ++p;
            if (p + 2 > text.size() || text[p] != '0' || (text[p + 1] != 'x' && text[p + 1] != 'X')) { pos = brace + 1; continue; }
            p += 2;
            size_t h1s = p;
            while (p < text.size() && IsHexChar(text[p])) ++p;
            if (p == h1s) { pos = brace + 1; continue; }
            uint64_t orig = std::stoull(text.substr(h1s, p - h1s), nullptr, 16);
            while (p < text.size() && (text[p] == ' ' || text[p] == '\t' || text[p] == ',')) ++p;
            if (p + 2 > text.size() || text[p] != '0' || (text[p + 1] != 'x' && text[p + 1] != 'X')) { pos = brace + 1; continue; }
            p += 2;
            size_t h2s = p;
            while (p < text.size() && IsHexChar(text[p])) ++p;
            if (p == h2s) { pos = brace + 1; continue; }
            uint64_t trans = std::stoull(text.substr(h2s, p - h2s), nullptr, 16);
            s_crossmap[orig] = trans;
            pos = p;
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
            Log(xorstr("[scan] loading %s\n"), s_nativesHppPath.c_str());
            if (!LoadNativesHpp(s_nativesHppPath)) {
                Log(xorstr("[scan] failed to load %s\n"), s_nativesHppPath.c_str());
                return false;
            }
            Log(xorstr("[scan] Natives.hpp = %zu entries\n"), s_hashes.size());
        }
        if (s_crossmap.empty()) {
            Log(xorstr("[scan] loading %s\n"), s_crossmapPath.c_str());
            if (!LoadCrossmap(s_crossmapPath)) {
                Log(xorstr("[scan] failed to load %s\n"), s_crossmapPath.c_str());
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
    static uintptr_t s_citizenTableVA = 0;
    static uintptr_t s_citizenSlot = 0;
    static uintptr_t s_citizenOrig = 0;
    static uintptr_t s_citizenCaveVA = 0;
    static uintptr_t s_citizenQueueVA = 0;
    static bool      s_citizenHookInstalled = false;

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

    struct CitizenEntry {
        uint64_t h0;
        uint64_t h1;
        uint64_t fn;
        uintptr_t slot;
    };
    static std::vector<CitizenEntry> s_citizenEntries;
    static std::unordered_map<uint64_t, uintptr_t> s_citizenHandlers;
    static std::vector<CitizenEntry> s_anchorCandidates;
    static std::unordered_map<uintptr_t, bool> s_anchorExhausted;
    static uint64_t s_ggtHandler = 0;

    static const uintptr_t kCitizenTableOffsets[] = {
        0x10BD58, 0x10BCE8, 0x10BC78, 0x10C2A8, 0x10C3A8,
        0x10C4A8, 0x10C5A8, 0x10C6A8, 0x10C7A8
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

    static bool WriteProtected(uintptr_t va, const void* src, size_t len) {
        DWORD oldProt = 0;
        if (!VirtualProtectEx(s_hProc, (LPVOID)va, len, PAGE_EXECUTE_READWRITE, &oldProt)) return false;
        bool ok = WriteMem(va, src, len);
        DWORD tmp;
        VirtualProtectEx(s_hProc, (LPVOID)va, len, oldProt, &tmp);
        return ok;
    }

    static uintptr_t AllocRemote(size_t size, DWORD prot) {
        return (uintptr_t)VirtualAllocEx(s_hProc, nullptr, size, MEM_COMMIT | MEM_RESERVE, prot);
    }

    static bool GetModule(const std::string& nameHint, uintptr_t* outBase, size_t* outSize) {
        HMODULE mods[1024];
        DWORD needed = 0;
        if (!EnumProcessModulesEx(s_hProc, mods, sizeof(mods), &needed, LIST_MODULES_64BIT)) return false;
        size_t count = needed / sizeof(HMODULE);
        std::string lowerHint = ToLower(nameHint);
        for (size_t i = 0; i < count; ++i) {
            char nm[MAX_PATH] = {};
            GetModuleBaseNameA(s_hProc, mods[i], nm, sizeof(nm));
            std::string lo = ToLower(nm);
            if (lo.find(lowerHint) != std::string::npos) {
                MODULEINFO mi = {};
                if (!GetModuleInformation(s_hProc, mods[i], &mi, sizeof(mi))) return false;
                *outBase = (uintptr_t)mods[i];
                *outSize = mi.SizeOfImage;
                return true;
            }
        }
        return false;
    }

    static std::vector<uint8_t> BuildCitizenShellcode(uintptr_t queueVA, uintptr_t origFn) {
        std::vector<uint8_t> c = {
            0x9C,
            0x50, 0x51, 0x41, 0x50, 0x41, 0x51,
            0x48, 0xBA, 0,0,0,0,0,0,0,0,
            0x52,
            0x48, 0x83, 0x82, 0xC0,0x00,0x00,0x00, 0x01,
            0x4C, 0x8B, 0x8A, 0xE0,0x00,0x00,0x00,
            0x4D, 0x85, 0xC9,
            0x74, 0x0F,
            0x4D, 0x8B, 0x01,
            0x4D, 0x85, 0xC0,
            0x74, 0x07,
            0x4C, 0x89, 0x82, 0xE8,0x00,0x00,0x00,
            0x0F, 0xB6, 0x02,
            0x85, 0xC0,
            0x0F, 0x84, 0x7B, 0x00, 0x00, 0x00,
            0xC6, 0x02, 0x00,
            0x4C, 0x8B, 0x8A, 0xE0,0x00,0x00,0x00,
            0x4D, 0x85, 0xC9,
            0x74, 0x19,
            0x4D, 0x8B, 0x01,
            0x4C, 0x89, 0x82, 0xF0,0x00,0x00,0x00,
            0x4C, 0x8B, 0x82, 0xE8,0x00,0x00,0x00,
            0x4D, 0x85, 0xC0,
            0x74, 0x03,
            0x4D, 0x89, 0x01,
            0x4C, 0x8D, 0x92, 0xA0,0x00,0x00,0x00,
            0x4C, 0x8D, 0x9A, 0xC8,0x00,0x00,0x00,
            0x4D, 0x89, 0x1A,
            0x8B, 0x4A, 0x18,
            0x41, 0x89, 0x4A, 0x08,
            0x48, 0x8D, 0x42, 0x20,
            0x49, 0x89, 0x42, 0x10,
            0x48, 0x8B, 0x42, 0x08,
            0x48, 0x8D, 0x8A, 0xA0,0x00,0x00,0x00,
            0x48, 0x83, 0xEC, 0x28,
            0xFF, 0xD0,
            0x48, 0x83, 0xC4, 0x28,
            0x48, 0x8B, 0x14, 0x24,
            0x4C, 0x8B, 0x8A, 0xE0,0x00,0x00,0x00,
            0x4D, 0x85, 0xC9,
            0x74, 0x0A,
            0x4C, 0x8B, 0x82, 0xF0,0x00,0x00,0x00,
            0x4D, 0x89, 0x01,
            0xC6, 0x42, 0x01, 0x01,
            0x5A, 0x41, 0x59, 0x41, 0x58, 0x59, 0x58, 0x9D,
            0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
            0,0,0,0,0,0,0,0,
        };
        std::memcpy(c.data() + 9, &queueVA, 8);
        std::memcpy(c.data() + 201, &origFn, 8);
        return c;
    }

    static bool LoadCitizenNativeTable() {
        s_citizenEntries.clear();
        s_citizenHandlers.clear();
        if (!GetModule(xorstr("citizen-scripting-core.dll"), &s_citizenCoreBase, &s_citizenCoreSize)) {
            Log(xorstr("[citizen] core.dll not found\n")); return false;
        }
        Log(xorstr("[citizen] core base 0x%llx size %zu\n"), (unsigned long long)s_citizenCoreBase, s_citizenCoreSize);

        s_citizenTableVA = 0;
        size_t count = 0;
        for (uintptr_t off : kCitizenTableOffsets) {
            uintptr_t tva = s_citizenCoreBase + off;
            uint64_t tBegin = 0, tEnd = 0;
            if (!ReadMem(tva, &tBegin, 8) || !ReadMem(tva + 8, &tEnd, 8)) continue;
            if (!tBegin || tEnd <= tBegin) continue;
            size_t bytes = (size_t)(tEnd - tBegin);
            if (bytes % sizeof(void*) != 0) continue;
            size_t cnt = bytes / sizeof(void*);
            if (cnt < 64 || cnt > 65536) continue;
            uint32_t valid = 0;
            for (size_t i = 0; i < 16 && i < cnt; ++i) {
                uint64_t entPtr = 0;
                if (!ReadMem(tBegin + i * 8, &entPtr, 8) || !entPtr) continue;
                uint64_t fn = 0;
                if (!ReadMem(entPtr + 0x18, &fn, 8)) continue;
                if (fn > 0x10000) ++valid;
            }
            if (valid < 2) continue;
            s_citizenTableVA = tva;
            count = cnt;
            Log(xorstr("[citizen] table @ +0x%llx  count=%zu\n"), (unsigned long long)off, cnt);
            break;
        }
        if (!s_citizenTableVA) { Log(xorstr("[citizen] table not found\n")); return false; }

        uint64_t tBegin = 0;
        ReadMem(s_citizenTableVA, &tBegin, 8);
        for (size_t i = 0; i < count; ++i) {
            uint64_t entPtr = 0;
            if (!ReadMem(tBegin + i * 8, &entPtr, 8) || !entPtr) continue;
            CitizenEntry ce = {};
            ReadMem(entPtr, &ce.h0, 8);
            ReadMem(entPtr + 8, &ce.h1, 8);
            ReadMem(entPtr + 0x18, &ce.fn, 8);
            ce.slot = entPtr + 0x18;
            if (!ce.fn) continue;
            s_citizenEntries.push_back(ce);
            if (ce.h0) s_citizenHandlers[ce.h0] = ce.fn;
            if (ce.h1 && ce.h1 != ce.h0) s_citizenHandlers[ce.h1] = ce.fn;
        }
        Log(xorstr("[citizen] loaded %zu entries\n"), s_citizenEntries.size());
        return !s_citizenEntries.empty();
    }

    static uintptr_t CitizenLookup(uint64_t hash) {
        auto it = s_citizenHandlers.find(hash);
        if (it != s_citizenHandlers.end()) return it->second;
        uint64_t low32 = hash & 0xFFFFFFFFull;
        it = s_citizenHandlers.find(low32);
        return it != s_citizenHandlers.end() ? it->second : 0;
    }

    static bool ProbeAnchor(uintptr_t slotVA, uintptr_t origFn, uintptr_t ggtHandler) {
        std::vector<uint8_t> sc = BuildCitizenShellcode(s_citizenQueueVA, origFn);
        WriteMem(s_citizenCaveVA, sc.data(), sc.size());

        uint64_t caveVA = s_citizenCaveVA;
        if (!WriteProtected(slotVA, &caveVA, 8)) return false;

        uint8_t zeros[2] = {};
        WriteMem(s_citizenQueueVA, zeros, 2);
        WriteMem(s_citizenQueueVA + 0x08, &ggtHandler, 8);
        uint32_t argCount = 0;
        WriteMem(s_citizenQueueVA + 0x18, &argCount, 4);
        uint8_t one = 1;
        WriteMem(s_citizenQueueVA, &one, 1);

        bool ok = false;
        for (int i = 0; i < 10; ++i) {
            Sleep(10);
            uint8_t done = 0;
            if (ReadMem(s_citizenQueueVA + 1, &done, 1) && done) {
                uint64_t r = 0;
                if (ReadMem(s_citizenQueueVA + 0xC8, &r, 8) && r != 0) ok = true;
                break;
            }
        }

        WriteProtected(slotVA, &origFn, 8);
        return ok;
    }

    static bool TryInstallAnchor(const CitizenEntry& c) {
        if (!ProbeAnchor(c.slot, c.fn, s_ggtHandler)) return false;
        s_citizenSlot = c.slot;
        s_citizenOrig = c.fn;
        std::vector<uint8_t> sc = BuildCitizenShellcode(s_citizenQueueVA, s_citizenOrig);
        WriteMem(s_citizenCaveVA, sc.data(), sc.size());
        WriteProtected(s_citizenSlot, &s_citizenCaveVA, 8);
        Log(xorstr("[citizen] anchor @ slot 0x%llx\n"), (unsigned long long)c.slot);
        return true;
    }

    static bool RotateAnchor() {
        if (s_citizenSlot && s_citizenOrig) {
            WriteProtected(s_citizenSlot, &s_citizenOrig, 8);
            s_anchorExhausted[s_citizenSlot] = true;
        }
        for (const auto& c : s_anchorCandidates) {
            if (s_anchorExhausted.count(c.slot)) continue;
            if (TryInstallAnchor(c)) return true;
        }
        return false;
    }

    bool InstallCitizenHook() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_citizenHookInstalled) return true;
        if (!s_hProc) { Log(xorstr("[citizen] not attached\n")); return false; }
        if (!LoadCitizenNativeTable()) return false;

        uint64_t ggtHash = 0;
        {
            auto it = s_hashes.find(xorstr("GET_GAME_TIMER"));
            if (it == s_hashes.end()) { Log(xorstr("[citizen] GET_GAME_TIMER not in Natives.hpp\n")); return false; }
            ggtHash = it->second;
        }
        s_ggtHandler = CitizenLookup(ggtHash);
        if (!s_ggtHandler) { Log(xorstr("[citizen] GET_GAME_TIMER not in citizen table\n")); return false; }
        Log(xorstr("[citizen] GGT handler 0x%llx\n"), (unsigned long long)s_ggtHandler);

        s_citizenQueueVA = AllocRemote(0x1000, PAGE_READWRITE);
        s_citizenCaveVA = AllocRemote(0x1000, PAGE_EXECUTE_READWRITE);
        if (!s_citizenQueueVA || !s_citizenCaveVA) { Log(xorstr("[citizen] alloc failed\n")); return false; }

        {
            uint64_t tlsCtl[3] = { (uint64_t)s_scrThreadInstVA, 0ull, 0ull };
            WriteMem(s_citizenQueueVA + 0xE0, tlsCtl, sizeof(tlsCtl));
            Log(xorstr("[citizen] scrThread::sm_Instance VA = 0x%llx (%s)\n"),
                (unsigned long long)s_scrThreadInstVA,
                s_scrThreadInstVA ? xorstr("TLS bridge enabled") : xorstr("TLS bridge OFF - scan didn't find it"));
        }

        s_anchorCandidates.clear();
        s_anchorExhausted.clear();
        for (const auto& e : s_citizenEntries) {
            if (e.fn >= s_gameBase && e.fn < s_gameBase + s_gameSize)
                s_anchorCandidates.push_back(e);
        }
        Log(xorstr("[citizen] %zu anchor candidates\n"), s_anchorCandidates.size());

        bool found = false;
        for (const auto& c : s_anchorCandidates) {
            if (TryInstallAnchor(c)) { found = true; break; }
            s_anchorExhausted[c.slot] = true;
        }
        if (!found) {
            Log(xorstr("[citizen] no working anchor\n"));
            VirtualFreeEx(s_hProc, (LPVOID)s_citizenQueueVA, 0, MEM_RELEASE);
            VirtualFreeEx(s_hProc, (LPVOID)s_citizenCaveVA, 0, MEM_RELEASE);
            s_citizenQueueVA = s_citizenCaveVA = 0;
            return false;
        }
        s_citizenHookInstalled = true;
        Log(xorstr("[citizen] hook installed\n"));
        return true;
    }

    bool IsCitizenHookInstalled() { return s_citizenHookInstalled; }

    void UninstallCitizenHook() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (!s_citizenHookInstalled) return;
        if (s_citizenSlot && s_citizenOrig) WriteProtected(s_citizenSlot, &s_citizenOrig, 8);
        if (s_citizenQueueVA) VirtualFreeEx(s_hProc, (LPVOID)s_citizenQueueVA, 0, MEM_RELEASE);
        if (s_citizenCaveVA)  VirtualFreeEx(s_hProc, (LPVOID)s_citizenCaveVA, 0, MEM_RELEASE);
        s_citizenQueueVA = s_citizenCaveVA = s_citizenSlot = s_citizenOrig = 0;
        s_citizenHookInstalled = false;
    }

    static bool CitizenRunOnce(uintptr_t handlerVA, const uint64_t* args, uint32_t argCount, uint64_t* outResult, uint32_t timeoutMs) {
        if (!s_citizenHookInstalled) return false;
        uint8_t zeros[2] = {};
        WriteMem(s_citizenQueueVA, zeros, 2);
        WriteMem(s_citizenQueueVA + 0x08, &handlerVA, 8);
        uint64_t argBuf[8] = {};
        for (uint32_t i = 0; i < argCount && i < 8; ++i) argBuf[i] = args[i];
        WriteMem(s_citizenQueueVA + 0x20, argBuf, sizeof(argBuf));
        uint32_t ac32 = argCount;
        WriteMem(s_citizenQueueVA + 0x18, &ac32, 4);
        uint64_t clearRes[3] = {};
        WriteMem(s_citizenQueueVA + 0xC8, clearRes, sizeof(clearRes));
        uint8_t one = 1;
        WriteMem(s_citizenQueueVA, &one, 1);

        auto start = GetTickCount64();
        while (GetTickCount64() - start < timeoutMs) {
            uint8_t done = 0;
            if (ReadMem(s_citizenQueueVA + 1, &done, 1) && done) {
                if (outResult) ReadMem(s_citizenQueueVA + 0xC8, outResult, sizeof(uint64_t) * 3);
                return true;
            }
            Sleep(1);
        }
        uint8_t reset = 0;
        WriteMem(s_citizenQueueVA, &reset, 1);
        return false;
    }

    static bool CitizenRun(uintptr_t handlerVA, const uint64_t* args, uint32_t argCount, uint64_t* outResult, uint32_t timeoutMs) {
        if (CitizenRunOnce(handlerVA, args, argCount, outResult, timeoutMs)) return true;
        Log(xorstr("[citizen] anchor 0x%llx exhausted, rotating\n"), (unsigned long long)s_citizenSlot);
        for (int tries = 0; tries < 8; ++tries) {
            if (!RotateAnchor()) return false;
            if (CitizenRunOnce(handlerVA, args, argCount, outResult, timeoutMs)) return true;
        }
        return false;
    }

    void LoadBlacklist(const std::string& /*unused*/) {
        s_blacklist.clear();
        for (const char* const* p = kBuiltinBlacklist; *p; ++p) {
            uint64_t h = NativeHashFromName(*p);
            if (h) s_blacklist[h] = true;
        }
        Log(xorstr("[blacklist] %zu built-in entries\n"), s_blacklist.size());
    }

    void SaveBlacklist(const std::string& /*unused*/) {}

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
        uintptr_t va = AllocRemote(s.size() + 1, PAGE_READWRITE);
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
            uintptr_t handlerVA = CitizenLookup(hash);
            if (!handlerVA) {
                const NativeInfo* ni = GetInfo(hash);
                if (ni) handlerVA = (uintptr_t)ni->handler;
            }
            if (!handlerVA) return nullptr;
            uint64_t r[3] = {};
            {
                std::lock_guard<std::mutex> lock(s_invokeMutex);
                if (!CitizenRun(handlerVA, args, argCount, r, 500)) return nullptr;
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

    uintptr_t   CitizenQueueBase() { return s_citizenQueueVA; }
    uintptr_t   GameBase() { return s_gameBase; }
    size_t      GameSize() { return s_gameSize; }
    uint32_t    Pid() { return s_pid; }
    uint32_t    NativeCount() { return static_cast<uint32_t>(s_natives.size()); }
    uint32_t    ScannedCount() { return s_scannedCount; }
    const std::string& BuildTag() { return s_buildTag; }
    const std::string& ProcessName() { return s_processName; }
    const std::unordered_map<std::string, uint64_t>& AllHashes() { return s_hashes; }
    const std::unordered_map<uint64_t, NativeInfo>& AllNatives() { return s_natives; }

    void SetNativesHppPath(std::string path) { s_nativesHppPath = std::move(path); }
    void SetCrossmapPath(std::string path) { s_crossmapPath = std::move(path); }
    void EnableLogging(bool on) { s_logging = on; }

}
