#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Builds the Simulation disc executable and overlays from the splat output
and checks them against the original files. See docs/decomp.md.

    python3 tools/build.py              # split if needed, build and check all units
    python3 tools/build.py ovl3 ovl5    # only these units
    python3 tools/build.py --split      # re-run splat first (after a symbol or yaml change)
"""
import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

import yaml

import psyq

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # decomp/
UNITS = ["exe", "ovl0", "ovl1", "ovl2", "ovl3", "ovl4", "ovl5"]
OUTPUT = {"exe": "SCUS_944.88"}  # others: <unit>.bin
CROSS = os.environ.get("CROSS", "mipsel-linux-gnu-")
AS_FLAGS = ["-EL", "-march=r3000", "-mtune=r3000", "-mabi=32", "-G0", "-no-pad-sections", "-Iinclude"]
BUILD = "build/sim"

# C++ sources: GCC 2.8.1, as Sony's PsyQ 4.4 compiler (tools/setup_toolchain.py).
# The host cpp preprocesses, as on decomp.me: the 32-bit cpp and cc1plus cannot
# stat files on a Windows drive under WSL, so cc1plus reads the preprocessed
# source from stdin.
TOOLCHAIN = os.path.join(ROOT, "tools", "toolchain")
CPP_FLAGS = ["-nostdinc", "-undef", "-D__GNUC__=2", "-D__GNUC_MINOR__=8", "-Dmips", "-D__mips__",
             "-DMIPSEL", "-Iinclude"]
CC1PLUS_FLAGS = ["-quiet", "-O2", "-g0", "-G0", "-fvtable-thunks"]
MASPSX_FLAGS = ["--aspsx-version=2.86"]  # ASPSX of PsyQ 4.6

# PsyQ library members (the yaml's lib subsegments, tools/psyq.py). splat links
# their .text, .rdata and .data; their .bss goes in its own NOLOAD section at
# this address, in the order of their .text. The uninitialized symbols that
# psylink placed apart from their member are defined at their addresses.
# See docs/psyq-libs.md.
PSYQ_BSS = {"exe": 0x801C9598}
PSYQ_SYMBOLS = {"_padRestPeriod": 0x801C9420, "_apiPadPatch": 0x801F0D50}


def run(cmd):
    r = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if r.returncode != 0:
        sys.exit("command failed: %s\n%s" % (" ".join(cmd), r.stdout))
    return r.stdout


def yaml_sha1(unit):
    with open(os.path.join(ROOT, "sim", unit + ".yaml")) as fh:
        return re.search(r"^sha1:\s*([0-9a-f]{40})", fh.read(), re.M).group(1)


def split(unit):
    shutil.rmtree(os.path.join(ROOT, "asm", "sim", unit), ignore_errors=True)
    # A source shared with another unit is named ../<unit>/<file>, which only
    # resolves (and so isn't replaced by a stub) when this unit's directory exists.
    os.makedirs(os.path.join(ROOT, "src", "sim", unit), exist_ok=True)
    run([sys.executable, "-m", "splat", "split", "sim/%s.yaml" % unit])


def lib_members(unit):
    """[(lib, member)] of the unit's lib subsegments, in .text order."""
    with open(os.path.join(ROOT, "sim", unit + ".yaml")) as fh:
        config = yaml.safe_load(fh)
    members = []
    for seg in config["segments"]:
        for sub in seg.get("subsegments", []) if isinstance(seg, dict) else []:
            if isinstance(sub, list) and len(sub) >= 4 and sub[1] == "lib":
                if (sub[2], sub[3]) not in members:
                    members.append((sub[2], sub[3]))
    return members


def psyq_script(unit, members):
    """Writes <unit>.psyq.ld: splat's script with the members' .bss and symbols."""
    with open(os.path.join(ROOT, BUILD, unit + ".ld")) as fh:
        script = fh.read()
    lines = ["    psyq_bss 0x%08X (NOLOAD) : SUBALIGN(4)" % PSYQ_BSS[unit], "    {"]
    lines += ["        build/psyq/%s.a:%s.o(.bss);" % m for m in members]
    lines += ["    }", ""]
    lines += ["    %s = 0x%08X;" % s for s in sorted(PSYQ_SYMBOLS.items())] + [""]
    marker = "    /DISCARD/ :"
    if script.count(marker) != 1:
        sys.exit("%s.ld: can't find the /DISCARD/ section" % unit)
    script = script.replace(marker, "\n".join(lines) + marker)
    with open(os.path.join(ROOT, BUILD, unit + ".psyq.ld"), "w") as fh:
        fh.write(script)
    return "%s/%s.psyq.ld" % (BUILD, unit)


def objects(unit):
    with open(os.path.join(ROOT, BUILD, unit + ".ld")) as fh:
        return sorted(set(re.findall(r"(%s/\S+?\.o)\(" % re.escape(BUILD), fh.read())))


def pipe(cmd, data):
    r = subprocess.run(cmd, cwd=ROOT, input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        sys.exit("command failed: %s\n%s" % (" ".join(cmd), r.stderr.decode(errors="replace")))
    return r.stdout


def compile_cpp(src, obj):
    if not os.path.exists(os.path.join(TOOLCHAIN, "gcc-2.8.1-psx", "cc1plus")):
        sys.exit("missing compiler: run tools/setup_toolchain.py first")
    pre = pipe(["cpp"] + CPP_FLAGS + [src], None)
    pre = re.sub(rb"(?m)^# 0", b"# 1", pre)  # cc1plus rejects line 0 markers
    asm = pipe([os.path.join(TOOLCHAIN, "gcc-2.8.1-psx", "cc1plus")] + CC1PLUS_FLAGS + ["-o", "-"], pre)
    with open(os.path.join(ROOT, obj[:-2] + ".s"), "wb") as fh:
        fh.write(asm)  # kept for diffing
    pipe([sys.executable, os.path.join(TOOLCHAIN, "maspsx", "maspsx.py")] + MASPSX_FLAGS
         + ["--run-assembler", "--gnu-as-path", CROSS + "as"] + AS_FLAGS + ["-o", obj], asm)


def assemble(obj):
    src = obj[len(BUILD) + 1:-2]
    os.makedirs(os.path.join(ROOT, os.path.dirname(obj)), exist_ok=True)
    if src.endswith(".cpp"):
        compile_cpp(src, obj)
    else:
        run([CROSS + "as"] + AS_FLAGS + ["-o", obj, src])


def build(unit, pool):
    list(pool.map(assemble, objects(unit)))
    elf = "%s/%s.elf" % (BUILD, unit)
    out = "%s/%s" % (BUILD, OUTPUT.get(unit, unit + ".bin"))
    script, libs = "%s/%s.ld" % (BUILD, unit), []
    members = lib_members(unit)
    if members:
        psyq.build_archives(members)
        script = psyq_script(unit, members)
        libs = ["--whole-archive"] + sorted({"build/psyq/%s.a" % lib for lib, _ in members}) + ["--no-whole-archive"]
    run([CROSS + "ld", "-EL", "-nostdlib", "--no-check-sections",
         "-T", script,
         "-T", "%s/%s.undefined_syms_auto.txt" % (BUILD, unit),
         "-T", "%s/%s.undefined_funcs_auto.txt" % (BUILD, unit),
         "-Map", "%s/%s.map" % (BUILD, unit), "-o", elf] + libs)
    run([CROSS + "objcopy", "-O", "binary", elf, out])
    with open(os.path.join(ROOT, out), "rb") as fh:
        got = hashlib.sha1(fh.read()).hexdigest()
    return out, got == yaml_sha1(unit)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("units", nargs="*", metavar="unit", help="one of: " + ", ".join(UNITS))
    ap.add_argument("--split", action="store_true", help="re-run splat for the units")
    args = ap.parse_args()
    for unit in args.units:
        if unit not in UNITS:
            ap.error("unknown unit %s" % unit)
    args.units = args.units or UNITS
    for unit in args.units:
        base = os.path.join(ROOT, "baserom", "sim", OUTPUT.get(unit, unit + ".bin"))
        if not os.path.exists(base):
            sys.exit("missing %s: copy the original files there first (docs/decomp.md)" % os.path.relpath(base, ROOT))
    ok = True
    with ThreadPoolExecutor() as pool:
        for unit in args.units:
            if args.split or not os.path.exists(os.path.join(ROOT, BUILD, unit + ".ld")):
                split(unit)
            out, match = build(unit, pool)
            print("%-5s %s %s" % (unit, "OK  " if match else "DIFF", out))
            ok &= match
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
