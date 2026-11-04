#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Compiles sources with NON_MATCHING defined, which the normal build never does,
and compares each function that replaces an INCLUDE_ASM with its original assembly,
relocated fields masked. See docs/decomp.md.

    python3 tools/check_nonmatching.py ovl1                 # every file of a unit
    python3 tools/check_nonmatching.py ovl1 langselect -v   # one file, with diffs
"""
import argparse
import glob
import os
import re
import struct
import subprocess
import sys

import build

ROOT = build.ROOT
OUT = "build/nm"
INSN = re.compile(r"/\* [0-9A-F]+ [0-9A-F]+ ([0-9A-F]{8}) \*/\s+(.*)")
RELOC = re.compile(r"^([0-9a-f]+) +(R_MIPS_\S+)", re.M)
MASKS = {"R_MIPS_HI16": 0xFFFF0000, "R_MIPS_LO16": 0xFFFF0000, "R_MIPS_26": 0xFC000000}


def original(path):
    """The function's words and their disassembly, from splat's .s file."""
    words, text = [], []
    with open(path) as fh:
        for line in fh:
            m = INSN.search(line)
            if m:
                words.append(struct.unpack("<I", bytes.fromhex(m.group(1)))[0])
                text.append(m.group(2))
    return words, text


def compile_file(unit, name):
    src = "src/sim/%s/%s.cpp" % (unit, name)
    obj = "%s/%s_%s.o" % (OUT, unit, name)
    os.makedirs(os.path.join(ROOT, OUT), exist_ok=True)
    if "-DNON_MATCHING" not in build.CPP_FLAGS:
        build.CPP_FLAGS.append("-DNON_MATCHING")
    gas = src in build.GAS_CXX
    flags = build.CC1PLUS_FLAGS + (["-mhard-float"] if src in build.HARD_FLOAT_CXX else [])
    build.compile_cc(src, obj, "gcc-2.8.1-psx", "cc1plus", flags, maspsx=not gas, expand_div=gas)
    return obj


def text_of(obj):
    """The object's .text, its functions and the masks of its relocations."""
    path = os.path.join(ROOT, obj)
    binary = path + ".text"
    subprocess.run([build.CROSS + "objcopy", "-O", "binary", "-j", ".text", path, binary], check=True)
    with open(binary, "rb") as fh:
        data = fh.read()
    os.remove(binary)
    syms = {}
    nm = subprocess.run([build.CROSS + "nm", path], stdout=subprocess.PIPE, check=True)
    for line in nm.stdout.decode().splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "Tt":
            syms[parts[2]] = int(parts[0], 16)
    relocs = subprocess.run([build.CROSS + "objdump", "-r", "-j", ".text", path],
                            stdout=subprocess.PIPE, check=True).stdout.decode()
    masks = {int(off, 16): MASKS[kind] for off, kind in RELOC.findall(relocs) if kind in MASKS}
    return data, syms, masks


def check(unit, name):
    """(function, instructions, differences) of the functions under NON_MATCHING."""
    src = "src/sim/%s/%s.cpp" % (unit, name)
    with open(os.path.join(ROOT, src)) as fh:
        source = fh.read()
    if "NON_MATCHING" not in source:
        return []
    data, syms, masks = text_of(compile_file(unit, name))
    # A method's splat name, from the g++ name CXX_NAMES links it to.
    for mangled, alias in build.CXX_NAMES.get(unit, {}).items():
        if alias in syms:
            syms[mangled] = syms[alias]
        if mangled in syms:
            syms[alias] = syms[mangled]
    results = []
    for path in sorted(glob.glob(os.path.join(ROOT, "asm/sim", unit, "nonmatchings", name, "*.s"))):
        func = os.path.basename(path)[:-2]
        # A function still only INCLUDE_ASM has its label too: skip what the
        # preprocessor kept as assembly with NON_MATCHING defined.
        if func not in syms or not re.search(r"#ifdef NON_MATCHING(?:(?!#endif).)*?INCLUDE_ASM\([^\n]*\b%s\)"
                                             % re.escape(func), source, re.S):
            continue
        words, text = original(path)
        start = syms[func]
        diffs = []
        for i, want in enumerate(words):
            off = start + i * 4
            got = struct.unpack_from("<I", data, off)[0] if off + 4 <= len(data) else None
            mask = masks.get(off, 0xFFFFFFFF)
            if got is None or got & mask != want & mask:
                diffs.append((i, want, got, text[i]))
        results.append((func, len(words), diffs))
    return results


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit", choices=build.UNITS)
    ap.add_argument("files", nargs="*", help="file names without .cpp (default: all of the unit)")
    ap.add_argument("-v", "--verbose", action="store_true", help="print the differing instructions")
    args = ap.parse_args()
    names = args.files or sorted(os.path.basename(p)[:-4]
                                 for p in glob.glob(os.path.join(ROOT, "src/sim", args.unit, "*.cpp")))
    same = total = 0
    for name in names:
        for func, count, diffs in check(args.unit, name):
            total += 1
            same += not diffs
            print("%-40s %4d insns  %s" % (func, count, "MATCH" if not diffs else "%d differ" % len(diffs)))
            if args.verbose:
                for i, want, got, text in diffs[:40]:
                    print("    +%03x want %08x got %s  %s"
                          % (i * 4, want, "%08x" % got if got is not None else "-", text))
    print("%d of %d under NON_MATCHING match" % (same, total))


if __name__ == "__main__":
    sys.exit(main())
