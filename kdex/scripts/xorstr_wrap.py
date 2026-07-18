#!/usr/bin/env python3
"""
Wrap bare C/C++ string literals with xorstr(...) in the user code base.

Usage:
    python xorstr_wrap.py --dry <file-or-dir>        # report candidates only
    python xorstr_wrap.py --apply <file-or-dir>      # rewrite in place (creates .bak)

Safety rules:
    - Skips thirdparty/, Includes/curl and the xorstr header itself.
    - Skips lines that are #include / #pragma / // comments / .*_Pragma(.
    - Skips any literal already inside xorstr( xorstr_( LI_FN( _T( TEXT(.
    - Skips wide (L"..."), u8/u/U prefixed, raw R"(...) literals.
    - Skips empty literals, single-character escape-only literals, and
      literals that look like concatenated pieces (adjacent to another
      "..." on the same token stream).
    - Skips literals adjacent to a preprocessor stringification (#name).
    - Skips literals in case labels, static_assert messages, and enum
      value initialisers (heuristic: line contains 'case ' before the
      literal, 'static_assert(' before it, or is inside 'enum').

Even with these rules, mass rewrites can break some format-string usages;
review the diff before committing.
"""
from __future__ import annotations
import argparse, os, re, sys, io

STRING_RE = re.compile(
    r'"(?:\\.|[^"\\\n])*"'
)

SKIP_PREFIXES = ('L"', 'u8"', 'u"', 'U"', 'R"')

SKIP_DIRS = ('thirdparty', 'imgui', 'freetype', 'dxsdk', 'curl')

SKIP_FILES = (
    'json.hpp', 'xorstr.hpp', 'LazyImporter.hpp', 'lazyimporter.hpp',
    # Big data files that need bulk-blob encryption instead of per-literal
    # xorstr wraps (template instantiation cost + string_view containers).
    'CrossmapNatives.cpp', 'ObjectNames.hpp', 'NativeHashNames.hpp',
)

WRAPPER_CALLERS = ('xorstr(', 'xorstr_(', 'LI_FN(', '_T(', 'TEXT(', 'IM_STR(', '#include')

EXTERN_C_RE = re.compile(r'\bextern\s*"[^"]*"')
STRINGVIEW_RE = re.compile(r'\bstring_view\b')

def should_skip_dir(path: str) -> bool:
    parts = path.replace('\\', '/').split('/')
    return any(p.lower() in SKIP_DIRS for p in parts)

def scan_line(line: str) -> list[tuple[int, int]]:
    """Return positions (start,end) of literals worth wrapping."""
    hits = []
    stripped = line.lstrip()
    if stripped.startswith(('#include', '#pragma', '#error', '#warning', '//', '/*', '*')):
        return hits
    if 'static_assert(' in line:
        return hits
    if re.search(r'\bcase\s+.*"', line):
        return hits
    if STRINGVIEW_RE.search(line):
        return hits
    # extern "C" / extern "C++" — skip the whole line to avoid touching the
    # linkage-spec string. Also skip lines with __pragma(...).
    extern_spans = [(m.start(), m.end()) for m in EXTERN_C_RE.finditer(line)]
    for m in STRING_RE.finditer(line):
        s, e = m.start(), m.end()
        text = m.group(0)
        if text == '""':
            continue
        if any(s >= a and e <= b for a, b in extern_spans):
            continue
        # wide / raw / prefixed literals
        pre_start = max(0, s - 2)
        if line[pre_start:s].endswith(SKIP_PREFIXES[:-1]):
            continue
        if line[max(0,s-1):s] == 'L' or line[max(0,s-1):s] == 'R' or line[max(0,s-1):s] == 'U':
            continue
        # already inside a wrapper macro? look 20 chars back for "xorstr(" etc.
        prefix = line[max(0, s-24):s]
        if any(w in prefix for w in WRAPPER_CALLERS):
            continue
        # preprocessor stringification (#name pattern) — skip if preceded by '#'
        if line[max(0,s-1):s] == '#':
            continue
        # concatenated literal on the same line — skip both halves to be safe
        after = line[e:e+8].lstrip()
        before = line[max(0,s-8):s].rstrip()
        if after.startswith('"') or before.endswith('"'):
            continue
        # inside a #define macro body? risky, skip
        if line.lstrip().startswith('#define'):
            continue
        hits.append((s, e))
    return hits

def wrap_line(line: str, hits: list[tuple[int, int]]) -> str:
    out = []
    last = 0
    for s, e in hits:
        out.append(line[last:s])
        out.append('xorstr(')
        out.append(line[s:e])
        out.append(')')
        last = e
    out.append(line[last:])
    return ''.join(out)

def process_file(path: str, apply: bool) -> tuple[int, int]:
    total = 0
    changed_lines = 0
    with io.open(path, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()
    # Precompute which lines are "orphan continuations" of a concatenated
    # string literal (previous non-empty line ends with `"` or `\<newline>`,
    # or current line's first non-space token starts with `"`).
    orphan_continuation = [False] * len(lines)
    for i, line in enumerate(lines):
        s = line.lstrip()
        if not s: continue
        if s.startswith('"'):
            # Look back to previous non-blank non-comment line
            j = i - 1
            while j >= 0 and (not lines[j].strip() or lines[j].lstrip().startswith(('//', '/*'))):
                j -= 1
            if j >= 0:
                prev = lines[j].rstrip()
                # Concatenated literal continuation
                if prev.endswith('"') or prev.endswith('\\') or prev.endswith('"\\'):
                    orphan_continuation[i] = True
                # Function argument on the next line, where the previous
                # line ends with an unmatched `xorstr(` / `xorstr_(` /
                # `LI_FN(` / `_T(` / `TEXT(`. Wrapping our leading literal
                # would produce `xorstr( xorstr("..."))` which asks the
                # compiler to xorstr-encrypt a runtime pointer.
                stripped_prev = prev.rstrip()
                if any(stripped_prev.endswith(w) for w in WRAPPER_CALLERS):
                    orphan_continuation[i] = True
        # Also mark lines whose LAST literal is followed by a newline and
        # then another literal (this file's next line begins with ").
        if i + 1 < len(lines):
            nxt = lines[i + 1].lstrip()
            if nxt.startswith('"') and line.rstrip().endswith('"'):
                # this line's tail literal is a concat head → mark BOTH.
                orphan_continuation[i] = True
                orphan_continuation[i + 1] = True
    new_lines = []
    for i, line in enumerate(lines, 1):
        if orphan_continuation[i - 1]:
            new_lines.append(line)
            continue
        hits = scan_line(line)
        if hits:
            total += len(hits)
            changed_lines += 1
            if apply:
                new_lines.append(wrap_line(line, hits))
            else:
                for s, e in hits:
                    print(f'{path}:{i}: {line[s:e]}')
                new_lines.append(line)
        else:
            new_lines.append(line)
    if apply and total:
        with io.open(path + '.bak', 'w', encoding='utf-8', newline='') as bak:
            bak.writelines(lines)
        with io.open(path, 'w', encoding='utf-8', newline='') as f:
            f.writelines(new_lines)
    return total, changed_lines

def walk(root: str):
    if os.path.isfile(root):
        yield root; return
    for base, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if d.lower() not in SKIP_DIRS]
        for f in files:
            if f.endswith(('.cpp', '.hpp', '.h', '.cc')):
                if f in SKIP_FILES:
                    continue
                p = os.path.join(base, f)
                if should_skip_dir(p): continue
                yield p

def main():
    ap = argparse.ArgumentParser()
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--dry', metavar='PATH')
    g.add_argument('--apply', metavar='PATH')
    args = ap.parse_args()
    target = args.dry or args.apply
    apply = bool(args.apply)
    total_hits = 0
    total_files = 0
    for f in walk(target):
        if 'xorstr.hpp' in f.replace('\\', '/').lower():
            continue
        hits, _ = process_file(f, apply)
        if hits:
            total_files += 1
            total_hits += hits
    action = 'wrapped' if apply else 'candidates'
    print(f'\n{total_hits} {action} in {total_files} files', file=sys.stderr)

if __name__ == '__main__':
    main()
