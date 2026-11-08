#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Turns the game's casts between pointers and 32-bit integers into guest addresses (one-off).

On the console, (int)p is p's address and (T *)a the pointer at address a. Guest
memory is a window at rt::base, so these become (int)rt::to_guest(p) and
(T *)rt::from_guest(a) (rt/gptr.h); on i686 base is 0 and nothing changes. This reads
Clang's AST of every source of port/game and wraps the operand of each such C-style
cast. Casts inside macros, and casts to function pointers, are listed for a hand edit.

    convert_casts.py <compile_commands.json>      (from WSL, any python3)
"""
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from check_layout import command_for, to_wsl  # noqa: E402

GAME = re.compile(r"[\\/]port[\\/]game[\\/]")
WIDE = re.compile(r"\b(long long|__int64|size_t|uintptr_t|intptr_t|ptrdiff_t|u64|s64)\b")


def loc_file(loc, state):
    if "spellingLoc" in loc:
        loc_file(loc["spellingLoc"], state)
        loc_file(loc["expansionLoc"], state)
    elif "file" in loc:
        state["file"] = loc["file"]


def walk_loc(value, state):
    if isinstance(value, dict):
        if "begin" in value:
            walk_loc(value["begin"], state)
            walk_loc(value["end"], state)
        else:
            loc_file(value, state)


def walk(node, state, out):
    if isinstance(node, dict):
        target = node.get("kind") == "CStyleCastExpr" and node.get("castKind") in ("PointerToIntegral",
                                                                                  "IntegralToPointer")
        if target:
            out.append([node, None])
        for key, value in node.items():
            if key in ("loc", "range"):
                walk_loc(value, state)
                if target and key == "range":
                    out[-1][1] = state.get("file")
                continue
            if isinstance(value, (dict, list)):
                walk(value, state, out)
    elif isinstance(node, list):
        for item in node:
            walk(item, state, out)


def in_macro(rng):
    return "spellingLoc" in rng["begin"] or "spellingLoc" in rng["end"]


def edit_for(cast):
    """[(offset, text, order)] insertions, or a reason to leave the cast."""
    rng = cast["range"]
    inner = cast["inner"][0]
    irng = inner["range"]
    if in_macro(rng) or in_macro(irng):
        return "in a macro"
    target = cast["type"].get("desugaredQualType", cast["type"]["qualType"])
    source = inner["type"].get("desugaredQualType", inner["type"]["qualType"])
    if cast["castKind"] == "PointerToIntegral":
        if WIDE.search(target) or "(*)" in source:
            return None if WIDE.search(target) else "function pointer"
        helper = "rt::to_guest("
    else:
        if WIDE.search(source):
            return None
        if "(*)" in target:
            return "to a function pointer"
        helper = "rt::from_guest("
    begin = irng["begin"]["offset"]
    end = irng["end"]["offset"] + irng["end"]["tokLen"]
    span = end - begin
    # Inserted in descending order at one offset, so the first ends up rightmost: an
    # inner cast's "(" before an outer one's, an outer ")" before an inner one, and "(" before ")".
    return [(begin, helper, (1 << 30) - span), (end, ")", span)]


def casts_of(entry, cwd):
    cmd = command_for(entry, "--target=i686-pc-windows-msvc")
    cmd[cmd.index("-fdump-record-layouts-complete")] = "-ast-dump=json"
    r = subprocess.run(cmd, cwd=cwd, capture_output=True)
    if not r.stdout:
        sys.stderr.write(f"{entry['file']}: no AST\n")
        return []
    found = []
    walk(json.loads(r.stdout), {}, found)
    return found


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    entries = [e for e in json.load(open(sys.argv[1])) if GAME.search(e["file"])]
    cwd = to_wsl(entries[0]["directory"])
    casts = {}
    with ThreadPoolExecutor(6) as pool:
        for result in pool.map(lambda e: casts_of(e, cwd), entries):
            for cast, file in result:
                if file and GAME.search(file) and "range" in cast:
                    key = (os.path.normpath(to_wsl(file.replace("\\", "/"))), cast["range"]["begin"].get("offset"),
                           cast["range"]["end"].get("offset"), cast["castKind"])
                    casts[key] = cast

    by_file, left = {}, []
    for (file, *_), cast in casts.items():
        by_file.setdefault(file, []).append(cast)
    total = 0
    for file, items in sorted(by_file.items()):
        path = Path(file)
        if path.suffix == ".c":
            left.extend(f"{file}:{c['range']['begin'].get('line', '?')}: C" for c in items)
            continue
        inserts = []
        for c in items:
            result = edit_for(c)
            if result is None:
                continue
            if isinstance(result, str):
                left.append(f"{file}:{c['range']['begin'].get('line', c['range']['begin'].get('expansionLoc', {}).get('line', '?'))}: {result}")
                continue
            inserts.extend(result)
        if not inserts:
            continue
        data = path.read_bytes().decode("latin-1")
        for offset, text, order in sorted(inserts, key=lambda i: (i[0], i[2]), reverse=True):
            data = data[:offset] + text + data[offset:]
        if '#include "gptr.h"' not in data:
            data = re.sub(r'^(#include [^\n]*\n)', '#include "gptr.h"\n\\1', data, count=1, flags=re.M)
        path.write_bytes(data.encode("latin-1"))
        total += len(inserts) // 2
    print(f"{total} casts changed")
    for line in sorted(set(left)):
        print(f"left: {line}")


if __name__ == "__main__":
    main()
