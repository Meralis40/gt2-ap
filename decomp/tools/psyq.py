#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Turns members of Sony's PsyQ libraries into ELF archives for GNU ld.
See docs/psyq-libs.md.

The game links 63 members of the PsyQ 4.6 libraries (libpad from 4.7). This
reads them from the .LIB files in decomp/psyq/<version>/, converts each one from
SN Systems' LNK format to an ELF relocatable object, and writes one archive per
library: build/psyq/<lib>.a, holding <member>.o.

    python3 tools/psyq.py LIBCD:BIOS_1 ...   # convert members (build.py does this)
"""
import os
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # decomp/
SDK = os.path.join(ROOT, "psyq")
OUT = os.path.join(ROOT, "build", "psyq")
CROSS = os.environ.get("CROSS", "mipsel-linux-gnu-")

# The libraries the game uses: SDK version (the folder under decomp/psyq/) and
# SHA-1 of the .LIB file.
LIBS = {
    "libapi": ("4.6", "38d8cb98d26da3c7028e20a9e122d9945be7cdfb"),
    "libc": ("4.6", "720ef685dd1cb726274f1915f3df7e7d7c03e786"),
    "libgte": ("4.6", "909ec172f2627c7ec2f35aeb584a42a1377785fa"),
    "libetc": ("4.6", "87d42ae9c7317aac96245d0d664d446921ea918e"),
    "libcd": ("4.6", "d5624294464b067b6bd1e0c29109f16e84468534"),
    "libcard": ("4.6", "e62ec20500d18c85015b5d066338e5c1bb7e3654"),
    "libpress": ("4.6", "7cc56853a6627b7278512197c9587eca1fa59ef0"),
    "libpad": ("4.7", "aa8cf942a08c3ce963ec155ea13cb2857c0f9a18"),
}

# Bytes where the game's members differ from the SDK's: (lib, member, section,
# offset, SDK bytes, game bytes). The game's libpad has 4.7's code but the
# version stamp 0x2046 (4.6 has 0x0046, 4.7 0x0047).
PATCHES = [
    ("libpad", "PDRESRES", ".data", 6, b"\x47\x00", b"\x46\x20"),
]


def read_lib(path):
    """Returns {member name: LNK bytes} of a .LIB archive."""
    with open(path, "rb") as fh:
        d = fh.read()
    if d[:4] != b"LIB\x01":
        sys.exit("%s: not a PsyQ library" % path)
    members = {}
    p = 4
    while p < len(d):
        # name (8, space padded), date, header size, total size, then the
        # exported names (length-prefixed, ending with 0) and the LNK data.
        name = d[p:p + 8].decode("ascii").strip()
        hdr, total = struct.unpack_from("<II", d, p + 12)
        members[name] = d[p + hdr:p + total]
        p += total
    return members


class Lnk:
    """A parsed LNK object: sections, symbols and relocations."""

    def __init__(self, data, name):
        self.name = name
        self.sections = {}  # LNK id -> {name, align, data}
        self.order = []     # section ids in order of definition
        self.symbols = {}   # LNK index -> {kind: def|ref|common, name, section, offset, size}
        self.locals = []    # (section id, offset, name)
        self.relocs = []    # (section id, offset, type, expression)
        self._parse(data)

    def _parse(self, d):
        if d[:4] != b"LNK\x02":
            sys.exit("%s: not a LNK object" % self.name)
        p = 4
        cur = None
        ptr = {}  # section id -> start of the last code block, which relocation offsets count from

        def rd(fmt):
            nonlocal p
            v = struct.unpack_from("<" + fmt, d, p)
            p += struct.calcsize("<" + fmt)
            return v if len(v) > 1 else v[0]

        def rstr():
            nonlocal p
            n = d[p]
            s = d[p + 1:p + 1 + n].decode("latin1")
            p += 1 + n
            return s

        def expr():
            op = rd("B")
            if op == 0:
                return ("val", rd("I"))
            if op == 2:
                return ("sym", rd("H"))
            if op == 4:
                return ("sec", rd("H"))
            if op == 44:  # add: right operand first
                r = expr()
                return ("add", expr(), r)
            sys.exit("%s: unsupported expression opcode %d" % (self.name, op))

        while True:
            op = rd("B")
            if op == 0:  # end
                break
            elif op == 2:  # code
                n = rd("H")
                s = self.sections[cur]["data"]
                ptr[cur] = len(s)
                s += d[p:p + n]
                p += n
            elif op == 6:  # switch section
                cur = rd("H")
            elif op == 8:  # uninitialized data
                self.sections[cur]["data"] += bytes(rd("I"))
            elif op == 10:  # relocation
                t, off = rd("BH")
                self.relocs.append((cur, ptr.get(cur, 0) + off, t, expr()))
            elif op == 12:  # exported symbol
                i, sec, off = rd("HHI")
                self.symbols[i] = dict(kind="def", name=rstr(), section=sec, offset=off)
            elif op == 14:  # imported symbol
                i = rd("H")
                self.symbols[i] = dict(kind="ref", name=rstr())
            elif op == 16:  # section
                i, group, align = rd("HHB")
                self.sections[i] = dict(name=rstr(), align=align, data=bytearray())
                self.order.append(i)
            elif op == 18:  # local symbol
                sec, off = rd("HI")
                self.locals.append((sec, off, rstr()))
            elif op == 28:  # file name
                rd("H")
                rstr()
            elif op == 46:  # processor type
                rd("B")
            elif op == 48:  # uninitialized symbol (XBSS): allocated by the linker
                i, sec, size = rd("HHI")
                self.symbols[i] = dict(kind="common", name=rstr(), size=size)
            elif op in (50, 60):  # debug line info
                rd("H")
            elif op == 52:
                rd("HB")
            elif op == 54:
                rd("HH")
            elif op == 56:
                rd("HI")
            elif op == 58:
                rd("HIH")
            elif op == 74:  # function start
                rd("HIHIHIHII")
                rstr()
            elif op in (76, 78, 80):  # function end, block start/end
                rd("HII")
            elif op == 82:  # def
                rd("HIHHI")
                rstr()
            elif op == 84:  # def2
                rd("HIHHI")
                for _ in range(rd("H")):
                    rd("H")
                rstr()
                rstr()
            else:
                sys.exit("%s: unsupported LNK opcode %d at %#x" % (self.name, op, p - 1))

    def section(self, name):
        for i in self.order:
            if self.sections[i]["name"] == name:
                return self.sections[i]
        return None


# LNK relocation types and their ELF ones
R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16 = 2, 4, 5, 6
RELOC = {16: R_MIPS_32, 74: R_MIPS_26, 82: R_MIPS_HI16, 84: R_MIPS_LO16}


def flatten(e, name):
    """sym+k or sec+k -> (kind, id, k)."""
    if e[0] == "val":
        return None, None, e[1]
    if e[0] in ("sym", "sec"):
        return e[0], e[1], 0
    a, b = flatten(e[1], name), flatten(e[2], name)
    if a[0] and b[0]:
        sys.exit("%s: unsupported relocation expression %r" % (name, e))
    return (a[0] or b[0]), (a[1] if a[0] else b[1]), (a[2] + b[2]) & 0xFFFFFFFF


def to_elf(lnk):
    """ELF32 little-endian MIPS relocatable object of a LNK object.

    Sections keep their LNK names and alignment. Relocations are REL, with the
    addend in the instruction. An uninitialized symbol becomes a COMMON symbol,
    so a definition elsewhere takes its place.
    """
    def targets(e):
        return {e[1]} if e[0] == "sec" else targets(e[1]) | targets(e[2]) if e[0] == "add" else set()
    used = {r[0] for r in lnk.relocs} | {i for r in lnk.relocs for i in targets(r[3])}
    secs = [i for i in lnk.order if lnk.sections[i]["data"] or i in used]
    secs += [i for i in lnk.order if i not in secs and any(
        s.get("section") == i for s in lnk.symbols.values() if s["kind"] == "def")]
    shndx = {sid: n + 1 for n, sid in enumerate(secs)}
    data = {sid: bytearray(lnk.sections[sid]["data"]) for sid in secs}
    nobits = {sid for sid in secs if lnk.sections[sid]["name"] in (".bss", ".sbss")}

    strtab = bytearray(b"\0")

    def string(s):
        off = len(strtab)
        strtab.extend(s.encode("latin1") + b"\0")
        return off

    # symbols: null, section symbols, locals, then globals
    syms = [(0, 0, 0, 0, 0, 0)]  # name, value, size, info, other, shndx
    secsym = {}
    for sid in secs:
        secsym[sid] = len(syms)
        syms.append((0, 0, 0, 3, 0, shndx[sid]))  # STB_LOCAL, STT_SECTION
    for sec, off, name in lnk.locals:
        if sec in shndx:
            syms.append((string(name), off, 0, 0, 0, shndx[sec]))  # STB_LOCAL, STT_NOTYPE
    first_global = len(syms)
    symidx = {}
    for i in sorted(lnk.symbols):
        s = lnk.symbols[i]
        symidx[i] = len(syms)
        if s["kind"] == "def":
            typ = 2 if lnk.sections[s["section"]]["name"] == ".text" else 1  # STT_FUNC, STT_OBJECT
            syms.append((string(s["name"]), s["offset"], 0, 0x10 | typ, 0, shndx[s["section"]]))
        elif s["kind"] == "ref":
            syms.append((string(s["name"]), 0, 0, 0x10, 0, 0))  # SHN_UNDEF
        else:
            align = 8 if s["size"] >= 8 else 4 if s["size"] >= 4 else 2 if s["size"] >= 2 else 1
            syms.append((string(s["name"]), align, s["size"], 0x11, 0, 0xFFF2))  # SHN_COMMON

    # relocations, with the addend written into the field
    rels = {sid: [] for sid in secs}
    for sid, off, t, e in lnk.relocs:
        if t not in RELOC:
            sys.exit("%s: unsupported relocation type %d" % (lnk.name, t))
        kind, ident, k = flatten(e, lnk.name)
        if kind is None:
            sys.exit("%s: relocation without a symbol" % lnk.name)
        sym = secsym[ident] if kind == "sec" else symidx[ident]
        w = struct.unpack_from("<I", data[sid], off)[0]
        if t == 16:
            w = k
        elif t == 74:
            w = (w & 0xFC000000) | ((k >> 2) & 0x03FFFFFF)
        elif t == 82:
            w = (w & 0xFFFF0000) | (((k + 0x8000) >> 16) & 0xFFFF)
        else:
            w = (w & 0xFFFF0000) | (k & 0xFFFF)
        struct.pack_into("<I", data[sid], off, w)
        rels[sid].append((off, RELOC[t], sym, k))
    for sid in secs:
        rels[sid] = order_hi16(rels[sid], lnk.name)

    # file layout: header, section contents, tables, section headers
    shstr = bytearray(b"\0")

    def shname(s):
        off = len(shstr)
        shstr.extend(s.encode() + b"\0")
        return off

    body = bytearray()
    headers = [(0,) * 10]

    def add(name, typ, flags, content, link=0, info=0, align=1, entsize=0, size=None):
        while (52 + len(body)) % max(align, 4):
            body.append(0)
        off = 52 + len(body)
        if typ != 8:  # SHT_NOBITS
            body.extend(content)
        headers.append((shname(name), typ, flags, 0, off, len(content) if size is None else size,
                        link, info, align, entsize))
        return len(headers) - 1

    flags = {".text": 6, ".rdata": 2, ".data": 3, ".sdata": 0x10000003, ".bss": 3, ".sbss": 0x10000003}
    for sid in secs:
        s = lnk.sections[sid]
        add(s["name"], 8 if sid in nobits else 1, flags.get(s["name"], 3), data[sid],
            align=s["align"] or 1, size=len(data[sid]))
    symtab_index = len(secs) + 1 + sum(1 for sid in secs if rels[sid])
    for sid in secs:
        if rels[sid]:
            content = b"".join(struct.pack("<II", off, (sym << 8) | typ) for off, typ, sym, _ in rels[sid])
            add(".rel" + lnk.sections[sid]["name"], 9, 0x40, content, link=symtab_index,
                info=shndx[sid], align=4, entsize=8)
    symtab = b"".join(struct.pack("<IIIBBH", *s) for s in syms)
    add(".symtab", 2, 0, symtab, link=symtab_index + 1, info=first_global, align=4, entsize=16)
    add(".strtab", 3, 0, strtab)
    shstrndx = len(headers)
    shname(".shstrtab")
    headers.append((len(shstr) - len(".shstrtab") - 1, 3, 0, 0, 52 + len(body), len(shstr), 0, 0, 1, 0))
    body.extend(shstr)
    while len(body) % 4:
        body.append(0)
    shoff = 52 + len(body)
    ehdr = struct.pack("<4sBBBB8sHHIIIIIHHHHHH", b"\x7fELF", 1, 1, 1, 0, bytes(8),
                       1, 8, 1, 0, 0, shoff, 0x1000, 52, 0, 0, 40, len(headers), shstrndx)
    return ehdr + body + b"".join(struct.pack("<10I", *h) for h in headers)


def order_hi16(rels, name):
    """Orders relocations so that GNU ld pairs each HI16 with a LO16 of the same
    expression: ld takes the addend of a HI16 from the next LO16 against the
    same symbol, but LNK relocations each carry their own expression."""
    lo = [r for r in rels if r[1] == R_MIPS_LO16]
    out, pending = [], {}
    for r in rels:
        if r[1] == R_MIPS_HI16:
            match = next((l for l in lo if (l[2], l[3]) == (r[2], r[3]) and l[0] > r[0]), None)
            match = match or next((l for l in lo if (l[2], l[3]) == (r[2], r[3])), None)
            if match is None:
                sys.exit("%s: HI16 at %#x has no LO16 with the same target" % (name, r[0]))
            pending.setdefault(match, []).append(r)
        else:
            out.append(r)
    # each LO16 is preceded by the HI16s that pair with it
    result = []
    for r in out:
        result.extend(pending.pop(r, []))
        result.append(r)
    return result


def sha1(path):
    import hashlib
    with open(path, "rb") as fh:
        return hashlib.sha1(fh.read()).hexdigest()


def lib_path(lib):
    version, _ = LIBS[lib]
    d = os.path.join(SDK, version)
    for f in os.listdir(d) if os.path.isdir(d) else []:
        if f.upper() == lib.upper() + ".LIB":
            return os.path.join(d, f)
    return None


def build_archives(members):
    """members: [(lib, member)] -> writes build/psyq/<lib>.a for each lib."""
    by_lib = {}
    for lib, member in members:
        by_lib.setdefault(lib, []).append(member)
    for lib, names in by_lib.items():
        if lib not in LIBS:
            sys.exit("unknown PsyQ library %s (see LIBS in tools/psyq.py)" % lib)
        path = lib_path(lib)
        if path is None:
            sys.exit("missing %s.LIB from PsyQ %s: copy it to %s (docs/psyq-libs.md)"
                     % (lib.upper(), LIBS[lib][0], os.path.relpath(os.path.join(SDK, LIBS[lib][0]), ROOT)))
        if sha1(path) != LIBS[lib][1]:
            sys.exit("%s: SHA-1 mismatch, expected PsyQ %s's (%s)" % (path, LIBS[lib][0], LIBS[lib][1]))
        lnk = read_lib(path)
        d = os.path.join(OUT, lib)
        os.makedirs(d, exist_ok=True)
        objs = []
        for name in names:
            if name not in lnk:
                sys.exit("%s has no member %s" % (path, name))
            obj = Lnk(lnk[name], "%s:%s" % (lib, name))
            for plib, pname, psec, off, old, new in PATCHES:
                if (plib, pname) == (lib, name):
                    data = obj.section(psec)["data"]
                    if data[off:off + len(old)] != old:
                        sys.exit("%s:%s: unexpected bytes at %s+%#x" % (lib, name, psec, off))
                    data[off:off + len(new)] = new
            out = os.path.join(d, name + ".o")
            with open(out, "wb") as fh:
                fh.write(to_elf(obj))
            objs.append(out)
        archive = os.path.join(OUT, lib + ".a")
        if os.path.exists(archive):
            os.remove(archive)
        subprocess.run([CROSS + "ar", "rcs", archive] + objs, check=True)


if __name__ == "__main__":
    members = []
    for a in sys.argv[1:]:
        if ":" not in a:
            sys.exit("expected LIB:MEMBER, got %s" % a)
        lib, member = a.split(":", 1)
        members.append((lib.lower(), member.upper()))
    build_archives(members)
