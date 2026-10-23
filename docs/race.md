# Race overlay (OVL0)

How the Simulation disc (`SCUS_944.88`) race overlay works, from `Race_Entry` down: the race loops and their state machine, one race frame, the car physics and CPU drivers, the course, timing, the HUD and the post-race menus. Found statically in Ghidra; nothing here has been tested in game yet. Open points are under [To verify](#to-verify).

Addresses are in the OVL0 space (`0x80010000–0x8005d5f7`, BSS `0x800a8d5c–0x801c93af`) unless they are resident (`0x8005d600`+, or above BSS). Every function in OVL0 and every resident function reachable from `Race_Entry` is named in the Ghidra project; this page only covers the structure.

## Entry and race types

`Race_Entry` (`0x80011f64`) is started by `Overlay_StartAt(0, Race_Entry, arg)` from OVL1 (title, demo, theater replays), OVL3 and OVL4 (GT Mode). `arg` 1 means attract demo or watch only: the race is only replayed. It loads the race text (`sR_*` labels at `0x801c6c50`, from `data-race.txd`) and the race font, copies the player's `CarWork` into `g_RacePlayerCarWork`, then runs the loop of `g_RaceSetup.game` (`g_RaceSetup` at `0x801d585c`, saved whole in replays):

| `game` | Function | Race loop |
|---|---|---|
| 0 arcade | `ArcadeRace_Run` | `ArcadeRaceLoop`. Returns to OVL1, or to OVL2/OVL4 depending on `g_Flow_GameMode` |
| 1 GT Mode | `GTRace_Run` | `GranTurismoRaceLoop`. Returns to OVL4 when `g_Flow_GameMode` is 3, else OVL1 |
| 2 development | `RaceDevelopment_Run` | A bare `RaceViewLoop`, no menus |

`raceType`:

| Value | Meaning |
|---|---|
| 0 | 2 players, split screen |
| 1 / 10 | Single-car run (100 laps) before a race or dirt race; its menu switches to 2 / 0xb |
| 2 / 0xb | Race / dirt race (1 car against a recorded car) |
| 3 | License test |
| 4 | Arcade single race |
| 6 | Time trial |
| 7 / 8 / 9 | Machine tests: 0–400 m, 0–1000 m, max speed |
| 0xc | Watch only (forces `arg` 1) |

Other `RaceSetup` fields named here: `frameStep` (1 = 60 Hz, 2 = 30 Hz), `showIntro`, `lapCount` (0 means 100) and `dirt`.

## Race loop state machine

`ArcadeRaceLoop` and `GranTurismoRaceLoop` share the `RaceLoop` layout (0x5e0 bytes) and derive from `RaceMenuLoop`. `RaceMenuLoop::Execute` runs `Init`, `Run` and `Finish`. In `Run`, each state handler returns 1 (exit), a state number (4–13), or 2 / 3 (back / forward; 0 repeats the last direction). The next state comes from the current one:

| State | Back (2) | Forward (3) |
|---|---|---|
| 4 load | exit | 5 |
| 5 | 4 | 8, or 11 with `arg` |
| 8 pre-race | 5 | 9 |
| 9 race (`RaceViewLoop::Start(0)`) | 8 | 10 |
| 10 after the race | 8 | 6 |
| 11 | 5 | 12 |
| 12 replay (`RaceViewLoop::Start(1)`) | 11 | 13 |
| 13 | 11 | 6 |
| 6 menus (`QuickMenu`) | 10, or 13 with `arg` | 7 |
| 7 | 6 | exit |

So a normal race goes 4 → 5 → 8 → 9 → 10 → 6 → 7, and the demo goes 4 → 5 → 11 → 12 → 13 → 6. `GranTurismoRaceLoop::State10` writes the car's dirt back to the garage car through `g_Race_DirtOut` (`0x801de8b8`).

## One race frame

`RaceViewLoop` (0x2f4 bytes) is the race screen. `Enter` sets up the video mode, the draw buffers and 4 draw layers. It loads the cars and the course, sets the intro length (180 frames, 120 for license tests, 0 without `showIntro`), and sets up the cars, the two cameras, the HUD and the pads.

`RaceViewLoop::Update` runs one frame:

1. Read the pads. START opens the pause menu (`PauseMenu_Update`: Continue or Exit); in the demo it leaves the race.
2. Count down the intro, update the particles and resync the ghost.
3. `Race_StepWorld` (`0x8003ebf0`), the physics step:
   - `Cars_UpdateDynamics`.
   - Car/wall collisions (skipped during the countdown).
   - Car-to-car collisions, then `Cars_UpdateOrientation` and `Cars_UpdateTrackPositions`.
   - The race clock (`RaceClock_Tick`).
   - License test end checks, time-trial ghost recording and playback, and car-to-car contacts.
4. The cameras (`RaceCam_Update`: the live camera, or track-side and onboard cameras in a replay).
5. `Hud_Update`, which also decides when the race ends.
6. The race music and car sounds.

The world is `g_RaceWorld` (`0x800a9504`, `RaceWorld`, 0x5d30 bytes). It holds the pads, the 6 race cars (`g_RaceCars`, `0x800a9688`, `RaceVehicle`, 0xb40 bytes each), the draw list, the particles, the countdown, the course and the car count. `g_RaceOrder` (`0x801c8578`) holds the car indices in race order; `RaceOrder_Update` writes each car's place at `RaceVehicle +0x77c`.

The frame rate is `g_FrameRate` (`0x801c8570`): 60, or 30 when `frameStep` is 0 or 2. `g_FrameDt` is 1/fps in 16.16.

## Car physics

Each `RaceVehicle` holds its physics state as `CarPhys` (0x798 bytes) at +0x2c, with 4 `CarPhysWheel` (0x68 bytes) at `CarPhys +0x460`. In functions that take the car array, decompiled offsets are `RaceVehicle` offsets, which are `CarPhys` offsets + 0x2c.

- **Setup:**
  - `CarPhys_Init` (`0x800319a8`) builds the car from its `CarParams`: axle distances and weight split, track widths, the engine torque curve, the drivetrain and gearbox, tyre grip and load curves, brakes, aero, steering limits, springs and dampers.
  - `CarPhys_Setup` places the car on the grid, picks its first checkpoint and racing line, and gives it a rolling start when there is no intro.
  - `RaceVehicle_Init` adds the input log, the start handicap (`RaceSetup.options[6]`) and the dirt.
- **Dynamics** (`Cars_UpdateDynamics`, `0x8003e0c4`, scratchpad work area at `0x1f800000`):
  - Aero, including slipstream, engine torque, tyre grip limit and slip angle.
  - Driver input, the drivetrain (`CarPhys_UpdateDrivetrain`: clutch, drive torque, rolling resistance, tyre forces, speed and engine integration, tyre wear), traction control and brakes.
  - The suspension (spring and damper per wheel, pitch, roll and the body matrix) and the gearbox (automatic or manual).
- **Driver input:** `Car_ApplyControls` turns a `RaceInput` (0xc bytes, recorded in run-length `InputLog`s for replays) into steering, throttle and brake ramps, then runs the clutch logic.
- **Collisions:**
  - Wall collisions use the 4 body corners against the course section walls.
  - Car-to-car collisions (`g_CarColl_Pairs`, `0x801c8740`) put each car's corners into the other car's frame, find the time of impact, apply an impulse weighted by catch-up, and separate the cars.
- **Catch-up (rubber band):** `g_CatchUp` (`0x80046f6c`) comes from the event parameters. AI cars are compared with the player (2 players: with each other), and the factor scales each car's own time step.
- **Units:**
  - Speeds are m/s in 4.12 fixed point (0x472 is 1 km/h).
  - Engine speed is rev/s in 4.12.
  - Angles are 4096 per turn.

`g_RaceEventParams` (`0x801c98a0`) is a copy of the event entry bytes +0x44..+0x83. It holds the rolling start speed, the license test kind and box, tyre wear, catch-up and grip scaling.

## CPU drivers and pits

- **Racing lines:** `g_AiLines` (`0x801c8568`, prepared by `AiLines_Init`) holds the course's lines, as points of 0x28 bytes (straight or arc, position, track segment, surface, distance, radius, slopes). Line 3 is the pit entry, line 4 the pit lane and line 5 the passing hints.
- **`Ai_Drive`** (`0x80037834`):
  - Speed control by corner type and braking for the target speed.
  - Lookahead steering with counter-steer.
  - Slowing behind other cars, and switching lines to pass.
  - Pit lane speed limits.
  - The driving parameters per drive class are `g_AiDriveParams`.
- **Pits** (`Pit_Update`, courses with a pit lane): a car enters when its tyres are worn or dirty, stops 5 s in its box, gets new tyres (`CarPhys_ServiceTyres`), and pit messages are posted (codes 0–0xc).

## Course

- **Loading:**
  - `Race_LoadCourse` loads the map image, the course textures and the course file into `0x800b4a34` (`CourseData`, 0x19c-byte header), and `Course_Relocate` turns its offsets into pointers.
  - It then loads the sky.
  - `Race_LoadBanners` re-points trackside banners to random sponsor images; the seed is kept in `g_RaceSetup` so a replay shows the same banners.
- **Sections** (`CourseSection`, 0xe8 bytes): `CourseSect_Locate` finds the car's section between section planes. `Ground_FindTriangle` (4×4 grid per section) and `Ground_GetHeight` give the ground, and `CourseSection_CollideWalls` the walls. Course files store points as (x, z, y).
- **Drawing** (`RaceView_DrawWorld`):
  - The sky.
  - The course itself (`Course_Draw`): the visible sections of the current section's visibility list, nearest first, with LOD. It returns a mask of visible object groups.
  - The course objects (`CourseObjects_DrawGroups`): LOD models, billboards and light flares.
  - The mesh drawers are large GTE routines in clipped and unclipped versions.
  - In split screen there are two views; there is also a 120×32 rear-view mirror, which skips the sky.

## Timing

- **Race clock:** `g_RaceClock` (`0x80046f64`) advances 100 units per frame at 30 fps, so 3 units are 1 ms. Times are stored in ms; 359999999 means no time.
- **Laps and splits:** `Car_UpdateLapProgress` counts laps and checkpoint splits. It finishes the drag machine tests at 400 m and 1000 m, and point-to-point courses at the last sector. 99-lap races stop after 2 hours (`Race_IsEnduranceTimeUp`).
- **License tests** (`LicenseTest_CheckEnd`): kind 2 is stop in the box, kind 3 reach the last checkpoint, kind 5 a lap count. Fail codes: 3 overshoot, 4 / 5 hit.
- **Time trial:** car 0 records its laps as ghosts (4 lap slots in P1's replay area, plus a best lap). Car 1 is the ghost, interpolated over 2 frames.
- **Player race data:** `g_RaceRecords` (`0x801d5e88`) and `g_GhostData` (`0x801da3a0`) are really two 0x4518-byte player blocks: a race record (0xfc) followed by that player's replay input log. The second block also holds the recorded car of dirt events.

## Screens

- **Countdown** (`Countdown_Update`): beeps at 420, 360 and 300 frames, then the start signal and race music at 240. A replay shows "REPLAY" instead of "START".
- **HUD** (`Hud_*`, `Tacho_*`, sprites `g_Hud_Sprites`, `0x8002f630`):
  - Tachometer, gear, speed and the boost gauge.
  - Lap and records panels, lap popups ("Time Invalid !", "Crash !", "Out of Course"), and messages such as Wrong Way and the pit messages.
  - The course map, the tyres, and the replay pedal and steering gauges.
- **Result screen** (`Result_Draw`):
  - Times, the 2P winner, and championship standings.
  - YOU WIN / YOU LOSE for dirt races.
  - License grades GOLD, SILVER, BRONZE, KIDS PRIZE or FAIL.
- **Race music** (`RaceMusic_*`): a random race track, intro tracks for World League and One Make events, and a finish track.
- **Car sound** (`CarSound_Update`):
  - Each car has two engine layers cross-faded by rpm (ENGN banks loaded by `Race_LoadEngineBanks` from `CarSetup.engineSound`), 4 loops and a one-shot.
  - Pan and Doppler come from the camera.

### Post-race menus (`QuickMenu`)

`QuickMenu` (0x240 bytes, `g_QuickMenu` at `0x801c90a4`) is the race overlay copy of OVL1's `BaseMenu`, with a layout shifted by 4 bytes. Its pages are `QuickMenuPage` (0x24 bytes: a `MenuPage` plus timer, selection, mode and arg). Before it, `PreQuickMenuIO` loads the fonts and `setting.tim` or `license_tim.tim`, and shows for at least 66 frames.

The race loop picks the root page by `raceType`:

- Race results and the lap list.
- The arcade, 2P battle and championship result menus.
- The time trial menu, session results and name entry.
- License results:
  - Medal award: `License_AwardMedal`. A run that only reaches the kids time counts as a fail until the fail counter reaches event byte +0x74.
  - The test selector and descriptions, best times and Demonstration.
  - The license-acquired page with its prize car.
- The GT race and machine test menus, rankings and name entry.
- Prize pages:
  - `Race_AwardEventPrize` and `Race_AwardChampionshipPrize`.
  - `PlayerStats_AddRaceResult`, which keeps `finishPlaceSum`, `finishCount` and `racesWon` for the status page's average finish.
  - The World League ending.
- Card pages:
  - Save replay or ghost (`CardMenu_StartSaveReplay`, `CardMenu_StartSaveGhost`, which fill the entry with `CardMenu_PrepareReplayEntry` / `CardMenu_PrepareGhostEntry`).
  - Load ghost, and save game.
- In-race Settings: Change Parts and Parts Setting, switched with L1/R1, with the power and torque graph.

The menu's choice goes back to the race loop in `QuickMenu.unk7c`: 0 replay, 1 retry or test run, 2 exit, 4 next race, start or continue.

A loaded replay or demo goes through `Replay_RestoreCarParams`. It copies the car parameters stored with it (`0x801da49c`, 0x1c0 bytes) into `g_RaceCarParams[0]` and sets `game` to 1.

## To verify

- **Race types 1 and 10.** The race loop code reads them as the single-car run before a race or dirt race. The post-race menu code reads 1 as a GT single race and 10 as a rally. Which is right?
- **Disputed `CarPhys` fields**, marked "Disputed" in their field comments:
  - +0x372: gear count, or transmission mode? Some code branches on it being 1 or 2.
  - +0x6fc: also set when a license test ends.
  - +0x774: pushed from behind, or slipstream build-up?
- **Unknown `CarPhys` contents:**
  - The `flags` bits.
  - The tyre blocks at +0x194 / +0x26c and the drivetrain block at +0x370, which have no sub-struct yet.
  - What the HUD tyre display shows (bytes at +0x4ae).
- **Surfaces:**
  - The meaning of the surface attribute bits and of surface type 1.
  - `g_SurfaceGrip` is only read as a bump height, so its name looks wrong.
  - Is `g_DragCurve` rolling resistance rather than drag?
- **Unknown codes and data:**
  - What pit message codes 0–0xc show.
  - The contents of the 0x342-byte lap logs (`0x800a8d70`, `0x800a90b2`) and of `g_TT_GhostState`.
  - Event entry byte meanings in `g_RaceEventParams` beyond the ones listed above.
- **Possibly misplaced or miscounted:**
  - `QuickMenu +0x240` / `+0x241` (replay available, show car) lie past its size and probably belong to the race loop.
  - `Ghost_ShouldHold` and its header value 0x203.
- **Rendering:**
  - `Matrix_ToEuler` reads element pairs that look transposed.
  - `Hud_DrawLapPanel` takes the place sprites' tpage from a different table entry than their UV.
  - What `RaceLoad_VSyncDraw` shows (6 screen bands while an arcade single race loads).
- **Other:**
  - The sign of the `RaceSetup.options[6]` start handicap against the player.
  - `0x801d5de9 == 1` sends a time trial to name entry, probably on a new course record.
