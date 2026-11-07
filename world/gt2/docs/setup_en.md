# Gran Turismo 2 Setup Guide

The client is a native program with Archipelago built in. Download it for Windows or Linux with
`gt2.apworld` from the [releases](https://github.com/Meralis40/gt2-ap/releases). The
[Universal Tracker](https://github.com/FarisTheAncient/Archipelago/tree/tracker) can track seeds.

## Requirements

- Your own NTSC-U 1.2 Simulation disc (SCUS-94488).
- [Archipelago](https://github.com/ArchipelagoMW/Archipelago/releases) 0.6.8 or later.

## Generating a seed

1. Put `gt2.apworld` in the `custom_worlds` folder of Archipelago.
2. Generate a template YAML with the Archipelago Launcher ("Generate Template Options"), and edit
   the options of `Gran Turismo 2`.
3. Generate as usual.

## Connecting

1. Run `gt2ap`. The first time, it asks for your disc image (the `.cue`) and remembers it in
   `gt2ap.cfg`; you can also give it on the command line, `gt2ap "<your disc>.cue"`. It opens on
   the Archipelago page.
2. Type the server (`archipelago.gg:12345`, or `ws://localhost:38281`), your slot name and the
   password if there is one. Ctrl+V pastes. Up and Down move between the rows.
3. Choose **Connect**. Each seed and slot gets its own memory card, in the `saves` folder.

## Playing

With a gamepad, the left stick steers. The d-pad still works, and **Options** has the game's own
key configuration, with presets that put the pedals on the right stick.

Press **Tab**, or click the right stick, to pause and see what you've unlocked: manufacturers,
tuning parts, trophies, credits received and checks done.
