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
