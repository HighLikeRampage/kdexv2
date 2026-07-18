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

SAFE_LEADIN  = set(',([{=?:')          # things a literal may safely follow
SAFE_TRAILOUT = set(',)];:?')          # things a literal may safely precede

def _adjacent_char_before(line: str, pos: int) -> str:
    """Return the first non-whitespace char before pos, or ''."""
    i = pos - 1
    while i >= 0 and line[i] in ' \t':
        i -= 1
    return line[i] if i >= 0 else ''

def _adjacent_char_after(line: str, pos: int) -> str:
    """Return the first non-whitespace char at/after pos, or ''."""
    i = pos
    while i < len(line) and line[i] in ' \t\r\n':
        i += 1
    return line[i] if i < len(line) else ''

WRAPPER_OPEN_RE = re.compile(r'\b(?:xorstr|xorstr_|xorstr_lite|LI_FN|IM_STR|IM_ASSERT|_T|TEXT)\s*\(')

def _inside_wrapper(line: str, pos: int) -> bool:
    """True if pos is inside an unclosed wrapper( ... at the same line.
    Ignores parens that occur inside strings/chars/comments (approx)."""
    for m in WRAPPER_OPEN_RE.finditer(line):
        if m.end() > pos:
            continue
        # scan from wrapper's `(` to pos, counting parens, skipping
        # string/char contents.
        i = m.end() - 1   # points at `(`
        depth = 0
        in_str = False
        in_chr = False
        while i < pos:
            c = line[i]
            if in_str:
                if c == '\\' and i + 1 < pos: i += 2; continue
                if c == '"': in_str = False
            elif in_chr:
                if c == '\\' and i + 1 < pos: i += 2; continue
                if c == "'": in_chr = False
            else:
                if c == '"': in_str = True
                elif c == "'": in_chr = True
                elif c == '(': depth += 1
                elif c == ')': depth -= 1
            i += 1
        if depth > 0:
            return True
    return False

def wrap_line(line: str) -> tuple[str, int]:
    out = []
    last = 0
    hits = 0
    # skip if line is a #include/#pragma/#define directive (also handle a
    # leading UTF-8 BOM on the first line of a file).
    ls = line.lstrip().lstrip('﻿')
    if ls.startswith('#'):
        return line, 0
    # skip const/constexpr array/pointer initialisers -- wrapping into
    # a runtime call would leave the const-init unable to compile.
    if re.search(r'\b(?:static\s+)?(?:const|constexpr)\s+[\w\s\*]*\[', line):
        return line, 0
    # static_assert's message argument must be a string literal.
    if 'static_assert(' in line:
        return line, 0
    for m in STRING_RE.finditer(line):
        s, e = m.start(), m.end()
        text = m.group(0)
        if text == '""':
            out.append(line[last:e]); last = e; continue
        # wide/raw prefixed
        pre_char = line[s-1] if s > 0 else ''
        if pre_char in ('L', 'R', 'u', 'U') and (s < 2 or not line[s-2:s-1].isalnum()):
            out.append(line[last:e]); last = e; continue
        # already inside a wrapper( ... on this line, accounting for
        # paren balance so `xorstr("a" MACRO "b")` catches both a and b.
        if _inside_wrapper(line, s):
            out.append(line[last:e]); last = e; continue
        # concat neighbour: previous non-space is `"` or an identifier
        # (a macro like IMGUI_VERSION spliced between two literals).
        b = _adjacent_char_before(line, s)
        if b and (b == '"' or b.isalnum() or b == '_'):
            out.append(line[last:e]); last = e; continue
        a = _adjacent_char_after(line, e)
        if a and (a == '"' or a.isalnum() or a == '_'):
            out.append(line[last:e]); last = e; continue
        out.append(line[last:s])
        out.append('xorstr_lite(')
        out.append(line[s:e])
        out.append(')')
        last = e
        hits += 1
    out.append(line[last:])
    return ''.join(out), hits

CONTINUATION_TAIL = ('"', '\\', '"\\', '{', '=', ',', '(', '?', ':')

def _prev_nonblank(lines: list[str], i: int) -> str:
    j = i - 1
    while j >= 0 and (not lines[j].strip() or lines[j].lstrip().startswith(('//', '/*', '*'))):
        j -= 1
    return lines[j] if j >= 0 else ''

def _next_nonblank(lines: list[str], i: int) -> str:
    j = i + 1
    while j < len(lines) and (not lines[j].strip() or lines[j].lstrip().startswith(('//', '/*', '*'))):
        j += 1
    return lines[j] if j < len(lines) else ''

def compute_orphan_lines(lines: list[str]) -> list[bool]:
    """Mark lines whose leading string literal is really a
    continuation of a previous line: concatenated string init,
    argument on the next line of an open wrapper( , brace-init
    block spanning multiple lines, etc."""
    orph = [False] * len(lines)
    for i, line in enumerate(lines):
        s = line.lstrip()
        if not s or not s.startswith('"'):
            continue
        prev = _prev_nonblank(lines, i).rstrip()
        if not prev:
            continue
        # concat: previous line ends with `"` (or trailing backslash)
        if prev.endswith('"') or prev.endswith('\\') or prev.endswith('"\\'):
            orph[i] = True; continue
        # brace / paren / equal / comma / colon / question -- we are
        # inside an initializer or an in-flight call that started on
        # the previous line.
        if prev.endswith('{') or prev.endswith('=') or prev.endswith('('):
            orph[i] = True; continue
        # previous line ends with a known wrapper open followed by
        # nothing.
        if any(prev.endswith(w) for w in WRAPPERS):
            orph[i] = True; continue
    # also mark the tail of a line whose *next* non-blank line begins
    # with a `"` (concat head).
    for i, line in enumerate(lines):
        if line.rstrip().endswith('"'):
            nxt = _next_nonblank(lines, i).lstrip()
            if nxt.startswith('"'):
                orph[i] = True
    return orph

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('path')
    ap.add_argument('--dry', action='store_true')
    args = ap.parse_args()
    with io.open(args.path, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()
    orphan = compute_orphan_lines(lines)
    total = 0
    new_lines = []
    for i, line in enumerate(lines):
        if orphan[i]:
            new_lines.append(line); continue
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
