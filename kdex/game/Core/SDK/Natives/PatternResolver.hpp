#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <Security/xorstr.hpp>

class PatternResolver {
public:
    struct Pattern {
        std::vector<uint8_t> bytes;
        std::vector<uint8_t> mask;
    };

    static Pattern compilePattern(std::string_view src) {
        Pattern p;
        size_t i = 0;
        while (i < src.size()) {
            while (i < src.size() && (src[i] == ' ' || src[i] == '\t' ||
                                       src[i] == '\n' || src[i] == '\r')) ++i;
            if (i >= src.size()) break;

            if (src[i] == '?') {
                p.bytes.push_back(0);
                p.mask.push_back(0);
                while (i < src.size() && src[i] == '?') ++i;
                continue;
            }

            char buf[3] = { 0, 0, 0 };
            buf[0] = src[i++];
            if (i < src.size() && src[i] != ' ' && src[i] != '\t' &&
                src[i] != '\n' && src[i] != '\r') buf[1] = src[i++];
            p.bytes.push_back(static_cast<uint8_t>(std::strtoul(buf, nullptr, 16)));
            p.mask.push_back(1);
        }
        return p;
    }

    static int64_t findPatternInBuf(const uint8_t* buf, size_t bufLen,
                                     const Pattern& pat, size_t start = 0) {
        const size_t n = pat.bytes.size();
        if (!n || bufLen < n) return -1;
        const size_t end = bufLen - n;
        const uint8_t first = pat.bytes[0];
        const bool firstMasked = pat.mask[0] != 0;
        for (size_t i = start; i <= end; ++i) {
            if (firstMasked && buf[i] != first) continue;
            size_t j = 1;
            for (; j < n; ++j) {
                if (pat.mask[j] && buf[i + j] != pat.bytes[j]) break;
            }
            if (j == n) return static_cast<int64_t>(i);
        }
        return -1;
    }

    static size_t unwrapPatternMatch(const uint8_t* image, size_t imageLen, size_t hitRva) {
        if (hitRva + 5 > imageLen) return hitRva;
        if (image[hitRva] != 0xE9) return hitRva;
        int32_t disp = 0;
        std::memcpy(&disp, image + hitRva + 1, 4);
        const int64_t target = static_cast<int64_t>(hitRva) + 5 + disp;
        if (target < 0 || static_cast<size_t>(target) + 1 >= imageLen) return hitRva;
        return static_cast<size_t>(target);
    }

    static uint64_t followStubInImage(const uint8_t* image, size_t imageLen,
                                       uint64_t moduleBase, uint64_t startRva) {
        auto isCode = [&](uint64_t addr) {
            return addr >= 0x10000ULL && addr < moduleBase + imageLen &&
                   addr >= moduleBase;
        };
        auto isHandler = [&](uint64_t addr) {
            if (!isCode(addr)) return false;
            const size_t rva = addr - moduleBase;
            if (rva >= imageLen) return false;
            const uint8_t b = image[rva];
            return b == 0x55 || b == 0x48 || b == 0x4C || b == 0x49 ||
                   b == 0xE9 || b == 0xEB ||
                   b == 0x40 || b == 0x53 || b == 0x56 || b == 0x57 ||
                   b == 0x41;
        };

        uint64_t cur = startRva;
        constexpr int MAX_HOPS = 3;
        std::vector<uint64_t> seen;

        for (int hop = 0; hop < MAX_HOPS; ++hop) {
            for (auto s : seen) if (s == cur) return 0;
            seen.push_back(cur);
            if (cur + 32 > imageLen) return 0;

            const uint8_t* b = image + cur;

            if (b[0] == 0x41 && b[1] == 0x51 && b[2] == 0x4C && b[3] == 0x8D && b[4] == 0x0D) {
                int32_t d = 0; std::memcpy(&d, b + 5, 4);
                const uint64_t r = moduleBase + cur + 9 + static_cast<int64_t>(d);
                return isHandler(r) ? r : 0;
            }

            if (b[0] == 0xFF && b[1] == 0x25) {
                int32_t d = 0; std::memcpy(&d, b + 2, 4);
                const uint64_t p = cur + 6 + static_cast<int64_t>(d);
                if (p + 8 > imageLen) return 0;
                uint64_t t = 0; std::memcpy(&t, image + p, 8);
                return (t && isHandler(t)) ? t : 0;
            }

            if (b[0] == 0xE9) {
                int32_t d = 0; std::memcpy(&d, b + 1, 4);
                const uint64_t n = cur + 5 + static_cast<int64_t>(d);
                if (n >= imageLen) return 0;
                cur = n;
                continue;
            }

            if (b[0] == 0x48 && b[1] == 0xB8 && b[10] == 0xFF && b[11] == 0xE0) {
                uint64_t t = 0; std::memcpy(&t, b + 2, 8);
                return isHandler(t) ? t : 0;
            }

            bool matched = false;
            for (int off = 1; off <= 4; ++off) {
                if (b[off] == 0x4C && b[off + 1] == 0x8D && b[off + 2] == 0x0D) {
                    const uint8_t prev = b[off - 1];
                    const bool basicPush = (prev >= 0x50 && prev <= 0x57);
                    const bool rexPush   = (off >= 2 && b[off - 2] == 0x41 &&
                                            prev >= 0x50 && prev <= 0x57);
                    if (!(basicPush || rexPush)) continue;
                    int32_t d = 0; std::memcpy(&d, b + off + 3, 4);
                    const uint64_t r = moduleBase + cur + off + 7 + static_cast<int64_t>(d);
                    if (isHandler(r)) return r;
                    matched = true;
                    break;
                }
            }
            if (matched) return 0;

            if (b[9] == 0xE9) {
                int32_t d = 0; std::memcpy(&d, b + 10, 4);
                const uint64_t n = cur + 14 + static_cast<int64_t>(d);
                if (n < imageLen) { cur = n; continue; }
            }

            if (isHandler(moduleBase + cur)) return moduleBase + cur;
            return 0;
        }
        return 0;
    }
};
