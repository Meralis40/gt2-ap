#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Finds functions declared with a different number of arguments than they're defined with.

The game's functions have C linkage, so the linker can't see this, and on the PlayStation
a call missing an argument often worked: the value was still in $a0. On x86 it reads
garbage. Also lists calls whose argument count doesn't match the definition. Run from
port/, with any python3.
"""
import re
import sys
from collections import defaultdict
from pathlib import Path

GAME = Path(__file__).resolve().parents[1] / "game"
DECL = re.compile(r"^[ \t]*(?:extern\s+)?(?:static\s+|inline\s+|const\s+|struct\s+|unsigned\s+|signed\s+)*"
                  r"[A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)*[\s*]+\*?([A-Za-z_]\w*)\(([^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)\s*(;|\{|$)", re.M)
SKIP = {"if", "while", "for", "switch", "return", "sizeof", "else"}


def count(args):
    args = args.strip()
    if args in ("", "void"):
        return 0
    if "..." in args:
        return None
    depth = 0
    n = 1
    for c in args:
        if c in "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            n += 1
    return n


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def main():
    decls = defaultdict(set)
    defs = defaultdict(set)
    for path in GAME.rglob("*"):
        if path.suffix not in (".cpp", ".c", ".h"):
            continue
        text = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        rel = path.relative_to(GAME).as_posix()
        for m in DECL.finditer(text):
            name, args, end = m.groups()
            if name in SKIP or "::" in m.group(0):
                continue
            n = count(args)
            if n is None:
                continue
            (defs if end == "{" else decls)[name].add((n, rel))
            if end == "$":
                defs[name].add((n, rel))

    bad = 0
    for name in sorted(defs):
        wanted = {n for n, _ in defs[name]}
        seen = {n for n, _ in decls.get(name, ())}
        if len(wanted) == 1 and not (seen - wanted):
            continue
        bad += 1
        print(name, "defined:", sorted(defs[name]), "declared:", sorted(d for d in decls.get(name, ()) if d[0] not in wanted))
    print(f"{bad} mismatches", file=sys.stderr)


if __name__ == "__main__":
    main()
