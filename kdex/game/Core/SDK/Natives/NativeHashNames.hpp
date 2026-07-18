#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Natives {
    const std::unordered_map<uint64_t, std::string>& HashToName();
}
