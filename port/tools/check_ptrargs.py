#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Finds C functions declared with an integer where another declaration has a pointer.

The game's functions have C linkage, so the linker joins declarations whatever their
parameter types. On i686 an int and a pointer are the same 4 bytes; on x86-64 a
pointer is a host address (rt::base + the guest address), so a caller that passes a
guest address as an int hands the callee a wrong pointer. Same for return types. Reads
Clang's AST of every source of port/game; lists each mismatch with its declarations.

Also lists the guest data names (extern references, bound to guest memory) whose type
holds a raw pointer: 8 bytes on x86-64 over a 4-byte slot. They need game_pointer.

    check_ptrargs.py <compile_commands.json>      (from WSL, any python3)
"""
import json
import re
import subprocess
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from check_layout import command_for, to_wsl  # noqa: E402

GAME = re.compile(r"[\\/]port[\\/]game[\\/]")
INT = re.compile(r"^(unsigned |signed )?(int|uint|long|short|char|u32|s32)$")


def kind(t):
    t = t.strip()
    if "*" in t or t.endswith("]") or t.startswith("game_pointer") or t.startswith("game_function"):
        return "pointer"
    if INT.match(t) or t in ("unsigned", "unsigned int", "unsigned long", "int"):
        return "int"
    return "other"


def split_params(t):
    """'R (A, B)' -> ('R', ['A', 'B'])."""
    depth, start = 0, None
    for i, c in enumerate(t):
        if c == "(" and depth == 0 and start is None and i > 0:
            start = i
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
    if start is None:
        return t, []
    ret, inner = t[:start].strip(), t[start + 1:t.rindex(")")]
    parts, depth, cur = [], 0, ""
    for c in inner:
        if c in "(<[":
            depth += 1
        elif c in ")>]":
            depth -= 1
        if c == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += c
    if cur.strip() and cur.strip() != "void":
        parts.append(cur.strip())
    return ret, parts


WRAPPED = re.compile(r"game_(pointer|function)<[^<>]*>")


def host_pointer(t):
    """True when the referenced type of an extern reference holds a raw pointer."""
    while WRAPPED.search(t):
        t = WRAPPED.sub("G", t)
    return "&" in t and "*" in t


def walk(node, out, linkage_c=False):
    if isinstance(node, dict):
        if node.get("kind") == "VarDecl" and node.get("storageClass") == "extern" and "name" in node:
            out.append(("ref:" + node["name"], node["type"]["qualType"]))
        if node.get("kind") in ("TypedefDecl", "TypeAliasDecl") and "name" in node and "type" in node:
            out.append(("typedef:" + node["name"], node["type"]["qualType"]))
        if node.get("kind") == "LinkageSpecDecl" and node.get("language") == "C":
            linkage_c = True
        if node.get("kind") == "FunctionDecl" and linkage_c and "name" in node:
            out.append((node["name"], node["type"].get("desugaredQualType", node["type"]["qualType"])))
        for value in node.values():
            if isinstance(value, (dict, list)):
                walk(value, out, linkage_c)
    elif isinstance(node, list):
        for item in node:
            walk(item, out, linkage_c)


def decls_of(entry, cwd):
    cmd = command_for(entry, "--target=i686-pc-windows-msvc")
    cmd[cmd.index("-fdump-record-layouts-complete")] = "-ast-dump=json"
    r = subprocess.run(cmd, cwd=cwd, capture_output=True)
    if not r.stdout:
        return entry["file"], []
    found = []
    walk(json.loads(r.stdout), found)
    # Clang leaves a reference's typedefs unexpanded ("pointer (&)"): expand them here.
    typedefs = {n[8:]: t for n, t in found if n.startswith("typedef:")}
    decls = [(n, t) for n, t in found if not n.startswith(("typedef:", "ref:"))]
    for n, t in found:
        if n.startswith("ref:"):
            for _ in range(8):
                t = re.sub(r"\b\w+\b",lambda m: f"({typedefs[m[0]]})" if m[0] in typedefs else m[0], t)
            if host_pointer(t):
                decls.append(("var:" + n[4:], t))
    return entry["file"], decls


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    entries = [e for e in json.load(open(sys.argv[1])) if GAME.search(e["file"])]
    cwd = to_wsl(entries[0]["directory"])
    seen = defaultdict(lambda: defaultdict(set))  # name -> type -> files
    with ThreadPoolExecutor(6) as pool:
        for file, decls in pool.map(lambda e: decls_of(e, cwd), entries):
            for name, t in decls:
                seen[name][t].add(Path(file).name)
    bad = 0
    for name in sorted(n for n in seen if n.startswith("var:")):
        bad += 1
        for t, files in sorted(seen[name].items()):
            print(f"{name[4:]}: host pointer in guest data: {t}    ({', '.join(sorted(files))})")
    for name in sorted(n for n in seen if not n.startswith("var:")):
        types = seen[name]
        if len(types) < 2:
            continue
        shapes = {t: split_params(t) for t in types}
        counts = {len(p) for _, p in shapes.values()}
        problems = []
        n = min(counts)
        for i in range(n):
            kinds = {kind(p[i]) for _, p in shapes.values()}
            if {"int", "pointer"} <= kinds:
                problems.append(f"argument {i + 1}")
        if {"int", "pointer"} <= {kind(r) for r, _ in shapes.values()}:
            problems.append("result")
        if problems:
            bad += 1
            print(f"{name}: {', '.join(problems)}")
            for t, files in sorted(types.items()):
                print(f"    {t}    ({', '.join(sorted(files)[:4])}{'...' if len(files) > 4 else ''})")
    print(f"{bad} guest names holding a host pointer or functions with an int and a pointer in one place")


if __name__ == "__main__":
    main()
