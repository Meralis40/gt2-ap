# Archipelago hooks

Where the client hooks or patches the Simulation disc (`SCUS_944.88`, NTSC-U 1.2) for the randomizer rules, and where it keeps its own state in RAM. Found statically in Ghidra. Tested in game so far: the AP state block location and the `License_IsEarned` replacement. The rest is untested (see [To verify](#to-verify)).

## Common notes

- **Almost every hook is in overlay 4** (GT mode menus). All overlays load at `0x80010000`, so a PC hook in the overlay window fires for whichever overlay is resident. Check that OVL4 is loaded first: the word at `0x80010014` is `0xafbf002c` (see `Overlay_StartAt`).
- **Patched overlay code is lost on every overlay switch.** `Overlay_Load` (`0x8005dad8`) inflates a fresh copy, calls `FlushCache`, then clears BSS (`0x800a8d5c–0x801c93af`). Re-apply code patches after that `FlushCache`. PC hooks in the emulator core need no re-applying.
- **"Return early"** below means: at the function entry, before its prologue, set `v0` and set `pc = ra`.
- Game state used by the hooks:

| What | Address | Notes |
|---|---|---|
| `g_GameState` | `0x801c98e0` | Not cleared by overlay switches |
| Garage 0 car `i` | `0x801cd558 + i * 0xa4` | `Car`: `carId` u32 at `+0x00`, `setup.manufacturerId` u8 at `+0x81`, `partsOwned` at `+0x9a` |
| Garage 0 credits | `0x801d1568` | s32 |
| Garage 0 current car | `0x801d156c` | s16 slot, -1 = none |
| `g_ShopTxn` (OVL4) | `0x801c3080` | Pending shop transaction: type u16 at `+0x00`, see below |
| Selected garage slot (OVL4) | `0x800a8d62` | s16, the car highlighted in the garage list |
| `g_MenuPageAttr` (OVL4) | `0x800a8d78` | Byte 0 is the dealership's manufacturer ID, `0xff` elsewhere |

Manufacturer IDs are in [manufacturers.md](game%20ids/manufacturers.md). [cars.json](game%20ids/cars.json) gives each car's `carId` (the raw u32 the game uses) and `manufacturerId`, so the client can look up the manufacturer of any car code.

## AP state block

**`0x801f0d80–0x801f1d7f` (4 KB), at the bottom of the stack area.** Nothing else in the game uses it:

- `start` clears `0x801c93b0–0x801f0d5f` (resident BSS) at boot. Statically, the last referenced variable is at `0x801f0d5c`.
- `0x801f0d60` is the PsyQ heap, but `Heap_InitDefault` (`0x8008ddb4`) sizes it as `(sp & ~0xf) - 0x10000 - 0x801f0d60`. With the BIOS stack top `0x801ffff0` that is negative, so the heap is empty: `InitHeap` only writes its 16-byte header at `0x801f0d60–0x801f0d6f`, and the one `malloc` call (`Overlay_OpenFile`, boot) fails.
- The rest, up to `0x801ffff0`, is stack. In game, the lowest stack write seen was `0x801f3408` (measured with [`stack_watermark.lua`](../scripts/redux/stack_watermark.lua) in PCSX-Redux across GT mode menus, races, license tests, replays, memory card and movies). That leaves 0x1688 bytes (5.6 KB) between the block end and the deepest stack use, so don't grow the block past `0x801f1d7f`. `Cd_ReadAtAsync` (`0x8007ad20`, 0x7008 bytes of buffers) is one of the deep frames.

No other gap in `0x801c93b0–0x801fffff` is safe. The large one at `0x801e360c–0x801ef5f0` is the VOL file table (`g_VolHeader`, 0xc000 bytes), which is full to `0x801ef5e3`.

The block isn't in the save, and nothing clears it after boot. The client writes it after boot and again after loading a save or reconnecting.

Re-run `stack_watermark.lua` if a later change (injected code calling game functions, for example) could deepen the stack.

Suggested layout (base `0x801f0d80`):

| Offset | Type | Contents |
|---|---|---|
| `+0x00` | `char[4]` | `GTAP`, so injected code or a debugger can check the client wrote it |
| `+0x04` | u8 | Layout version |
| `+0x05` | u8 | Enabled rules: bit 0 licenses, bit 1 manufacturers, bit 2 parts, bit 3 trophies |
| `+0x06` | u8 | Progressive License count (0–6) |
| `+0x07` | u8 | Trophy count |
| `+0x08` | u8 | Trophies needed for PFL/EPL |
| `+0x09` | u8 | Trophies needed for GTW |
| `+0x10` | u8[8] | Unlocked manufacturers, bit = manufacturer ID (0–38 used) |
| `+0x18` | u8[8] | Buyable parts, bit = `PartId` (same layout as `Car.partsOwned`) |
| `+0x100` | | Injected code, if any |

## Progressive License

**Hook: `License_IsEarned` (OVL4 `0x8001915c`).** It returns 1 when all 10 tests of a license have a medal. Replace it so that `count` Progressive Licenses earn the licenses in game order, B (5), A (4), IC (3), IB (2), IA (1), S (0):

```
earned(license) = license >= 6 - count
```

Every OVL4 license check goes through this function:

- `License_GetHighest` (`0x800191c4`), which `Race_CheckEntry` uses for each event's license requirement.
- `License_CheckEntry` (`0x80019b88`): a license's tests need the next lower license, so B tests are always open and A tests need 1 Progressive License. Decided: keep this gating, so each Progressive License opens the next license's tests.
- The menu checks for the IA/S-only elements (`FUN_OVL4__8001b680` and `FUN_OVL4__8001b6f0`), and the license icons (`MenuPage_DrawElements`).

It is a whole-function replacement that fits in place (7 of 26 instructions). It reads the count at `0x801f0d86`. Tested in game by writing it with the PCSX-Redux memory editor:

```
8001915c  3c02801f  lui   v0, 0x801f
80019160  90420d86  lbu   v0, 0x0d86(v0)   ; Progressive License count
80019164  24030006  li    v1, 6
80019168  00621823  subu  v1, v1, v0       ; lowest earned LicenseId
8001916c  0083102a  slt   v0, a0, v1
80019170  03e00008  jr    ra
80019174  38420001  _xori v0, v0, 1
```

Keep the real test results as locations; don't patch the other copies:

- **OVL0 `License_IsEarned_OVL0` (`0x8004dcac`)** runs after a test. At `0x8004e1b0` and `0x8004e1cc` it is called before and after `License_AwardMedal` to detect a newly earned license. This is the natural "license obtained" location hook.
- **OVL2 `License_IsEarned_OVL2` (`0x80023598`)** is in the arcade menus (track list unlocks). It is not on the GT mode path.

## Manufacturer lock

There are three hooks: buying a car, changing the current car, and entering a race with a car that is already owned. Buying is the only way a car of a locked manufacturer can enter the garage, apart from prize cars.

**Buying, new and used: `Dealer_CanBuyNewCar` (OVL4 `0x80017914`, `a0` = car ID, `a1` = price, `a2` = garage).** Both dealership tiles and the used car list set up a type 1 transaction (`Txn_SetBuyNewCar`), which `Shop_CheckTransaction` checks with this function before the confirmation page. To refuse, return early with `v0 = 0`: `Shop_CheckTransaction` then returns 0, and `SimMap_Update` plays the error sound and stays on the page (`0x80014d50` → `0x80014e20`). Look up the manufacturer from the car ID in `cars.json`. Inside a dealership, `g_MenuPageAttr & 0xff` is also the manufacturer, but not in the used car lots.

**Changing the current car: `SimMap_Update` case `0xab` (OVL4 `0x80014c34`).** This is the garage "change car" action. It builds a type 7 transaction (`FUN_OVL4__8001da98`), and `Shop_CheckTransaction` sets `currentCar` straight away. The slot is the s16 at `0x800a8d62`; its manufacturer is the byte at `0x801cd558 + slot * 0xa4 + 0x81`. To refuse, set `pc = 0x80014e20`, which plays the error sound and returns. Don't refuse inside `Shop_CheckTransaction` instead: this case shows whatever page it returns, and 0 isn't a valid page.

**Racing: `Race_CheckEntry` (OVL4 `0x8001973c`, `a0` = race code, `a1` = `int *msgPage`).** This covers a locked car that was already current, or one won as a prize. Read the current car as above. To refuse, write `0x8000001e` to `*a1` (the "car not allowed" message used for the allowed-cars rule) and return early with `v0 = -8`.

**Entering a dealership** isn't blocked by the hooks above. The map tile navigates to the dealer's top page through the generic path in `SimMap_Update` (`pendingPage = element.targetPage`, `0x800147fc`). `MenuPage_Load` sets `g_MenuPageAttr` from the page footer (`0x8002149c`). Blocking entry would need a map from page ID to manufacturer, built from the page data. Blocking the purchase gives the same result with less work.

`Garage_SellCar` and prize cars (`Race_AwardEventPrize`, `Race_AwardChampionshipPrize` in OVL0) don't change `currentCar`.

## Tuning parts

**Hook: `Tune_CanBuyPart` (OVL4 `0x80017b40`, `a0` = car slot, `a1` = `PartId`, `a2` = garage).** Its checks run in this order: part not offered for this car (-8), credits (-1), already owned (-3), weight reduction stage order (-5, -6), racing mod needs weight reduction 3 (-7).

Both the check (`Shop_CheckTransaction`, type 3) and the purchase (`Tune_BuyPart`) call it. So a hook here can only add refusals: parts the car doesn't offer stay unavailable.

To refuse a locked part, return early with `v0 = -8`. The shop then shows message page `0x80000019`, the same as for a part the car doesn't offer. If the error sound without a message is better, hook `SimMap_Update` at `0x80014de4` instead (part tile, `PartId` = `s8 - 0x14`) and set `pc = 0x80014e20`.

Not affected:

- `Tune_InstallPart`, which reinstalls an owned stage (txn type 4).
- Wheels (type 5).
- The racing modification preview page (`SimMap_Update` case `0x9c`). Its purchase still goes through `Tune_CanBuyPart` with `PartId` 34.

## Trophies for the leagues

`Race_CheckEntry` gates PFL/EPL on `Events_AllRegionalLeaguesWon` (`0x8001847c`, all GJL/GBL/GFL/GGL/GIL/GUL events won) and GTW on `Events_AllProLeaguesWon` (`0x8001850c`, all PFL/EPL events won). With the trophy setting on, both checks count trophies instead. Install these hooks only when the setting is on.

- **PFL/EPL: `Events_AllRegionalLeaguesWon`.** Return early with `v0 = (trophies >= proNeeded)`. On failure, `Race_CheckEntry` returns -9 with message page `0x8000002f`.
- **GTW: `Events_AllProLeaguesWon`.** Return early with `v0 = (trophies >= gtwNeeded)`. On failure, `Race_CheckEntry` returns -10 with message page `0x80000030`.
- The regional leagues are unchanged: they only need their license, and some have a power limit.

`Race_CheckEntry` is the only caller of both functions, so the hooks change nothing else. The game's two messages probably say to win the previous leagues; read them in game to see whether they still fit.

## To verify

- Message pages `0x8000001e`, `0x8000002f`, `0x80000030`, `0x80000019`: read their text in game to decide which ones fit.
- `FUN_80060840(0)` is the error sound (inferred: called with 0 on every refusal, 1 on success).
- OVL1 has its own `UsedCar_CanBuy` / `UsedCar_Buy` (`0x8001ead8` / `0x8001eb60`), which adds a car with `Garage_AddCar` from a menu next to memory card calls. Its purpose is unknown. GT mode's used car lots use the OVL4 path above.
