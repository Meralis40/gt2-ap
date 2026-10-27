# SPDX-License-Identifier: GPL-2.0-or-later
# Exports the Ghidra names of SCUS_944.88 and its overlays as splat symbol files.
#
# Writes one file per link unit into decomp/sim/symbols/ (see docs/decomp.md):
#   exe.txt   boot code, resident code and the main-space data/BSS labels
#   ovlN.txt  overlay N functions and labels, plus every resident symbol
#             (the overlay links against resident code)
#
# Only named symbols are exported: functions, and labels whose source is
# USER_DEFINED or IMPORTED (PsyQ signatures) that are not inside a function.
# Names are made valid for the assembler ("Class::Method" -> "Class__Method",
# "~Class" -> "Class_dtor") and made unique within each link unit.
#
# Run it from the Script Manager, or with ReVa's run-script. Set OUT_DIR in the
# script globals or the SPLAT_SYMBOLS_DIR environment variable to write
# somewhere else.
#
# @category GT2
# @runtime PyGhidra

import os
import re

from ghidra.program.model.symbol import SourceType, SymbolType

OVERLAY_BASE = 0x80010000
RESIDENT_START = 0x8005d600
OVERLAYS = ["OVL%d" % i for i in range(6)]


def out_dir():
    d = globals().get("OUT_DIR") or os.environ.get("SPLAT_SYMBOLS_DIR")
    if d:
        return d
    # scripts/ghidra/<this file> -> <repo>/decomp/sim/symbols
    src = getSourceFile().getAbsolutePath()
    repo = os.path.dirname(os.path.dirname(os.path.dirname(src)))
    return os.path.join(repo, "decomp", "sim", "symbols")


def sanitize(name):
    name = name.replace("::", "__")
    name = re.sub(r"~(\w+)", r"\1_dtor", name)
    name = re.sub(r"[^A-Za-z0-9_]", "_", name)
    if name[0].isdigit():
        name = "_" + name
    return name


def collect(program):
    """Returns {space name: [(offset, name, kind)]} with kind 'func' or 'data'."""
    fm = program.getFunctionManager()
    st = program.getSymbolTable()
    out = {}
    for f in fm.getFunctions(True):
        if f.isExternal():
            continue
        # A thunk takes its target's name unless it was given one of its own.
        if f.isThunk() and f.getSymbol().getSource() != SourceType.USER_DEFINED:
            continue
        a = f.getEntryPoint()
        out.setdefault(a.getAddressSpace().getName(), []).append((a.getOffset(), f.getName(True), "func"))
    for s in st.getAllSymbols(True):
        if s.getSymbolType() != SymbolType.LABEL or not s.isPrimary():
            continue
        if s.getSource() not in (SourceType.USER_DEFINED, SourceType.IMPORTED):
            continue
        a = s.getAddress()
        if not a.isMemoryAddress() or fm.getFunctionContaining(a) is not None:
            continue
        out.setdefault(a.getAddressSpace().getName(), []).append((a.getOffset(), s.getName(True), "data"))
    return out


def write(path, entries):
    by_addr = {}
    for off, name, kind in entries:
        by_addr.setdefault(off, (name, kind))  # first one wins (overlay before resident)
    seen = {}
    lines = []
    for off in sorted(by_addr):
        name, kind = by_addr[off]
        name = sanitize(name)
        if name in seen:
            name = "%s_%08X" % (name, off)
        seen[name] = off
        lines.append("%s = 0x%08X;%s" % (name, off, " // type:func" if kind == "func" else ""))
    with open(path, "w") as fh:
        fh.write("\n".join(lines) + "\n")
    return len(lines)


def run():
    syms = collect(currentProgram)
    main = [e for sp, es in syms.items() if not sp.startswith("OVL") for e in es if e[0] >= OVERLAY_BASE]
    resident = [e for e in main if e[0] >= RESIDENT_START]
    d = out_dir()
    if not os.path.isdir(d):
        os.makedirs(d)
    print("exe.txt: %d" % write(os.path.join(d, "exe.txt"), main))
    for ovl in OVERLAYS:
        n = write(os.path.join(d, ovl.lower() + ".txt"), syms.get(ovl, []) + resident)
        print("%s.txt: %d" % (ovl.lower(), n))


run()
