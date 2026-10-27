# Matching build

The Simulation disc executable (`SCUS_944.88`) and its six uncompressed overlays can be rebuilt byte for byte from the disassembly. This is the base for a matching decompilation: functions are rewritten in C++ one at a time, with each build checked against the originals. So far the OVL3 functions are C++, and so are their copies in other overlays, which build the same sources: `Max3`, `Min3`, `Rgb555_ToHsv`, `RaceCarPick_Random`, `RaceCarPick_SetColor` and `RaceSetup_InitEvent` (also in OVL1, OVL2 and OVL4), `RaceSetup_SetCarFromEntry` and `RaceSetup_SetCarFromGarage` (also in OVL2 and OVL4), `CarWork_Clear`, `CarWork_LoadParts` and `CarWork_SetStagesFromSetup` (twice in OVL4), `Car_StoreSetup` (also in OVL4), and `ArcadeSel_SetupRaceCars`, `ArcadeSel_LoadGarageCar`, `CarWork_LoadGarageCarAlt` and `ArcadeGarageRace_Entry` (OVL3 only). All 55 functions of OVL5 (the movie player) are C++: the player (`Movie_*`), its CD stream ring (`MovieStream_*`), the MDEC output (`MdecOut_*`), the `psxMovieLoop` screen loop and the movie entries. `psxMovieLoop` is a real C++ class, derived from the resident `psxViewLoop` and `ScreenViewLoop`, which `viewloop.h` declares in vtable order. `movieloop.cpp` emits its vtable, type name, `__tf` function and typeinfo, and defines the overlay's buffers in `.bss`. The rest of OVL5's `.rodata` (the VLC table and the display positions) is still the original data. The Sony PsyQ library code in the EXE is linked from the original library objects ([psyq-libs.md](psyq-libs.md)), and the C++ runtime from GCC 2.8.1's `libgcc`, compiled from the GNU sources ([runtime-libs.md](runtime-libs.md)). All 27 functions of the game's own C library (`syslib`) are C, built with GCC 2.95.2 and GNU as: `Sys_Init`, `memset`, `strcpy`, `strcmp`, `sprintf`, `strncpy`, `strlen`, `memcpy`, `strcat`, `strncasecmp`, `malloc`, `abs`, `memcmp`, `Printf_EmitString`, `Printf_EmitNumber`, `Printf_FormatDec64`, `Printf_FormatOct64`, `Printf_FormatHex64`, `Printf_ParseInt`, `Printf_Format`, `InitHeap`, `Heap_InitDefault`, `free`, `Printf_PutcToBuffer`, `vsprintf`, `Heap_SplitBlock` and `Heap_MergeNext`, in `src/sim/exe/syslib.c`, with its `.rodata` (the hex digits and `Printf_Format`'s jump tables) ([C sources](#c-sources)). 233 of the 786 functions of the game's resident code are C++, in 43 files under `src/sim/exe/` that keep the others as `INCLUDE_ASM`: `ScreenViewLoop`, `psxViewLoop` and `psxViewLoopDoubleBuffer` are real classes, and the rest is Ghidra's decompilation as it is ([Resident code](#resident-code)). In the boot code, the splash screen class `VendorBootLogo` and `Boot_Init` are C++ too (`src/sim/exe/bootlogo.cpp`). Everything else is assembly.

The disassembly comes from [splat](https://github.com/ethteck/splat) and uses the names from the Ghidra project ([ghidra-setup.md](ghidra-setup.md)). Only the configs, the symbol files and the build script are in the repo. The original files and everything generated from them stay outside git.

## Requirements

Linux, or WSL on Windows (Ubuntu 24.04 is what this was tested on):

```bash
sudo apt install binutils-mipsel-linux-gnu python3-venv cpp
cd decomp
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
python3 tools/setup_toolchain.py
```

- `requirements.txt` pins splat 0.50.1 (with spimdisasm and rabbitizer).
- The build uses GNU binutils 2.42 (`mipsel-linux-gnu-as`, `ld`, `objcopy`); set `CROSS` to use another prefix.
- `setup_toolchain.py` downloads the compilers (GCC 2.8.1 for the game, GCC 2.95.2 for its C library), and the GCC 2.8.1 sources of the runtime the EXE links, into `tools/toolchain/` (ignored), checking pinned SHA-256s. See [Compiler](#compiler).

## Original files

Copy the executable and the overlays (from `gt2-discinfo extract` and `gt2-discinfo overlays`, see [ghidra-setup.md](ghidra-setup.md#4-ghidra-project)) into `decomp/baserom/sim/`:

| File | SHA-1 |
|---|---|
| `SCUS_944.88` | `3030aa271c0a4022fc69ce09d76a6bc75e69a32a` |
| `ovl0.bin` | `b8e5d9ea443c41ad7206882a4b8deb579c769948` |
| `ovl1.bin` | `06158d43bfd520bf565c92cefaf08b4a6df6532b` |
| `ovl2.bin` | `2c81ba83708d99dc41c1d3688806c5a7c179708b` |
| `ovl3.bin` | `7f7af7658baac06a4f1ab144edd7db8d48c2563c` |
| `ovl4.bin` | `76d3d705ad6e452030f98d15a4a8a3b0f5bdc280` |
| `ovl5.bin` | `c591c1801241b59ba3329bb4508f6c84d6773556` |

The EXE also needs eight PsyQ libraries, in `decomp/psyq/4.6/` (`LIBAPI.LIB`, `LIBC.LIB`, `LIBGTE.LIB`, `LIBETC.LIB`, `LIBCD.LIB`, `LIBCARD.LIB`, `LIBPRESS.LIB`) and `decomp/psyq/4.7/` (`LIBPAD.LIB`). [psyq-libs.md](psyq-libs.md#the-libraries) gives their source and SHA-1s. Like the original files, they stay out of git.

## Building

```bash
cd decomp
.venv/bin/python tools/build.py
```

For each unit (`exe`, `ovl0` … `ovl5`) this runs splat if its output is missing, assembles every file, converts the PsyQ library members the unit uses into `build/psyq/<lib>.a` (`tools/psyq.py`), compiles its libgcc objects into `build/psyq/libgcc.a` (`tools/libgcc.py`), links with the splat linker script, extracts the binary with `objcopy` into `build/sim/`, and compares its SHA-1 with the one in the unit's yaml. It prints `OK` or `DIFF` per unit and exits non-zero on any difference. Pass unit names to build only those, and `--split` to re-run splat after changing a yaml or a symbol file.

## Layout

| Path | Contents |
|---|---|
| `decomp/sim/<unit>.yaml` | splat config of each unit: segments, options, expected SHA-1 |
| `decomp/sim/symbols/<unit>.txt` | Names exported from Ghidra |
| `decomp/src/sim/<unit>/*.cpp`, `*.c` | Decompiled functions (C for the EXE's `syslib`) |
| `decomp/include/common.h` | Declarations shared by the sources |
| `decomp/include/gamestate.h`, `carwork.h`, `race.h` | `GameState`, garage, `CarWork` and race types, from the Ghidra types |
| `decomp/include/syslib.h` | The EXE's C library (`HeapBlock`) and declarations |
| `decomp/include/resident.h` | Types of the resident code, exported from Ghidra ([Resident code](#resident-code)) |
| `decomp/include/movie.h` | OVL5 movie player types (`MoviePlayer`, `MovieStream`, `MdecOut`, `psxMovieLoop`) and declarations |
| `decomp/include/viewloop.h` | The resident screen loop classes (`ScreenViewLoop`, `psxViewLoop`, `psxViewLoopDoubleBuffer`) |
| `decomp/tools/build.py` | Build and check |
| `decomp/tools/psyq.py` | Converts PsyQ library members to ELF archives |
| `decomp/tools/libgcc.py`, `libgcc_config/` | Compiles the EXE's libgcc objects from the GCC 2.8.1 sources |
| `decomp/tools/setup_toolchain.py` | Downloads the compiler and the GCC 2.8.1 sources into `tools/toolchain/` (ignored) |
| `decomp/asm/`, `assets/`, `build/`, `include/*.inc` | Generated by splat and the build (ignored) |
| `decomp/baserom/` | The original files (ignored) |
| `decomp/psyq/` | The PsyQ libraries (ignored) |

Each overlay is linked on its own at `0x80010000`, against the resident symbols, which splat writes to `build/sim/<unit>.undefined_*_auto.txt` as absolute addresses. The EXE has three segments: the PS-X EXE header, the boot code (`0x80010000`, overwritten by the first overlay) and the resident code (`0x8005d600`). The resident segment links the PsyQ members and the libgcc objects from `build/psyq/` with splat's `lib` subsegments, and `build.py` adds their `.bss` to the linker script as `build/sim/exe.psyq.ld`. For OVL5, `build.py` writes `build/sim/ovl5.cxx.ld`: it places the overlay's `.bss` at `0x800a8d5c` (`BSS_VRAM`), where the overlays' BSS starts, and defines the g++ names that its C++ classes reference (`Enter__11psxViewLoop`, `__ti11psxViewLoop`...) as the exported names (`CXX_NAMES`).

## Symbols

[`scripts/ghidra/export_splat_symbols.py`](../scripts/ghidra/export_splat_symbols.py) writes `decomp/sim/symbols/`:

- `exe.txt`: boot and resident functions, and the main-space labels.
- `ovlN.txt`: overlay N's functions and labels (including its BSS view from `overlay_bss.py`), then the resident ones. An overlay label wins over a main-space label at the same address.

It exports every function (a thunk only when it has a name of its own, like `CdGetSector_Thunk`; otherwise it takes its target's name), and the labels named by hand or by the PsyQ signatures that are not inside a function. Names are made valid for the assembler (`ArcadeMenu::Enter` becomes `ArcadeMenu__Enter`, `~ArcadeMenu` becomes `ArcadeMenu_dtor`), and a duplicate within a unit gets its address appended.

After renaming things in Ghidra, run the script (Script Manager, category **GT2**, or ReVa `run-script` with `OUT_DIR` set when the script is run inline), then `tools/build.py --split`. The build must still match: names never change the bytes.

## What it took to match

- **Section alignment.** GNU as gives every section 16-byte alignment, and the linker would pad between them. The configs set `subalign: 4` (the original sections follow each other on 4-byte boundaries) and turn off splat's segment and section end alignment.
- **Strings.** splat decodes EUC-JP and Shift-JIS text and writes it as UTF-8, which as stores as UTF-8, so the configs restrict string detection to ASCII. Other text stays as `.word`.
- **Data inside code.** OVL0 has three tables inside its code (`0x8002ee98`, `0x80046bac`, `0x800489d8`). They contain words that decode as MIPS III instructions, which as rejects for the R3000. They are `textbin` subsegments (raw bytes kept in `.text`), so they stay at their address.
- **Boot data.** The boot code and its data (`0x80011d30`, mostly zeros after `0x80033de2`) are their own segment. In one segment with the resident code, the linker would move the boot data after the resident code.

## Compiler

The game's libraries match PsyQ 4.6 (libpad 4.7; see [psyq-libs.md](psyq-libs.md) for the objects and how to link them), but its code was not built with the PsyQ 4.6 compiler, which is GCC 2.95.2. It matches Sony's GCC 2.8.x builds, the compilers of PsyQ 4.3 (GCC 2.8.0) and PsyQ 4.4 (GCC 2.8.1), so the game used an older compiler with the newer libraries. The game code is C++ built with `-O2 -g0 -G0 -fvtable-thunks -fno-exceptions` (constructors and destructors of OVL5's `psxMovieLoop` have no exception cleanups, and the flag changes nothing elsewhere). Its vtables have the thunk layout (0, the `__tf` function, then 4-byte function pointers), and the EXE header's `gp` is 0. The C++ runtime linked with it (`__main`, RTTI, `__divdi3`) is GCC 2.8.1's own `libgcc`, built with `-fexceptions`; [runtime-libs.md](runtime-libs.md) has it and the game's libc.

The build uses the same open-source toolchain as decomp.me's `gcc2.8.1-psx` compiler:

- **GCC 2.8.1** rebuilt from GNU sources with a PlayStation target (`mips-sony-psx`: R3000, little endian, soft float, unsigned `char`), [decompals/old-gcc](https://github.com/decompals/old-gcc) release 0.17. Only `cc1plus` is used. The game's C library was built with a newer compiler ([C sources](#c-sources)).
- **[maspsx](https://github.com/mkst/maspsx)** at the decomp.me commit (`4ffa8b5`). It rewrites the compiler output the way Sony's ASPSX assembled it, then runs GNU as. `--aspsx-version=2.86` (ASPSX of PsyQ 4.6) makes small `li` constants `addiu`, as in the game; with `-G0`, maspsx treats ASPSX 2.79 (PsyQ 4.4) the same.
  `setup_toolchain.py` patches one rule: after a `div`, maspsx adds a `nop` when the next instruction reads the quotient, even past a label. The game has no `nop` there in `Rgb555_ToHsv` (OVL3), so the patch makes it match. Sony's ASPSX 2.79 and 2.86 do add the `nop`, though, so the original source probably differs from ours before that branch, and the patch may be dropped once it does.
  Sony's ASPSX, run under wibo with maspsx's own test setup (`aspsx/`, the binaries from its `aspsx` release), adds that `nop` in every version from 2.56 to 2.86, with or without `-0` (which only drops the division checks): after `divu` and an `sltu` that reads the quotient, after `div` and a `sw` or `sh` of it, and past a label before a `beq` on it. ASPSX also puts a label before a load delay `nop`, so a jump to it lands on the `nop`. maspsx copies all of this correctly. The game has no such `nop` in any of its 7 places where an instruction reads the quotient right after the `mflo`, so that code wasn't assembled by ASPSX. One of them is `Credits_FormatCommas`, which matches with GNU as ([Format](#resident-code)); the other 6 are in functions that don't match yet, in files that are still assembled with maspsx.

A `.cpp` file goes through:

1. The host `cpp` (`-nostdinc -undef`, GCC 2.8 and MIPS macros, `-Iinclude`). `# 0` line markers become `# 1`, which `cc1plus` accepts.
2. `cc1plus -quiet -O2 -g0 -G0 -fvtable-thunks -fno-exceptions`, reading from stdin. The 32-bit compiler can't `stat` files on a Windows drive under WSL (inode numbers overflow), so it gets no file names. Its output is kept as `build/sim/src/.../<file>.cpp.s` for diffing.
3. maspsx, then GNU as with the same flags as the disassembly.

decomp.me's PsyQ packages (`psyq4.3` to `psyq4.6`) run Sony's executables under wibo. With PsyQ 4.3 and 4.4's `CC1PLPSX.EXE`, the same functions match as with this rebuild, and the ones that don't match here don't match there either: they need a different source, not a different compiler. Sony's ASPSX also checks divisions unless given `-0`; the game has no checks, and maspsx doesn't add them.

## C sources

`syslib` (`0x8008ce08–0x8008dfc4`, [runtime-libs.md](runtime-libs.md#the-games-library)) is the game's libc, and was not built like the game. Its functions have C names (`memset`, `malloc`), and the compilers give the same code for them as C and as C++, so it is C, as a libc would be.

- **GCC 2.95.2.** It is the version of PsyQ 4.6's compiler (PsyQ 4.6 libraries are linked in the EXE), but the library was assembled with GNU as (below), so it may come from a plain GNU toolchain rather than from PsyQ. Its rebuild from [decompals/old-gcc](https://github.com/decompals/old-gcc) (`gcc-2.95.2-psx`, release 0.17) matches 18 functions. GCC 2.8.0 and 2.8.1 match 11 of them and miss `strncpy`, `memcpy`, `InitHeap`, `vsprintf` and `Printf_FormatHex64` by scheduling and register choices. 2.5.7 to 2.7.2 match 3 or 4, and 2.91.66 matches fewer than 2.95.2. No option tried (`-O1`, `-O3`, scheduling and GCSE switches) matches more than `-O2`.
- **Signed `char`.** The library reads characters with `lb` (`strlen`, `strcmp`, `strncasecmp`), where the rebuilt compilers' unsigned `char` gives `lbu`. `build.py` passes `-fsigned-char`, so the sources use plain `char`.
- **Strict aliasing.** In `Heap_SplitBlock`, the load of the new block's `size` is scheduled above the stores to `next` and `prev`, which have pointer type, but not above the store to the other block's `size`. Only type-based alias analysis allows that, so `build.py` passes `-fstrict-aliasing` (off by default in 2.95.2). The other functions match with or without it.
- **Inline assembly.** `memset` stores the unaligned head and tail of the fill with a lone `swr` and a lone `swl`, which the compiler doesn't emit by itself, so the source uses `__asm__` for those two stores.
- **GNU as.** The library was assembled by GNU as, not Sony's ASPSX. When a load is followed by a label and then an instruction that reads the loaded register, ASPSX (and maspsx) put the label first and then the `nop`, so a jump to the label lands on the `nop`. GNU as puts the `nop` before the label. `Printf_ParseInt` jumps past its `nop` (`j` at `0x8008d894`), so it matches only with GNU as. Every other function matches with both. The GNU as of the time encoded `move` as `addu`, where binutils 2.42 uses `or`.
- **Building.** A `.c` file is preprocessed like a `.cpp` file and compiled with `gcc-2.95.2-psx/cc1 -quiet -O2 -g0 -G0 -fsigned-char -fstrict-aliasing`. `build.py` rewrites each `move` as `addu` and passes the output straight to GNU as, with the same flags as the disassembly; maspsx isn't used.
- **One file.** `[0x7d608, c, syslib]` is a single `c` subsegment, and `[0x81774, .rodata, syslib]` places its `.rodata`: the two hex digit strings (string literals in `Printf_FormatHex64`) and the three jump tables of `Printf_Format`'s switches. GCC aligns the tables to 8 bytes within the section, which starts at `0x80090f74`, only 4-byte aligned; splat's linker script puts every input section on 4 bytes (`SUBALIGN(4)`), so the tables land at their original offsets.

## Decompiling a function

1. In the unit's yaml, give the function's address range its own subsegment: `[0x0, cpp, carwork]` makes `src/sim/<unit>/carwork.cpp`, and the next `asm` subsegment starts after the function.
   A function that another unit has too uses that unit's file: OVL4's `[0x5404, cpp, ../ovl3/carwork]` builds `src/sim/ovl3/carwork.cpp`. `build.py` creates `src/sim/<unit>/` before splitting, so the `..` resolves and splat sees the existing file.
   When the copy has other names there (`Demo_PickRandomCar` in OVL1, the `_OVL2` suffixes, OVL4's second CarWork copy), the unit gets its own file that `#define`s them and `#include`s the shared source, as `src/sim/ovl1/randompick.cpp` does.
2. Write the function in that file. Keep it `extern "C"` (see `include/common.h`) while its callers are assembly, so the symbol keeps the name the disassembly uses.
3. Run `tools/build.py --split <unit>`, and compare `build/sim/src/.../<file>.cpp.s` with the original assembly until the unit prints `OK`.

splat never overwrites an existing source file.

## Resident code

The game's resident code (`0x8005d600` to the libgcc block at `0x80085974`) is split into 62 subsegments, one per group of related functions (`vol`, `records`, `garage`, `carmodel`, `cardmenu`, `seq`, `cdaudio`, `viewloop`...). The boundaries follow the function names, since the code shows no object boundaries (no padding between objects). The 43 groups with at least one matched function are `cpp` files; their other functions are `INCLUDE_ASM("asm/sim/exe/nonmatchings/<file>", <name>)`, from the asm files splat writes for a `cpp` subsegment. The 19 others are still `asm` subsegments. `start`, `setjmp` and `longjmp` stay assembly.

- **First pass.** Each function was tried once with Ghidra's decompilation unchanged, compiled on its own and compared with the original, relocations masked. 204 matched, and are in the files as Ghidra wrote them (with `uint`, `undefined4` and the like spelled out as C types, and the trailing `return;` dropped). A few match only with a signed type where Ghidra has an unsigned one (`signed char`, `short`); their source spells it. 579 don't match yet, mostly from different code shapes (Ghidra's loops and expressions, globals that start inside a table, field signedness), Ghidra's partial-variable syntax, or switch tables.
- **Types.** `include/resident.h` is Ghidra's `DataTypeWriter` export of the types the matched code uses, adapted for `cc1plus`: enums become integer typedefs of their Ghidra size with the constants in an anonymous enum (two enums share constant names), fields named like C++ keywords get a `_`, and `bool` is a byte as in Ghidra (GCC 2.8.1's `bool` is 4 bytes). It checks with the Ghidra sizes and field offsets of all 231 structures except the ISO 9660 records `IsoContext` and `IsoDirRecord`, whose fields are not aligned. `McDirEntry.next` is `signed char`: the code stores -1 there.
- **ScreenViewLoop.** `screenviewloop.cpp` (`0x800833a0-0x800834fc`) is the class itself: constructor, destructor, `Execute`, `Run`, the four empty steps, and what g++ emits with them, the `__tf` function, the vtable and the type name (its `.rodata` subsegment at `0x800908ec`) and the typeinfo (`.comm`, placed at `0x801c9400`). The assembly that is left (the boot logo, `psxViewLoop`) references it by the Ghidra names, so `build.py`'s `CXX_NAMES` for the EXE defines them as the g++ names (`ScreenViewLoop__Run = Run__14ScreenViewLoop`), and `__ti14ScreenViewLoop` as the typeinfo's address. The file is assembled by GNU as instead of maspsx (`GAS_CXX`): in `Run`, the loop's `j` lands after the load delay `nop` that comes before the label, which only GNU as gives, as in syslib's `Printf_ParseInt`. With GCC 2.95.2 it doesn't match, and assembling all the C++ with GNU as breaks the other units, so it is a property of this file (perhaps the screen loop classes came from a separate library). `format.cpp` is the other `GAS_CXX` file; for both, `build.py` writes GCC's `div`/`divu` macros out as `div $0` and `mflo` (`mfhi` for `rem`), since GNU as would expand the macros with division checks.
- **psxViewLoop.** `viewloop.cpp` (`0x80080820-0x80080eec`) derives from `ScreenViewLoop`: the static vblank callback `VSyncHandler`, constructor, destructor, `SetVideoMode` and the virtual steps, with its vtable and type name (`.rodata` at `0x800906c8`) and typeinfo (`0x801f0cf0`); the class layout (0x6c bytes) is in `viewloop.h`, which OVL5's `psxMovieLoop` uses. Everything matches. Notes from matching it: `Enter` computes the new buffer index into an `unsigned char` local and stores it after `videoModeDelay` (found with [decomp-permuter](https://github.com/simonlindholm/decomp-permuter); otherwise the allocator gives `g_LastBufferIndex` and the index a0 and a1 the other way round); `vsync` is set by an inline `VSyncCallback` constructor in the initializer list (the original addresses it through its own pointer); the pad raw buffers are `volatile`, and `Step` reads the port's status byte into a local before taking the pointer to its buttons (that keeps `&g_PadSystem` in a register and `result = 0` ahead of the load, as in the original); and `UpdateDisplay` reads the vblank count, `swapPending` and `videoModePending` into `int` locals first.
- **psxViewLoopDoubleBuffer.** `viewloopdb.cpp` (`0x8007fe8c-0x80080088`) derives from `psxViewLoop` and adds the two buffers' VRAM y (`bufferY`, 0x70 bytes): constructor, destructor, `Leave` (flips and draws twice more so both buffers hold the last frame), `SetVideoMode`, `BeginDraw` (the draw area in `g_DrawArea`, offset by the buffer's y; the offset is read first, then the area into `int` locals) and `SwapDisplay`, with its vtable and type name (`.rodata` at `0x80090630`) and typeinfo (`0x801f0ce0`). `Gpu_Reset`, just before it, belongs to the previous group.
- **VendorBootLogo** (boot code). `bootlogo.cpp` (`0x80010a24-0x80010f1c`) derives from `psxViewLoopDoubleBuffer` and shows each splash screen, fading it in and out. The file also has `Boot_ClearVram`, `Boot_ShowSplashScreens` and `Boot_Init`, since g++ emits `__tf14VendorBootLogo` after them, at the end of the file; its `.rodata` (`0x80011d38`) is `Boot_Init`'s `"/"`, then the vtable and type name. Everything matches except `Boot_ShowSplashScreens` (`NON_MATCHING`): its C++ has the original's shape (a `g_VolDir` local next to the symbol, the loop test at the top), but strength reduction gives the byte fields their own pointer, which takes the register the original uses for `D_80033DC0`'s high half; two and a half hours of decomp-permuter on a C version (about 430,000 candidates) didn't remove that pointer either. Notes: a local declared in a loop body makes g++ keep the test at the top (`Boot_ClearVram`); `Draw` needs `left` computed before `level = frame` (load order) and its two constants in locals set before the loop. The splash images (`g_SplashScreens`) stay the original data.
- **ButtonPrompt.** `buttonprompt.cpp` (`0x8006dcb8-0x8006e1cc`) is a box with one message, confirmed or cancelled with a button, used instead of the save dialog when saving is not offered. `ButtonPrompt_Init`, `ButtonPrompt_Update` and `ButtonPrompt_Draw` are rewritten with the file's own `ButtonPrompt`, `ButtonPromptDesc` and `TextStyle` (the template `TextItem_Init` copies) structs, and all match. Notes: `Init`'s size is a `switch` with a `case 0` beside `default` and the cases in the order 0, 4, 2, 6; its text spacing goes through an `int` local (for the original's `lb`). `Draw` calls `Prim_AddDrawMode` at the end of each branch (the compiler merges the two calls), sets the text brightness through a `TextItem` pointer, and declares `ot + 2` before the text offset (register order).
- **CardDialog.** `carddialog.cpp` (`0x8006e1cc-0x8006ebf0`) holds the memory card dialogs: a message over two option boxes, picked with left and right and confirmed or cancelled. `CardDialog_Init`, `CardDialog_Update` and `CardDialog_Draw` are rewritten with `CardDialogDesc` (the 0x30-byte description `Init` reads) and the same `TextStyle` as `ButtonPrompt`, and all match. In `resident.h`, `CardDialog.sfx` and `choice` are `signed char` (plain `char` is unsigned with these flags, and `Update` tests `sfx >= 0`), and `boxWidth` and `boxHeight` are `ushort`. Notes: `Init` reads the font and text height into locals first and gives each option's spacing its own `int`. `Draw` uses one variable for the slide offset, the closing box width and the flash width, one (`left`) for the left box and the closing box's centre, and one `t` for every animation step; permuter found that `t = anim + 17` goes before the `anim < -1` test. The slide offset is `(boxWidth << 1) * t / 12`. The rest of the file matches too: `FUN_8006ea38` adds a sprite scaled around a point as a `POLY_FT4`, and `CardSlots_Update` maps each port's driver state to the slot status with a `switch`, whose jump table is the file's `.rodata` subsegment (`0x8008fab0`). It reads the ports through an inline accessor returning `&g_Card` (found with permuter): with `g_Card` written directly the compiler folds the ports' offset into the loop pointer.
- **Format.** `format.cpp` (`0x80068734-0x80068e2c`) writes numbers as text, and all four functions match: `Time_FormatMs` (a time as `m:ss.mmm`, or `h:mm:ss.t` from an hour on, returning the length), `Time_FormatDiff` (a signed difference, `s.mmm` under a minute), `Speed_Format` and `Credits_FormatCommas` (a number with a comma every three digits). The file is assembled by GNU as (`GAS_CXX`), with GCC's division macros written out as `divu $0` and `mflo`: after `Credits_FormatCommas`'s `divu`, the `sltu` reads the quotient with no `nop` between, which no ASPSX gives, and the copy loop's `j` lands after the load delay `nop` before its label, as in `ScreenViewLoop::Run`. Each digit pair is written as `d = x / 10` then `x - d * 10`, with one `d` for all of them, as the game reuses one register. `Time_FormatMs` uses one variable for the milliseconds in the short form and for the hours and the tenths in the long one. In `Time_FormatDiff` (found with permuter), the milliseconds are copied to an `int` before the minute test and passed on from it, the hundreds' remainder gets its own variable, and the plus-minus sign (`0xb1`) is stored through a `signed char *` (the game loads it as `-79`). The two `0xffffffff` tests use `|`, not `||`, as does `Credits_FormatCommas`'s digit test, through an `int` holding `d != 0`. Its copy loop is a `goto` loop (the game's has the test at the bottom and `','` reloaded on each pass, so GCC's loop optimizer didn't see it), indexed by the digit count's variable; the digits are plain `char`, and the loop's load is cast to `signed char`, as the game reads the first one with `lbu` and the others with `lb`.
- **Typeinfo.** No assembly references the classes' typeinfo once their `__tf` functions are C++, so `CXX_NAMES` gives the `.comm` symbols (`__ti14ScreenViewLoop`...) their addresses.
- **Code that nothing names.** Inside a `cpp` subsegment splat writes only the functions it has symbols for, so unnamed code between them would be dropped. 49 unreferenced functions that Ghidra had never disassembled were created in Ghidra (they export as `FUN_xxxxxxxx`), and the `j CdGetSector` thunk at `0x8007cfd4`, which OVL5 also calls, is `CdGetSector_Thunk`.

## Next steps

- Decompile more of OVL3 (its functions are copies of OVL4 code).
- Fix the resident functions that Ghidra's output doesn't match, and replace the Ghidra names (`uVar1`, `field_0x10`) and `resident.h` with real headers as they are.
- Drop `extern "C"` once callers are C++, so names get GCC 2 mangling.
