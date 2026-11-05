#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""One-shot fork of decomp/ into port/game/.

Copies the headers and sources, keeps the C++ side of each NON_MATCHING pair and
drops the matching build's assembly includes. Lines it can't convert are listed
in port/game/FORK_REPORT.txt. It refuses to run over an existing fork: port/game
is edited by hand from then on.
"""
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DECOMP = ROOT / "decomp"
GAME = ROOT / "port" / "game"
DROP_LINE = re.compile(r'^\s*(#define INCLUDE_ASM_USE_MACRO_INC\b.*|#include "include_asm\.h".*)$')
LEFTOVER = re.compile(r"\b(INCLUDE_ASM|INCLUDE_RODATA)\b|__asm__|\basm\s*\(")
IF = re.compile(r"^\s*#\s*(ifdef|ifndef|if)\b(.*)$")
ELSE = re.compile(r"^\s*#\s*else\b")
ELIF = re.compile(r"^\s*#\s*elif\b")
ENDIF = re.compile(r"^\s*#\s*endif\b")


def strip_non_matching(lines):
    """Resolves NON_MATCHING conditionals, leaving all the others alone."""
    out = []
    stack = []  # for each open #if: None if unrelated, else whether its branch is kept
    for line in lines:
        keeping = all(k for k in stack if k is not None)
        if m := IF.match(line):
            name = m.group(2).strip().split("//")[0].strip()
            if name == "NON_MATCHING" and m.group(1) in ("ifdef", "ifndef"):
                stack.append(m.group(1) == "ifdef")
                continue
            if keeping:
                out.append(line)
            stack.append(None)
        elif ELSE.match(line) and stack and stack[-1] is not None:
            stack[-1] = not stack[-1]
        elif ELIF.match(line) and stack and stack[-1] is not None:
            raise SystemExit(f"#elif on NON_MATCHING: {line}")
        elif ENDIF.match(line) and stack:
            if stack.pop() is None and keeping:
                out.append(line)
        elif keeping:
            out.append(line)
    if stack:
        raise SystemExit("unbalanced conditionals")
    return out


def convert(src, dst, report):
    lines = src.read_text(encoding="utf-8", errors="surrogateescape").split("\n")
    kept = [l for l in strip_non_matching(lines) if not DROP_LINE.match(l)]
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text("\n".join(kept), encoding="utf-8", errors="surrogateescape")
    rel = dst.relative_to(GAME).as_posix()
    report.extend(f"{rel}:{n}: {l.strip()}" for n, l in enumerate(kept, 1) if LEFTOVER.search(l))


def main():
    if GAME.exists():
        sys.exit(f"{GAME} exists: the fork is one-shot")
    report = []
    for path in sorted((DECOMP / "include").iterdir()):
        if path.suffix == ".h" and path.name != "include_asm.h":
            convert(path, GAME / "include" / path.name, report)
    for path in sorted((DECOMP / "src" / "sim").rglob("*")):
        if path.suffix in (".cpp", ".c", ".h"):
            convert(path, GAME / path.relative_to(DECOMP / "src" / "sim"), report)
    (GAME / "FORK_REPORT.txt").write_text("\n".join(report) + "\n", encoding="utf-8")
    print(f"forked; {len(report)} lines to convert by hand, see FORK_REPORT.txt")


if __name__ == "__main__":
    main()
