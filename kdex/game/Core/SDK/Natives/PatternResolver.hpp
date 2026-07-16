#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

class PatternResolver {
public:
  struct Pattern {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> mask;
  };

  static Pattern compilePattern(const std::string& patternStr) {
    Pattern pat;
    std::string part;
    for (size_t i = 0; i < patternStr.size(); i++) {
      char c = patternStr[i];
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        if (!part.empty()) {
          if (part == "?" || part == "??") {
            pat.bytes.push_back(0);
            pat.mask.push_back(0);
          } else {
            pat.bytes.push_back(static_cast<uint8_t>(std::stoul(part, nullptr, 16)));
            pat.mask.push_back(1);
          }
          part.clear();
        }
      } else {
        part += c;
      }
    }
    if (!part.empty()) {
      if (part == "?" || part == "??") {
        pat.bytes.push_back(0);
        pat.mask.push_back(0);
      } else {
        pat.bytes.push_back(static_cast<uint8_t>(std::stoul(part, nullptr, 16)));
        pat.mask.push_back(1);
      }
    }
    return pat;
  }

  static int64_t findPatternInBuf(const uint8_t* buf, size_t bufLen, const Pattern& pat, size_t start = 0) {
    if (pat.bytes.empty() || bufLen < pat.bytes.size()) return -1;
    size_t n = pat.bytes.size();
    size_t end = bufLen - n;
    for (size_t i = start; i <= end; i++) {
      if (pat.mask[0] && buf[i] != pat.bytes[0]) continue;
      bool ok = true;
      for (size_t j = 1; j < n; j++) {
        if (pat.mask[j] && buf[i + j] != pat.bytes[j]) {
          ok = false;
          break;
        }
      }
      if (ok) return static_cast<int64_t>(i);
    }
    return -1;
  }

  static uint64_t followStub(const uint8_t* image, size_t imageLen, uint64_t rva, uint64_t moduleBase, uint64_t imageSize) {
    if (rva >= imageSize) return rva;

    auto isValidCodeAddress = [moduleBase, imageSize](uint64_t addr) -> bool {
      if (addr < 0x10000) return false;
      if (addr >= moduleBase + imageSize) return false;
      return true;
    };

    auto isValidHandlerAddress = [imageLen, &isValidCodeAddress](uint64_t addr) -> bool {
      if (!isValidCodeAddress(addr)) return false;
      if (addr < 0x10000) return false;
      return true;
    };

    uint64_t currentRva = rva;
    const int MAX_HOPS = 3;
    std::vector<uint64_t> seen;

    for (int hop = 0; hop < MAX_HOPS; hop++) {
      for (auto s : seen) if (s == currentRva) return currentRva;
      seen.push_back(currentRva);

      if (currentRva + 32 > imageLen) return currentRva;
      const uint8_t* b = image + currentRva;

      if (b[0] == 0x41 && b[1] == 0x51 && b[2] == 0x4C && b[3] == 0x8D && b[4] == 0x0D) {
        int32_t d = *reinterpret_cast<const int32_t*>(b + 5);
        uint64_t result = currentRva + 9 + d;
        if (isValidHandlerAddress(moduleBase + result)) return moduleBase + result;
        return currentRva;
      }

      if (b[0] == 0xFF && b[1] == 0x25) {
        int32_t d = *reinterpret_cast<const int32_t*>(b + 2);
        uint64_t ptrRva = currentRva + 6 + d;
        if (ptrRva + 8 <= imageLen) {
          uint64_t target = *reinterpret_cast<const uint64_t*>(image + ptrRva);
          if (isValidHandlerAddress(target)) return target;
        }
        return currentRva;
      }

      if (b[0] == 0xE9) {
        int32_t d = *reinterpret_cast<const int32_t*>(b + 1);
        uint64_t next = currentRva + 5 + d;
        if (next < imageSize) {
          currentRva = next;
          continue;
        }
        return currentRva;
      }

      if (b[0] == 0x48 && b[1] == 0xB8 && currentRva + 12 <= imageLen && b[10] == 0xFF && b[11] == 0xE0) {
        uint64_t target = *reinterpret_cast<const uint64_t*>(b + 2);
        if (isValidHandlerAddress(target)) return target;
        return currentRva;
      }

      for (int off = 1; off <= 4; off++) {
        if (off + 7 > static_cast<int>(imageLen)) break;
        if (b[off] == 0x4C && b[off + 1] == 0x8D && b[off + 2] == 0x0D) {
          uint8_t prev = off > 0 ? b[off - 1] : 0;
          bool isPush = (prev >= 0x50 && prev <= 0x57);
          if (!isPush) continue;
          int32_t d = *reinterpret_cast<const int32_t*>(b + off + 3);
          uint64_t result = currentRva + off + 7 + d;
          if (isValidHandlerAddress(moduleBase + result)) return moduleBase + result;
        }
      }

      if (b[9] == 0xE9) {
        int32_t d = *reinterpret_cast<const int32_t*>(b + 10);
        uint64_t next = currentRva + 14 + d;
        if (next < imageSize) {
          currentRva = next;
          continue;
        }
      }

      return currentRva;
    }
    return currentRva;
  }
};
