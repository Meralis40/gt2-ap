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

import libgcc
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
# -fno-builtin: the game calls memcpy for constant sizes (ReplayDir_Init), where GCC's
# builtin copies inline; nothing else changes with it.
CC1PLUS_FLAGS = ["-quiet", "-O2", "-g0", "-G0", "-fvtable-thunks", "-fno-exceptions", "-fno-builtin"]
# C sources (the game's libc, syslib): GCC 2.95.2 with signed char and type based
# alias analysis (Heap_SplitBlock needs it), assembled by
# GNU as instead of ASPSX (it puts a load delay nop before a label, where ASPSX
# and maspsx put it after). The GNU as of the time encoded move as addu.
CC1_FLAGS = ["-quiet", "-O2", "-g0", "-G0", "-fsigned-char", "-fstrict-aliasing"]
MOVE = re.compile(rb"(?m)^\tmove\t(\$\w+),(\$\w+)")
MASPSX_FLAGS = ["--aspsx-version=2.86"]  # ASPSX of PsyQ 4.6
# C++ sources assembled by GNU as, like the C ones: ScreenViewLoop::Run,
# Credits_FormatCommas, CardMenu_SetState, CarInfoA_FindIndex, CdAudio_ExecScript,
# GameDataTable_SearchId, CarWork_GetPartIdx and Records_GetSectorTime jump past the load delay nop before a
# label, as only GNU as places it (garage2.cpp, the rest of garage.cpp, needs it too), and Gpu_SetDisplayStart has its GPU
# store in the return's delay slot, which GCC leaves empty for a volatile store, and
# ReplayEntry_GetInfo has a store in a j's delay slot that GCC leaves empty (replaydir.cpp),
# as Options_ApplyNeGconCalib has in its return's (savefile.cpp) and the SPU stores
# in seq.cpp have in theirs and in a jal's.
# GCC 2.8.1 writes divisions as assembler macros, which GNU as
# expands with divide-by-zero checks the game doesn't have, so they are written
# out as div $0 and mflo (mfhi for rem), as GCC 2.95 does. ASPSX would put a nop
# after the mflo when the next instruction reads it; the game has none (FUN_800835dc in
# padbuttons.cpp, PowerGraphView_ComputeScale in powergraph.cpp).
GAS_CXX = {"src/sim/exe/screenviewloop.cpp", "src/sim/exe/format.cpp", "src/sim/exe/cardmenu.cpp",
           "src/sim/exe/carinfo.cpp", "src/sim/exe/carmodel.cpp", "src/sim/exe/cdaudio.cpp",
           "src/sim/exe/cdstream.cpp", "src/sim/exe/gamedata.cpp", "src/sim/exe/garage.cpp",
           "src/sim/exe/garage2.cpp", "src/sim/exe/padbuttons.cpp",
           "src/sim/exe/powergraph.cpp", "src/sim/exe/records.cpp", "src/sim/exe/replaydir.cpp",
           "src/sim/exe/savefile.cpp", "src/sim/exe/seq.cpp", "src/sim/exe/sfx.cpp"}
# C++ sources compiled with -mhard-float: the sound driver (seq.cpp) was built on its
# own. GCC then counts the FPU registers as free, which raises its threshold for moving
# constants out of loops: SfxVoices_Update loads 2, 1 and -1 into registers before its
# loop.
HARD_FLOAT_CXX = {"src/sim/exe/seq.cpp"}
DIV = re.compile(rb"(?m)^\t(div|divu|rem|remu)\t(\$\w+),(\$\w+),(\$\w+)$")


def div_expand(m):
    op, dest, a, b = m.groups()
    if dest in (b"$0", b"$zero"):
        return m.group(0)
    mf = b"mfhi" if op.startswith(b"rem") else b"mflo"
    return b"\t%s\t$0,%s,%s\n\t%s\t%s" % (op.replace(b"rem", b"div"), a, b, mf, dest)

# PsyQ library members (the yaml's lib subsegments, tools/psyq.py). splat links
# their .text, .rdata and .data; their .bss goes in its own NOLOAD section at
# this address, in the order of their .text. The uninitialized symbols that
# psylink placed apart from their member are defined at their addresses.
# See docs/psyq-libs.md.
PSYQ_BSS = {"exe": 0x801C9598}
PSYQ_SYMBOLS = {"_padRestPeriod": 0x801C9420, "_apiPadPatch": 0x801F0D50}

# GCC 2.8.1's libgcc objects (the yaml's libgcc lib subsegments, tools/libgcc.py),
# placed like the PsyQ members: their .bss (__main's flag) at this address, the
# first word of the BSS that the startup code clears, and their common symbols
# at their addresses. See docs/runtime-libs.md.
LIBGCC_BSS = {"exe": 0x801C93B0}
LIBGCC_SYMBOLS = {"__ti9type_info": 0x801C9408, "__CTOR_LIST__": 0x801C9410, "__DTOR_LIST__": 0x801C9418,
                  "__ti16__user_type_info": 0x801F0D20, "__ti14__si_type_info": 0x801F0D30,
                  "__ti17__class_type_info": 0x801F0D40}


# Units whose .bss doesn't follow their data: an overlay's BSS starts at the end
# of the EXE's (OVL5's g_Movie, buffers and psxMovieLoop typeinfo).
BSS_VRAM = {"ovl5": 0x800A8D5C}

# Names defined as other names in a unit's link: the g++ names that its C++
# classes reference, as the symbols the Ghidra export names (resident code and
# data of the base classes), and, for the classes it defines, the Ghidra names
# that its assembly references, as the g++ names. The typeinfo objects of
# those classes (.comm) are placed at their addresses.
CXX_NAMES = {"exe": {
    "ScreenViewLoop__ScreenViewLoop": "__14ScreenViewLoop", "ScreenViewLoop__ScreenViewLoop_dtor": "_._14ScreenViewLoop",
    "ScreenViewLoop__Execute": "Execute__14ScreenViewLoop", "ScreenViewLoop__Run": "Run__14ScreenViewLoop",
    "ScreenViewLoop__Enter": "Enter__14ScreenViewLoop", "ScreenViewLoop__Step": "Step__14ScreenViewLoop",
    "ScreenViewLoop__Flip": "Flip__14ScreenViewLoop", "ScreenViewLoop__Leave": "Leave__14ScreenViewLoop",
    "ScreenViewLoop____tf": "__tf14ScreenViewLoop", "ScreenViewLoop__vtable": "__vt_14ScreenViewLoop",
    "__ti14ScreenViewLoop": "0x801C9400",
    "psxViewLoop__VSyncHandler": "VSyncHandler__11psxViewLoop", "psxViewLoop__psxViewLoop": "__11psxViewLoop",
    "psxViewLoop__psxViewLoop_dtor": "_._11psxViewLoop", "psxViewLoop__SetVideoMode": "SetVideoMode__11psxViewLoopUi",
    "psxViewLoop__Flip": "Flip__11psxViewLoop", "psxViewLoop__Leave": "Leave__11psxViewLoop",
    "psxViewLoop__BeginFrame": "BeginFrame__11psxViewLoop", "psxViewLoop__Update": "Update__11psxViewLoop",
    "psxViewLoop__BeginDraw": "BeginDraw__11psxViewLoop", "psxViewLoop__Draw": "Draw__11psxViewLoop",
    "psxViewLoop__EndDraw": "EndDraw__11psxViewLoop", "psxViewLoop__DrawFrame": "DrawFrame__11psxViewLoop",
    "psxViewLoop__SwapDisplay": "SwapDisplay__11psxViewLoop",
    "psxViewLoop__ApplyVideoMode": "ApplyVideoMode__11psxViewLoop",
    "psxViewLoop__UpdateDisplay": "UpdateDisplay__11psxViewLoop", "psxViewLoop____tf": "__tf11psxViewLoop",
    "psxViewLoop__vtable": "__vt_11psxViewLoop", "__ti11psxViewLoop": "0x801F0CF0",
    "psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer": "__23psxViewLoopDoubleBuffer",
    "psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor": "_._23psxViewLoopDoubleBuffer",
    "psxViewLoopDoubleBuffer__Leave": "Leave__23psxViewLoopDoubleBuffer",
    "psxViewLoopDoubleBuffer__SetVideoMode": "SetVideoMode__23psxViewLoopDoubleBufferUi",
    "psxViewLoopDoubleBuffer__BeginDraw": "BeginDraw__23psxViewLoopDoubleBuffer",
    "psxViewLoopDoubleBuffer__SwapDisplay": "SwapDisplay__23psxViewLoopDoubleBuffer",
    "psxViewLoopDoubleBuffer____tf": "__tf23psxViewLoopDoubleBuffer",
    "psxViewLoopDoubleBuffer__vtable": "__vt_23psxViewLoopDoubleBuffer",
    "__ti23psxViewLoopDoubleBuffer": "0x801F0CE0",
    "VendorBootLogo__VendorBootLogo": "__14VendorBootLogo", "VendorBootLogo__VendorBootLogo_dtor": "_._14VendorBootLogo",
    "VendorBootLogo__InitLayer": "InitLayer__14VendorBootLogo", "VendorBootLogo__Update": "Update__14VendorBootLogo",
    "VendorBootLogo__Draw": "Draw__14VendorBootLogo", "VendorBootLogo____tf": "__tf14VendorBootLogo",
    "VendorBootLogo__vtable": "__vt_14VendorBootLogo", "__ti14VendorBootLogo": "0x801697D0",
}, "ovl5": {
    "__11psxViewLoop": "psxViewLoop__psxViewLoop", "_._11psxViewLoop": "psxViewLoop__psxViewLoop_dtor",
    "Enter__11psxViewLoop": "psxViewLoop__Enter", "Step__11psxViewLoop": "psxViewLoop__Step",
    "Leave__11psxViewLoop": "psxViewLoop__Leave", "BeginFrame__11psxViewLoop": "psxViewLoop__BeginFrame",
    "DrawFrame__11psxViewLoop": "psxViewLoop__DrawFrame", "SwapDisplay__11psxViewLoop": "psxViewLoop__SwapDisplay",
    "ApplyVideoMode__11psxViewLoop": "psxViewLoop__ApplyVideoMode", "__tf11psxViewLoop": "psxViewLoop____tf",
    "__ti11psxViewLoop": "psxViewLoop__typeinfo",
    "Run__14ScreenViewLoop": "ScreenViewLoop__Run", "Execute__14ScreenViewLoop": "ScreenViewLoop__Execute",
}, "ovl4": {
    # GTMenu and GTLoadingMenu (gtmenu.cpp): their resident bases...
    "__23psxViewLoopDoubleBuffer": "psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer",
    "_._23psxViewLoopDoubleBuffer": "psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor",
    "SetVideoMode__23psxViewLoopDoubleBufferUi": "psxViewLoopDoubleBuffer__SetVideoMode",
    "Leave__23psxViewLoopDoubleBuffer": "psxViewLoopDoubleBuffer__Leave",
    "BeginDraw__23psxViewLoopDoubleBuffer": "psxViewLoopDoubleBuffer__BeginDraw",
    "SwapDisplay__23psxViewLoopDoubleBuffer": "psxViewLoopDoubleBuffer__SwapDisplay",
    "__tf23psxViewLoopDoubleBuffer": "psxViewLoopDoubleBuffer____tf",
    "__ti23psxViewLoopDoubleBuffer": "psxViewLoopDoubleBuffer__typeinfo",
    "Enter__11psxViewLoop": "psxViewLoop__Enter", "Step__11psxViewLoop": "psxViewLoop__Step",
    "Flip__11psxViewLoop": "psxViewLoop__Flip", "BeginFrame__11psxViewLoop": "psxViewLoop__BeginFrame",
    "Update__11psxViewLoop": "psxViewLoop__Update", "Draw__11psxViewLoop": "psxViewLoop__Draw",
    "EndDraw__11psxViewLoop": "psxViewLoop__EndDraw", "DrawFrame__11psxViewLoop": "psxViewLoop__DrawFrame",
    "ApplyVideoMode__11psxViewLoop": "psxViewLoop__ApplyVideoMode",
    "UpdateDisplay__11psxViewLoop": "psxViewLoop__UpdateDisplay",
    "Run__14ScreenViewLoop": "ScreenViewLoop__Run",
    # ...GTMenu's methods that are still assembly (BeginFrame in menudraw)...
    "BeginFrame__6GTMenu": "GTMenu__BeginFrame", "Draw__6GTMenu": "GTMenu__Draw",
    # ...and the names the assembly calls them by, and their typeinfo.
    "GTMenu__GTMenu": "__6GTMenu", "GTMenu__GTMenu_dtor": "_._6GTMenu",
    "GTMenu__Enter": "Enter__6GTMenu", "GTMenu__Update": "Update__6GTMenu",
    "GTMenu__Leave": "Leave__6GTMenu", "GTMenu____tf": "__tf6GTMenu", "GTMenu__vtable": "__vt_6GTMenu",
    "__ti6GTMenu": "0x801C3060",
    "GTLoadingMenu__GTLoadingMenu": "__13GTLoadingMenu", "GTLoadingMenu__GTLoadingMenu_dtor": "_._13GTLoadingMenu",
    "GTLoadingMenu__Enter": "Enter__13GTLoadingMenu", "GTLoadingMenu__Update": "Update__13GTLoadingMenu",
    "GTLoadingMenu__Draw": "Draw__13GTLoadingMenu", "GTLoadingMenu____tf": "__tf13GTLoadingMenu",
    "GTLoadingMenu__vtable": "__vt_13GTLoadingMenu", "__ti13GTLoadingMenu": "0x801C3050",
    # GTBattleMenu (gtbattlemenu.cpp)
    "GTBattleMenu__GTBattleMenu": "__12GTBattleMenu", "GTBattleMenu__GTBattleMenu_dtor": "_._12GTBattleMenu",
    "GTBattleMenu__Enter": "Enter__12GTBattleMenu", "GTBattleMenu__Update": "Update__12GTBattleMenu",
    "GTBattleMenu__Draw": "Draw__12GTBattleMenu", "GTBattleMenu____tf": "__tf12GTBattleMenu",
    "GTBattleMenu__vtable": "__vt_12GTBattleMenu", "__ti12GTBattleMenu": "0x801C3070",
}}


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
    """Writes <unit>.psyq.ld: splat's script with the library members' .bss and symbols."""
    with open(os.path.join(ROOT, BUILD, unit + ".ld")) as fh:
        script = fh.read()
    lines = []
    for name, addr, libs, symbols in [("libgcc_bss", LIBGCC_BSS, ["libgcc"], LIBGCC_SYMBOLS),
                                      ("psyq_bss", PSYQ_BSS, sorted(psyq.LIBS), PSYQ_SYMBOLS)]:
        objs = [m for m in members if m[0] in libs]
        if not objs:
            continue
        lines += ["    %s 0x%08X (NOLOAD) : SUBALIGN(4)" % (name, addr[unit]), "    {"]
        lines += ["        build/psyq/%s.a:%s.o(.bss);" % m for m in objs]
        lines += ["    }", ""]
        lines += ["    %s = 0x%08X;" % s for s in sorted(symbols.items())] + [""]
    marker = "    /DISCARD/ :"
    if script.count(marker) != 1:
        sys.exit("%s.ld: can't find the /DISCARD/ section" % unit)
    script = script.replace(marker, "\n".join(lines) + marker)
    with open(os.path.join(ROOT, BUILD, unit + ".psyq.ld"), "w") as fh:
        fh.write(script)
    return "%s/%s.psyq.ld" % (BUILD, unit)


def cxx_script(unit, script):
    """Writes <unit>.cxx.ld: the script with the unit's .bss at BSS_VRAM and its CXX_NAMES."""
    with open(os.path.join(ROOT, script)) as fh:
        text = fh.read()
    if unit in BSS_VRAM:
        section = ".%s_bss (NOLOAD)" % unit
        if text.count(section) != 1:
            sys.exit("%s: can't find the %s section" % (script, section))
        text = text.replace(section, ".%s_bss 0x%08X (NOLOAD)" % (unit, BSS_VRAM[unit]))
    names = ["    %s = %s;" % n for n in sorted(CXX_NAMES.get(unit, {}).items())]
    marker = "    /DISCARD/ :"
    if text.count(marker) != 1:
        sys.exit("%s: can't find the /DISCARD/ section" % script)
    text = text.replace(marker, "\n".join(names + ["", marker]))
    out = "%s/%s.cxx.ld" % (BUILD, unit)
    with open(os.path.join(ROOT, out), "w") as fh:
        fh.write(text)
    return out


def objects(unit):
    with open(os.path.join(ROOT, BUILD, unit + ".ld")) as fh:
        return sorted(set(re.findall(r"(%s/\S+?\.o)\(" % re.escape(BUILD), fh.read())))


def pipe(cmd, data):
    r = subprocess.run(cmd, cwd=ROOT, input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        sys.exit("command failed: %s\n%s" % (" ".join(cmd), r.stderr.decode(errors="replace")))
    return r.stdout


def compile_cc(src, obj, gcc, cc, flags, maspsx=True, expand_div=False):
    if not os.path.exists(os.path.join(TOOLCHAIN, gcc, cc)):
        sys.exit("missing compiler: run tools/setup_toolchain.py first")
    pre = pipe(["cpp"] + CPP_FLAGS + [src], None)
    pre = re.sub(rb"(?m)^# 0", b"# 1", pre)  # cc1plus rejects line 0 markers
    asm = pipe([os.path.join(TOOLCHAIN, gcc, cc)] + flags + ["-o", "-"], pre)
    with open(os.path.join(ROOT, obj[:-2] + ".s"), "wb") as fh:
        fh.write(asm)  # kept for diffing
    if maspsx:
        pipe([sys.executable, os.path.join(TOOLCHAIN, "maspsx", "maspsx.py")] + MASPSX_FLAGS
             + ["--run-assembler", "--gnu-as-path", CROSS + "as"] + AS_FLAGS + ["-o", obj], asm)
    else:
        if expand_div:
            asm = DIV.sub(div_expand, asm)
        pipe([CROSS + "as"] + AS_FLAGS + ["-o", obj], MOVE.sub(rb"\taddu\t\1,\2,$0", asm))


def assemble(obj):
    src = obj[len(BUILD) + 1:-2]
    os.makedirs(os.path.join(ROOT, os.path.dirname(obj)), exist_ok=True)
    if src.endswith(".cpp"):
        gas = src in GAS_CXX
        flags = CC1PLUS_FLAGS + (["-mhard-float"] if src in HARD_FLOAT_CXX else [])
        compile_cc(src, obj, "gcc-2.8.1-psx", "cc1plus", flags, maspsx=not gas, expand_div=gas)
    elif src.endswith(".c"):
        compile_cc(src, obj, "gcc-2.95.2-psx", "cc1", CC1_FLAGS, maspsx=False)
    else:
        run([CROSS + "as"] + AS_FLAGS + ["-o", obj, src])


def build(unit, pool):
    list(pool.map(assemble, objects(unit)))
    elf = "%s/%s.elf" % (BUILD, unit)
    out = "%s/%s" % (BUILD, OUTPUT.get(unit, unit + ".bin"))
    script, libs = "%s/%s.ld" % (BUILD, unit), []
    members = lib_members(unit)
    if members:
        psyq.build_archives([m for m in members if m[0] != "libgcc"])
        libgcc.build_archive([name for lib, name in members if lib == "libgcc"])
        script = psyq_script(unit, members)
        libs = ["--whole-archive"] + sorted({"build/psyq/%s.a" % lib for lib, _ in members}) + ["--no-whole-archive"]
    if unit in BSS_VRAM or unit in CXX_NAMES:
        script = cxx_script(unit, script)
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
