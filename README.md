# GT2 Archipelago

Gran Turismo 2 (Simulation and Arcade Mode) for [Archipelago](https://github.com/ArchipelagoMW/Archipelago). Early work in progress, not playable yet; see [docs/PLAN.md](docs/PLAN.md).

You need your own NTSC-U 1.2 discs (Simulation SCUS-94488, Arcade SCUS-94455). No game data is distributed.

## Building

Requires CMake 3.24+ and a C++20 compiler (MSVC 2022+, GCC 12+, Clang 15+).

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

On Windows without Ninja on the PATH, use the `vs` / `vs-release` presets instead.

## Tools

`gt2-discinfo` inspects and extracts disc images:

```bash
gt2-discinfo info "Gran Turismo 2 (USA) (Simulation Mode).cue"
```

## License

The client is GPL-2.0-or-later ([LICENSE](LICENSE)); the Archipelago world in `world/` is MIT. See [docs/licensing.md](docs/licensing.md).
