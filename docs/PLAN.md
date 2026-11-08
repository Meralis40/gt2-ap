# GT2 Archipelago — Project Plan

Randomizer for Gran Turismo 2 (Simulation + Arcade) for [Archipelago](https://github.com/ArchipelagoMW/Archipelago).
Reference projects: [ctr-native-ap](https://github.com/dowlle/ctr-native-ap) (target result). The route is a native source port built from `decomp/`: see [port.md](port.md).

## Decisions

| Topic | Decision |
|---|---|
| Target version | NTSC-U 1.2 (Simulation SCUS-94488, Arcade SCUS-94455) |
| Assets | User provides CD images; client extracts/verifies them. No assets distributed. |
| Platforms | Windows, Linux/Steam Deck, as x86-64 builds (docs/port.md). macOS is possible but not built |
| Runtime | Native source port, forked into `port/`; guest RAM at its real address |
| Client stack | C++20, CMake, SDL3, `apclientpp`. Python for tooling and the AP world. |
| Licenses | Client GPL-2.0-or-later, AP world MIT. mbedTLS, not OpenSSL. See [licensing.md](licensing.md). |
| RE / prototype tool | PCSX-Redux (debugger + Lua). Test/prototype scripts are generated and kept in the repo. |

## Phases

M0 spikes, boot, splash, main menu, GT mode, race, AP, sound, movies, release. Details and results are in [port.md](port.md).

## Open questions

- None currently.
