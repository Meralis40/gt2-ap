# GT Mode (OVL4)

How the Simulation disc (`SCUS_944.88`) GT Mode overlay works, from `GTMode_Run` down: the menu loop, how pages are loaded and drawn, shops, how events are started, and the resident code it uses (car model renderer, menu music, CD streaming). Found statically in Ghidra; nothing here has been tested in game yet. Open points are under [To verify](#to-verify).

Addresses are in the OVL4 space (`0x80010000–0x80052a73`, BSS `0x800a8d5c–0x801c93af`) unless they are resident (`0x8005d600`+, or above BSS). Every function reachable from `GTMode_Run` is named in the Ghidra project; this page only covers the structure.

## Flow

`GTMode_Run` (`0x80013628`) is the overlay entry point:

1. Load the GT Mode data:
   - The car spec strings and the car info, with the car colours.
   - The used cars of the current 10-day period (`UsedCar_LoadPeriod`).
   - The event data (`g_EventDataBuf`) and the race data (`g_GtModeRaceDataBuf`).
   - The entry rules of every event (`EventRules_BuildAll`).
2. Run `GTMenu` (the simulation map and every shop and event page) until it quits.
3. Run `GTLoadingMenu`: 12 frames of a dark grey screen.
4. Act on `g_Flow_GtMenuExit` (`0x801ef5f5`):

| Exit | Then |
|---|---|
| 2 | Start the selected event (`g_GtMenu_SelectedEvent`, `0x800c1c10`), see below. `Overlay_StartAt(0, 0x80011f64)` with `g_Flow_GameMode` 3 and `g_Flow_HomeOverlay` 4 |
| 3 | `GTMode_SetupEventRace` with no player car (raceType 0xc), then OVL0 as above |
| 4 | OVL1 at `0x8001172c` (`MainMenu_StartNextDemo`) |
| other | `Overlay_Start(1)`: back to the main menu, `g_Flow_GameMode` 0 |

Exit 2 depends on the event:

- **License test** (`Event_IsLicense`): day + 1, load the prize info and the license data, then `GTMenu_StartLicenseTest` fills `g_RaceSetup` (`RaceSetup_InitLicenseTest`).
- **Machine test** (`Event_GetMachineTestType` ≥ 0): `RaceSetup_InitMachineTest`. Its opponents come from `g_GameState.machineTests`. `g_Race_EventIndex` is -1, so no event result is stored.
- **Race:** day + 1, `ChampRaceList_Build` (`g_ChampRaceList`, `0x801d5df4`: the race codes and courses of a championship), `Event_LoadPrizeInfo` (`g_EventPrize`, `0x801d55ac`: the championship prize, 6 place prizes and up to 4 prize cars). Free races (`FREERACE*`, `FREECHAMP*`) then run `GTBattleMenu` to pick the opponents. Last, `GTMode_SetupEventRace` (`0x80013108`) fills `g_RaceSetup`.

The race data is reloaded before every start, because `GTMenu` and `GTLoadingMenu` use `g_OVL4_SharedBuffer` (`0x80024430`) as their packet buffer.

### Event race setup

`GTMode_SetupEventRace` fills `g_RaceSetup` from the race data:

- A course of `none` picks a random one (`Course_PickRandom`).
- **Free race:** 6 AI cars drawn from the opponent pool, and `RaceSetup.maxPower` is the player's power + 5.
- **Dirt event** (`Event_IsDirt`): 1 car against a recorded car (files `DON0001` etc., `GhostEvent_ApplyFile`), raceType 10. The player's `CarWork` then goes to `g_RaceCarWorkAlt` (`0x801d5fa0`) instead of `g_RaceCarWork` (`0x801da4b8`), because the recorded data occupies the second. Both are scratch areas inside other race buffers.
- Unless there is no player car, the current garage car becomes car 0. Its parameters are built into `g_RaceCarParams` (`0x801de8ba`, `CarParams[6]`).

### Free race opponents (`GTBattleMenu`)

`GTBattleMenu` is not interactive. It shows a progress bar while it scores every car of the race data, one car per frame:

1. `BattleTable_LoadNext` builds each car's default setup and parameters and stores its score, weight × 300000 / power². It also stores flags for racing modification, drivetrain and no turbo.
2. `BattleTable_Sort` sorts the cars by score.
3. `BattleTable_PickOpponents` finds the player's score and takes cars below and above it into `g_Battle_Pool` (`0x800b43e8`, `OpponentPool`). It only keeps cars whose racing modification flag matches the player's.

The pool sizes come from `g_FreeRacePoolParams` (`0x800243f0`), by `Event_GetFreeRaceIndex`: 50/20 (FREERACE01), 80/50, 120/80, 120/80.

## Menu loop

- **`GTMenu`** (0x864 bytes, built on the stack) is a `psxViewLoopDoubleBuffer` with a 64 KB packet pool and three draw layers: cursor, page, and car/3D. `g_GTMenu` (`0x800a8d5c`) points to it.
  - `Enter` loads the cursor, item and font TIMs. It also sets up:
    - The shop transaction, and the garage and used car lists.
    - The current car, loaded into `g_CarWork` (`0x800b4490`, `CarWork`, 0x17e8 bytes).
    - The menu music bank.
    - The view (`MenuView_Init`, which enters page `g_Flow_GtMenuPage`, `0x801ef5f8`, and restarts its music `g_Flow_GtMenuBgm`).
    - Then it puts the cursor on the default element.
  - `Update` reads the pads. A pending page change or wheel preview counts down first, and input is ignored meanwhile. Then it acts by list mode:
    - 0: the map cursor.
    - 1: the garage list (`g_GarageList`, `0x800a8d60`). START moves the selected car to the top (`Garage_MoveCar`).
    - 2: the used car list (`g_UsedCarList`, `0x800a8d68`).
    - 3: the car wash. When the spin ends, the car's dirt is cleared.
  - `BeginFrame` links the page background into the OT and runs a pending page change (`MenuPage_Enter`). The screen is blanked for 4 frames after a change.
  - `Draw` draws the map cursor (list mode 0 only), then the page (`MenuPage_DrawElements`).
- **`MapCursor`** (`MapCursor_Update`): the d-pad, and L1/L2/R1/R2 for the diagonals, jump to the best hotspot in that direction. The drawn position follows with a damped spring. Circle or Cross on a hotspot calls `SimMap_Update`. Square or Triangle goes back (`GTMenu_GoBack`).
- **`SimMap_Update`** (`0x80014380`) handles a confirmed element:
  - The car wash costs 50 Cr.
  - Wheels: flag bit 28 starts a wheel purchase.
  - Shop dialogs: `Shop_CheckTransaction`, then `Shop_ExecuteTransaction` on "yes".
  - Any other selectable element (bit 24) goes to its target page after 3 frames. Entering a dealership is this generic path.
- **Menu music** (`MenuBgm_*`): a sequence player. A track change stops the music and starts the new track 2 frames later. The volume comes from `g_GameState.options2[5]`.

## Pages

A page comes from `gtmenudat` (index loaded by `MenuData_Load`, one per language):

- `MenuPage_LoadById` reads it into `g_MenuPageBuf`. Page ids with bit 31 set are message pages: the low bits index the u16 table at `solodata + 4`. `solodata` also holds a sorted table from car id to info page.
- `MenuPage_Load` parses it: sprite groups (12-byte sprites and 0x4c-byte elements), the top-level elements (`g_MenuElements`, `0x801c3150`, up to 64, count in `g_MenuElementCount`), then a footer:
  - `g_MenuPageAttr` (`0x800a8d78`): byte 0 manufacturer, bit 8 keep the back page, bit 9 skip black tiles, byte 2 music track.
  - `g_MenuPageBackPage` (`0x800a8d70`).
  - The background picture id. The picture streams from the CD sector callback straight into VRAM at (0x300, 0) (`BgPicStream`, `g_BgPicStream` at `0x800b9514`). The background is drawn from a prebuilt display list in 16-word chunks.
- `MenuPage_Enter` then pushes the back page (unless bit 8), and sets up the elements that need it:
  - A wheel element shows the current car.
  - Part shop kinds 0x14–0x46 preview the part and prepare its purchase.
  - Kind 9 opens the garage list.
  - Kind 0x50 builds the used car list of the page's manufacturer.

### Elements (`MenuElement`, 0x4c bytes)

The low 16 bits of the flags are the kind. `MenuPage_DrawElements` (`0x8001b9ac`) draws the dynamic part of each element (values, icons, the car); the page image itself is drawn from the background. Its plate comment lists every kind and flag bit. The main ones:

- **Flag bits:**
  - Bit 24: selectable.
  - Bit 25: default cursor position.
  - Bit 28: wheel element.
  - Bit 31: use the number font.
  - Bit 27: the element's target is an event code. Bits 19–23 then draw the best finish, the prize, the power limit, the required license or the won mark.
- **Kinds:**
  - Kind 10 draws the 3D car model into the element's rectangle (`CarPreview_DrawModel`).
  - Kinds 0x53–0x93 are the license test medals from `g_GameState.licenses`, in six blocks: IC, B, A, IB, IA, S.
  - Kinds 0xb2–0xba are the car spec sheet, and 0xbc–0xcd are the status page stats.
- **Entry gates:**
  - `MenuElement_IsActive` decides where the cursor can stop.
  - `Race_CheckEntry` gates a race in this order: car selected, PFL/EPL need the regional leagues won, GTW needs the pro leagues won, dirt tyres, license, drivetrain, turbo, racing modification or stock, max power, allowed cars. Each refusal has its own message page.
  - `License_CheckEntry` requires the previous license.

### Text

Two bitmap fonts: a text font (267 glyphs) and a number font (24 glyphs: digits and unit letters), drawn with `GTText_*`, `GTNum_*` and `GTFont_*`. Values in ps are shown as hp with ×1000/1014. `FormatPrice`, `FormatRaceTime` and the other `Format*` helpers build the strings.

## Shops

Every purchase goes through `g_ShopTxn` (`0x801c3080`, `ShopTxn`, 0x34 bytes):

| Type | Meaning |
|---|---|
| 0 | None |
| 1 | Buy a car |
| 2 | Car bought: make it current |
| 3 | Buy a part (parts 7, 9, 0x12–0x14, 0x20 and 0x22 are installed at once) |
| 4 | Install an owned part |
| 5 | Wheels |
| 6 | Sell a car |
| 7 | Select a garage car (applied at once by `Shop_CheckTransaction`) |

- `Shop_CheckTransaction` returns the page to show: a confirmation or a refusal message (bit 31), or 0 for an error sound.
- `Shop_ExecuteTransaction` carries it out and returns the next page. Afterwards it calls `CarWork_RebuildParams`, which throws its results away.
- `Car_GetSellPrice` always returns 0.

## Car data

- `Car` (0xa4 bytes):
  - `stat94` is the weight, with the drivetrain in bits 13–15.
  - `stat96` is the max torque.
  - `flags98` bits 0–13 hold the max power in ps, and bit 15 means a racing modification is installed.
  - `dirt` (+0xa2) is cleared by the car wash.
- `CarWork` (0x17e8 bytes) holds every stage entry and index per tuning category, plus the installed part slots at 0x17b0.
- `CarParams` (0x1c0 bytes) is the per-race physics block built by `CarParams_BuildFromSetup`.

## Resident code used here

- **Car model renderer:**
  - `CarModel_Draw(cam, inst, mirror)` picks the LOD, culls the bounding box, then draws:
    - The body, fully inside the view (`CarBody_DrawUnclipped`) or crossing it (`CarBody_DrawClipped`, `Poly_Clip`).
    - The wheels (`CarModel_DrawWheel`).
    - The shadow (`CarModel_DrawShadow`).
  - The frustum planes are loaded into the GTE light and colour matrices, which is how `Clip_TestPoints` computes its outcodes.
  - Car textures are loaded into VRAM slots (`Tim_LoadToSlot`), and paint colours are blended into the CLUT (`Clut_Blend`).
  - The disc under the car is `GroundDisc_Draw`.
- **Sound:**
  - `SpuFile_Load` streams a sound bank from CD. The header stays in RAM and is relocated as `INST` (`Inst_Relocate`) or `ENGN` (`Engn_Relocate`); the samples are DMAed to SPU RAM.
  - The sequencer plays `SEQG` songs (0x48 bytes, 16 tracks) through a linked list of `SeqPlayer`s and `g_SeqChannels` (`0x801f0248`, 25 channels).
- **CD streaming:** `Cd_ReadInflate` reads through a ring buffer of sectors (`CdAudio_SetupStream`, `CdStream_WaitSector`, `CdStream_ReleaseSector`) on `g_CdAudio` (`0x801f0510`).

## To verify

- Exit code 3 (an event race with no player car, raceType 0xc): what in the menu triggers it, and what it is for.
- Why `MenuElement` flag bit 30 makes a race exit with 3 instead of 2.
- Element kinds 0xae (refused for 8 car codes, `gb0cn`…) and 0x98 (depends on the page's manufacturer).
- `Car.dirt` is inferred from the car wash only.
- `Car.flags98` bit 14 (set when byte +9 of the stock transmission entry is below 3).
- `PlayerStats` +0x4c (races won?), +0x50/+0x54 (shown as a `%d.%02d` ratio) and +0x58/+0x5c (prize money?).
- `CarWork` 0x84–0x988 is mostly unknown (only `wheelIdx` at 0x688), and so are many `CarParams` fields. Why `g_RaceCarParams` starts at +2 is unclear.
- The car view camera fields at 0x9c–0xd0 (position, rotation, viewport, projection) are only partly named.
- Event entry byte +0x45 (`ChampRaceList.param45`, `RaceSetup.unk0f`), `RaceSetup.unk0d` and `RaceSetup.unk582`.
- The GTE scratchpad layout at `0x1f800000` is not typed (444+ references across overlays).
