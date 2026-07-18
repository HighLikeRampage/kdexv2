#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <Security/xorstr.hpp>

namespace Natives {

    struct NativePattern {
        std::string canonical;
        std::unordered_map<int, std::string> variants;
    };

    extern const std::unordered_map<std::string, NativePattern> CROSSMAP_NATIVES;

    const NativePattern* findNativePattern(std::string name);

    const std::string* findPatternForBuild(std::string name, int build);

}
