#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Builds the GCC 2.8.1 libgcc objects that the EXE links. See docs/runtime-libs.md.

The game's C++ runtime and __divdi3 are GCC 2.8.1's own libgcc, built with
-fexceptions. This compiles the six objects from the GNU sources, which
tools/setup_toolchain.py extracts to tools/toolchain/gcc-2.8.1-src/, with the
build's cc1 and cc1plus, and writes build/psyq/libgcc.a, holding <object>.o
(the yaml's lib_path, shared with the PsyQ archives).

    python3 tools/libgcc.py _divdi3 tinfo ...   # build objects (build.py does this)
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # decomp/
TOOLCHAIN = os.path.join(ROOT, "tools", "toolchain")
SRC = os.path.join(TOOLCHAIN, "gcc-2.8.1-src")
CONFIG = os.path.join(ROOT, "tools", "libgcc_config")  # tconfig.h, tm.h, stddef.h
OUT = os.path.join(ROOT, "build", "psyq")
CROSS = os.environ.get("CROSS", "mipsel-linux-gnu-")
AS_FLAGS = ["-EL", "-march=r3000", "-mtune=r3000", "-mabi=32", "-G0", "-no-pad-sections"]

# The objects: source file and its defines. libgcc2.c builds one object per
# L_<name>, as GCC's Makefile does for the archive member <name>.
OBJECTS = {
    "_exit": ("libgcc2.c", ["-DL_exit",
                            # The original compiler's configuration: no libc
                            # headers and an empty EXIT_BODY, so exit calls
                            # neither __bb_exit_func nor _cleanup.
                            "-Dinhibit_libc", "-DEXIT_BODY="]),
    "__main": ("libgcc2.c", ["-DL__main"]),
    "_ctors": ("libgcc2.c", ["-DL_ctors"]),
    "_op_delete": ("libgcc2.c", ["-DL_op_delete"]),
    "_divdi3": ("libgcc2.c", ["-DL_divdi3"]),
    "tinfo": ("cp/tinfo.cc", []),
}

# The GCC 2.8 driver's predefines for a little-endian MIPS target, and
# LIBGCC2_CFLAGS from GCC's Makefile.in (-g1 doesn't change the code).
CPP_FLAGS = ["-nostdinc", "-undef", "-D__GNUC__=2", "-D__GNUC_MINOR__=8", "-Dmips", "-D__mips__",
             "-D__mips", "-DMIPSEL", "-D__MIPSEL__", "-D_MIPSEL", "-D__CHAR_UNSIGNED__",
             "-D__OPTIMIZE__", "-DIN_LIBGCC2", "-D__GCC_FLOAT_NOT_NEEDED",
             "-I" + CONFIG, "-I" + SRC, "-I" + os.path.join(SRC, "config"),
             "-I" + os.path.join(SRC, "cp", "inc")]
CPP_FLAGS_C = ["-D__LANGUAGE_C__"]
CPP_FLAGS_CXX = ["-x", "c++", "-D__cplusplus", "-D__GNUG__=2", "-D__LANGUAGE_C_PLUS_PLUS__"]
CC1_FLAGS = ["-quiet", "-O2", "-G0", "-fexceptions", "-g0"]


def pipe(cmd, data):
    r = subprocess.run(cmd, cwd=ROOT, input=data, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if r.returncode != 0:
        sys.exit("command failed: %s\n%s" % (" ".join(cmd), r.stderr.decode(errors="replace")))
    return r.stdout


def gas_source(asm):
    """Turns cc1's output into what GNU as assembles to the game's bytes.

    The original was assembled by a GNU as of its time, not through ASPSX:
    __divdi3 has GNU as's divide-by-zero checks (bnez, divu, break 7), which
    the game's own code lacks. Two things changed since: modern GNU as encodes
    move as or where the old one used addu, and it rewrites the CFA advances in
    .eh_frame. The original compiler had no named sections either, so GCC wrote
    the unwind tables to .data and ended them with a zero word (dwarf2out.c).
    """
    asm = re.sub(rb"(?m)^(\s+)move\s+(\$\w+),(\$\w+)\s*$", rb"\1addu\t\2,\3,$0", asm)
    asm = re.sub(rb"(?m)^\s+\.section\s+\.eh_frame\b.*$", b"\t.data", asm)
    if b"__FRAME_BEGIN__" in asm:
        asm += b"\t.data\n\t.word\t0\n"
    return asm


def compile_object(name):
    src, defines = OBJECTS[name]
    cxx = src.endswith(".cc")
    pre = pipe(["cpp"] + CPP_FLAGS + (CPP_FLAGS_CXX if cxx else CPP_FLAGS_C) + defines
               + [os.path.join(SRC, src)], None)
    pre = re.sub(rb"(?m)^# 0", b"# 1", pre)  # cc1 rejects line 0 markers
    cc1 = os.path.join(TOOLCHAIN, "gcc-2.8.1-psx", "cc1plus" if cxx else "cc1")
    asm = gas_source(pipe([cc1] + CC1_FLAGS + ["-o", "-"], pre))
    base = os.path.join(OUT, "libgcc", name)
    with open(base + ".s", "wb") as fh:
        fh.write(asm)  # kept for diffing
    pipe([CROSS + "as"] + AS_FLAGS + ["-o", base + ".o", base + ".s"], None)
    return base + ".o"


def build_archive(names):
    """names: libgcc objects -> writes build/psyq/libgcc.a."""
    if not names:
        return
    if not os.path.exists(os.path.join(SRC, "libgcc2.c")):
        sys.exit("missing GCC 2.8.1 source: run tools/setup_toolchain.py first")
    for name in names:
        if name not in OBJECTS:
            sys.exit("unknown libgcc object %s (see OBJECTS in tools/libgcc.py)" % name)
    os.makedirs(os.path.join(OUT, "libgcc"), exist_ok=True)
    objs = [compile_object(name) for name in names]
    archive = os.path.join(OUT, "libgcc.a")
    if os.path.exists(archive):
        os.remove(archive)
    subprocess.run([CROSS + "ar", "rcs", archive] + objs, check=True)


if __name__ == "__main__":
    build_archive(sys.argv[1:] or list(OBJECTS))
