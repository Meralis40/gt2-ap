# Ghidra setup

Reverse-engineering environment for phase 1: Ghidra, the PS1 loader, and an MCP server so Claude can work in the same Ghidra project.

## Versions (checked 2026-10-19)

| Component | Version | Why |
|---|---|---|
| [Ghidra](https://github.com/NationalSecurityAgency/ghidra/releases) | **12.1.3** | Newest version supported by both plugins below |
| [ghidra_psx_ldr](https://github.com/lab313ru/ghidra_psx_ldr/releases) | 2026.10.03 | PS-X EXE loader, PsyQ signatures, GTE instructions, overlays |
| [ReVa](https://github.com/cyberkaida/reverse-engineering-assistant/releases) | v7.3.1 | MCP server inside Ghidra; tools built for LLM use; no Python bridge |
| JDK | 21 (64-bit) | Required by Ghidra 12. Amazon Corretto 21 is fine |

Extensions are built for one exact Ghidra version, so the file names below must match `12.1.3`.

Alternative: [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) 2.0 works too, but only on Ghidra 12.1.4, where ReVa has no build yet.

## 1. JDK 21

Check with `java -version`. If it isn't 21, install a JDK 21 (e.g. Eclipse Temurin or Amazon Corretto). Ghidra asks for the JDK path on its first start if it can't find one.

## 2. Ghidra 12.1.3

1. Download `ghidra_12.1.3_PUBLIC_20260817.zip` from the Ghidra releases page.
2. Unzip it to a folder **without spaces**, e.g. `E:\tools\ghidra_12.1.3_PUBLIC`.
3. Start it with `ghidraRun.bat` (Windows) or `./ghidraRun` (Linux/macOS).

## 3. Extensions

Download:
- `ghidra_12.1.3_PUBLIC_20261004_ghidra_psx_ldr.zip` (ghidra_psx_ldr release 2026.10.03)
- `ghidra_12.1.3_PUBLIC_20260903_reverse-engineering-assistant.zip` (ReVa release v7.3.1)

Install both. Don't unzip them:

1. In the Ghidra project window: **File → Install Extensions**.
2. Click **+**, pick each zip, check both are ticked.
3. Restart Ghidra.

Enable ReVa (it needs to be turned on in two places):

1. Project window: **File → Configure**, click the plug icon (top right), tick **ReVa Application Plugin**.
2. Open any program in the Code Browser (the dragon icon): **File → Configure**, plug icon, tick **ReVa Plugin**. Then **File → Save Tool** so it stays enabled.

By default ReVa listens on `localhost:8080` (changeable in the project window's tool options). It includes PyGhidra scripting tools. Keep it on localhost only.

## 4. Ghidra project

Create the project **outside this repository**, e.g. `E:\re\gt2\gt2.gpr`: it holds the game's code, which must never be committed.

Extract the executables with our tool:

```bash
gt2-discinfo extract "Simulation.cue" SCUS_944.88 E:/re/gt2/SCUS_944.88
gt2-discinfo extract "Arcade.cue" SCUS_944.55 E:/re/gt2/SCUS_944.55
```

Import `SCUS_944.88` (**File → Import File**). The format should be detected as **PSX executable**. Run auto-analysis with the defaults. The PSX loader applies PsyQ signatures, which names most SDK functions.

### Overlays

All game-mode code is in overlays stored in `GT2.OVL` in the disc root. They all load at the same address: **`0x80010000`**.

`GT2.OVL` starts with a table of little-endian `(offset, compressed size)` u32 pairs; the first offset gives the table length (`0x30`, so 6 entries on both discs). Each entry is a gzip stream. Decompress them with:

```bash
gt2-discinfo overlays "Simulation.cue" E:/re/gt2/ovl-sim
gt2-discinfo overlays "Arcade.cue" E:/re/gt2/ovl-arcade
```

This writes `ovl0.bin` … `ovl5.bin` and prints each overlay's load range.

Memory layout of `SCUS_944.88` (Simulation):

| Range | Contents |
|---|---|
| `0x80010000–0x8005d5ff` | Overlay window. In the EXE file this holds boot-only code, overwritten by the first overlay. |
| `0x8005d600–0x800a8d5b` | Resident code and data (the EXE entry point is `0x8005d600`). |
| `0x800a8d5c–0x801c93af` | BSS. `GT2.OVL` is read whole into its start, then BSS is cleared after each switch. |

Loader, in the Simulation EXE:

- `FUN_80010000` (boot): looks up `gt2.ovl` and reads its 0x30-byte table to `0x801ef61c`.
- `FUN_8005da7c`: switches overlays. It inflates entry N to `0x80010000` (inflate core `FUN_800847d0`), then calls `FlushCache` and clears BSS.
- `0x80091174`: table of overlay entry points, indexed like `GT2.OVL`. At boot, `FUN_8005d6e0` loads overlay 1.

| # | Size | Entry point | Likely contents (from strings, unconfirmed) |
|---|---|---|---|
| 0 | `0x4d5f8` | `0x80012254` | Race (`GranTurismoRaceLoop`, `ArcadeRaceLoop`, `QuickMenu`) |
| 1 | `0x3c8c4` | `0x80011384` | Title / boot (`data-title.txd`) |
| 2 | `0x43544` | `0x80011750` | Arcade menus, track select |
| 3 | `0x02cec` | `0x80012c00` | Unknown (small, no strings) |
| 4 | `0x42a74` | `0x80013628` | GT mode menus (`GTBattleMenu`, `GTLoadingMenu`) |
| 5 | `0x020e0` | `0x800114b8` | Movie player (`psxMovieLoop`) |

The Arcade disc (`SCUS_944.55`) uses the same scheme with the same load address `0x80010000`, the same entry points and the same role for each overlay. The resident code shifts down by 0x40:

| | Simulation `SCUS_944.88` | Arcade `SCUS_944.55` |
|---|---|---|
| Overlay window | `0x80010000–0x8005d5ff` | `0x80010000–0x8005d5bf` |
| EXE entry point | `0x8005d600` | `0x8005d5c0` |
| BSS | `0x800a8d5c–0x801c93af` | `0x800a8b04–0x801c917f` |
| Overlay switch | `FUN_8005da7c` | `FUN_8005da3c` |
| Entry-point table | `0x80091174` | `0x80090f58` |
| Overlay at boot | 1 (title) | 5 (movie) |

Arcade overlay sizes differ slightly; `gt2-discinfo overlays` prints them.

Game assets live in `GT2.VOL`; see [gt2-vol.md](gt2-vol.md) for its format and `gt2-discinfo vol`.

To load them into the program, with `SCUS_944.88` open in the Code Browser, do this for each `ovlN.bin`:

1. **File → Add To Program**, pick the file. The format is **Raw Binary**.
2. **Options…**: tick **Overlay**, set **Base Address** to `80010000` and **Block Name** to e.g. `ovl0`.
3. **Window → Memory Map**: tick **X** (execute) on every `ovlN` block. Raw Binary blocks are added non-executable, and auto-analysis then leaves most overlay code undisassembled.
4. Run auto-analysis (**Analysis → Auto Analyze**).

ReVa cannot parse overlay-qualified addresses like `OVL0::80059704`. Refer to overlay locations by symbol instead (`FUN_OVL0__80059704`, `LAB_OVL0__80059704`), for example to create a function there.

Each overlay becomes its own address space (`ovl0::80010000`, …), so the overlays don't collide, and their calls into resident code (`0x8005d600`+, or `0x8005d5c0`+ on Arcade) still resolve to the main program.

### Per-overlay BSS

Every overlay switch clears the whole BSS (`Overlay_ClearBss`), and each overlay then uses that range for its own variables: the same address can be a menu pointer in one overlay and a race variable in another. In a single main-space BSS, their labels and types collide.

[`scripts/ghidra/overlay_bss.py`](../scripts/ghidra/overlay_bss.py) gives each overlay space its own BSS:

1. It adds an uninitialized `ovlN_bss` block at the BSS range to each overlay space. Code in an overlay then resolves BSS addresses to its own space, both in the listing and in the decompiler.
2. It moves each overlay's existing references into its block, along with the labels, data and comments at addresses only that overlay uses.
3. It lists the user labels at addresses that several overlays use. They stay in main space and need moving by hand to the overlay they describe.

Boot code and resident code keep the main-space BSS. That's right for boot, which runs before any overlay. For resident code, BSS state is per overlay anyway.

Run it once the overlays are loaded and analyzed:

1. In the Code Browser: **Window → Script Manager**, then the **Manage Script Directories** button, and add `scripts/ghidra` from this repository.
2. Run `overlay_bss.py` (category **GT2**). Ghidra must have been started with PyGhidra (`pyghidraRun`). With ReVa, `run-script` with `scriptName: overlay_bss.py` works once the directory is registered.

Running it again does nothing harmful: existing blocks are kept, and only references still pointing at main space are moved. Set `DRY_RUN = True` at the top to see the per-overlay counts first.

The BSS range is `0x800a8d5c–0x801c93af` on Simulation, checked against `Overlay_ClearBss`. The Arcade range in the script comes from the EXE header and hasn't been tried.

In scripts, `getAddressSpace("OVL1").getAddress(offset)` returns a main-space address when the offset is outside the overlay's blocks. `getAddressInThisSpaceOnly(offset)` always stays in the overlay space; the script uses it to create the blocks.

## 5. Connecting Claude (MCP)

With Ghidra open on the project, ReVa serves MCP at `http://localhost:8080/mcp/message`.

This repository's `.mcp.json` declares it as project-scoped server `ReVa`, so Claude Code sessions started in this folder offer to connect to it (you approve it once). The equivalent command is:

```bash
claude mcp add --scope project --transport http ReVa http://localhost:8080/mcp/message
```

Ghidra must be running with the project open before the session starts; otherwise the server shows as failed. Start a new session after opening Ghidra.
