#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compares the layout of every game structure between two targets.

Guest data has the console's layout, so a structure must keep its size, alignment and
field offsets on every target the port builds for. This compiles each source of
port/game for both targets with Clang's record layout dump, and lists every structure
whose layout differs. Classes with a vtable are listed apart: their vtable pointer is a
host pointer, and they live in memory only port code reads (docs/port.md).

    check_layout.py [-v] <compile_commands.json> <target flags A> <target flags B>

For example, from WSL:
    check_layout.py build/port-win/compile_commands.json \\
        "--target=i686-pc-windows-msvc" "--target=x86_64-pc-windows-msvc"
    check_layout.py build/port-linux/compile_commands.json "-m32 -malign-double" "-m64"

A Windows command line runs its clang++.exe through WSL interop. Run with any python3.
"""
import json
import re
import shlex
import subprocess
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor

DROP_WITH_ARG = {"-o", "-c", "-MF", "-MT", "-MQ"}
DROP_PREFIX = ("--target=", "-m32", "-m64", "-malign-double", "-MD", "-MMD")
DUMP = ["-fsyntax-only", "-Xclang", "-fdump-record-layouts-complete"]


def split(command):
    if "\\" in command or re.match(r"^[A-Za-z]:", command):
        # A Windows command line: quotes only, backslashes are path separators.
        return [a[1:-1] if a.startswith('"') and a.endswith('"') else a
                for a in shlex.split(command, posix=False)]
    return shlex.split(command)


def winpath(arg):
    return arg if not re.match(r"^[A-Za-z]:\\", arg) else arg.replace("\\", "/")


def to_wsl(path):
    m = re.match(r"^([A-Za-z]):[\\/](.*)$", path)
    return f"/mnt/{m.group(1).lower()}/{m.group(2)}".replace("\\", "/") if m else path


def command_for(entry, target):
    args = entry["arguments"] if "arguments" in entry else split(entry["command"])
    out = [args[0]]
    skip = False
    for a in args[1:]:
        if skip:
            skip = False
            continue
        if a in DROP_WITH_ARG:
            skip = True
            continue
        if a.startswith(DROP_PREFIX) or a == entry["file"] or a.replace("\\", "/") == entry["file"]:
            continue
        if a.startswith("-o") and len(a) > 2 and not a.startswith("-O"):
            continue
        # -w would hide what -Werror= in the target flags asks for.
        out.append("-Wno-everything" if a == "-w" else winpath(a))
    if re.match(r"^[A-Za-z]:", out[0]):
        out[0] = to_wsl(out[0])
        if not out[0].lower().endswith(".exe"):
            out[0] += ".exe"
    return out + shlex.split(target) + DUMP + [entry["file"]]


BLOCK = re.compile(r"\*\*\* Dumping AST Record Layout\n(.*?)(?=\n\*\*\* Dumping AST Record Layout|\Z)", re.S)
OFFSET = re.compile(r"^\s*(\d+)(:\d+-\d+)? \|", re.M)


def parse(text):
    """{record name: layout text}, the name being the first line of the dump."""
    records = {}
    for m in BLOCK.finditer(text):
        block = m.group(1).strip("\n")
        lines = [l for l in block.split("\n") if l.strip()]
        if not lines:
            continue
        name = lines[0].split("|", 1)[1].strip()
        records[name] = "\n".join(lines)
    return records


def dump(entry, target, cwd):
    cmd = command_for(entry, target)
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, errors="replace")
    errors = r.stderr.count(" error: ") if r.returncode != 0 else 0
    if r.returncode != 0 and not errors:
        errors = 1
    # The layouts Clang printed before an error in a function body are still right.
    return parse(r.stdout), errors, r.stderr


def main():
    verbose = "-v" in sys.argv[1:]
    args = [a for a in sys.argv[1:] if a != "-v"]
    if len(args) != 3:
        sys.exit(__doc__)
    path, target_a, target_b = args
    entries = [e for e in json.load(open(path))
               if re.search(r"[\\/]port[\\/]game[\\/]", e["file"])]
    cwd = to_wsl(entries[0]["directory"]) if entries else "."

    layouts = {target_a: defaultdict(set), target_b: defaultdict(set)}
    failed = defaultdict(int)
    with ThreadPoolExecutor(8) as pool:
        jobs = [(e, t, pool.submit(dump, e, t, cwd)) for e in entries for t in (target_a, target_b)]
        for entry, target, job in jobs:
            records, errors, stderr = job.result()
            if errors:
                failed[target] += 1
                if verbose:
                    sys.stderr.write(f"{entry['file']} ({target}):\n{stderr[-3000:]}\n")
            for name, text in records.items():
                layouts[target][name].add(text)

    differ, virtual, missing = [], [], 0
    for name in sorted(set(layouts[target_a]) | set(layouts[target_b])):
        a, b = layouts[target_a].get(name, set()), layouts[target_b].get(name, set())
        if not a or not b:
            missing += 1  # only seen where a source failed to compile
            continue
        if a == b or name.startswith("struct __NSConstantString"):  # Clang's own
            continue
        has_vptr = any("vtable pointer" in t or "vftable pointer" in t or "vfptr" in t for t in a | b)
        (virtual if has_vptr else differ).append((name, a, b))

    for name, a, b in differ:
        print(f"== {name}")
        for label, texts in ((target_a, a), (target_b, b)):
            for t in sorted(texts):
                print(f"-- {label}\n{t}")
    print(f"{len(differ)} structures differ; {len(virtual)} with a vtable differ"
          + (": " + ", ".join(n for n, _, _ in virtual) if virtual else ""))
    total = len(set(layouts[target_a]) | set(layouts[target_b]))
    print(f"{total} records compared over {len(entries)} sources")
    for target, n in failed.items():
        print(f"{n} sources have errors with {target} (-v shows them); {missing} records seen on one side only")
    sys.exit(1 if differ else 2 if failed else 0)


if __name__ == "__main__":
    main()
