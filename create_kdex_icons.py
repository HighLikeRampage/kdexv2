#!/usr/bin/env python3
"""
create_kdex_icons.py
Downloads FontAwesome 6 Free Solid, subsets it to the 22 glyphs
used by kdex, and rewrites kdex/framework/data/fonts.h replacing
the old 'icons' and 'fa_icons' arrays with a single 'kdex_icons' array.

Usage:
    pip install fonttools requests
    python create_kdex_icons.py
"""

import sys
import io
import re
import urllib.request

try:
    from fontTools import subset as ft_subset
    from fontTools.ttLib import TTFont
except ImportError:
    print("ERROR: fonttools not installed. Run: pip install fonttools")
    sys.exit(1)

FA6_URL = "https://cdn.jsdelivr.net/npm/@fortawesome/fontawesome-free@6.5.2/webfonts/fa-solid-900.ttf"

FONTS_H_PATH = "kdex/framework/data/fonts.h"

NEEDED = [
    (0xE19B, "gun          -> FA_GUN"),
    (0xF002, "search       -> FA_SEARCH"),
    (0xF013, "gear         -> FA_GEAR"),
    (0xF017, "clock        -> FA_CLOCK"),
    (0xF02B, "tag          -> FA_TAG"),
    (0xF03A, "list         -> FA_LIST"),
    (0xF044, "edit         -> FA_EDIT"),
    (0xF04B, "play         -> FA_PLAY"),
    (0xF04D, "stop         -> FA_STOP_BTN"),
    (0xF05B, "crosshairs   -> ICON_FOCUS"),
    (0xF06E, "eye          -> FA_EYE"),
    (0xF07B, "folder       -> FA_FOLDER"),
    (0xF0AC, "globe        -> FA_GLOBE"),
    (0xF0AD, "wrench       -> ICON_WRENCH"),
    (0xF0C0, "users        -> FA_USERS / ICON_GROUP"),
    (0xF0C5, "copy         -> FA_COPY (Events tab)"),
    (0xF0E7, "bolt         -> FA_BOLT"),
    (0xF11C, "keyboard     -> FA_KEYBOARD"),
    (0xF120, "terminal     -> FA_TERMINAL"),
    (0xF121, "code         -> FA_CODE (Executor tab)"),
    (0xF188, "bug          -> ICON_BUG"),
    (0xF1EB, "wifi         -> FA_WIFI"),
    (0xF2B5, "handshake    -> FA_HANDSHAKE"),
]

def download_font(url):
    print(f"Downloading {url} ...")
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req, timeout=30) as r:
        data = r.read()
    print(f"  Downloaded {len(data):,} bytes")
    return data

def subset_font(font_data, unicodes):
    print(f"Checking {len(unicodes)} glyphs in font ...")
    font = TTFont(io.BytesIO(font_data))
    cmap = font.getBestCmap()

    found, missing = [], []
    for cp in unicodes:
        (found if cp in cmap else missing).append(cp)

    if missing:
        print(f"  WARNING: {len(missing)} glyph(s) NOT in font:")
        for cp in missing:
            label = next((l for c, l in NEEDED if c == cp), "?")
            print(f"    U+{cp:04X}  {label}")
    print(f"  Subsetting {len(found)} found glyphs ...")

    opts = ft_subset.Options()
    opts.set(layout_features="*", name_IDs="*", notdef_outline=True)
    s = ft_subset.Subsetter(options=opts)
    s.populate(unicodes=found)
    s.subset(font)

    buf = io.BytesIO()
    font.save(buf)
    return buf.getvalue()

def to_c_array(name, data):
    lines = [f"unsigned char {name}[{len(data)}] = {{"]
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)

def strip_c_array(src, array_name):
    pattern = (
        r"unsigned\s+char\s+" + re.escape(array_name) +
        r"\s*\[\s*\d*\s*\]\s*=\s*\{[^}]*\}\s*;"
    )
    result, count = re.subn(pattern, "", src, flags=re.DOTALL)
    return result, count

def main():
    raw = download_font(FA6_URL)
    subset_data = subset_font(raw, [cp for cp, _ in NEEDED])
    print(f"  Subset size: {len(subset_data):,} bytes")

    c_array = to_c_array("kdex_icons", subset_data)

    print(f"Reading {FONTS_H_PATH} ...")
    with open(FONTS_H_PATH, "r", encoding="utf-8") as f:
        src = f.read()

    src, n1 = strip_c_array(src, "icons")
    src, n2 = strip_c_array(src, "fa_icons")
    print(f"  Removed 'icons' occurrences: {n1}")
    print(f"  Removed 'fa_icons' occurrences: {n2}")

    src = src.rstrip()
    src += "\n\n" + c_array + "\n"

    with open(FONTS_H_PATH, "w", encoding="utf-8") as f:
        f.write(src)

    print(f"Written {FONTS_H_PATH}")
    print(f"  Array: unsigned char kdex_icons[{len(subset_data)}]")
    print("Done. Rebuild the project.")

if __name__ == "__main__":
    main()
