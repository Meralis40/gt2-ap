# GT2 Archipelago — Project Plan

Randomizer for Gran Turismo 2 (Simulation + Arcade) for [Archipelago](https://github.com/ArchipelagoMW/Archipelago).
Reference projects: [ctr-native-ap](https://github.com/dowlle/ctr-native-ap) (target result), [Quest64-Recomp](https://github.com/FuzzynessUK/Quest64-Recomp).

## Decisions

| Topic | Decision |
|---|---|
| Target version | NTSC-U 1.2 (Simulation SCUS-94488, Arcade SCUS-94455) |
| Assets | User provides CD images; client extracts/verifies them. No assets distributed. |
| Platforms | Windows, Linux, macOS (macOS covered by CI only) |
| Final runtime | Static recompilation (Option A) |
| Stepping stone | Embedded emulator core with code hooks (Option B), to validate addresses and AP design early |
| Option B core | Beetle PSX (forked, with execution hooks added), embedded in our own frontend |
| Client stack | C++20, CMake, SDL3, Dear ImGui, `apclientpp`. Python for tooling and the AP world. |
| Licenses | Client GPL-2.0-or-later, AP world MIT. Beetle built without lightrec and Vulkan; mbedTLS, not OpenSSL. See [licensing.md](licensing.md). |
| RE / prototype tool | PCSX-Redux (debugger + Lua). Test/prototype scripts are generated and kept in the repo. |

## Architecture principle

Reverse engineering, the virtual CD layer, and the AP game layer are **backend-independent**.
The runtime (emulator core now, recompilation later) sits behind a small interface (read/write game state, code hooks, events) so it can be swapped without touching the rest.

## Phases

0. **Foundations** — repo, CMake, CI (Win/Linux/macOS), disc image reader (BIN/CUE, hash check), ISO9660 + GT2 archive extraction.
1. **Reverse engineering** — Ghidra + PS1 loader + PsyQ signatures, `splat` config and shared symbol map, overlay catalogue (file, load address, loader function), RAM map of game state (credits, garage, licenses, event wins, Arcade unlocks).
2. **Quick prototype** — memory-script on an emulator: detect a race win, grant credits via AP. Throwaway; validates location/item design.
3. **Runtime backend**
   - B (first): own frontend around a GPL-compatible emulator core; hooks on PCs, injected code, both discs via the virtual CD layer.
   - A (target): MIPS → C recompiler (delay slots, GTE, per-overlay sections + overlay-aware dispatch) and runtime (renderer, SPU, CD/XA, MDEC, BIOS, pad, memcard).
4. **AP game layer** — `apclientpp`, location detection hooks, item application, traps, notifications, per-seed saves.
5. **Arcade mode** — second executable + overlays, shared AP state.
6. **AP world** — Python world in the AP repo layout, designed from phase 2 findings.
7. **Custom content** — file overrides through the virtual CD layer, later AP-aware content.

## Notes

- Overlays: each overlay recompiled as its own section at its load address; runtime tracks the loaded overlay per region by hooking the loader.
- Licensing: avoid DuckStation code (non-commercial/no-derivatives since 2024). Beetle PSX / PCSX-Redux are GPLv2.

## Open questions

- None currently.
