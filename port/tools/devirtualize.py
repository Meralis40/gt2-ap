#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Replaces C++ virtual functions in the game's classes by the console's vtable word (one-off).

A class with C++ virtual functions starts with a host vtable pointer, 8 bytes on x86-64,
which moves its fields away from the console's offsets, and decompiled code reaches them
by those offsets. So the classes keep a 4-byte word where the console has its vtable
pointer, holding that vtable's address, and a call goes through a table the port builds
for each class (rt/vtable.h, gen/vtables.cpp).

    devirtualize.py calls <compile_commands.json>   rewrites the dynamically dispatched
                                                    calls F() to v_F(); run it first, while
                                                    the functions are still virtual
    devirtualize.py tables                          writes gen/vtables.cpp from the headers as
                                                    they were (GT2AP_HEADERS: a copy with virtual)

The conversion is done: gen/vtables.cpp is now edited by hand when a class gains a
function or an override, and so are the v_ dispatchers in the class that has it first.

From WSL, with any python3.
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

PORT = Path(__file__).resolve().parents[1]
GAME = re.compile(r"[\\/]port[\\/]game[\\/]")


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


def walk(node, state, virtuals, out):
    """virtuals: ids of virtual methods; out: (file, offset of the name, name) of
    unqualified calls of them."""
    if isinstance(node, dict):
        kind = node.get("kind")
        if kind == "CXXMethodDecl" and node.get("virtual"):
            virtuals.add(node["id"])
        for key, value in node.items():
            if key in ("loc", "range"):
                walk_loc(value, state)
                continue
            if isinstance(value, (dict, list)):
                walk(value, state, virtuals, out)
        if kind == "MemberExpr" and node.get("referencedMemberDecl") and "range" in node:
            end = node["range"]["end"]
            base = node.get("inner", [{}])[0].get("type", {})
            base = base.get("desugaredQualType", base.get("qualType", ""))
            base = re.sub(r"^(const |struct |class )+|[\s*&]+$|\bconst\b", "", base).strip()
            if "spellingLoc" not in end and "offset" in end:
                out.append((state.get("file"), end["offset"], node.get("name"), node["referencedMemberDecl"],
                            base, False))
    elif isinstance(node, list):
        for item in node:
            walk(item, state, virtuals, out)


def calls_of(entry, cwd):
    cmd = command_for(entry, "--target=i686-pc-windows-msvc")
    cmd[cmd.index("-fdump-record-layouts-complete")] = "-ast-dump=json"
    r = subprocess.run(cmd, cwd=cwd, capture_output=True)
    if not r.stdout:
        sys.stderr.write(f"{entry['file']}: no AST\n")
        return []
    virtuals, members = set(), []
    walk(json.loads(r.stdout), {}, virtuals, members)
    # By name: a call of a virtual function of the object's class. "virtual" in the AST is
    # only as written, which an out-of-line definition or an override isn't.
    names = virtual_names()
    return [(f, off, name) for f, off, name, decl, base, _ in members if name in names.get(base, ())]


def virtual_names():
    """{class: names of its virtual functions, inherited ones included}"""
    classes = read_classes()
    return {name: {s[0] for s in slots(classes, name)} for name in classes}


def rewrite_calls(path):
    entries = [e for e in json.load(open(path)) if GAME.search(e["file"])]
    cwd = to_wsl(entries[0]["directory"])
    sites = set()
    with ThreadPoolExecutor(6) as pool:
        for result in pool.map(lambda e: calls_of(e, cwd), entries):
            for file, offset, name in result:
                if file and GAME.search(file):
                    sites.add((str(Path(to_wsl(file.replace("\\", "/"))).resolve()), offset, name))
    by_file = {}
    for file, offset, name in sites:
        by_file.setdefault(file, set()).add((offset, name))
    total = 0
    for file, items in sorted(by_file.items()):
        data = Path(file).read_bytes().decode("latin-1")
        for offset, name in sorted(items, reverse=True):
            if data[offset:offset + len(name)] != name:
                sys.stderr.write(f"{file}:{offset}: expected {name}\n")
                continue
            if data[offset - 2:offset] == "::" or not re.match(r"\s*\(", data[offset + len(name):]):
                continue  # Base::F() is a static call; F without a call isn't one
            data = data[:offset] + "v_" + data[offset:]
            total += 1
        Path(file).write_bytes(data.encode("latin-1"))
    print(f"{total} calls in {len(by_file)} files")


# The classes, their bases and their vtables on the console (decomp/sim/symbols). RaceLoop
# stands for GranTurismoRaceLoop.
CLASSES = [
    ("ScreenViewLoop", None, 0x800908EC, "viewloop.h"),
    ("psxViewLoop", "ScreenViewLoop", 0x800906C8, "viewloop.h"),
    ("psxViewLoopDoubleBuffer", "psxViewLoop", 0x80090630, "viewloop.h"),
    ("VendorBootLogo", "psxViewLoopDoubleBuffer", 0x80011D40, "resident.h"),
    ("psxMovieLoop", "psxViewLoop", 0x8001156C, "movie.h"),
    ("QuickMenu", "psxViewLoopDoubleBuffer", 0x8005A7FC, "ovl0.h"),
    ("PreQuickMenuIO", "psxViewLoopDoubleBuffer", 0x8005A7B4, "ovl0.h"),
    ("RaceViewLoop", "psxViewLoopDoubleBuffer", 0x8002EF98, "ovl0.h"),
    ("RaceMenuLoop", None, 0x8002EF58, "ovl0.h"),
    ("ArcadeRaceLoop", "RaceMenuLoop", 0x8002F000, "ovl0.h"),
    ("RaceLoop", "RaceMenuLoop", 0x8002F110, "ovl0.h"),
    ("BaseMenu", "psxViewLoopDoubleBuffer", 0x8002106C, "ovl1.h"),
    ("ArcadeMenu", "psxViewLoopDoubleBuffer", 0x80026FB8, "ovl2.h"),
    ("ArcadeRacingMenu", "psxViewLoopDoubleBuffer", 0x80027010, "ovl2.h"),
    ("GTMenu", "psxViewLoopDoubleBuffer", 0x80022CA4, "gtmenu.h"),
    ("GTLoadingMenu", "psxViewLoopDoubleBuffer", 0x80022C5C, "gtmenu.h"),
    ("GTBattleMenu", "psxViewLoopDoubleBuffer", 0x80022D04, "gtmenu.h"),
]
DECL = re.compile(r"^(\s*)(?:virtual\s+)?((?:[\w:]+[\s*&]+)+?)(~?\w+)\(([^)]*)\)\s*(override)?\s*;", re.M)


def class_body(text, name):
    m = re.search(rf"^(?:class|struct) {name}\b[^;{{]*\{{", text, re.M)
    depth, i = 1, m.end()
    while depth:
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        i += 1
    return m.end(), i - 1


def read_classes():
    """{name: (base, address, header, [(ret, fn, params)] of its virtual functions)}"""
    out = {}
    for name, base, address, header in CLASSES:
        # The headers as they were with virtual (GT2AP_HEADERS), once apply_headers has run.
        text = (Path(os.environ.get("GT2AP_HEADERS", PORT / "game" / "include")) / header).read_text(encoding="utf-8")
        begin, end = class_body(text, name)
        virtuals = []
        for m in DECL.finditer(text, begin, end):
            line = text[m.start():m.end()]
            if ("virtual" in line or m.group(5)) and not m.group(3).startswith("~"):
                virtuals.append((m.group(2).strip(), m.group(3), m.group(4).strip()))
        out[name] = (base, address, header, virtuals)
    return out


def slots(classes, name):
    """The slots of name's root, in order: [(fn, ret, params, introducer)]."""
    base = classes[name][0]
    inherited = slots(classes, base) if base else []
    have = {s[0] for s in inherited}
    return inherited + [(fn, ret, params, name) for ret, fn, params in classes[name][3] if fn not in have]


def root(classes, name):
    while classes[name][0]:
        name = classes[name][0]
    return name


def implementer(classes, name, fn):
    while fn not in {v[1] for v in classes[name][3]}:
        name = classes[name][0]
    return name


def args_of(params):
    if not params or params == "void":
        return ""
    return ", ".join(p.split()[-1].lstrip("*&") for p in params.split(","))


def write_tables():
    classes = read_classes()
    lines = ["// Generated by port/tools/devirtualize.py: the port's tables for the game's classes,",
             "// found by the address of the class's vtable on the console (rt/vtable.h).",
             '#include "gptr.h"', '#include "common.h"', '#include "gtmenu.h"', '#include "movie.h"',
             '#include "ovl0.h"', '#include "ovl1.h"', '#include "ovl2.h"', '#include "viewloop.h"',
             '#include "vtable.h"', '#include "rt.h"', "", "namespace {", ""]
    for name, (base, address, header, _) in classes.items():
        top = root(classes, name)
        entries = []
        for i, (fn, ret, params, intro) in enumerate(slots(classes, name)):
            impl = implementer(classes, name, fn)
            sep = ", " if params and params != "void" else ""
            lines.append(f"{ret} {name}_{fn}({top} *self{sep}{params})")
            lines.append(f"{{\n    return static_cast<{name} *>(self)->{impl}::{fn}({args_of(params)});\n}}\n")
            entries.append(f"    reinterpret_cast<void *>(&{name}_{fn}),  // {i}")
        lines.append(f"void *const kVt_{name}[] = {{  // {address:#010x}")
        lines += entries
        lines.append("};")
        lines.append("")
    lines += ["}  // namespace", "", "void *const *rt::vtable(rt::u32 address)", "{", "    switch (address) {"]
    for name, (_, address, _, _) in classes.items():
        lines.append(f"    case {address:#010x}: return kVt_{name};")
    lines += ["    }", '    Rt_Fatal("no vtable %08x", address);', "}", ""]
    (PORT / "gen" / "vtables.cpp").write_text("\n".join(lines), encoding="utf-8", newline="\n")

    # The dispatchers, in the class that has the function first.
    for name, (base, address, header, _) in classes.items():
        top = root(classes, name)
        mine = [s for s in slots(classes, name) if s[3] == name]
        print(f"// {header} {name} {address:#010x}")
        for i, (fn, ret, params, intro) in enumerate(slots(classes, name)):
            if intro != name:
                continue
            sep = ", " if params and params != "void" else ""
            types = ", ".join(p.rsplit(None, 1)[0] if " " in p else p for p in params.split(",")) if params and params != "void" else ""
            print(f"    {ret} v_{fn}({params}) {{ return vptr.slot<{ret}({top} *{', ' + types if types else ''})>({i})(this{sep}{args_of(params)}); }}")


def apply_headers():
    """Drops virtual and override, adds the vtable word to the roots and the dispatchers."""
    classes = read_classes()
    roots_word = {"ScreenViewLoop": None, "RaceMenuLoop": "    int state;"}
    for name, (base, address, header, _) in classes.items():
        path = PORT / "game" / "include" / header
        text = path.read_text(encoding="utf-8")
        begin, end = class_body(text, name)
        body = text[begin:end]
        body = re.sub(r"\bvirtual\s+", "", body)
        body = re.sub(r"\s+override\s*;", ";", body)
        top = root(classes, name)
        disp = []
        for i, (fn, ret, params, intro) in enumerate(slots(classes, name)):
            if intro != name:
                continue
            sep = ", " if params and params != "void" else ""
            types = ", ".join(p.rsplit(None, 1)[0] if " " in p else p for p in params.split(",")) \
                if params and params != "void" else ""
            disp.append(f"    {ret} v_{fn}({params}) {{ return vptr.slot<{ret}({top} *{', ' + types if types else ''})>"
                        f"({i})(this{sep}{args_of(params)}); }}")
        if disp:
            body = body.rstrip() + "\n    // Calls through the vtable word (rt/vtable.h).\n" + "\n".join(disp) + "\n"
        if name in ("QuickMenu", "PreQuickMenuIO"):
            body = body.rstrip() + f"\n    {name}() {{ vptr.address = {address:#010x}; }}\n"
        if name in roots_word:
            word = "    game_vptr vptr;  // the console's vtable pointer: its vtable's address\n"
            after = roots_word[name]
            if after is None:
                body = re.sub(r"(public:\n)", r"\1" + word.replace("\\", "\\\\"), body, count=1)
            else:
                body = body.replace(after + "\n", after.rstrip() + "\n" + word, 1) if after + "\n" in body else \
                    re.sub(r"(    int state;[^\n]*\n)", r"\1" + word, body, count=1)
        text = text[:begin] + body + text[end:]
        if '#include "vtable.h"' not in text and '#include "vtable.h"' not in text:
            text = text.replace('#include "gptr.h"\n', '#include "gptr.h"\n#include "vtable.h"\n', 1)
        path.write_text(text, encoding="utf-8", newline="\n")
    # The constructors set the word.
    for name, (_, address, _, _) in classes.items():
        for path in (PORT / "game").rglob("*.cpp"):
            text = path.read_text(encoding="utf-8")
            m = re.search(rf"^{name}::{name}\([^)]*\)[^{{;]*\{{\n", text, re.M)
            if m:
                text = text[:m.end()] + f"    vptr.address = {address:#010x};  // {name}'s vtable\n" + text[m.end():]
                path.write_text(text, encoding="utf-8", newline="\n")
                break
        else:
            print(f"no constructor for {name} in a .cpp")


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "headers":
        apply_headers()
    elif len(sys.argv) == 3 and sys.argv[1] == "calls":
        rewrite_calls(sys.argv[2])
    elif len(sys.argv) == 2 and sys.argv[1] == "tables":
        write_tables()
    else:
        sys.exit(__doc__)
