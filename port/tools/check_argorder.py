#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Lists calls whose result can depend on the order their arguments are evaluated in.

C++ leaves that order open: the Microsoft ABI goes right to left, the others mostly left
to right, and the PlayStation's code has one fixed order that the C++ must keep. A hit is
a call with two arguments that contain calls, an argument that writes something (++, --,
=) next to other arguments, or a call taking &x next to an argument that reads x. Run
from port/, with any python3. Prints file:line and the call; each hit needs reading.
"""
import re
import sys
from pathlib import Path

GAME = Path(__file__).resolve().parents[1] / "game"
NOT_CALLS = {"if", "while", "for", "switch", "return", "sizeof", "else", "do", "case", "defined", "offsetof",
             "__attribute__", "__asm__", "alignof", "static_cast", "reinterpret_cast", "const_cast"}
# Reads nothing that a call next to it could change, and changes nothing.
PURE = {"abs", "strlen", "Mips_ReadWord", "Mips_Lwl2", "RaceSetup_Get", "Math_Abs", "HW_REG", "LodAt"}
IDENT = re.compile(r"[A-Za-z_]\w*")
TYPE_CAST = re.compile(r"\(\s*(?:const\s+|unsigned\s+|signed\s+|struct\s+)*[A-Za-z_]\w*(?:\s+[A-Za-z_]\w*)*\s*\**\s*\)")


def strip(text):
    """Blanks comments, strings and preprocessor lines, keeping line numbers."""
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        two = text[i:i + 2]
        if two == "//":
            while i < n and text[i] != "\n":
                i += 1
        elif two == "/*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            out.append(re.sub(r"[^\n]", " ", text[i:j]))
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            out.append(" 0 " if j - i < 3 else "0".ljust(j + 1 - i))
            i = j + 1
        else:
            out.append(c)
            i += 1
    return re.sub(r"^[ \t]*#(?:.*\\\n)*.*$", lambda m: re.sub(r"[^\n]", " ", m.group(0)), "".join(out), flags=re.M)


def matching(text, start):
    """The index of the ) for the ( at start, and the top-level comma positions."""
    depth = 0
    commas = []
    for i in range(start, len(text)):
        c = text[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                return i, commas
        elif c == "," and depth == 1:
            commas.append(i)
    return -1, commas


def calls_in(arg):
    bare = TYPE_CAST.sub(" ", arg)
    names = [m.group(1) for m in re.finditer(r"([A-Za-z_]\w*)\s*\(", bare)]
    return [n for n in names if n not in NOT_CALLS and n not in PURE]


def writes(arg):
    return bool(re.search(r"\+\+|--|(?<![=!<>+\-*/%&|^])=(?!=)|[+\-*/%&|^]=|<<=|>>=", arg))


def scan(path):
    text = strip(path.read_text(encoding="utf-8", errors="replace"))
    rel = path.relative_to(GAME).as_posix()
    for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", text):
        name = m.group(1)
        if name in NOT_CALLS:
            continue
        open_paren = m.end() - 1
        close, commas = matching(text, open_paren)
        if close < 0 or not commas:
            continue
        before = text[:m.start()].rstrip()
        after = text[close + 1:].lstrip()
        if after[:1] == "{" or (before[-1:].isalnum() and not re.search(r"\breturn$", before)):
            continue  # a definition or a declaration
        edges = [open_paren] + commas + [close]
        args = [text[edges[i] + 1:edges[i + 1]] for i in range(len(edges) - 1)]
        with_calls = [a for a in args if calls_in(a)]
        reason = None
        if len(with_calls) >= 2:
            reason = "two arguments call"
        elif any(writes(a) for a in args) and len(args) > 1:
            reason = "an argument writes"
        else:
            for a in with_calls:
                for addr in re.findall(r"&\s*([A-Za-z_]\w*)", a):
                    if any(a2 is not a and re.search(r"\b" + addr + r"\b", a2) for a2 in args):
                        reason = "an argument reads what a call sets"
        if reason:
            line = text.count("\n", 0, m.start()) + 1
            print(f"{rel}:{line}: {reason}: {' '.join(text[m.start():close + 1].split())[:150]}")


def main():
    files = sorted(p for p in GAME.rglob("*") if p.suffix in (".cpp", ".c", ".h"))
    for path in files:
        scan(path)


if __name__ == "__main__":
    sys.exit(main())
