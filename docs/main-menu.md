# Main menu (OVL1)

How the Simulation disc (`SCUS_944.88`) title overlay works, from `MainMenu_Run` down: the menu framework, each menu page, and the resident code they share (list widget, kanji font, memory-card UI, sound, CD reads). Found statically in Ghidra; nothing here has been tested in game yet. Open points are under [To verify](#to-verify).

Addresses are in the OVL1 space (`0x80010000–0x8004c8c3`, BSS `0x800a8d5c–0x801c93af`) unless they are resident (`0x8005d600`+, or above BSS). Every function reachable from `MainMenu_Run` is named in the Ghidra project; this page only covers the structure.

## Flow

`MainMenu_Run` (`0x80011384`) is the overlay entry point:

1. `MainMenu_LoadCommonAssets`. On first boot only (`g_Flow_IntroShown`, `0x801ef5f0`): load the strings and run a menu on `g_Page_AutoLoad`.
2. Load the strings, `Options_Apply`, load the title assets.
3. `g_Flow_GameMode` 2 (back from a Replay Theater replay) goes straight back to the Replay Theater. Any other non-zero mode (back from a demo race) restarts OVL1 at `MainMenu_StartNextDemo` (`Overlay_StartAt(1, …)`).
4. Loop: a menu on `g_Page_MainMenu`, then a menu on the root page of `g_Flow_MainMenuChoice`:

| Choice | Page | Then |
|---|---|---|
| GT Mode | none | `Overlay_Start(4)`, `g_Flow_GameMode` 3 |
| Replay Theater | `g_Page_ReplayTheater` (`0x8004b2cc`) | If the menu leaves with `BaseMenu.result != 0`: `Race_PrepareStart`, then `Race_Entry` (OVL0 `0x80011f64`) with arg 1 |
| Options | `g_Page_Options` (`0x8004bfd4`) | `Options_Apply` |
| Save (also the `default` case) | `g_Page_SaveGame` (`0x8004b4c4`) | |
| Load | `g_Page_LoadGame` (`0x8004b56c`) | |
| Data Transfer | `g_Page_DataTransfer` (`0x8004c4a0`) | |
| Demo replay | none | Main menu idle for 900 frames (15 s): loads demo `g_Flow_DemoReplayIndex` (wrapping at `DemoReplay_Count`) and plays it like a theater replay |

## Menu framework

- **`BaseMenu`** (0x220 bytes, built on the stack for each menu) is a `psxViewLoopDoubleBuffer` with 6 draw layers, both pads and a stack of up to 16 pages. `ScreenViewLoop::Execute` runs it until it returns 0 or 1. `g_CurrentMenu` (`0x800a8d68`) points to the running one.
  - `Enter` loads `arc_fontinfo` into `g_OVL1_FontInfo` and builds the 4 fonts, opens the pads, sets the card slot statuses and the kanji font, then calls `enter(0)` on the top page.
  - `Update` reads the pads, runs the leaving page during a transition, then the top page's `update(1)`.
  - `Draw` cross-fades the leaving page (layer 3) and the top page (layer 2) over 16 frames.
- **`MenuPage`** (0x14 bytes): `enter(back)`, `update(arg)`, `draw(layer)`, title colour, title. `update` returns:
  - 1: a page was pushed (`PushPage`, `0x800122a0`); the menu calls its `enter(0)`.
  - 2: back. `PopPage` runs, then `enter(1)` on the page below; an empty stack ends the menu.
  - 3 or 4: leave the menu.
- **`RootMenuPage`** (0x1c bytes) adds a delay and the `back` flag. Every main menu choice starts on one. On `enter`, the root page waits 16–20 frames and starts the music (`Music_PlayTrack`). It then pushes the real page; when that page returns (`back = 1`), it returns 4.
- **`MenuList`** (0x30 bytes, resident `MenuList_*`, `0x8006cdcc`–`0x8006d50c`) is the vertical list used by most pages. A callback `int fn(MenuListEventId, MenuListEvent *, index)` does the per-row work. Its events are init, reveal, close, update, draw, cursor leave/enter, open, and is-enabled (also asked on confirm).
  - Up/Down move the cursor (flag 4 makes it wrap); Left/Right jump a page (flag 0x10).
  - Cross or Circle confirm; Triangle or Square cancel.
  - `MenuList_Update` returns the confirmed index, -1 cancel, -2 nothing, -3 cursor moved, or -4 confirm on a disabled row.
- **`MenuSprite`** (0x14 bytes, `0x80016394`–`0x80016410`) is a menu row drawn as an image (`SpriteDesc`). It zooms in when it opens and collapses when it closes. The main menu, Replay Theater and Data Transfer rows use it, with one sprite table per language.
- **Sounds:** `Sfx_PlayMenu(id)`: 0 error, 1 skip, 3 confirm, 4 back, 6 cursor.

## Strings

OVL1 holds two gzip-compressed string blocks, each with 7 language blocks (JPN, USA, ENG, FRA, GER, ITA, SPA). The strings sit at the same offset in every language block. Each loader copies the block of `g_GameState.language` to a fixed address:

| Block | Source (gzip) | Size per language | Copied to | Labels |
|---|---|---|---|---|
| A (`MainMenu_LoadStringsA`) | `0x80021104` | 0xe47 | `0x801b9630` (OVL1 BSS) | `s_*` |
| B (`MainMenu_LoadStringsB`) | `0x80022d80` | 0x7a9 | `0x801ef6b0` (above BSS, survives overlay switches) | `sB_*` |

The labels carry the USA text as comments. The Japanese block is Shift-JIS. Japanese text is drawn with the kanji font: `KanjiFont_*` reads glyphs from the BIOS kanji ROM through `Krom2RawAdd`, caches them in VRAM, and re-uploads them every frame. Other languages use the 4 bitmap fonts.

## Pages

### Auto load (first boot)

`g_Page_AutoLoad` (`0x8004b900`, `AutoLoad_*`) only tries memory card slot 1:

1. It looks for `BASCUS-94455GAME`. If there is no card or no file, it ends quietly.
2. It reads the file into `g_SaveFileBuf` (`0x800a8e10`), with a progress bar.
3. It checks the CRC and, if it matches, restores `g_GameState` (`GameState_ReadSave`).
4. It shows "Auto Loading Complete", "Save Data is Corrupt!" or "Loading Data Failed" for 3–4 s; any face button skips the message.

The save file (`GameSaveFile`, 0x7ea0 bytes, 0x7f00 on the card, 4 blocks):

| Offset | Contents |
|---|---|
| `0x0000` | `McFileHeader` (0x200): `SC`, icon type, block count, Shift-JIS title, CLUT, 3 icon frames |
| `0x0200` | First 0x7c9c bytes of `g_GameState` |
| `0x7e9c` | CRC-32 (`Crc32`, reflected, table at `0x800a6acc`) of bytes `0x0000–0x7e9b` |

### Main menu

`g_Page_MainMenu` (`0x8004bc78`, `MainMenu_*`) is an 8-row `MenuList` with a 3-row window (`g_MainMenu_List`, `0x8004bc28`). Rows 0 and 7 are empty padding; the cursor is clamped to rows 1–6. `g_MainMenu_RowChoice` (`0x8004bc04`) maps each row to a `MainMenuChoice`. Confirming a row also stores whether START was held, in `g_Flow_MainMenuStartHeld` (`0x801ef601`); nothing in OVL1 reads it.

### Save / Load

`g_Page_SaveGameCard` and `g_Page_LoadGameCard` only set up the shared memory-card UI (see below) with `g_OVL1_CardMenuConfig` (`0x8004b1a0`). Then they call `CardMenu_StartSave` or `CardMenu_StartLoad`.

### Replay Theater

`g_Page_ReplayTheaterMenu` (`0x8004b374`) has 4 sprite rows. They push `g_ReplayTheater_SubPages` (`0x8004b158`):

- **Load Replay** (`LoadReplay_*`): card UI, play-replay mode. On success it sets `BaseMenu.result`, which makes `MainMenu_Run` start the replay.
- **Rename & Delete** (`RenameDeleteReplay_*`): card UI, edit-replay mode.
- **Copy Replay** (`CopyReplay_*`): copies replay files from memory card 1 to card 2. This uses an OVL1 copy engine (`ReplayCopy_*`, context `g_ReplayCopy` at `0x800a8dd8`, 21 states in `g_ReplayCopy_States` at `0x8004b7d8`), not the shared card UI.
- **Demonstration** (`DemoSelect_*`, `DemoStart_*`): picks one of the demo replays on the disc and loads it with `DemoReplay_Load`.

### Options

`g_Page_OptionsMenu` (`0x8004c07c`) is a tab bar (`TabSelector_*`) with 5 tabs: Global, Race, Key Configuration, 1P Analog Settings and 2P Analog Settings.

- **Global and Race tabs:** option lists. `Option_Get`/`Option_Set` map option ids 0–6 to `g_GameState.options`, 7–13 to `options2`, and 14 to vibration.
- **Key Configuration** (`KeyConfig_*`): presets, layouts and per-button assignment, including NeGcon codes.
- **Analog Settings** (`AnalogSettings_*`, `SteerCalib_*`, `PedalCalib_*`): NeGcon steering and pedal calibration (centre, lock position, margin).

### Data Transfer

`g_Page_DataTransferMenu` (`0x8004c548`) has 3 sprite rows. All three read another save through an OVL1 card engine (`DataXfer_*`, context `g_DataXfer` at `0x800b1588`, 12 states in `g_DataXfer_States` at `0x8004c878`):

- **Trade** (`DataTransferCarList_*`, `DataTransferBuyCar_*`): lists the cars in another GT2 save's garage and buys them with your credits.
- **Mix Records** (`DataTransferMix_*`): merges another GT2 save's records into yours.
  - Course records keep the lower time (`Mix_MergeCourseRecords`).
  - License times are merged by `Mix_MergeLicenseTimes`.
  - The three machine-test rankings go through `MachineTest_InsertRecord` (8 places).
- **Convert** (`DataTransferConvert_*`): reads a GT1 save (`BASCUS-94194GT`, checked by `Gt1Save_Check`). `Convert_ApplyGt1Licenses` then gives every GT2 B (and then A) license test at least `MEDAL_KIDS` if the matching GT1 license is complete.

### Unused pages

These page tables are never referenced on this disc:

- `g_Page_LangSelect` (`0x8004b954`): a 5-language list over a background image, probably left over from the PAL build. Confirming a language pushes `g_Page_LangSelectFadeOut`.
- `g_Page_OptionsUnusedWait` (`0x8004c028`) and `DataTransferUnusedWait_*` (`0x8001dbc4`): delay-only pages.

## Shared memory-card UI (resident)

`CardMenu_*` (`0x8007284c`–`0x80073010`) is the save, load and replay screen code that several overlays share. Its context is `g_CardMenu` (`0x801c94ac`, `CardMenu`, 0xa6ac bytes, in the caller's work buffer).

- `CardMenu_SetState` picks a handler from `g_CardMenu_States` (`0x800921d4`, 38 states). Each handler is called with event 0 (enter), 1 (update) or 2 (draw), and returns the next state or -1 to stay.
- `CardMenu_Update` returns 1 when the user leaves (state code 0x27) and 2 when a replay was loaded to play (0x28).
- Modes (`CardMode`): save replay, play replay, edit replay, load ghost, save game, load game.

Below it:

- **Replay file** (`BASCUS-94455REPLAY`): a directory (`ReplayDir`, 0x1580 bytes, entries `ReplayDirEntry`, 0x5c bytes) plus a sector map (`SectorMap_*`) of the data blocks. CRCs cover the directory and each entry.
- **Card driver** (`McCard_*`, `McOp_*`, `McPort_*`): asynchronous operations on `g_McPorts` (`0x801f0724`, 2 × `McPort`, 0x264 bytes each). `McCard_StartTransfer` reads (cmd 6) or writes (cmd 7). Writes are read back to check them, bad sectors are remapped, and each operation is retried up to 10 times. `McCard_GetResult` returns 1 while busy, then the result.
- **Card slot status:** `CardSlots_Update` maps each port's driver state (`McPort +0x25d`) to the menu status in `g_CardSlotStatus` (`0x800a8d64`): 0 ready, 1 checking, 2 no card, 3 not formatted, 4 error.

## Race and demo start

- **`g_RaceSetup`** (`0x801d585c`, `RaceSetup`, 0x58c bytes, just after `g_GameState` and not in the save) describes the next race: `game`, `raceType`, event code, course, and up to 6 `RaceCar`. Each `RaceCar` has a car id, colour, `CarSetup` and driver type: 1 CPU, 2 recorded car, 3 player 1, 4 player 2.
- **`MainMenu_StartNextDemo`** (`0x8001172c`) builds an arcade race for the attract demo:
  - It loads the arcade data and demo setup number `idx`.
  - `RaceSetup_BuildArcade` keeps the loaded setup.
  - `Demo_PickRandomCar` picks cars that are available and not already in the race, preferring saturated colours.
  - It then enters `Race_Entry` with arg 1.
- **`Race_PrepareStart`** (`0x80010edc`) runs before a theater replay starts. It reloads the event data and rebuilds every car's parameters (`CarParams_BuildFromSetup`), choosing what to load from `game` and `raceType`.

## To verify

- The slot status values come from how the card states branch. Check them in game with no card, with an unformatted card and with a corrupt card.
- The meanings of `RaceSetup.game` and `raceType` are inferred from the code that sets them. Race types 1, 2, 10 and 0xb (the recorded dirt events) are tentative.
- In `Race_PrepareStart`, game 0 stores `RaceSetup_ReloadArcade`'s 0/1 result in `g_EventDataBuf`. That looks like a decompiler register mix-up.
- `KeyConfig.analogMap` and the `g_KeyConfig_Presets` layout are tentative.
- The GT1 save offsets used by `Convert_ApplyGt1Licenses` (+0x2b5c → B, +0x2b64 → A) come from the code order only.
- No struct type yet for `g_ReplayCopy`, `g_DataXfer`, or the 0x1c0-byte car parameter block.
- These functions are called only from OVL0 and are still unnamed:
  - `FUN_80072e7c` and `FUN_80072eb4` start the save-replay and save-ghost card modes.
  - `FUN_800724f8` and `FUN_80072598` fill the new replay entry for those modes.
  - `FUN_80069f28` copies the ghost car parameters into `g_RaceCarParams`.
