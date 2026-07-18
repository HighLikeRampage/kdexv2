#!/usr/bin/env python3
"""
Regenerate CrossmapNatives / NativeHashNames / ObjectNames as a single
encrypted binary blob per file plus a small runtime decrypt+populate
init. Kills the tens of thousands of per-literal xorstr_lite lambdas
that were sending cl.exe/link.exe into the tens of minutes range.

Reads plain source dumps from a directory (the pre-wrap versions we
extracted from git) and writes into the project tree.
"""
from __future__ import annotations
import io, os, re, struct, sys

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME_DIR = os.path.join(PROJECT_ROOT, "game")

CROSSMAP_KEY  = 0x9E37AB1D
HASHNAMES_KEY = 0xC2B2AE35
OBJECTNAMES_KEY = 0x27D4EB2F

# ---------------------------------------------------------------------
# stream cipher (matches the runtime decrypt embedded in each .cpp)
# ---------------------------------------------------------------------
def encrypt_blob(plain: bytes, key: int) -> bytes:
    out = bytearray(len(plain))
    k = (key ^ 0x9E3779B9) & 0xFFFFFFFF
    for i, b in enumerate(plain):
        k = (k * 1103515245 + 12345) & 0xFFFFFFFF
        k ^= (k >> 13)
        out[i] = b ^ ((k >> 16) & 0xff)
    return bytes(out)

def emit_byte_array(name: str, data: bytes) -> str:
    """Emit `alignas(16) static const unsigned char <name>[] = { ... };`
    with 32 bytes per line for cl.exe-friendly parsing."""
    lines = [f"alignas(16) static const unsigned char {name}[{len(data)}] = {{"]
    for chunk_start in range(0, len(data), 32):
        chunk = data[chunk_start:chunk_start + 32]
        lines.append("    " + ",".join(f"0x{b:02X}" for b in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)

# ---------------------------------------------------------------------
# parsers for the three plain files
# ---------------------------------------------------------------------
def parse_hashnames(text: str) -> list[tuple[int, str]]:
    entries = []
    for m in re.finditer(r'\{\s*(0x[0-9A-Fa-f]+)ULL\s*,\s*"([^"]*)"\s*\}', text):
        entries.append((int(m.group(1), 16), m.group(2)))
    return entries

def parse_objectnames(text: str) -> list[tuple[int, str]]:
    entries = []
    for m in re.finditer(r'\{\s*(0x[0-9A-Fa-f]+)\s*,\s*"([^"]*)"\s*\}', text):
        entries.append((int(m.group(1), 16), m.group(2)))
    return entries

def parse_crossmap(text: str) -> list[tuple[str, str, list[tuple[int, str]]]]:
    """Return list of (name, canonical, [(build, pattern), ...])."""
    # Structure per entry:
    #   { "NAME", { "CANON", { { build, "PAT" }, ... } } }
    entries = []
    # Iterate positions of top-level entries: pattern {\s*"NAME",\s*{\s*"CANON",\s*{
    entry_re = re.compile(
        r'\{\s*"((?:[^"\\]|\\.)*)"\s*,\s*\{\s*"((?:[^"\\]|\\.)*)"\s*,\s*\{',
        re.DOTALL,
    )
    variant_re = re.compile(r'\{\s*(\d+)\s*,\s*"((?:[^"\\]|\\.)*)"\s*\}')
    pos = 0
    while True:
        m = entry_re.search(text, pos)
        if not m:
            break
        name = m.group(1)
        canonical = m.group(2)
        # Walk from end of match forward until matching brace closes the variants sub-block
        depth = 1
        i = m.end()
        variant_start = i
        while i < len(text) and depth > 0:
            c = text[i]
            if c == '{': depth += 1
            elif c == '}': depth -= 1
            i += 1
        # The chunk text[variant_start:i-1] is the variants body
        variants = []
        for v in variant_re.finditer(text, variant_start, i - 1):
            variants.append((int(v.group(1)), v.group(2)))
        entries.append((name, canonical, variants))
        pos = i
    return entries

# ---------------------------------------------------------------------
# blob encoders
# ---------------------------------------------------------------------
def encode_hashnames(entries: list[tuple[int, str]]) -> bytes:
    buf = bytearray()
    buf += struct.pack("<I", len(entries))
    for k, v in entries:
        vb = v.encode("utf-8", errors="strict")
        assert len(vb) < 65536
        buf += struct.pack("<QH", k, len(vb))
        buf += vb
    return bytes(buf)

def encode_objectnames(entries: list[tuple[int, str]]) -> bytes:
    buf = bytearray()
    buf += struct.pack("<I", len(entries))
    for k, v in entries:
        vb = v.encode("utf-8", errors="strict")
        assert len(vb) < 65536
        buf += struct.pack("<IH", k, len(vb))
        buf += vb
    return bytes(buf)

def encode_crossmap(entries: list[tuple[str, str, list[tuple[int, str]]]]) -> bytes:
    buf = bytearray()
    buf += struct.pack("<I", len(entries))
    for name, canonical, variants in entries:
        nb = name.encode("utf-8")
        cb = canonical.encode("utf-8")
        assert len(nb) < 65536
        buf += struct.pack("<H", len(nb)) + nb
        buf += struct.pack("<I", len(cb)) + cb
        buf += struct.pack("<H", len(variants))
        for build, pat in variants:
            pb = pat.encode("utf-8")
            buf += struct.pack("<iI", build, len(pb))
            buf += pb
    return bytes(buf)

# ---------------------------------------------------------------------
# runtime decrypt template (identical in every generated file)
# ---------------------------------------------------------------------
DECRYPT_HELPERS = r"""
namespace {

inline void kd_decrypt_blob(const unsigned char* in, size_t n, unsigned char* out, uint32_t key) noexcept {
    uint32_t k = key ^ 0x9E3779B9u;
    for (size_t i = 0; i < n; ++i) {
        k = k * 1103515245u + 12345u;
        k ^= (k >> 13);
        out[i] = static_cast<unsigned char>(in[i] ^ static_cast<unsigned char>((k >> 16) & 0xffu));
    }
}

struct kd_blob_reader {
    const unsigned char* p;
    const unsigned char* end;
    template<typename T> T read() noexcept {
        T v{};
        std::memcpy(&v, p, sizeof(T));
        p += sizeof(T);
        return v;
    }
    std::string read_str(size_t n) noexcept {
        std::string s(reinterpret_cast<const char*>(p), n);
        p += n;
        return s;
    }
};

}
"""

# ---------------------------------------------------------------------
# writers
# ---------------------------------------------------------------------
def write_hashnames(entries):
    plain = encode_hashnames(entries)
    enc = encrypt_blob(plain, HASHNAMES_KEY)
    hpp = f"""#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace Natives {{
    const std::unordered_map<uint64_t, std::string>& HashToName();
}}
"""
    cpp = f"""#include "NativeHashNames.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

{DECRYPT_HELPERS}

namespace Natives {{

{emit_byte_array("HASHNAMES_BLOB", enc)}
static constexpr uint32_t HASHNAMES_KEY = 0x{HASHNAMES_KEY:08X}u;
static constexpr size_t HASHNAMES_BLOB_SIZE = sizeof(HASHNAMES_BLOB);

static std::unordered_map<uint64_t, std::string> BuildHashToNameMap() {{
    std::vector<unsigned char> plain(HASHNAMES_BLOB_SIZE);
    kd_decrypt_blob(HASHNAMES_BLOB, HASHNAMES_BLOB_SIZE, plain.data(), HASHNAMES_KEY);
    kd_blob_reader r{{ plain.data(), plain.data() + plain.size() }};
    uint32_t count = r.read<uint32_t>();
    std::unordered_map<uint64_t, std::string> m;
    m.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {{
        uint64_t key = r.read<uint64_t>();
        uint16_t len = r.read<uint16_t>();
        m.emplace(key, r.read_str(len));
    }}
    return m;
}}

const std::unordered_map<uint64_t, std::string>& HashToName() {{
    static const std::unordered_map<uint64_t, std::string> map = BuildHashToNameMap();
    return map;
}}

}}
"""
    with open(os.path.join(GAME_DIR, "Core", "SDK", "Natives", "NativeHashNames.hpp"), "w", encoding="utf-8", newline="") as f:
        f.write(hpp)
    with open(os.path.join(GAME_DIR, "Core", "SDK", "Natives", "NativeHashNames.cpp"), "w", encoding="utf-8", newline="") as f:
        f.write(cpp)
    print(f"NativeHashNames: {len(entries)} entries, blob {len(enc)} bytes", file=sys.stderr)

def write_objectnames(entries):
    plain = encode_objectnames(entries)
    enc = encrypt_blob(plain, OBJECTNAMES_KEY)
    hpp = f"""#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

extern std::unordered_map<uint32_t, std::string> object_names;

std::string GetObjectNameFromHash(uint32_t hash);
"""
    cpp = f"""#include "ObjectNames.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

{DECRYPT_HELPERS}

namespace {{

{emit_byte_array("OBJECT_NAMES_BLOB", enc)}
static constexpr uint32_t OBJECT_NAMES_KEY = 0x{OBJECTNAMES_KEY:08X}u;
static constexpr size_t OBJECT_NAMES_BLOB_SIZE = sizeof(OBJECT_NAMES_BLOB);

static std::unordered_map<uint32_t, std::string> BuildObjectNamesMap() {{
    std::vector<unsigned char> plain(OBJECT_NAMES_BLOB_SIZE);
    kd_decrypt_blob(OBJECT_NAMES_BLOB, OBJECT_NAMES_BLOB_SIZE, plain.data(), OBJECT_NAMES_KEY);
    kd_blob_reader r{{ plain.data(), plain.data() + plain.size() }};
    uint32_t count = r.read<uint32_t>();
    std::unordered_map<uint32_t, std::string> m;
    m.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {{
        uint32_t key = r.read<uint32_t>();
        uint16_t len = r.read<uint16_t>();
        m.emplace(key, r.read_str(len));
    }}
    return m;
}}

}}

std::unordered_map<uint32_t, std::string> object_names = BuildObjectNamesMap();

std::string GetObjectNameFromHash(uint32_t hash) {{
    auto it = object_names.find(hash);
    if (it != object_names.end()) return it->second;
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%X", hash);
    return std::string(buf);
}}
"""
    with open(os.path.join(GAME_DIR, "Core", "Features", "ObjectNames.hpp"), "w", encoding="utf-8", newline="") as f:
        f.write(hpp)
    with open(os.path.join(GAME_DIR, "Core", "Features", "ObjectNames.cpp"), "w", encoding="utf-8", newline="") as f:
        f.write(cpp)
    print(f"ObjectNames: {len(entries)} entries, blob {len(enc)} bytes", file=sys.stderr)

def write_crossmap(entries):
    plain = encode_crossmap(entries)
    enc = encrypt_blob(plain, CROSSMAP_KEY)
    hpp = f"""#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Natives {{

    struct NativePattern {{
        std::string canonical;
        std::unordered_map<int, std::string> variants;
    }};

    extern const std::unordered_map<std::string, NativePattern> CROSSMAP_NATIVES;

    const NativePattern* findNativePattern(std::string_view name);

    const std::string* findPatternForBuild(std::string_view name, int build);

}}
"""
    cpp = f"""#include "CrossmapNatives.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

{DECRYPT_HELPERS}

namespace Natives {{

{emit_byte_array("CROSSMAP_BLOB", enc)}
static constexpr uint32_t CROSSMAP_KEY = 0x{CROSSMAP_KEY:08X}u;
static constexpr size_t CROSSMAP_BLOB_SIZE = sizeof(CROSSMAP_BLOB);

static std::unordered_map<std::string, NativePattern> BuildCrossmapNatives() {{
    std::vector<unsigned char> plain(CROSSMAP_BLOB_SIZE);
    kd_decrypt_blob(CROSSMAP_BLOB, CROSSMAP_BLOB_SIZE, plain.data(), CROSSMAP_KEY);
    kd_blob_reader r{{ plain.data(), plain.data() + plain.size() }};
    uint32_t count = r.read<uint32_t>();
    std::unordered_map<std::string, NativePattern> m;
    m.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {{
        uint16_t nlen = r.read<uint16_t>();
        std::string name = r.read_str(nlen);
        uint32_t clen = r.read<uint32_t>();
        NativePattern p;
        p.canonical = r.read_str(clen);
        uint16_t vcount = r.read<uint16_t>();
        p.variants.reserve(vcount);
        for (uint16_t j = 0; j < vcount; ++j) {{
            int32_t build = r.read<int32_t>();
            uint32_t plen = r.read<uint32_t>();
            p.variants.emplace(build, r.read_str(plen));
        }}
        m.emplace(std::move(name), std::move(p));
    }}
    return m;
}}

const std::unordered_map<std::string, NativePattern> CROSSMAP_NATIVES = BuildCrossmapNatives();

const NativePattern* findNativePattern(std::string_view name) {{
    auto it = CROSSMAP_NATIVES.find(std::string(name));
    if (it != CROSSMAP_NATIVES.end()) {{
        return &(it->second);
    }}
    return nullptr;
}}

const std::string* findPatternForBuild(std::string_view name, int build) {{
    const NativePattern* entry = findNativePattern(name);
    if (!entry) return nullptr;
    auto it = entry->variants.find(build);
    if (it != entry->variants.end() && !it->second.empty())
        return &it->second;
    if (!entry->canonical.empty())
        return &entry->canonical;
    return nullptr;
}}

}}
"""
    with open(os.path.join(GAME_DIR, "Core", "SDK", "Natives", "CrossmapNatives.hpp"), "w", encoding="utf-8", newline="") as f:
        f.write(hpp)
    with open(os.path.join(GAME_DIR, "Core", "SDK", "Natives", "CrossmapNatives.cpp"), "w", encoding="utf-8", newline="") as f:
        f.write(cpp)
    print(f"CrossmapNatives: {len(entries)} entries, blob {len(enc)} bytes", file=sys.stderr)

def main():
    src_dir = sys.argv[1] if len(sys.argv) > 1 else "/tmp"
    with open(os.path.join(src_dir, "hashnames_plain.hpp"), "r", encoding="utf-8", errors="replace") as f:
        write_hashnames(parse_hashnames(f.read()))
    with open(os.path.join(src_dir, "objectnames_plain.hpp"), "r", encoding="utf-8", errors="replace") as f:
        write_objectnames(parse_objectnames(f.read()))
    with open(os.path.join(src_dir, "crossmap_plain.cpp"), "r", encoding="utf-8", errors="replace") as f:
        write_crossmap(parse_crossmap(f.read()))

if __name__ == "__main__":
    main()
