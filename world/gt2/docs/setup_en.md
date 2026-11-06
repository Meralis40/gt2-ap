# Gran Turismo 2 Setup Guide

The client is the native port in this repository, which has Archipelago built in. It isn't
released yet, so for now it has to be built from source (see `docs/port.md`). This world generates
seeds, and the [Universal Tracker](https://github.com/FarisTheAncient/Archipelago/tree/tracker)
can track them.

## Requirements

- Your own NTSC-U 1.2 Simulation disc (SCUS-94488).
- [Archipelago](https://github.com/ArchipelagoMW/Archipelago/releases) 0.6.8 or later.

## Generating a seed

1. Put `gt2.apworld` in the `custom_worlds` folder of Archipelago.
2. Generate a template YAML with the Archipelago Launcher ("Generate Template Options"), and edit
   the options of `Gran Turismo 2`.
3. Generate as usual.

## Connecting

1. Run `gt2ap "<your disc>.cue"`. It opens on the Archipelago page.
2. Type the server (`archipelago.gg:12345`, or `ws://localhost:38281`), your slot name and the
   password if there is one. Ctrl+V pastes. Up and Down move between the rows.
3. Choose **Connect**. Each seed and slot gets its own memory card, in the `saves` folder.
