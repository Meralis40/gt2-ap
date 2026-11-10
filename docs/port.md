# Native port

How the game runs natively on PC, from the C++ in `decomp/`, with the Archipelago client built in. The sources are forked into `port/`, which doesn't have to match; `decomp/` stays the matching build.

## Memory model

Guest memory is a 4 GB window of the address space at `rt::base`, and a PlayStation address `a` is at `base + a`. On x86-64 the system picks the base. On i686, which still builds, it is 0, so guest RAM sits at its real address `0x80000000`. Guest data keeps the console's layout and values on both (`rt/gptr.h`):

- **Pointers in guest data** are `game_pointer<T>`: 4 bytes, the PlayStation address. They convert to and from `T *`. `(int)p` gives the address, as on the console. In code, `rt::to_guest(p)` and `rt::from_guest(a)` convert a host pointer and an address, and `rt::guest(a)` gives a fixed address such as the scratchpad's.
- **Code pointers in guest data** are `game_function<F>`: the function's PlayStation address, as in the loaded image. A call looks it up in `gen/functions_<unit>.cpp`, in the loaded overlay's table and then the executable's. A function the console didn't have gets an address from `0xc0000000` when first stored.
- **The game's data names** are references (`extern T (&name)`), which `rt::bindGuestData` points at `base + address` (`gen/guest_data.cpp`).
- **The game runs on a stack inside the window** on x86-64 (`0x80c00000`, 4 MB), so its locals have guest addresses too. Data the port adds and guest pointers reach goes in `rt::guestAlloc`'s 4 MB from `0x80200000`.
- **The screen loops, menus and race loops** have no C++ virtual functions. A C++ vtable pointer would be 8 bytes on x86-64, and decompiled code reaches their fields by the console's offsets. Each class keeps a `game_vptr` word where the console has its vtable pointer, holding that vtable's address. `v_F()` calls through the port's table for it (`rt/vtable.h`, `gen/vtables.cpp`).
- **Game code has no C++ library headers** (`-nostdlibinc`). `long` is `int`, as on the console, and `size_t` is the compiler's.

On x86-64, a host address that ends up in a `game_pointer` stops the game with its caller ("host address … stored as a guest pointer"). A cast between a pointer and a 32-bit integer is a compile error. These tools check what the compiler can't:

- `tools/check_layout.py` compares every game structure between two targets.
- `tools/check_ptrargs.py` finds C functions declared with an `int` where another declaration has a pointer.

The one-off conversions are in `tools/ref_externs.py`, `convert_members.py`, `convert_casts.py` and `devirtualize.py`. The x86-64 builds' frames and sound match the i686 build's byte for byte on the attract demo and three recorded races.

macOS isn't built yet; the window doesn't need a fixed address, so it is possible.

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

`port/` builds as part of the top-level CMake project with `GT2AP_BUILD_PORT=ON`. On Windows:

```bash
cmake --preset port-windows64
cmake --build --preset port-windows64
```

From a Visual Studio developer prompt the toolchain file finds Visual Studio and the Windows SDK by itself; otherwise its `VS_VC`, `WIN_SDK` and `WIN_SDK_VERSION` name the installed versions. The C runtime is linked statically. CMake fetches SDL3 3.2.8, which needs git on the PATH.

On Linux, with Clang, CMake, Ninja and the X11, PulseAudio and ALSA development packages:

```bash
cmake --preset port-linux64
cmake --build --preset port-linux64
```

`.github/workflows/package.yml` lists the packages. The C++ runtime is linked statically. Only X11 is built in, so a Wayland desktop needs XWayland. This has been run under WSLg only.

The 32-bit builds, `port-windows` and `port-linux`, still work. `port-linux` needs `gcc-multilib`, `g++-multilib` and the i386 libraries to run. The 32-bit builds are the reference that the x86-64 builds' recordings are compared with.

## Releases

`.github/workflows/package.yml` builds both clients, runs the world's tests and packs `gt2.apworld` with Archipelago's Build APWorlds tool, from the commit named in `ARCHIPELAGO_COMMIT`. CI runs it on every push to `main` or `port` and on pull requests, and keeps the results as artifacts.

To release, set `world_version` in `world/gt2/archipelago.json` and `VERSION` in `CMakeLists.txt`, then push a tag `v<world_version>`. `release.yml` stops if the tag and `world_version` differ, otherwise it publishes a GitHub release with the three files. A tag with a suffix, like `v0.2.0-rc1`, makes a prerelease.

Run it with `gt2ap [<disc.cue>]`, which opens on the Archipelago connection page (below). With no disc it uses the `disc` line of `gt2ap.cfg`, or asks for the image with a file dialog and saves it there; the image must hold the NTSC-U 1.2 executable (its SHA-1 is checked). A test run with no disc and none saved stops with the usage. `--vblanks <n>` stops after n vblanks with no window and no pacing, and `--dump <every> <dir>` writes a frame every so many vblanks as a PPM file. `--audio <file>` saves the sound.

## The tree

| Path | What |
|---|---|
| `port/game` | The forked sources, edited by hand. `FORK_REPORT.txt` is what `fork.py` left to convert, and is out of date since those edits |
| `port/rt` | Runtime: guest memory, loading, the setjmp point, the pump (`irq.cpp`), stubs (`stubs.cpp`) |
| `port/hw` | The hardware: GPU, GTE, CD, memory card, pad, and the SDL3 window (`host.cpp`) |
| `port/gen` | Generated and checked in: `guest_data.cpp` binds data names to guest addresses, `functions_<unit>.cpp` gives each function's PlayStation address, `vtables.cpp` the classes' tables |
| `port/ap` | The Archipelago client: `ws.cpp` (websocket, mbedTLS), `session.cpp` (protocol, on its own thread), `state.cpp` (items, locations, a seed's files) |
| `port/tools` | `fork.py`, `gen_syms.py`, `update_syms.sh`, `gen_functions.py`, `check_protos.py`, `check_argorder.py`, `check_layout.py`, `check_ptrargs.py`, `fake_ap_server.py`, and the one-off conversions above |

`gen_syms.py` takes the names the objects leave undefined. It reports function names instead of binding them: those are for the port to define. `gen_functions.py` lists every function the port defines that the symbol files give an address, per unit. The 333 words of guest data that hold one (menu pages, state tables, callbacks) are declared as `game_function` members. `Overlay_StartAt`'s default entries are host functions in `overlay.cpp`; they never enter guest memory.

## Archipelago

The connection page (`game/ovl1/apconnect.cpp`) comes up before the autoload, because the seed decides which memory card the game reads. On it, the keyboard types instead of being the controller; Ctrl+V pastes. The server and slot are kept in `gt2ap.cfg`, the password isn't. **Play offline** leaves every rule as the game has it, on `gt2ap.mcd`.

With a gamepad connected, controller 1 is a DualShock (`0x73`) instead of a digital controller: the game's own analog handler and its analog key maps apply, so the left stick steers, and Options can put the pedals on the right stick. `host.cpp` starts a stick just past `Pad_ScaleStick`'s dead zone, and makes the d-pad and arrow keys push the left stick, since the analog map doesn't steer with them. Triggers stay digital L2 and R2. Recordings keep the sticks (version 2); version 1 still plays, as a digital controller.

Tab or the right stick's click opens the tracker in a session: manufacturers, tuning parts, trophies, credits and checks. It's drawn by `host.cpp` over the picture rather than in the game's menus, so it opens anywhere, and `Host_Frame` holds the game until it closes: a pause between vblanks, which neither races nor recordings can see.

A session uses `saves/<seed>_<slot>.mcd` and `saves/<seed>_<slot>.locations`. `ap.h` is all game code sees: the rule functions in `docs/ap-hooks.md` ask it directly, and Ap_Poll applies what the session's thread has received once a vblank.

- **Locations.** A win is reported from `RaceVehicle_OnLapDone`, not from the event results: in a championship the game only keeps the championship's result, and locations are per race. `ChampRaceList_Build` notes each race's event. A check goes to the `.locations` file before the server, and the file is resent on every connection, so one made while the server is away isn't lost. Practice runs and replays don't count.
- **Checks into the save.** What the server has checked, by this client or another on the slot (or `/send_location`), shows in the save: `GTMenu::Update` raises an event won to 1st, one finished to 6th, and a license test passed to a Kid's prize, never lowering a better result. A championship keeps one result, in its first race's slot, so it is won once all 5 races are checked won (finished once all are finished). A license location alone writes nothing: only the tests' own checks do. Nothing is sent back, and prizes aren't given.
- **Items.** The server resends them all on connecting, so what's held is rebuilt from that list. Credits are the exception, since they must apply once per save: `GameState.apItemsApplied`, two bytes the game leaves alone, counts the items applied, and because it's saved with the credits, loading an older save applies the later ones again. `GTMenu::Update` applies them.
- **Goal.** All 5 GT World League races checked, on either side, sends the goal.
- **Connection lost.** The session retries every 5 s. The game goes on with what it holds.
- **Recordings.** A run with a page can't be recorded or replayed, so `--record` and test runs skip it. `--connect <server> <slot> [<password>]` joins without the buttons. With `--replay` it runs beside the replay and the recording's card stays in use, which is how a recorded race can be tried as a check. Items aren't part of a recording, so the session has to grant what the recording needs.

- **Compression.** The protocol deprecates connections without it, so `ws.cpp` negotiates `permessage-deflate`. It asks for `server_no_context_takeover`, so each message inflates alone through `discio`, and it sends uncompressed, which the extension allows. A server that won't drop its context is refused, since that would need a streaming inflate.

A run on a real server (Archipelago `main`, `MultiServer.py` with a seed from `world/gt2`, items given with `/send` on its console) connected, took compressed messages, and got a check and its item back from a replayed race. `wss` reached `archipelago.gg` with its certificate verified, up to the websocket request, since no room was hosted there. `tools/fake_ap_server.py` stands in for a room with no seed and no rules, and covers a wrong password or slot, a dropped connection and the goal; it doesn't compress. Not covered yet: typing on the page, license tests as checks, and the shops' refusals by hand.

## Status

**The game reaches M5's first goal.** From the disc image, it boots, shows the title screen, runs GT Mode's map and Garage page, and plays the attract race as a 3D replay with HUD, cars and track. All of this was checked in test runs, by looking at dumped frames. Race physics were compared with PCSX-Redux frame by frame: two rally replays and the attract demo keep every car in step with it to the last frame. Only the car sound state differs, in which SPU voices it took.

| Milestone | State |
|---|---|
| M0-M3 | Done |
| M4, GT mode | Map and Garage page reached. Shops, dealers, car previews and memory card saves haven't been driven yet |
| M5, race | The attract replay runs. Car meshes look wrong |
| M6, AP | Works on a local Archipelago server with a generated seed (below). Not yet played through by hand |
| M7, sound | Written (below). SPU, reverb and XA run; checked by ear, by dumps and with a recorded race |
| M8, movies | Put off: movies are skipped, with a log line |
| M9, release | CI and release workflows written (below). Not yet run on GitHub |

Test runs take `--input <vblank> <buttons>`, with the bits in `hw/host.cpp` (`0x4000` is Cross; the title menu confirms with Cross, not Start).

A vblank falls on every 32nd pump, in test runs and windowed runs alike, and the host's clock only paces them. So a run depends on nothing but the memory card and the buttons, and a data read gets at most 32 sectors a vblank (3.8 MB/s).

`--record <file>` saves the memory card as it stands at startup and the buttons of every vblank, until the client closes or crashes. `--replay <file>` plays one back, in a window on the real clock, or at full speed with `--vblanks <n>` (with `--dump` for frames), stopping at the end of the file or at n. The replay uses the recording's card and leaves `gt2ap.mcd` alone; in a window, the controls come back when the file ends. The game's random numbers all come from the vblank count and the counters that follow it, so they need no restoring: each vblank stores the random seed only as a check, and a replay logs the first difference. A recording survives a rebuild as long as the changes don't alter what happens on the way, so a physics fix can make the car leave the road where the recording kept it on.

A windowed run stayed up for 20 s with SDL3 and the D3D driver in the process, so guest memory coexists with them, but nobody has looked at that window or tried the controls yet.

Controls: arrows, Enter (Start), Backspace (Select), X (Cross), C (Circle), S (Square), D (Triangle), Q and E (L1, R1), 1 and 3 (L2, R2), or the first gamepad. The memory card is `gt2ap.mcd`, in the working directory.

Known problems:

- **Sound.** `hw/spu.cpp` reads the SPU registers once a vblank from the plain copy the game writes, so a key on or off takes effect at the next vblank, as the sound driver expects. It runs on the vblank count, and the game reads the envelopes, so sound is part of what a recording repeats. Music is XA on the disc (stereo, 37.8 kHz), played at drive speed. The output is simple where hardware isn't: XA is resampled with a Catmull-Rom curve instead of the real filter, the reverb computes both sides at once, and there's no SPU interrupt, capture or CD audio. `--audio <file>` writes the output as raw 16-bit stereo at 44100 Hz.
- **Movies.** MDEC isn't implemented. The movie entries in `game/ovl5/movieloop.cpp` log the movie, skip it and go on to OVL1. The Simulation disc's only one comes at the end of the game. `DecDCTin` and `DecDCTout` still stop with their name, which the Arcade menus' course previews would reach.
- **Not checked.** `-fno-strict-return` hides functions that fall off their end. Any of them whose callers use the result got it from `$v0` on the PlayStation, so they need checking: `Tim_LoadClutAndImage` was one. `tools/check_protos.py` covers argument counts only: it found `Race_LoadEngineBanks`.
- **Argument order.** The Microsoft ABI evaluates arguments right to left, so a call must not pass a value next to the call that sets it. `tools/check_argorder.py` lists suspects, and all 50 hits were harmless when it was last read through. Other targets evaluate left to right.
- **Layout.** The full layout check isn't in place yet.
- **Spin-waits** are fixed as they're found; the watchdog reports them after 2 s (Windows only).
- **Kanji ROM.** It isn't on the disc, so its glyphs are blank. Only memory card screens use them.
- **Inflate** no longer yields, so a stream decompresses in one frame. Formatted output is still the game's own (`Printf_Format`, which the text code needs).
