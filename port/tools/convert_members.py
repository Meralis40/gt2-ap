#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Turns the pointer members of the game's structures into guest pointers (one-off).

A structure in guest memory holds 4-byte PlayStation addresses, so each member of
pointer type becomes game_pointer<T>, and each function pointer game_function<F>
(rt/gptr.h). This reads Clang's AST of every source of port/game, from a
compile_commands.json, and rewrites the members' declarations in place. Members it
can't rewrite safely (several in one declaration, inside a macro, unnamed types, C
sources) are listed for a hand edit.

    convert_members.py <compile_commands.json>      (from WSL, any python3)
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


def is_target(node):
    """A structure member, or an extern reference to guest data (tools/ref_externs.py)."""
    kind = node.get("kind")
    if kind == "FieldDecl":
        return True
    return (kind == "VarDecl" and node.get("storageClass") == "extern"
            and "&" in node.get("type", {}).get("qualType", ""))


def unreference(t):
    """The referenced type of t: "X *&" -> "X *", "T (&)[4]" -> "T [4]", "R (*&)(A)" -> "R (*)(A)"."""
    t = t.strip()
    if t.endswith("&"):
        return t[:-1].strip()
    t = t.replace("(*&)", "(*)", 1)
    return re.sub(r"\s*\(&\)\s*", " ", t, count=1)


def walk(node, state, out):
    """Visits nodes in document order, tracking the file that locations elide."""
    if isinstance(node, dict):
        if "offset" in node or "spellingLoc" in node:
            loc_file(node, state)
        if is_target(node):
            out.append((node, dict(state)))
        for key, value in node.items():
            if key == "loc" or key == "range":
                walk_loc(value, state)
                if is_target(node) and key == "loc":
                    out[-1][1]["file"] = state.get("file")
                continue
            if isinstance(value, (dict, list)):
                walk(value, state, out)
    elif isinstance(node, list):
        for item in node:
            walk(item, state, out)


def loc_file(loc, state):
    if "spellingLoc" in loc:
        loc_file(loc["spellingLoc"], state)
        loc_file(loc["expansionLoc"], state)
    elif "file" in loc:
        state["file"] = loc["file"]


def walk_loc(value, state):
    if not isinstance(value, dict):
        return
    if "begin" in value:
        walk_loc(value["begin"], state)
        walk_loc(value["end"], state)
    else:
        loc_file(value, state)


def split_top(text, open_c, close_c, sep=","):
    depth, parts, cur = 0, [], ""
    for c in text:
        if c in open_c:
            depth += 1
        elif c in close_c:
            depth -= 1
        if c == sep and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += c
    parts.append(cur)
    return parts


def guest_type(t):
    """The guest form of pointer type t ("char *", "void (*)(int)", "short (*)[2]"),
    or None when t isn't a pointer."""
    t = t.strip()
    m = re.fullmatch(r"(.*)\(\*\)\s*(\(.*\))", t)  # function pointer
    if m:
        return f"game_function<{m.group(1).strip()}{m.group(2)}>"
    m = re.fullmatch(r"(.*)\(\*\)\s*((?:\[\w*\])+)", t)  # pointer to array
    if m:
        return f"game_pointer<{m.group(1).strip()}{m.group(2)}>"
    m = re.fullmatch(r"(.*\S)\s*\*\s*(const|volatile)?", t)
    if m and not m.group(2):
        inner = m.group(1).strip()
        return f"game_pointer<{guest_type(inner) or inner}>"
    return None


def edit_for(field, source):
    """(begin, end, replacement) for a member, or a reason it's left alone."""
    loc, rng = field["loc"], field["range"]
    if "spellingLoc" in loc or "spellingLoc" in rng["begin"] or "spellingLoc" in rng["end"]:
        return "in a macro"
    qual = field["type"].get("desugaredQualType", field["type"]["qualType"])
    if "(unnamed" in qual or "(anonymous" in qual or "(lambda" in qual:
        return "unnamed type"
    if field["kind"] == "VarDecl":
        return edit_extern(field, source, unreference(qual))
    dims = ""
    m = re.fullmatch(r"(.*?)\s*((?:\[\d*\])+)", qual)
    if m and not qual.endswith(")"):
        qual, dims = m.group(1), m.group(2)
    elif re.search(r"\)\s*(\[\d*\])+$", qual) and "(*)" not in qual.replace("(*)[", ""):
        pass
    new = guest_type(qual)
    if new is None:
        return None
    begin = rng["begin"]["offset"]
    end = rng["end"]["offset"] + rng["end"]["tokLen"]
    name = field.get("name", "")
    decl = source[begin:end]
    if not name or name not in decl:
        return "no name"
    after = decl[decl.rindex(name) + len(name):].strip()
    if after and not re.fullmatch(r"(\[[^\]]*\])+", after):
        after = dims  # e.g. an array of function pointers
    return begin, end, f"{new} {name}{after}"


def edit_extern(var, source, qual):
    """extern T *(&name)[dims]; -> extern game_pointer<T> (&name)[dims];"""
    dims = ""
    m = re.fullmatch(r"(.*?)\s*((?:\[\d*\])+)", qual)
    if m and "(*)" not in m.group(1):
        qual, dims = m.group(1), m.group(2)
    new = guest_type(qual)
    if new is None:
        return None
    rng = var["range"]
    begin = rng["begin"]["offset"]
    end = rng["end"]["offset"] + rng["end"]["tokLen"]
    decl = source[begin:end]
    name = var["name"]
    if f"(&{name})" not in decl:
        return "not a plain reference"
    prefix = re.match(r'\s*extern\s*(?:"C"\s*)?', decl)
    return begin, end, f"{prefix.group(0) if prefix else ''}{new} (&{name}){dims}"


def fields_of(entry, cwd):
    cmd = command_for(entry, "--target=i686-pc-windows-msvc")
    cmd[cmd.index("-fdump-record-layouts-complete")] = "-ast-dump=json"
    r = subprocess.run(cmd, cwd=cwd, capture_output=True)
    if not r.stdout:
        sys.stderr.write(f"{entry['file']}: no AST\n{r.stderr.decode(errors='replace')[-1500:]}\n")
        return []
    tree = json.loads(r.stdout)
    found = []
    walk(tree, {}, found)
    return [(state.get("file"), field) for field, state in found]


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    entries = [e for e in json.load(open(sys.argv[1])) if GAME.search(e["file"])]
    cwd = to_wsl(entries[0]["directory"])
    fields = {}
    with ThreadPoolExecutor(6) as pool:
        for result in pool.map(lambda e: fields_of(e, cwd), entries):
            for file, field in result:
                if file and GAME.search(file) and "range" in field and not field.get("isImplicit"):
                    fields[(os.path.normpath(to_wsl(file.replace("\\", "/"))), field["range"]["begin"].get("offset"),
                            field["loc"].get("offset"))] = field

    by_file, skipped = {}, []
    for (file, begin, _), field in fields.items():
        by_file.setdefault(file, []).append(field)
    total = 0
    for file, members in sorted(by_file.items()):
        path = Path(file)
        source = path.read_bytes().decode("utf-8")
        raw = path.read_bytes()
        begins = {}
        for f in members:
            begins.setdefault(f["range"]["begin"].get("offset"), []).append(f)
        edits = []
        for f in members:
            result = edit_for(f, raw.decode("latin-1"))
            line = f["loc"].get("line", "?")
            if result is None:
                continue
            if isinstance(result, str):
                skipped.append(f"{file}:{line}: {f.get('name')}: {result}")
                continue
            if len(begins[f["range"]["begin"]["offset"]]) > 1:
                skipped.append(f"{file}:{line}: {f.get('name')}: several in one declaration")
                continue
            if path.suffix == ".c" or path.name == "syslib.h":
                skipped.append(f"{file}:{line}: {f.get('name')}: C")
                continue
            edits.append(result)
        if not edits:
            continue
        data = raw.decode("latin-1")  # offsets are bytes
        for begin, end, text in sorted(edits, reverse=True):
            data = data[:begin] + text + data[end:]
        path.write_bytes(data.encode("latin-1"))
        total += len(edits)
        del source
    print(f"{total} members changed in {len(by_file)} files")
    for s in sorted(set(skipped)):
        print(f"left: {s}")


if __name__ == "__main__":
    main()
