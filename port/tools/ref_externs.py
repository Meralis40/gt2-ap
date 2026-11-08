#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Turns the extern declarations of guest data into references (one-off conversion).

Guest data is at base + its PlayStation address, and base is only known once the window
is mapped, so the linker can't bind the names any more: each one becomes a reference,
`extern T (&name);`, that rt::bindGuestData sets (gen/guest_data.cpp). The names are
the ones gen/guest_syms.S binds. Run from port/ with any python3; C sources are listed
instead of changed.
"""
import re
import sys
from pathlib import Path

PORT = Path(__file__).resolve().parents[1]
SET = re.compile(r"\.set\s+SYM\((\w+)\)")
UNIT = re.compile(r"^(.+)_(exe|ovl[0-5])$")
EXTERN = re.compile(r'\bextern\b(\s*"C")?')
IDENT = re.compile(r"[A-Za-z_]\w*")


def names():
    out = set()
    for name in SET.findall((PORT / "gen" / "guest_syms.S").read_text()):
        out.add(name)
        if m := UNIT.match(name):
            out.add(m.group(1))
    return out


def statement_end(text, i):
    """Index of the ';' ending the declaration at i, or None for a block or definition."""
    depth = 0
    while i < len(text):
        c = text[i]
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        elif c == "{" or (c == "=" and depth == 0):
            return None
        elif c == ";" and depth == 0:
            return i
        elif text.startswith("//", i):
            i = text.find("\n", i)
            if i < 0:
                return None
        elif text.startswith("/*", i):
            i = text.find("*/", i) + 1
        i += 1
    return None


def convert(text, guest):
    out, pos, count = [], 0, 0
    for m in EXTERN.finditer(text):
        if m.start() < pos:
            continue
        line_start = text.rfind("\n", 0, m.start()) + 1
        if "//" in text[line_start:m.start()] or text[line_start:m.start()].lstrip().startswith("*"):
            continue
        end = statement_end(text, m.end())
        if end is None:
            continue
        decl = text[m.end():end]
        new, last = [], 0
        for ident in IDENT.finditer(decl):
            name = ident.group(0)
            if name not in guest:
                continue
            after = decl[ident.end():].lstrip()
            before = decl[:ident.start()].rstrip()
            if after.startswith("(") or before.endswith("&") or before.endswith("(&"):
                continue
            new.append(decl[last:ident.start()])
            new.append(f"(&{name})")
            last = ident.end()
            count += 1
        if not new:
            continue
        out.append(text[pos:m.end()])
        out.append("".join(new) + decl[last:])
        pos = end
    out.append(text[pos:])
    return "".join(out), count


def main():
    guest = names()
    total, c_files = 0, []
    for path in sorted(PORT.glob("**/*")):
        if path.suffix not in (".cpp", ".h", ".c") or any(p in path.parts for p in ("gen", "spikes", ".scan")):
            continue
        text = path.read_text(encoding="utf-8")
        new, count = convert(text, guest)
        if not count:
            continue
        if path.suffix == ".c":
            c_files.append(path)
            continue
        path.write_text(new, encoding="utf-8", newline="")
        total += count
    print(f"{total} declarations changed")
    for path in c_files:
        print(f"C source, not changed: {path.relative_to(PORT)}")


if __name__ == "__main__":
    sys.exit(main())
