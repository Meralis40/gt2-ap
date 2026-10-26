#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Downloads the C++ compiler used by the matching build into tools/toolchain/.

- GCC 2.8.1 rebuilt for the PlayStation (decompals/old-gcc, mips-sony-psx target),
  the counterpart of Sony's GCC 2.8.x (PsyQ 4.3, 4.4), which built the game.
- maspsx, which rewrites the compiler output the way Sony's ASPSX assembled it,
  before GNU as. Same commit as decomp.me's gcc2.8.1-psx, with one fix
  (patch_maspsx).
- The GCC 2.8.1 sources of the libgcc objects the EXE links (tools/libgcc.py,
  docs/runtime-libs.md), extracted to gcc-2.8.1-src/.

All are pinned by SHA-256. See docs/decomp.md.
"""
import hashlib
import io
import os
import shutil
import stat
import sys
import tarfile
import urllib.request
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # decomp/
DEST = os.path.join(ROOT, "tools", "toolchain")
GCC_URL = "https://github.com/decompals/old-gcc/releases/download/0.17/gcc-2.8.1-psx.tar.gz"
GCC_SHA256 = "f6f6e883942d4d3289d048236c672e71ed410e546aaae8ff655952f1567e1be0"
MASPSX_COMMIT = "4ffa8b5106f9780d5d4aa7609f184949433ddbbc"
MASPSX_URL = "https://github.com/mkst/maspsx/archive/%s.zip" % MASPSX_COMMIT
MASPSX_SHA256 = "ef60294812d9e4bf6f97ccac973102ebedcc233b8afbccab770be6eac104713c"
GCC_SRC_URL = "https://ftp.gnu.org/gnu/gcc/gcc-2.8.1.tar.gz"
GCC_SRC_SHA256 = "3b30fbfdf93e628373d90d174243f3267b0eec9ebe792bb64fd15b8828c2ea4c"
# The files tools/libgcc.py compiles and includes, under gcc-2.8.1/.
GCC_SRC_FILES = [
    "COPYING", "libgcc2.c", "longlong.h", "gbl-ctors.h", "defaults.h", "gansidecl.h",
    "machmode.h", "machmode.def", "config/mips/mips.h", "config/mips/xm-mips.h",
    "cp/tinfo.cc", "cp/tinfo.h", "cp/inc/exception", "cp/inc/new", "cp/inc/typeinfo",
]


def fetch(url, sha256):
    print("downloading", url)
    with urllib.request.urlopen(url) as r:
        data = r.read()
    got = hashlib.sha256(data).hexdigest()
    if got != sha256:
        sys.exit("SHA-256 mismatch for %s: %s" % (url, got))
    return data


def patch_maspsx(path):
    # After a div/divu, maspsx puts a nop before an instruction that reads the
    # quotient, looking past a label to find it. The game has none in
    # Rgb555_ToHsv (OVL3 0x80011dec), where the mflo is followed by a label and
    # a beqz on its register, so this stops maspsx looking past the label.
    # Sony's ASPSX 2.79 and 2.86 do add the nop, so this probably stands in
    # for a source difference before that branch and may be dropped.
    with open(path) as fh:
        src = fh.read()
    old = """                next_instruction = self.get_next_instruction(
                    skip=0, ignore_set=True, ignore_label=True
                )
                extra_nops = self._handle_nop_before_next_instruction("""
    if src.count(old) != 2:
        sys.exit("maspsx: unexpected source, can't patch %s" % path)
    with open(path, "w") as fh:
        fh.write(src.replace(old, old.replace("ignore_label=True", "ignore_label=False")))


def main():
    gcc_dir = os.path.join(DEST, "gcc-2.8.1-psx")
    maspsx_dir = os.path.join(DEST, "maspsx")
    shutil.rmtree(DEST, ignore_errors=True)
    os.makedirs(gcc_dir)

    with tarfile.open(fileobj=io.BytesIO(fetch(GCC_URL, GCC_SHA256))) as tar:
        tar.extractall(gcc_dir, filter="data")
    for name in os.listdir(gcc_dir):
        p = os.path.join(gcc_dir, name)
        os.chmod(p, os.stat(p).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    with zipfile.ZipFile(io.BytesIO(fetch(MASPSX_URL, MASPSX_SHA256))) as z:
        z.extractall(DEST)
    os.rename(os.path.join(DEST, "maspsx-" + MASPSX_COMMIT), maspsx_dir)
    patch_maspsx(os.path.join(maspsx_dir, "maspsx", "__init__.py"))

    src_dir = os.path.join(DEST, "gcc-2.8.1-src")
    with tarfile.open(fileobj=io.BytesIO(fetch(GCC_SRC_URL, GCC_SRC_SHA256))) as tar:
        for name in GCC_SRC_FILES:
            member = tar.getmember("gcc-2.8.1/" + name)
            member.name = name
            tar.extract(member, src_dir, filter="data")
    print("toolchain ready in", os.path.relpath(DEST, ROOT))


if __name__ == "__main__":
    main()
