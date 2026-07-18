#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

extern std::unordered_map<uint32_t, std::string> object_names;

std::string GetObjectNameFromHash(uint32_t hash);
