# C and C++ runtime

Where the libc, libgcc and C++ runtime code and data are in the Simulation disc executable, where they come from, and how the build links them instead of disassembling them. The PsyQ libraries are in [psyq-libs.md](psyq-libs.md).

## Summary

- **libc** is not PsyQ's, apart from five BIOS stubs. `memset`, `strcpy`, `sprintf`, `malloc`, `free`, `InitHeap` and the rest (`syslib`, `0x8008ce08–0x8008dfc4`) are the game's own C code, compiled with GCC 2.95.2 (newer than the game's compiler), assembled with GNU as, and no frame info. No version of LIBC2 or LIBC comes close. They are decompiled, not linked: `src/sim/exe/syslib.c` ([decomp.md](decomp.md#c-sources)).
- **The C++ runtime and the libgcc helpers** (`0x80085974–0x80086658`) are GCC 2.8.1's own `libgcc`, built with `-fexceptions`: `exit`, `__main` and the constructor lists, RTTI (`tinfo.cc`), `__builtin_delete` and `__divdi3`. They are not in any PsyQ library. They rebuild **byte for byte** from the GCC 2.8.1 sources with the compiler the build already uses (`tools/toolchain/gcc-2.8.1-psx`); a GNU ld link at their addresses gives the same `.text`, `.rodata` and `.data` as the EXE. The matching build compiles and links them ([In the build](#in-the-build)).
- Six objects, 0xce4 bytes of code, 0x1b0 of `.rodata`, 0x458 of `.data`, plus seven BSS symbols.
- Nothing else from libgcc is linked: there is no soft float (`__addsf3`, `__fixsfsi`, …), no other 64-bit helper (`__muldi3`, `__udivdi3`, `__ashldi3`, `__cmpdi2`, …), no `__builtin_new`, `__builtin_vec_new`, `__builtin_vec_delete`, `__pure_virtual`, `__dynamic_cast` or exception handling (`__throw`, `__eh_*`).
- The overlays have none of this code; they call the resident copies (`__divdi3` from 41 places, `__builtin_delete` from OVL0).

## Method

1. **PsyQ.** Every `.text` member of LIBSN, LIBMATH, LIBC and LIBC2 from the 4.6 and 4.7 SDKs (`Psy-Q_46.zip`, `Psy-Q_47.zip`, as in [psyq-libs.md](psyq-libs.md#method)) was searched for in the EXE and all six overlays, with the fields its relocations cover masked, whole and function by function. 4.7's `LIBSN.LIB` is in a newer archive format (`LIB` version 2, directory at the end), which needed its own reader. The signature database's LIBSN for 2.6 to 4.6 was compared too.
2. **GCC.** The runtime's unwind tables (below) give its object boundaries. Each object was compiled from the GCC 2.8.1 sources (`gcc-2.8.1.tar.gz` from [ftp.gnu.org](https://ftp.gnu.org/gnu/gcc/), 8,447,495 bytes, SHA-256 `3b30fbfdf93e628373d90d174243f3267b0eec9ebe792bb64fd15b8828c2ea4c`) with libgcc's own flags, compared with relocations masked, then linked with `mipsel-linux-gnu-ld` 2.42 at the EXE's addresses and compared unmasked.

## Where it comes from

| Code | Candidate | Result |
|---|---|---|
| `bcopy`, `strncmp`, `bzero`, `printf`, `puts` | PsyQ LIBC | 4.6 (`A63.OBJ` has the 4.6 stamp). Already linked; see [psyq-libs.md](psyq-libs.md) |
| `syslib` | PsyQ LIBC2, LIBC (2.6–4.7) | No match, and no near match |
| `__divdi3` | PsyQ LIBSN | No. LIBSN 3.3–4.6's `_DIVDI3.OBJ` is 0x98 bytes and calls `__udivmoddi4`; 4.7's `divdi3` is 0x5f4 bytes. The EXE's is 0x5d4, with `__udivmoddi4` inlined |
| `__main`, `exit`, RTTI | PsyQ LIBSN | No. LIBSN has its own startup (`SNMAIN.OBJ`, `__SN_ENTRY_POINT`) and constructor handling (`snctors`); neither is in the EXE |
| `__builtin_delete` | PsyQ LIBSN | The code is the same as 4.7's `op_delete` (36 bytes, `if (p) free (p)`), but 4.6's `_OP_DELE.OBJ` differs, and the object's unwind table is GCC's |
| everything from `exit` to `__divdi3` | GCC 2.8.1 `libgcc2.c` and `cp/tinfo.cc` | **Byte for byte**, built with the build's `cc1`/`cc1plus` |
| floating point | PsyQ LIBMATH | Not in any unit |

The decomp.me `gcc2.8.1-psx` toolchain ([decompals/old-gcc](https://github.com/decompals/old-gcc)) ships `cc1`, `cc1plus`, `cpp`, `gcc` and `g++`, but no `libgcc`. Its compilers rebuild the game's libgcc from the GNU sources, so the original was built from the same GCC 2.8.1 sources. Which compiler distribution built it isn't known: it is not in the 4.6 or 4.7 SDK, nor in the signature database.

## Unwind tables

GCC 2.8.1 built libgcc with `-fexceptions` (`LIBGCC2_CFLAGS` in its `Makefile.in`), which makes it emit DWARF 2 unwind tables. On a target without named sections there is no `.eh_frame`, so `dwarf2out.c` writes them to `.data`, after the object's own data, and ends them with a zero word. Each object then has, in `.data`, a 20-byte CIE (`10 00 00 00 00 00 00 00 01 00 01 7c 40 0c 1d 00 09 40 1f 00`: version 1, code alignment 1, data alignment −4, return address column 64), one FDE per function with its address and size, and a zero word.

The EXE has six of them, all at `0x800a70cc–0x800a7524`, right before the PsyQ `.data`, and none anywhere else: the game's own code was compiled without `-fexceptions`. Nothing reads them (no `__register_frame_info`, and the constructor list is empty).

## Objects

In link order. Addresses are where each section starts; sizes are the object's section sizes. The `.data` sizes include the unwind table and its zero word.

| Object | Source | `.text` | `.rodata` | `.data` | Functions (address, size) |
|---|---|---|---|---|---|
| `_exit` | `libgcc2.c` `-DL_exit` | `80085974` 0x1c | | `800a70cc` 0x44 | `exit` 80085974 0x1c |
| `__main` | `libgcc2.c` `-DL__main` | `80085990` 0x138 | | `800a7110` 0xa0 | `__do_global_dtors` 80085990 0x6c, `__do_global_ctors` 800859fc 0x9c, `__main` 80085a98 0x30 |
| `tinfo` | `cp/tinfo.cc` | `80085ac8` 0x598 | `80090a24` 0xaf | `800a71b0` 0x2f8 | see below |
| `_op_delete` | `libgcc2.c` `-DL_op_delete` | `80086060` 0x24 | | `800a74a8` 0x3c | `__builtin_delete` 80086060 0x24 |
| `_divdi3` | `libgcc2.c` `-DL_divdi3` | `80086084` 0x5d4 | `80090ad4` 0x100 | `800a74e4` 0x28 | `__divdi3` 80086084 0x5d4 (with `__udivmoddi4` inlined) |
| `_ctors` | `libgcc2.c` `-DL_ctors` | | | `800a750c` 0x18 | none: only the CIE and the two lists, as commons |

`tinfo`'s functions, by their mangled names (Ghidra's in brackets where they differ):

| Address | Size | Name |
|---|---|---|
| `80085ac8` | 0x34 | `_._9type_info` (`type_info::~type_info`) |
| `80085afc` | 0x40 | `__rtti_class` (unnamed in Ghidra) |
| `80085b3c` | 0x3c | `__rtti_si` |
| `80085b78` | 0x38 | `__rtti_user` |
| `80085bb0` | 0x14 | `dcast__C16__user_type_infoRC9type_infoiPvPC9type_infoT3` (`__user_type_info::dcast`) |
| `80085bc4` | 0x60 | `dcast__C14__si_type_infoRC9type_infoiPvPC9type_infoT3` (`__si_type_info::dcast`) |
| `80085c24` | 0x1e0 | `dcast__C17__class_type_infoRC9type_infoiPvPC9type_infoT3` (`__class_type_info::dcast`) |
| `80085e04` | 0x34 | `__17__class_type_infoPCcPCQ217__class_type_info9base_infoUi` (constructor) |
| `80085e38` | 0x50 | `__tf17__class_type_info` |
| `80085e88` | 0x20 | `_._17__class_type_info` |
| `80085ea8` | 0x30 | `__14__si_type_infoPCcRC16__user_type_info` (constructor) |
| `80085ed8` | 0x50 | `__tf14__si_type_info` |
| `80085f28` | 0x20 | `_._14__si_type_info` |
| `80085f48` | 0x2c | `__16__user_type_infoPCc` (constructor) |
| `80085f74` | 0x50 | `__tf16__user_type_info` |
| `80085fc4` | 0x20 | `_._16__user_type_info` |
| `80085fe4` | 0xc | `__ne__C9type_infoRC9type_info` |
| `80085ff0` | 0xc | `__eq__C9type_infoRC9type_info` |
| `80085ffc` | 0xc | `name__C9type_info` |
| `80086008` | 0x18 | `__9type_infoPCc` (constructor) |
| `80086020` | 0x40 | `__tf9type_info` |

`tinfo2.cc` (the `__tf` functions of the built-in types, `__dynamic_cast`, `__throw_type_match_rtti`), `new1.cc`/`new2.cc` and `exception.cc` aren't linked: the game uses none of them.

### Data

| Address | Size | Item | Object |
|---|---|---|---|
| `80090a24` | 0xa | `"bad_alloc"`, from the `new` header | `tinfo` `.rodata` |
| `80090a34` | 0x58 | `_vt.17__class_type_info`, `_vt.14__si_type_info` (`80090a4c`), `_vt.16__user_type_info` (`80090a64`), `_vt.9type_info` (`80090a7c`) | `tinfo` `.rodata` |
| `80090a8c` | 0x47 | the type names `"17__class_type_info"`, `"14__si_type_info"`, `"16__user_type_info"`, `"9type_info"` | `tinfo` `.rodata` |
| `80090ad4` | 0x100 | `__clz_tab` (static) | `_divdi3` `.rodata` |
| `800a7110` | 4 | `p.2`, `__do_global_dtors`' pointer, `= __DTOR_LIST__ + 1` | `__main` `.data` |
| `801c93b0` | 4 | `initialized.7`, `__main`'s flag (local `.bss`) | `__main` |
| `801c9408` | 8 | `__ti9type_info` (common) | `tinfo` |
| `801c9410` | 8 | `__CTOR_LIST__` (common) | `_ctors` |
| `801c9418` | 8 | `__DTOR_LIST__` (common) | `_ctors` |
| `801f0d20` | 0xc | `__ti16__user_type_info` (common) | `tinfo` |
| `801f0d30` | 0xc | `__ti14__si_type_info` (common) | `tinfo` |
| `801f0d40` | 0xc | `__ti17__class_type_info` (common) | `tinfo` |

`initialized` is the first word of the BSS that the startup code clears (`0x801c93b0–0x801f0d60`). `__CTOR_LIST__` and `__DTOR_LIST__` are libgcc's empty two-word lists: nothing filled them in, so the game runs no global constructors or destructors.

The objects import `_exit` (libapi `C58.OBJ`, `8008c988`), `free` (`syslib`, `8008de24`), `__do_global_dtors`, `__builtin_delete` and the two lists; all of them are in the EXE.

## Building them

The objects match when compiled like this (`decomp/tools/libgcc.py` does it):

1. Preprocess with libgcc's defines, `-DIN_LIBGCC2 -D__GCC_FLOAT_NOT_NEEDED -DL_<name>`, and the GCC 2.8 MIPS little-endian macros (`-D__mips__ -D__MIPSEL__ -D__CHAR_UNSIGNED__`, as in `build.py`). Stock headers do: `tconfig.h` includes `mips/xm-mips.h`, `tm.h` includes `mips/mips.h`, and a small `stddef.h` replaces the host's. `tinfo.cc` also needs `-I cp/inc` and the C++ predefines (`-D__cplusplus -D__GNUG__=2`).
2. `exit` also needs `-Dinhibit_libc -DEXIT_BODY=`. Without them it calls `__bb_exit_func` and `_cleanup` too. The original compiler's configuration had no libc headers (`inhibit_libc`, as for a cross compiler) and an empty `EXIT_BODY`.
3. `cc1` (or `cc1plus` for `tinfo.cc`) `-quiet -O2 -G0 -fexceptions -g0`. libgcc used `-g1`, which doesn't change the code.
4. Assemble with GNU as directly, **not** through maspsx, after two rewrites of the compiler's output:
   - `move rd,rs` becomes `addu rd,rs,$0`. Modern GNU as encodes `move` as `or`; the original assembler used `addu`.
   - `.section .eh_frame` becomes `.data`, and a zero word is added after the table, as GCC 2.8.1 writes it without named sections. In `.eh_frame`, modern GNU as also rewrites the CFA advances (`DW_CFA_advance_loc4` to `DW_CFA_advance_loc`).

   The original assembler checked divisions: `__divdi3` has `bnez rt; divu; break 7` after each `divu`, as GNU as expands `divu rd,rs,rt`. maspsx with `--expand-div` gives a longer sequence (0x5f8 bytes) and without it no checks (0x58c bytes). The game's own code has no checks, which is one more sign that libgcc was built separately.
5. Link at the EXE's addresses with each section aligned to 4 (gas gives 16): `.text` in the order above from `0x80085974`, `.rodata` from `0x80090a24`, `.data` from `0x800a70cc`, `__main`'s `.bss` at `0x801c93b0`, and the commons as symbols at their addresses.

## In the build

No original objects exist for them (no PsyQ library has them, and the original `libgcc` isn't available), so the build compiles them from the GNU sources, as above, and links them like the PsyQ members.

- **The source.** `tools/setup_toolchain.py` downloads `gcc-2.8.1.tar.gz`, checks its SHA-256 and extracts the 15 files used (`libgcc2.c`, `longlong.h`, `gbl-ctors.h`, `defaults.h`, `gansidecl.h`, `machmode.h`, `machmode.def`, `config/mips/mips.h`, `config/mips/xm-mips.h`, `cp/tinfo.cc`, `cp/tinfo.h`, `cp/inc/`, and `COPYING`) into `tools/toolchain/gcc-2.8.1-src/` (ignored). After updating, run it again: `build.py` stops if the source is missing.
- **The configuration headers** (`tconfig.h`, `tm.h`, `stddef.h`) are in `decomp/tools/libgcc_config/`.
- **Compiling.** `tools/libgcc.py` builds the objects the yaml lists, with the flags and rewrites above, into `build/psyq/libgcc.a` (the yaml's `lib_path`, shared with the PsyQ archives), holding `_exit.o`, `__main.o`, `tinfo.o`, `_op_delete.o`, `_divdi3.o` and `_ctors.o`. It keeps each object's assembly next to it (`build/psyq/libgcc/<object>.s`). `build.py` runs it on every EXE build.
- **Placement.** `exe.yaml` has a `lib` subsegment for each object's `.text`, `.rodata` and `.data`: `[0x76174, lib, libgcc, _exit, .text]` links `build/psyq/libgcc.a:_exit.o(.text)`. In `build/sim/exe.psyq.ld`, `build.py` adds a `libgcc_bss` NOLOAD section at `0x801c93b0` for `__main`'s `.bss`, and defines the six commons at their addresses (`LIBGCC_BSS` and `LIBGCC_SYMBOLS` in `build.py`). Defined there, the `COMMON` symbols take no space.
- **Names.** The game references five runtime functions: `__main` (from `main`), `__divdi3` (41 calls in the EXE and overlays), `__builtin_delete` (`~ScreenViewLoop`, OVL0's `~RaceMenuLoop`), and `__rtti_si` and `__rtti_user` (the game's `__tf` functions). Their names are the same in Ghidra and in the objects. The objects' other symbols have mangled names (`_._9type_info`, `__tf9type_info`, `__ti9type_info`, `_vt.9type_info`) where `exe.txt` has Ghidra's (`type_info__type_info_dtor`, `type_info__typeinfo`, `type_info__vtable`). Only the runtime itself references them, so the Ghidra names don't reach the link.

## The game's library

`syslib` (`0x8008ce08–0x8008dfc4`) comes before the libc stubs, so `memset`, `memcpy`, `strcpy` and the rest resolved to it and only the five names it doesn't define came from LIBC (see [psyq-libs.md](psyq-libs.md#around-the-libraries)). It is plain C without frame info, built with GCC 2.95.2 `-O2`, signed `char` and GNU as ([decomp.md](decomp.md#c-sources)), where the game used GCC 2.8.1. It has its own startup code and data:

- **Startup**, `start` (`0x8005d600`, 0xe0, the EXE's entry point), the first code in the resident segment and not LIBSN's `SNMAIN.OBJ`. It saves `a0`, `a1`, `s0`–`s8`, `gp`, `sp` and `ra` at `0x8009113c` (the start of the EXE's `.data`), switches to the stack at `0x800a8cb4` if it is set, clears `0x801c93b0–0x801f0d60`, calls `Sys_Init`, then `main` with the saved `a0` and `a1`, then `exit` with `main`'s result, and returns to the caller with the saved registers.
- **Functions** (address, size): `Sys_Init` 8008ce08 0x28, `memset` 8008ce30 0xac, `strcpy` 8008cedc 0x24, `strcmp` 8008cf00 0x34, `sprintf` 8008cf34 0x30, `strncpy` 8008cf64 0x60, `strlen` 8008cfc4 0x1c, `memcpy` 8008cfe0 0x40, `strcat` 8008d020 0x40, `strncasecmp` 8008d060 0x70, `malloc` 8008d0d0 0xb0, `abs` 8008d180 0x14, `memcmp` 8008d194 0x50, `Printf_EmitString` 8008d1e4 0x154, `Printf_EmitNumber` 8008d338 0x258, `Printf_FormatDec64` 8008d590 0x154, `Printf_FormatOct64` 8008d6e4 0xc0, `Printf_FormatHex64` 8008d7a4 0xe4, `Printf_ParseInt` 8008d888 0x4c, `Printf_Format` 8008d8d4 0x4a0, `InitHeap` 8008dd74 0x40, `Heap_InitDefault` 8008ddb4 0x70, `free` 8008de24 0x80, `Printf_PutcToBuffer` 8008dea4 0x20, `vsprintf` 8008dec4 0x40, `Heap_SplitBlock` 8008df04 0x7c, `Heap_MergeNext` 8008df80 0x44.
- **`.rdata`** `0x80090f74–0x8009113c`: the digit strings `"0123456789abcdef"` and `"0123456789ABCDEF"`, and the jump tables of `Printf_Format`'s switches.
- **`.data`** `0x800a8cb4–0x800a8d5c`: the startup stack pointer (`0x800a8cb4`, 0), the powers of ten from 10¹⁹ down to 1 as 64-bit numbers for `Printf_FormatDec64`, and `g_HeapBase`, `g_HeapSize` and `g_StackSize` (0x10000) at `0x800a8d50`. `Heap_InitDefault` gives the heap the space from the end of the BSS to the caller's stack minus `g_StackSize`.

`malloc` is called once, from the boot code (`0x80010060`), and `free` only from `__builtin_delete`. There is no `__builtin_new`, so no linked code uses a plain `new`.
