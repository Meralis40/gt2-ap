# Arcade mode (OVL2, OVL3) and movies (OVL5)

How the Simulation disc (`SCUS_944.88`) runs the arcade mode menus (OVL2), the small launcher that builds races from GT Mode garage cars (OVL3), and the movie player (OVL5). Found statically in Ghidra; nothing here has been tested in game yet. Open points are under [To verify](#to-verify).

Addresses are in the space of the overlay being described (all three load at `0x80010000`, BSS `0x800a8d5c–0x801c93af`) unless they are resident (`0x8005d600`+, or above BSS). Every function in OVL2, OVL3 and OVL5 is named in the Ghidra project; this page only covers the structure. Functions that are copies of OVL1, OVL4 or resident code have the same name with an `_OVL2` suffix in OVL2; in OVL3 they keep the OVL4 name.

## Flow

`ArcadeMode_Run` (`0x80011750`) is the OVL2 entry point, started from the title menu (OVL1). It loads the strings (`ArcadeMenu_LoadStrings`: one of 7 language blocks of 0x314 bytes, inflated to `g_OVL2_Strings`), then runs an `ArcadeMenu` on the root page until it ends. `ArcadeMenu::Update` turns the result of the active page into `g_ArcadeMenuExit` (ram `0x801ef5f4`), which `ArcadeMode_Run` then acts on:

| `g_ArcadeMenuExit` | Page result | Next |
|---|---|---|
| 0 | 4 | Back to the title (`Overlay_Start(1)`) |
| 1 | 3 | Race: `ArcadeRace_Setup`, the `ArcadeRacingMenu` loading screen, then OVL3 |
| 2 / 3 | 5 / 6 | Bonus item: `Movie_ArcadeEntry` (OVL5) with 1 / 0 |
| 4 | | OVL1 entry `0x8001172c` |

For a race, the 0x2d4-byte race request (`g_ArcadeRaceReq`, OVL2 `0x801c3350`, filled by the pages as the `g_ArcSel_*` fields) is copied to `0x801d5fa0` (`g_RaceCarWorkAlt`, which survives the overlay switch) and OVL3 starts. OVL3 copies it back and starts the race overlay with `Overlay_StartAt(0, Race_Entry, 0)`.

### Race request

| Offset | Field |
|---|---|
| +0 | Level (`g_ArcSel_Level`) |
| +1 | Class: 0–3 S/A/B/C, 4 home garage, 5 guest garage, 6 rally |
| +2 | Race type: 0 two players, 4 single race, 6 time trial (and rally) |
| +4 | Laps (from `g_GameState.options[1]`, `options[4]` for 2P) |
| +6 / +8 | Garage (-1 arcade car list, 0 home, 1 guest) / garage slot |
| +0x10, +0x14, +0x16 | Car, racing modification, colour |
| +0x1c, +0x1c0, +0x244 | Player car setups, written by `ArcadeRace_Setup` |
| +0xa0… | Player 2: garage, slot, car, setting, colour, transmission |
| +0xb8 / +0x1b8 / +0x1bc | Course name / name hash / record index |
| +0x2c8 | Use garage cars (OVL3 builds them) |
| +0x2cc | Dirt course (forces dirt tyres) |
| +0x2d0 | TIM for the time trial loading screen |

### Race setup (`ArcadeRace_Setup`)

It fills `g_RaceSetup` from the request:

- **4 single race:** event `g_ArcadeEventCodes[level * 4 + class]`, built by `RaceSetup_BuildArcade_OVL2` (6 cars, random opponents), and the player car from the arcade car list or a garage. `g_ArcadeLevelByte[level]` goes to `RaceSetup.pad57f`.
- **0 two players:** 2 cars, each from the car list or a garage.
- **6 time trial:** 1 car, 100 laps, the best-lap car name from `CourseRecord_GetCurrent`.

It then fills `g_ArcadeLoadInfo` (`0x801c31f0`) for the loading screen: two random seeds, race type, car count, names and car colours. `ArcadeRacingMenu` shows it, with the t002n model (`g_ArcadeT002nModel`) drawn as a body only and one of 4 camera presets.

## Menu framework

`ArcadeMenu` (0x298 bytes, partly typed) derives from `psxViewLoopDoubleBuffer` like OVL1's `BaseMenu`, but its layout from +0x1c4 differs: the current page is at +0x1c8, the pad at +0x1a4, the CD stream reader at +0x224 and a CD lock byte at +0x226.

- `Enter`: double-buffered view (packets `0x801a31e0`, OT `0x8018f860`), 6 layers, the background images per language, the arcade game data (`GameData_LoadArcade` into `g_EventDataBuf`), `CarInfoA`, the t002n model, two `ArcCarView`s and two `ArcLogo`s, the preview movie, fonts, pads, card slots and kanji font. Then `enter(0)` of the root page.
- `Update`: pads, card slots, `CarLogoArc_Update`, the frame hook, the leaving page during a transition, then the active page. Results: 1 enter the pushed page, 2 pop (16-frame vertical slides), 3–6 leave (see [Flow](#flow)). Then the logo, car view, course map and movie updates.
- `Draw`: the active page and the leaving one (`ArcadeMenu_DrawPage`: title, underline and a banner in the page colour), the background tile and the movie frame.
- `BeginFrame`: the hook, kanji glyph uploads, dirty car colours and one pending logo upload per frame.
- Page stack: `ArcadeMenu_SetRootPage`, `PushPage`, `PopPage`; hooks: `SetHook`, `CancelHook`. Same as OVL1.

### Pages

Each page (`ArcPage`, 0x54 bytes) is an OVL1-style page header (enter/update/draw, colour, title) followed by work words. Update returns 0 stay, 1 push, 2 back, 3–6 leave.

```
Root ─ ARCADE MODE ─┬─ GAME SELECTION 1P ─┬─ race ─ LEVEL ─ CLASS ─┐
                    │                     ├─ rally ─ rally CLASS ───┤
                    │                     └─ time trial ─ CLASS ────┤
                    │                                               ├─ CAR SELECTION ──────┐
                    │                                               └─ HOME / GUEST GARAGE ┤
                    ├─ GAME SELECTION 2P ─ 2PLAYER BATTLE ─────────────────────────────────┤
                    │                                                                      └─ COURSE SELECTION ─ RaceLaunch (3)
                    ├─ BONUS ITEMS ─ BonusSelect ─ BonusLaunch (5 / 6)
                    └─ LOAD GUEST GARAGE
```

| Page (`g_ArcPage_*`) | Handlers | Notes |
|---|---|---|
| Root | `ArcadeRoot_*` | First entry: arcade music, garage summaries, course lists, car unlocks |
| ARCADE MODE | `ModeSelect_*` | 1P, 2P, bonus items, load guest garage (`g_ArcModePages`) |
| GAME SELECTION 1P | `Game1PSelect_*` | Race type `{4, 6, 6}` for race / rally / time trial; picks the course set |
| GAME SELECTION 2P | `Game2PSelect_*` | Race or rally, race type 0 |
| LEVEL SELECTION | `LevelSelect_*` | 3 levels (`ArcLevel_HasFourthLevel` is always 0) |
| CLASS SELECTION | `ClassSelect_*`, `RallyClassSelect_*` | S (hidden until a class S car is unlocked), A, B, C, home, guest; rally: rally cars, home, guest |
| CAR SELECTION | `CarSelect_*` | Car picker (locked cars disabled), colour, TRANSMISSION AT/MT, SETTINGS Racing/Drift |
| HOME / GUEST GARAGE | `GarageSelect_*`, `GarageList_*` | Garage car list (rally: dirt-tyre cars only), transmission |
| 2PLAYER BATTLE | `Battle2P_*` | Two of everything; per-player step machine in `Battle2P_UpdatePlayer` (`Arc2PPlayer`) |
| COURSE SELECTION | `CourseSelect_*` | Course picker (2 rows when a reverse list has courses), preview movie, map, records |
| RaceLaunch | `RaceLaunch_*` | 24-frame delay, then 3 (race) |
| BONUS ITEMS | `BonusItems_*` | Courses whose license is earned, with medals; "Obtain Licences in Disk 2…" |
| BonusSelect / BonusLaunch | `BonusSelect_*`, `BonusLaunch_*` | Two items: the first needs every bonus course won, the second stats byte 0x15d |
| LOAD GUEST GARAGE | `LoadGuest_*`, `GuestLoad_*` | Memory card loader, see below |

### Cars and courses

- **Car lists** (`g_ArcCarList_*`): per class S, A, B, C and rally, the count, car codes, availability, default colours, maker logos and ratings. Class S and rally cars unlock from arcade results (`ArcCarList_UpdateUnlocks`). `CarRatingTable_Build` gives every car of the two lists a rating (`ArcCarRating`): weight, power, tyres and a power class from `CarRating_WeightPowerScore` (the same code as OVL4 `Battle_WeightPowerScore`).
- **Course lists** (`g_ArcCourses_*`): race, race reverse, time trial, time trial reverse, rally, 2P, 2P rally. `g_Arc_CourseSet` picks one (0 race, 1 time trial, 2 rally, 3 2P, 4 2P rally). A course is available when its license is earned; reverse courses also need bit 2 of their arcade result.
- **Guest garage:** `GuestLoad_*` is a copy of OVL1's DataXfer card loader (states 0–10 in `g_GuestLoad_States`). State 7 reads a GT2 save, checks its CRC and copies its garage to `g_GameState.garages[1]` (`SaveFile_CopyGarageToPlayer2`, resident `0x8006a2b4`).

### Widgets

Older copies of the resident widgets, with their own layouts (types in `/GT2/Arcade`):

- `TextItem_*_OVL2` (`ArcTextItem`, 0x2c bytes, x/y at +0x10), `IconItem_*` (`ArcIconItem`, a sprite with zoom and pulse), `MenuItem_*_OVL2` / `MenuList_*_OVL2` (lists of text or icon items, not callbacks), `Dialog_*_OVL2` (two-option dialog, like `CardDialog`).
- `GridPicker_*`: 2D picker (rows × items with a slide animation and a draw callback), used for cars, courses and the 2P pickers.
- Car spec panel (`CarSpecPanel_*`, `g_ArcCarSpecPanel`, 0xc80 bytes): two slots that alternate, so one car closes while the next opens. Each slot has the name, an icon, a power graph (`PowerGraphView_*_OVL2`, curves sampled every 250 rpm), 3 `StatBar` 10-segment meters, `ColorChips` and `SpecTable` (weight in lb, power in hp, torque in ft-lb).
- `ArcCarView` (0x448 bytes): a 3D car preview streamed from CD and inflated a few kilobytes per frame from a menu hook (`ArcCarView_DecodeHook`), from the arcade list or a garage car, with an optional reflection.
- `ArcLogo` (0x2c bytes): double-buffered maker logos, from the logo archive or read from CD.
- Course select: `CourseInfoPanel` (two numbers of the course entry), `CourseRecordPanel` (4 sector times, total, car and driver), `CourseMap` (image streamed from `VOLID_ARCADE_COURSE_MAP`) and `CoursePreviewMovie`. The map and the movie share the CD lock at menu +0x226.
- Music: `ArcadeBgm_*` loads sound bank 0xeb and `VOLID_SOUND_ARCADE_SEQ` through `MenuBgmBank_*_OVL2`.

The course preview movie uses a copy of the OVL5 movie library (`Movie_*_OVL2`, `0x8002589c–0x8002665f`) with a 2-frame ring, decoding to VRAM `0x280,0x100` (`StrPlayer_Poll`). Its player is at `0x800efd60`, and the stream table at `0x80053508`.

## OVL3: garage cars for arcade races

OVL3 exists because the GT Mode car code (copies of OVL4 `CarWork_*`, `Car_StoreSetup`, `RaceSetup_SetCarFromGarage`) does not fit next to the OVL2 menus. Its entry `ArcadeGarageRace_Entry` (`0x80012c00`):

1. Copies the race request from `0x801d5fa0` to `g_Ovl3_RaceSel`.
2. If +0x2c8 (use garage cars) is set, loads the GT Mode data (`GameData_LoadGtMode` into `g_Ovl3_GtData`) and `ArcadeSel_SetupRaceCars` puts the chosen garage cars into `g_RaceSetup`:
   - 0 two players: each player with a garage gets `ArcadeSel_LoadGarageCar(garage, slot, dirt)`, driver types 3 and 4.
   - 4 single race and 6 time trial: car 0, driver type 3. A time trial records the garage and slot in setup +0x582 / +0x584, and the car is also loaded into `g_RaceCarWorkAlt`, where `Race_Entry` takes the player `CarWork` from.
   - `ArcadeSel_LoadGarageCar` fits dirt tyres (tyre slots 3 and 4 at stage 7) for a dirt course, or puts stock tyres back, and stores the result in the garage car.
3. Starts `Race_Entry`.

`RaceSetup_InitEvent`, `RaceSetup_SetCarFromEntry` and `RaceCarPick_Random` are dead copies here.

## OVL5: movie player

`Movie_DefaultEntry` (`0x800114b8`, the default entry) plays movie 0x18 and then starts OVL1. `Movie_ArcadeEntry` (from the bonus items) plays movie 0x1a (which = 0) or 0x19, then also starts OVL1.

| Function | Movie | Buffers | Display mode | vsync interval | Notes |
|---|---|---|---|---|---|
| `Movie_Play18` | 0x18 | 3 | 0x31 | 2 | y + 24 |
| `Movie_Play19` | 0x19 | | 0x33 | | |
| `Movie_Play1A` | 0x1a | 2 | 0x13 | 4 | y + 8, can't skip |

### `psxMovieLoop` (0x158 bytes)

- `Enter`: frame ring of 32 buffers of 0x46e0 bytes from `0x800a8d60` (`g_MovieRing`), two run-length buffers of 0x20000 bytes (`g_MovieVlcBufA/B`), two MDEC column buffers (`g_MovieSliceBufA/B`), then `Movie_Open(movieIndex)` and `Movie_Start`.
- `Update`: Start skips (unless `noSkip`). On a ready frame, after pre-buffering (`startDelay`, 2 frames in triple-buffer mode) and while fewer than `bufferCount - 1` frames wait for display, `Movie_Step` decodes into the next display buffer. End of stream returns 0. A stall (status 3, or busy for more than 900 vblanks) calls `RestartRead` (vtable +0x48).
- `UpdateDisplay`: every `vsyncInterval` vblanks, shows the next decoded buffer and turns the display on after the first frame. `OnFrameDecoded` (vtable +0x4c) counts decoded frames.

### Pipeline (`MoviePlayer`, 0x50 bytes)

1. **CD stream** (`MovieStream`, 0x1c bytes): `Movie_Open` takes the start sector from `g_MovieSectors` (ram `0x80092390`, relative to `g_StreamLba`), minus 0x19 trailing sectors. `Movie_SetCdRange` sets up a `g_CdAudio` stream request (mode 0xc0, channel 7) with a per-sector callback. `MovieStream_ReadSector` checks the 0x20-byte sector header (status 0x160, type `0x5349` "SI"; sector of frame, sectors per frame, frame number, last frame) and appends the 0x7e0 data bytes to the current ring frame, dropping a frame on a sector gap. While the MDEC is busy, sectors are deferred and read in the MDEC callback.
2. **Decode** (`Movie_DecodeVlc`): a Polyphony format, not the standard STR bitstream. The per-macroblock DC and quantiser bytes (6 per 16×16 macroblock) are LZ-compressed (resident `Lz_Decompress`), and the AC codes are a Huffman bitstream decoded with the GTE leading-zero counter and the table at ram `0x800114cc`. The output is MDEC run-length codes, padded with `0xfe00`.
3. **MDEC** (`MdecOut`, 0x18 bytes): `DecDCTin` of the frame, then `DecDCTout` one 16-pixel column at a time (24-bit output for movies). Each finished column is uploaded to VRAM from the MDEC callback; when the last upload's GPU DMA ends, the hook calls `onFrameDone`.
4. `Movie_Step` starts the MDEC on frame n, then decodes frame n + 1 on the CPU while the MDEC runs.

## To verify

- **No struct yet** for the 0x2d4-byte race request (fields are in the label comment at `0x801c3350` and in `g_ArcSel_*`), `CarSpecPanel`, `CourseMap` or `GridPicker`. `ArcadeMenu` past +0x294 is unknown.
- **Duplicate movie types:** `ArcStrRing` / `ArcStrDecoder` are the same as `MovieStream` / `MdecOut`; `ArcStrPlayer` (0x5c) and `MoviePlayer` (0x50) differ.
- **Rally race type:** the 1P race types are `{4, 6, 6}`, so a rally would use race type 6 like a time trial.
- **Unknown meanings:**
  - The two numbers of `CourseInfoPanel` (course entry +0 and +4).
  - `SpecTable_Set` spec[1] (power format) and spec[3] (torque suffix).
  - `g_ArcCarList_Colors` (assumed default colours), `g_ArcCarList_Spec3`.
  - `ArcBonus_IsStat349Set` (stats byte 0x15d) and what the two bonus items are.
  - `g_ArcadeLevelByte` (copied to `RaceSetup.pad57f`).
- **Movies:** what movies 0x18, 0x19 and 0x1a are, and the number of entries in `g_MovieSectors`.
- **Loading screen:** why only the body of the t002n model is drawn.
- **Resident leftovers:** `g_InitRange` (`0x800a6f18`) and `0x801c93c3` are written at boot but never read; what controller command 0x4b (`PadCmd_4B`) does.
