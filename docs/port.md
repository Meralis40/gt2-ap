# Native port

How the game runs natively on PC, from the C++ in `decomp/`, with the Archipelago client built in. The sources are forked into `port/`, which doesn't have to match; `decomp/` stays the matching build.

## Memory model

The port is a 32-bit x86 build, and guest RAM is mapped at its real address, `0x80000000`. Disc pointers, `int`/pointer casts, fixed offsets and per-overlay BSS then work unchanged, and a GPU link becomes a pointer with `link | 0x80000000`. macOS is out of scope: it has no 32-bit support.

## M0 results

`port/spikes/m0` tests these assumptions. All passed on Windows 11 with Clang 23 (`--target=i686-pc-windows-msvc`), lld-link and the VS 18 x86 libraries:

| Check | Result |
|---|---|
| Guest RAM reserved at `0x80000000`, 16 MB | Works, with `/LARGEADDRESSAWARE` |
| Scratchpad reserved at `0x1F800000` | Works |
| `.set` in an assembly file binds `extern` names to those addresses | Works, with `/FIXED`, `/DYNAMICBASE:NO`, `/SAFESEH:NO` |
| 24-bit OT walk | Works |
| libc `setjmp`/`longjmp`, entry point passed as the `longjmp` value | Works from a live trampoline frame |
| Jump into guest RAM | Faults, and a vectored handler reports the guest address |
| 12 layout values of real decomp types | Same under GCC 2.8.1 and Clang |

**Windows toolchain.** Clang with the MSVC target works with what VS installs, so llvm-mingw isn't needed unless a later check needs GCC-style layout. MSVC-target Clang uses MS bitfield layout. The decomp has bitfields, but the probe covers only `OtTag` among them, so the full layout check has to confirm them.

**Build notes.** Use `-idirafter` for the MSVC and SDK headers: with `-isystem` they take precedence over Clang's own, varargs break, and `printf` prints garbage without any compile error.

**Linux.** The same eight runtime checks pass under WSL Ubuntu-24.04 with `g++ -m32 -no-pie` (GCC 13, needs `gcc-multilib` and `g++-multilib`), and the 12 layout values are the same with `-m32 -malign-double`. This is the WSL kernel; a native Linux desktop and Steam Deck are untested. Assembly files need a `.note.GNU-stack` section, or the linker marks the stack executable.

**Not tested yet.** The real game's early reservation has to be retested once SDL3 and the other libraries load in the same process.

## Building

`port/` builds as part of the top-level CMake project with `GT2AP_BUILD_PORT=ON`, on a 32-bit toolchain. On Windows:

```bash
cmake -S . -B build/port-win -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/i686-windows-clang.cmake -DGT2AP_BUILD_PORT=ON -DGT2AP_BUILD_TESTS=OFF
cmake --build build/port-win --target gt2ap
```

The toolchain file's `VS_VC`, `WIN_SDK` and `WIN_SDK_VERSION` name the installed versions. The Linux build isn't set up yet.

Run it with `gt2ap <disc.cue>`, or `gt2ap --baserom decomp/baserom/sim` in development.

## The tree

| Path | What |
|---|---|
| `port/game` | The forked sources, edited by hand. `FORK_REPORT.txt` is what `fork.py` left to convert, and is out of date since those edits |
| `port/rt` | Runtime: guest memory, loading, the setjmp point, stubs (`stubs.cpp`) |
| `port/gen` | Generated and checked in: `guest_syms.S` binds data names to guest addresses, `fixups_<unit>.cpp` lists the code pointers in guest data |
| `port/tools` | `fork.py`, `gen_syms.py`, `gen_fixups.py` |

`gen_syms.py` takes the names the objects leave undefined. It reports function names instead of binding them: those are for the port to define. `gen_fixups.py` found 336 slots, with none pointing at a function the port doesn't define. `Overlay_StartAt`'s default entries aren't among them, because they're overlay addresses in resident data; `overlay.cpp` has them as a table of host functions.

## Status

**M1 is done, from the baserom files.** The game runs `Sys_Init` and reaches `Game_Main`, then stops at its first `CdControlF`, in `Boot_Init`. Loading from a real disc is written but hasn't run.

Known gaps:

- `g_HwRegs` is a plain copy of the I/O page, and most stubs do nothing or stop with their name. Writes to GPU, DMA and SPU registers have no effect yet, and GPUSTAT always reads as ready.
- `SimMap_Update` is a stub. It has no C++ (M4).
- `RaceMenuLoop`, `RaceViewLoop`, `QuickMenu` and `PreQuickMenuIO` still have to become real classes: `QuickMenu` and `PreQuickMenuIO` have lost their constructors for now.
- `-O2`, `-malign-double`, `-mno-ms-bitfields` and the full layout check aren't in use yet.
- Inflate no longer yields, so a stream decompresses in one frame. Formatted output is still the game's own (`Printf_Format`, which the text code needs).
- Spin-waits other than `Boot_WaitVSyncs` don't call `Rt_Pump` yet.
