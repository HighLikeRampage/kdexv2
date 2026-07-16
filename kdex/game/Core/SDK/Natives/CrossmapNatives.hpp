#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace Natives {

    struct NativePattern {
        std::string_view canonical;
        std::unordered_map<int, std::string_view> variants;
    };

    extern const std::unordered_map<std::string_view, NativePattern> CROSSMAP_NATIVES;

    const NativePattern* findNativePattern(std::string_view name);

    const std::string_view* findPatternForBuild(std::string_view name, int build);

}
