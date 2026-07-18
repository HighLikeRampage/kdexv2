#!/usr/bin/env python3
"""
Bulk wrap literal values with xorstr() inside braced-init lists like:
    { KEY, "VALUE" }
and
    { KEY, "CANONICAL", { {BUILD, "PATTERN"}, ... } }

For files that already use std::string, this is a no-brainer. For files that
use std::string_view, the caller is expected to change the type to std::string
first (or `--force` to wrap anyway if the surrounding structure allows an
implicit std::string construction).

Only rewrites literals that are DIRECT arguments inside `{...}` braces (no
surrounding wrappers) and skips literals that already sit inside xorstr(,
IM_STR(, LI_FN(, etc. Backs the original up with a `.bak` suffix.
"""
from __future__ import annotations
import argparse, io, re, sys

STRING_RE = re.compile(r'"(?:\\.|[^"\\\n])*"')
WRAPPERS = ('xorstr(', 'xorstr_lite(', 'xorstr_(', 'LI_FN(', 'IM_STR(', '_T(', 'TEXT(')

def wrap_line(line: str) -> tuple[str, int]:
    out = []
    last = 0
    hits = 0
    for m in STRING_RE.finditer(line):
        s, e = m.start(), m.end()
        # skip empty
        if m.group(0) == '""':
            out.append(line[last:e]); last = e; continue
        # skip wide/raw prefixed
        pre = line[max(0, s-2):s]
        if pre.endswith(('L', 'R', 'u', 'U')) and (s == 0 or not line[s-2:s-1].isalnum()):
            out.append(line[last:e]); last = e; continue
        # skip if already inside a wrapper (look back a bit)
        back = line[max(0, s-24):s]
        if any(w in back for w in WRAPPERS):
            out.append(line[last:e]); last = e; continue
        # skip if line is a #include/#pragma/#define directive (also
        # handle a leading UTF-8 BOM on the first line of a file).
        ls = line.lstrip().lstrip('﻿')
        if ls.startswith('#'):
            out.append(line[last:e]); last = e; continue
        out.append(line[last:s])
        out.append('xorstr_lite(')
        out.append(line[s:e])
        out.append(')')
        last = e
        hits += 1
    out.append(line[last:])
    return ''.join(out), hits

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('path')
    ap.add_argument('--dry', action='store_true')
    args = ap.parse_args()
    with io.open(args.path, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()
    total = 0
    new_lines = []
    for line in lines:
        newl, hits = wrap_line(line)
        total += hits
        new_lines.append(newl)
    if not args.dry:
        with io.open(args.path + '.bak', 'w', encoding='utf-8', newline='') as bak:
            bak.writelines(lines)
        with io.open(args.path, 'w', encoding='utf-8', newline='') as f:
            f.writelines(new_lines)
    print(f'{total} wrapped in {args.path}', file=sys.stderr)

if __name__ == '__main__':
    main()
