# PsyQ libraries

Which Sony PsyQ library objects are in the Simulation disc executable, which SDK version they come from, and how the matching build links them instead of disassembling them.

## Summary

- All PsyQ code is in the resident part of `SCUS_944.88`, at `0x80086658–0x8008ce08` and `0x8008dfc4–0x8008e024`. The boot code and the six overlays have none: they call the resident copies.
- 63 objects from 8 libraries: libpress, libcard, libpad, libcd, libgte, libetc, libapi and libc (only five BIOS stubs from libc). There is no libgpu, libgs, libspu, libsnd or libc2: the game has its own GPU, sound and string code.
- The objects are from PsyQ **4.6**, except libpad, whose code is from **4.7**. libpad's version stamp is `0x2046`, which matches neither stock release, so linking it takes a 2-byte patch.
- **They are linked as they are.** Converted to ELF and linked with GNU ld at their addresses, the 63 objects rebuild the library `.text`, `.rdata` and `.data` byte for byte (after the libpad stamp patch), and their `.bss` lands where the code expects it. Nothing uses `$gp`. The matching build does this ([In the build](#in-the-build)); it needs the `.LIB` files, which stay out of git.

## Method

1. **Find.** [ghidra_psx_ldr](https://github.com/lab313ru/ghidra_psx_ldr) ships the [psx_psyq_signatures](https://github.com/lab313ru/psx_psyq_signatures) database in `data/psyq/<version>/` (installed under `%APPDATA%\ghidra\ghidra_12.1.3_PUBLIC\Extensions\ghidra_psx_ldr\data\psyq`). For each PsyQ version from 2.6 to 4.7, it has each `.LIB` member's `.text` with relocated fields masked. A script searched every unit for every member of every version. Members of 16 to 32 bytes also match game functions by chance; those hits were dropped. A second pass, allowing differences, found no near misses in library code.
2. **Check.** With the original libraries (`Psy-Q_46.zip` and `Psy-Q_47.zip` from [psx.arthus.net](https://psx.arthus.net/sdk/Psy-Q/)), a small reader of the LIB and LNK formats compared each member's `.text` with the EXE, masking exactly the fields its relocations cover. It then evaluated every relocation against the EXE to find where each member's sections and imported symbols ended up.
3. **Link.** The 63 members were converted with `psyq-obj-parser -n -c` and linked with `mipsel-linux-gnu-ld` 2.42, one output section per run of code or data, at the addresses from step 2. The output was compared with the EXE.

The survey scripts and the SDK are outside the repo; the build's converter is `decomp/tools/psyq.py`.

## Versions

### Version stamps

Sony puts an 8-byte stamp `50 73 <lib> 00 00 00 <version, 16 bits>` (`"Ps"`, library number, version) at the start of the first `.text` or `.data` of most libraries. The executable has eight:

| Address | Section | Library | Stamp | Stock 4.6 | Stock 4.7 |
|---|---|---|---|---|---|
| `8008bb08` | `.text` | 9, libgte (`MSC00.OBJ`) | `0x0046` | same | `0x0047` |
| `8008cc88` | `.text` | 0, libapi (`C114.OBJ`) | `0x0046` | same | `0x0047` |
| `8008e004` | `.text` | 1, libc (`A63.OBJ`) | `0x0046` | same | `0x0047` |
| `800a762c` | `.data` | 14, libpress (`LIBPRESS.OBJ`) | `0x0046` | same | `0x0047` |
| `800a7674` | `.data` | 3, libcard (`INIT.OBJ`) | `0x0046` | same | `0x0047` |
| `800a7694` | `.data` | 12, libpad (`PDRESRES.OBJ`) | **`0x2046`** | `0x0046` | `0x0047` |
| `800a7804` | `.data` | 4, libcd (`BIOS_1.OBJ`) | `0x0046` | same | `0x0047` |
| `800a8c24` | `.data` | 6, libetc (`VSYNC.OBJ`) | `0x0046` | same | `0x0047` |

### By library

| Library | Version | Evidence |
|---|---|---|
| libapi | 4.6 | `C114.OBJ` stamp; every other member is the same in 4.6 and 4.7 |
| libc | 4.6 | `A63.OBJ` stamp |
| libgte | 4.6 | `MSC00.OBJ` stamp |
| libetc | 4.6 | `VSYNC.OBJ` stamp; same code in 4.1 (4.3 for `INTR_VB.OBJ`) to 4.7 |
| libcd | 4.6 | `BIOS_1.OBJ` stamp; same code in 4.6 and 4.7 |
| libcard | 4.6 | `INIT.OBJ` stamp; same code in 4.5 to 4.7 |
| libpress | 4.6 | stamp; same code in 4.6 and 4.7 |
| libpad | 4.7 code, stamp `0x2046` | `PDRESRES.OBJ` is 0x1e70 bytes and its code matches 4.7 only (4.6's is 0x1e10). Its data matches 4.7 except the stamp, which is `46 20` where 4.7 has `47 00`. The other 7 members are the same in 4.6 and 4.7. Probably a libpad update released between 4.6 and 4.7 |

So the game used the 4.6 SDK with a newer libpad, as [decomp.md](decomp.md#compiler) says. Stock 4.7's `LIBPAD.LIB` gives the same bytes once the stamp is patched.

## Objects

In link order. A size is the member's `.text` size. The functions are Ghidra's names and sizes, which can be shorter than the space to the next function (the rest is alignment `nop`s).

The members with identical code were told apart by their relocations:

- `80089f18` is `S_021.OBJ` (`CdGetSector`): it calls `CD_getsector` (`8008a488`, `BIOS_2.OBJ`).
- `8008a088` is `S_014.OBJ` (`CdSyncCallback`), which writes `CD_cbsync` (`800a780c`). `8008a0a8` is `S_015.OBJ` (`CdReadyCallback`), which writes `CD_cbready` (`800a7810`).
- The libcard stubs (`C112`, `C171`, `A74`–`A80`) are libcard's: libapi has `C112` too, but libcard's `INIT.OBJ` and `CARD.OBJ` call them.
- The member at `8008cc88` is libapi's `C114.OBJ`, which defines `_96_remove` (Ghidra calls it `CdRemove`).

| Address | Size | Library | Member | Matching versions | Functions (address, size) |
|---|---|---|---|---|---|
| `80086658` | 0x690 | libpress | `LIBPRESS.OBJ` | 4.6–4.7 | `DecDCTReset` 80086658 0x34, `DecDCTGetEnv` 8008668c 0x8c, `DecDCTPutEnv` 80086718 0x98, `DecDCTin` 800867b0 0x30, `LIBPRESS_OBJ_188` 800867e0 0x2c, `LIBPRESS_OBJ_1B4` 8008680c 0x20, `DecDCTout` 8008682c 0x20, `DecDCTinSync` 8008684c 0x2c, `LIBPRESS_OBJ_220` 80086878 0x10, `DecDCToutSync` 80086888 0x38, `LIBPRESS_OBJ_268` 800868c0 0x10, `DecDCTinCallback` 800868d0 0x24, `DecDCToutCallback` 800868f4 0x24, `MDEC_reset` 80086918 0xd4, `LIBPRESS_OBJ_394` 800869ec 0xc, `LIBPRESS_OBJ_3A0` 800869f8 0x10, `MDEC_in` 80086a08 0x90, `LIBPRESS_OBJ_440` 80086a98 0x8c, `MDEC_in_sync` 80086b24 0x84, `LIBPRESS_OBJ_550` 80086ba8 0x10, `LIBPRESS_OBJ_560` 80086bb8 0x84, `LIBPRESS_OBJ_5E4` 80086c3c 0x10, `LIBPRESS_OBJ_5F4` 80086c4c 0x18, `MDEC_timeout` 80086c64 0x7c |
| `80086ce8` | 0x100 | libcard | `INIT.OBJ` | 4.5–4.7 | `InitCARD` 80086ce8 0x90, `StartCARD` 80086d78 0x48, `StopCARD` 80086dc0 0x28 |
| `80086de8` | 0x10 | libcard | `C112.OBJ` | 3.0–4.7 | `_bu_init` 80086de8 0xc |
| `80086df8` | 0x40 | libcard | `CARD.OBJ` | 4.1–4.7 | `_card_clear` 80086df8 0x34 |
| `80086e38` | 0x10 | libcard | `C171.OBJ` | 3.0–4.7 | `_card_info` 80086e38 0xc |
| `80086e48` | 0x10 | libcard | `A79.OBJ` | 3.0–4.7 | `_card_read` 80086e48 0xc |
| `80086e58` | 0x10 | libcard | `A78.OBJ` | 3.0–4.7 | `_card_write` 80086e58 0xc |
| `80086e68` | 0x10 | libcard | `A74.OBJ` | 3.0–4.7 | `InitCARD2` 80086e68 0xc |
| `80086e78` | 0x220 | libcard | `PATCH.OBJ` | 4.5–4.7 | `_patch_card_info` 80086e78 0x44, `_patch_card` 80086f54 0x94, `_patch_card2` 80086fe8 0x70, `_copy_memcard_patch` 80087058 0x34 |
| `80087098` | 0x10 | libcard | `A75.OBJ` | 3.0–4.7 | `StartCARD2` 80087098 0xc |
| `800870a8` | 0x10 | libcard | `A76.OBJ` | 3.0–4.7 | `StopCARD2` 800870a8 0xc |
| `800870b8` | 0x80 | libcard | `END.OBJ` | 4.1–4.7 | `_ExitCard` 800870b8 0x70 |
| `80087138` | 0x10 | libcard | `A80.OBJ` | 3.0–4.7 | `_new_card` 80087138 0xc |
| `80087148` | 0xb0 | libpad | `PDDIRINI.OBJ` | 4.6–4.7 | `PadInitDirect` 80087148 0xb0 |
| `800871f8` | 0x110 | libpad | `PDMAIINI.OBJ` | 4.6–4.7 | `PadStartCom` 800871f8 0xcc, `PadStopCom` 800872c4 0x44 |
| `80087308` | 0xd0 | libpad | `PDENT2.OBJ` | 4.6–4.7 | `PadGetState` 80087308 0xb8 |
| `800873d8` | 0xe0 | libpad | `PDENT4.OBJ` | 4.6–4.7 | `PadInfoAct` 800873d8 0x80, `PDENT4_OBJ_80` 80087458 0xc, `PDENT4_OBJ_8C` 80087464 0xc, `PDENT4_OBJ_98` 80087470 0xc, `PDENT4_OBJ_A4` 8008747c 0xc, `PDENT4_OBJ_B0` 80087488 0xc, `PDENT4_OBJ_C0` 80087498 0x14 |
| `800874b8` | 0x40 | libpad | `PDCMD1.OBJ` | 4.6–4.7 | `PadSetAct` 800874b8 0x40 |
| `800874f8` | 0x170 | libpad | `PDCMD2.OBJ` | 4.6–4.7 | `PadSetActAlign` 800874f8 0x7c, `PDCMD2_OBJ_7C` 80087574 0x28, `PDCMD2_OBJ_A4` 8008759c 0x94, `PDCMD2_OBJ_138` 80087630 0x34 |
| `80087668` | 0x1e70 | libpad | `PDRESRES.OBJ` | 4.7 | `Pad_SioPollPort` 80087860 0x334, `Pad_SioSendFirst` 80087c84 0x224, `Pad_SioXchgByte` 80087ea8 0x27c, `Pad_SioWaitAckEnd` 80088124 0x90, `Pad_SioWaitRx` 800881b4 0x28, `PadAct_SetParams` 800881dc 0x10, `Pad_InfoNextCmd` 800881ec 0x84, `Pad_InfoParseReply` 80088270 0x270, `Pad_InfoSize` 800884e0 0x38, `Pad_InfoAlloc` 80088518 0xd4, `PadCmd_Config` 80088a8c 0x20, `PadCmd_QueryModel` 80088aac 0x14, `PadCmd_QueryMode` 80088ac0 0x20, `PadCmd_QueryAct` 80088ae0 0x20, `PadCmd_QueryComb` 80088b00 0x20, `PadCmd_4B` 80088b20 0x14, `Pad_TimerStart` 80088b34 0x20, `Pad_TimerExpired` 80088b54 0xa0 |
| `800894d8` | 0xa40 | libpad | `PDDIRRES.OBJ` | 4.6–4.7 | `_padInitDirPort` 800894d8 0xf0, `PDDIRRES_OBJ_F0` 800895c8 0x68, `PDDIRRES_OBJ_158` 80089630 0x74, `PDDIRRES_OBJ_1CC` 800896a4 0x5c, `PDDIRRES_OBJ_228` 80089700 0x24, `PDDIRRES_OBJ_24C` 80089724 0x10, `PDDIRRES_OBJ_25C` 80089734 0x64, `PDDIRRES_OBJ_2C0` 80089798 0xc, `PDDIRRES_OBJ_2EC` 800897c4 0x24, `PDDIRRES_OBJ_310` 800897e8 0x8, `PDDIRRES_OBJ_318` 800897f0 0xd4, `PDDIRRES_OBJ_3EC` 800898c4 0x4c, `PDDIRRES_OBJ_438` 80089910 0x58, `PDDIRRES_OBJ_584` 80089a5c 0x10, `PDDIRRES_OBJ_594` 80089a6c 0x20, `PDDIRRES_OBJ_5B4` 80089a8c 0xd4, `PDDIRRES_OBJ_688` 80089b60 0x2c, `PDDIRRES_OBJ_6B4` 80089b8c 0x10, `PDDIRRES_OBJ_6C4` 80089b9c 0xe8, `PDDIRRES_OBJ_7AC` 80089c84 0x128, `PDDIRRES_OBJ_8D4` 80089dac 0xc, `PDDIRRES_OBJ_8E8` 80089dc0 0x28, `PDDIRRES_OBJ_910` 80089de8 0x10, `PDDIRRES_OBJ_920` 80089df8 0x10, `_dirFailAuto` 80089e08 0xd0, `PDDIRRES_OBJ_A00` 80089ed8 0x10, `PDDIRRES_OBJ_A10` 80089ee8 0x2c |
| `80089f18` | 0x20 | libcd | `S_021.OBJ` | 4.6–4.7 | `CdGetSector` 80089f18 0x20 |
| `80089f38` | 0x150 | libcd | `EVENT.OBJ` | 4.3–4.7 | `CdInit` 80089f38 0x80, `EVENT_OBJ_80` 80089fb8 0x10, `CdReset` 80089fc8 0x2c, `EVENT_OBJ_BC` 80089ff4 0x10, `EVENT_OBJ_CC` 8008a004 0x28, `EVENT_OBJ_F4` 8008a02c 0x28, `EVENT_OBJ_11C` 8008a054 0x28 |
| `8008a088` | 0x20 | libcd | `S_014.OBJ` | 4.6–4.7 | `CdSyncCallback` 8008a088 0x14 |
| `8008a0a8` | 0x20 | libcd | `S_015.OBJ` | 4.6–4.7 | `CdReadyCallback` 8008a0a8 0x14 |
| `8008a0c8` | 0x3c0 | libcd | `S_016.OBJ` | 4.6–4.7 | `CdControl` 8008a0c8 0x13c, `CdControlF` 8008a204 0x134, `CdControlB` 8008a338 0x11c, `S_016_OBJ_38C` 8008a454 0x30 |
| `8008a488` | 0x100 | libcd | `BIOS_2.OBJ` | 4.6–4.7 | `CD_getsector` 8008a488 0x100 |
| `8008a588` | 0x1580 | libcd | `BIOS_1.OBJ` | 4.6–4.7 | `CD_getintr` 8008a588 0x64, `BIOS_1_OBJ_64` 8008a5ec 0x200, `BIOS_1_OBJ_264` 8008a7ec 0x100, `BIOS_1_OBJ_364` 8008a8ec 0x4c, `BIOS_1_OBJ_3B0` 8008a938 0x80, `BIOS_1_OBJ_430` 8008a9b8 0x80, `BIOS_1_OBJ_4B0` 8008aa38 0x80, `BIOS_1_OBJ_550` 8008aad8 0x14, `CD_sync` 8008aaec 0x138, `BIOS_1_OBJ_69C` 8008ac24 0x30, `BIOS_1_OBJ_6CC` 8008ac54 0xe8, `BIOS_1_OBJ_7B4` 8008ad3c 0x30, `CD_ready` 8008ad6c 0x138, `BIOS_1_OBJ_91C` 8008aea4 0x30, `BIOS_1_OBJ_94C` 8008aed4 0x130, `BIOS_1_OBJ_A7C` 8008b004 0x30, `CD_cw` 8008b034 0x2cc, `BIOS_1_OBJ_D78` 8008b300 0x30, `BIOS_1_OBJ_DA8` 8008b330 0xe8, `BIOS_1_OBJ_E90` 8008b418 0x28, `CD_vol` 8008b440 0x88, `CD_flush` 8008b4c8 0xd4, `CD_initvol` 8008b59c 0xf0, `CD_initintr` 8008b68c 0x4c, `CD_init` 8008b6d8 0x1d0, `BIOS_1_OBJ_1320` 8008b8a8 0x10, `CD_datasync` 8008b8b8 0x118, `BIOS_1_OBJ_1448` 8008b9d0 0x50, `CD_set_test_parmnum` 8008ba20 0xc, `BIOS_1_OBJ_14A4` 8008ba2c 0x34, `BIOS_1_OBJ_14D8` 8008ba60 0xa4 |
| `8008bb08` | 0x90 | libgte | `MSC00.OBJ` | 4.6 | `InitGeom` 8008bb10 0x80 |
| `8008bb98` | 0xe0 | libgte | `PATCHGTE.OBJ` | 4.1–4.7 | `_patch_gte` 8008bb98 0xac, `PATCHGTE_OBJ_DC` 8008bc74 0x4 |
| `8008bc78` | 0x6c0 | libetc | `INTR.OBJ` | 4.1–4.7 | `ResetCallback` 8008bc78 0x30, `InterruptCallback` 8008bca8 0x30, `DMACallback` 8008bcd8 0x30, `VSyncCallback` 8008bd08 0x34, `VSyncCallbacks` 8008bd3c 0x30, `StopCallback` 8008bd6c 0x30, `RestartCallback` 8008bd9c 0x30, `CheckCallback` 8008bdcc 0x10, `GetIntrMask` 8008bddc 0x18, `SetIntrMask` 8008bdf4 0x18, `startIntr` 8008be0c 0xd8, `trapIntr` 8008bee4 0x1a4, `INTR_OBJ_410` 8008c088 0xc, `setIntr` 8008c0b4 0xb0, `INTR_OBJ_4EC` 8008c164 0x98, `stopIntr` 8008c1fc 0xa0, `restartIntr` 8008c29c 0x68, `intr_memclr` 8008c314 0x24 |
| `8008c338` | 0x210 | libetc | `VSYNC.OBJ` | 4.1–4.7 | `VSync` 8008c338 0xa4, `VSYNC_OBJ_164` 8008c49c 0x14, `v_wait` 8008c4b0 0x88 |
| `8008c548` | 0x120 | libetc | `INTR_VB.OBJ` | 4.3–4.7 | `startIntrVSync` 8008c548 0x58, `trapIntrVSync` 8008c5a0 0x6c, `setIntrVSync` 8008c60c 0x2c, `intrvb_memclr` 8008c638 0x24 |
| `8008c668` | 0x2a0 | libetc | `INTR_DMA.OBJ` | 4.1–4.7 | `startIntrDMA` 8008c668 0x4c, `trapIntrDMA` 8008c6b4 0x180, `setIntrDMA` 8008c834 0xa4, `INTR_DMA_OBJ_270` 8008c8d8 0x8, `intrdma_memclr` 8008c8e0 0x24 |
| `8008c908` | 0x10 | libapi | `C68.OBJ` | 2.6–4.7 | `FlushCache` 8008c908 0xc |
| `8008c918` | 0x10 | libapi | `A36.OBJ` | 2.6–4.7 | `EnterCriticalSection` 8008c918 0x10 |
| `8008c928` | 0x10 | libapi | `A08.OBJ` | 2.6–4.7 | `OpenEvent` 8008c928 0xc |
| `8008c938` | 0x10 | libapi | `A12.OBJ` | 2.6–4.7 | `EnableEvent` 8008c938 0xc |
| `8008c948` | 0x10 | libapi | `A37.OBJ` | 2.6–4.7 | `ExitCriticalSection` 8008c948 0x10 |
| `8008c958` | 0x10 | libapi | `A09.OBJ` | 2.6–4.7 | `CloseEvent` 8008c958 0xc |
| `8008c968` | 0x10 | libapi | `C73.OBJ` | 2.6–4.7 | `GPU_cw` 8008c968 0xc |
| `8008c978` | 0x10 | libapi | `A81.OBJ` | 2.6–4.7 | `Krom2RawAdd` 8008c978 0xc |
| `8008c988` | 0x10 | libapi | `C58.OBJ` | 2.6–4.7 | `_exit` 8008c988 0xc |
| `8008c998` | 0x10 | libapi | `A91.OBJ` | 2.6–4.7 | `ChangeClearPAD` 8008c998 0xc |
| `8008c9a8` | 0x290 | libapi | `PAD.OBJ` | 4.6–4.7 | `SetInitPadFlag` 8008c9a8 0xc, `ReadInitPadFlag` 8008c9b4 0x10, `PAD_init` 8008c9c4 0x90, `InitPAD` 8008ca54 0x90, `StartPAD` 8008cae4 0x30, `PAD_OBJ_16C` 8008cb14 0x78, `PAD_OBJ_1E4` 8008cb8c 0x68, `PAD_OBJ_24C` 8008cbf4 0x40 |
| `8008cc38` | 0x10 | libapi | `L03.OBJ` | 2.6–4.7 | `SysDeqIntRP` 8008cc38 0xc |
| `8008cc48` | 0x10 | libapi | `L02.OBJ` | 2.6–4.7 | `SysEnqIntRP` 8008cc48 0xc |
| `8008cc58` | 0x10 | libapi | `L10.OBJ` | 2.6–4.7 | `ChangeClearRCnt` 8008cc58 0xc |
| `8008cc68` | 0x10 | libapi | `A07.OBJ` | 2.6–4.7 | `DeliverEvent` 8008cc68 0xc |
| `8008cc78` | 0x10 | libapi | `A25.OBJ` | 2.6–4.7 | `HookEntryInt` 8008cc78 0xc |
| `8008cc88` | 0x20 | libapi | `C114.OBJ` | 4.6 | `CdRemove` 8008cc90 0xc |
| `8008cca8` | 0x10 | libapi | `A23.OBJ` | 2.6–4.7 | `ReturnFromException` 8008cca8 0xc |
| `8008ccb8` | 0x10 | libapi | `A24.OBJ` | 2.6–4.7 | `ResetEntryInt` 8008ccb8 0xc |
| `8008ccc8` | 0x70 | libapi | `CHCLRPAD.OBJ` | 4.1–4.7 | `_remove_ChgclrPAD` 8008ccc8 0x68 |
| `8008cd38` | 0xa0 | libapi | `PATCH.OBJ` | 4.1–4.7 | `EnablePAD` 8008cd38 0x14, `DisablePAD` 8008cd4c 0x14, `_patch_pad` 8008cd60 0x70 |
| `8008cdd8` | 0x10 | libapi | `A21.OBJ` | 2.6–4.7 | `PAD_init2` 8008cdd8 0xc |
| `8008cde8` | 0x10 | libapi | `A18.OBJ` | 2.6–4.7 | `InitPAD2` 8008cde8 0xc |
| `8008cdf8` | 0x10 | libapi | `A19.OBJ` | 2.6–4.7 | `StartPAD2` 8008cdf8 0xc |
| `8008dfc4` | 0x10 | libc | `C39.OBJ` | 3.0–4.7 | `bcopy` 8008dfc4 0xc |
| `8008dfd4` | 0x10 | libc | `C24.OBJ` | 3.0–4.7 | `strncmp` 8008dfd4 0xc |
| `8008dfe4` | 0x10 | libc | `C40.OBJ` | 3.0–4.7 | `bzero` 8008dfe4 0xc |
| `8008dff4` | 0x10 | libc | `C63.OBJ` | 3.0–4.7 | `printf` 8008dff4 0xc |
| `8008e004` | 0x20 | libc | `A63.OBJ` | 4.6 | `puts` 8008e00c 0xc |

## Around the libraries

| Range | Contents |
|---|---|
| up to `80085974` | Game code |
| `80085974–80086658` | GCC 2.8.1's `libgcc`: `exit`, `__main` and the constructor lists, RTTI (`tinfo.cc`), `__builtin_delete`, `__divdi3`. See [runtime-libs.md](runtime-libs.md) |
| `80086658–8008ce08` | PsyQ: libpress, libcard, libpad, libcd, libgte, libetc, libapi |
| `8008ce08–8008dfc4` | Not PsyQ: `Sys_Init`, `memset`, `strcpy`, `strcmp`, `sprintf`, `strncpy`, `strlen`, `memcpy`, `strcat`, `strncasecmp`, `malloc`, `abs`, `memcmp`, a printf with 64-bit numbers, `InitHeap`, `free`, `vsprintf`. None is close to any version of LIBC2. The game's own library, in C ([runtime-libs.md](runtime-libs.md#the-games-library)) |
| `8008dfc4–8008e024` | libc 4.6: the BIOS stubs `bcopy`, `strncmp`, `bzero`, `printf`, `puts` |

`__builtin_delete`'s code is the same as libsn 4.7's `op_delete.obj` but not 4.6's `_OP_DELE.OBJ`, and `__divdi3` matches no libsn. Both are from GCC 2.8.1's own `libgcc`, which rebuilds byte for byte from the GNU sources with the build's compiler ([runtime-libs.md](runtime-libs.md)).

The libc stubs come after the game's string functions. So the game's library was linked before `LIBC.LIB`: `memset`, `memcpy`, `strcpy` and the rest resolved to the game's copies, and only the five names it doesn't define came from libc.

## Sections

The libraries use absolute addresses (`lui`/`addiu` pairs) everywhere: no instruction uses `$gp`, and no member has `.sdata` or `.sbss`. So `-G0` and the header's `gp` of 0 are no problem.

Each section kind holds the members in the same order as `.text`, packed at 4-byte alignment with no gaps. The members say 8-byte alignment, but psylink didn't apply it: `0x80090bd4` and `0x800a7524` aren't multiples of 8. The members' sections, as placed:

| Object | `.rdata` | `.data` | `.bss` |
|---|---|---|---|
| libpress `LIBPRESS` | `80090bd4` 0x50 | `800a7524` 0x150 |  |
| libcard `INIT` |  | `800a7674` 0x10 |  |
| libcard `PATCH` |  |  | `801c9598` 0x10 |
| libcard `END` |  |  | `801c95a8` 0x10 |
| libpad `PDMAIINI` |  | `800a7684` 0x10 |  |
| libpad `PDENT4` | `80090c24` 0x20 |  |  |
| libpad `PDRESRES` |  | `800a7694` 0xb0 | `801c95b8` 0x10 |
| libpad `PDDIRRES` |  | `800a7744` 0x10 | `801c95c8` 0x270 |
| libcd `EVENT` | `80090c44` 0x20 | `800a7754` 0x10 |  |
| libcd `S_016` |  | `800a7764` 0x80 |  |
| libcd `BIOS_2` |  | `800a77e4` 0x20 |  |
| libcd `BIOS_1` | `80090c64` 0x260 | `800a7804` 0x310 | `801c9838` 0x30 |
| libgte `MSC00` |  | `800a7b14` 0x10 |  |
| libgte `PATCHGTE` |  |  | `801c9868` 0x10 |
| libetc `INTR` | `80090ec4` 0x70 | `800a7b24` 0x1100 |  |
| libetc `VSYNC` | `80090f34` 0x10 | `800a8c24` 0x20 |  |
| libetc `INTR_VB` |  | `800a8c44` 0x30 |  |
| libetc `INTR_DMA` | `80090f44` 0x30 | `800a8c74` 0x30 |  |
| libapi `PAD` |  | `800a8ca4` 0x10 |  |
| libapi `CHCLRPAD` |  |  | `801c9878` 0x10 |
| libapi `PATCH` |  |  | `801c9888` 0x10 |

| Section | Range | Before it | After it |
|---|---|---|---|
| `.text` | `80086658–8008ce08`, `8008dfc4–8008e024` | `__divdi3` | the game's library; then `.rdata` |
| `.rdata` | `80090bd4–80090f74` | the C++ runtime's | the game's library's (`80090f74`) |
| `.data` | `800a7524–800a8cb4` | game data | the game's library's (`800a8cb4`) |
| `.bss` | `801c9598–801c9898` | game `.bss` | the game's library's (`801c9898`) |

The EXE header's data fields (`d_addr` `0x8009113c`, `d_size` `0x17c20`) end at `0x800a8d5c`, where the BSS starts.

### Uninitialized symbols

Three members declare uninitialized symbols (PsyQ's XBSS, written as op 48 with a section). psylink placed them apart from their member's `.bss`:

| Symbol | Size | Member | Address |
|---|---|---|---|
| `_padRestPeriod` | 8 | libpad `PDRESRES.OBJ` | `801c9420`, inside the game's `.bss` (Ghidra labels `__CTOR_LIST__` at `801c9410`) |
| `_apiPadPatch` | 16 | libapi `PAD.OBJ` | `801f0d50`, at the end with the game's common symbols |
| `StMode` | 4 | libcd `BIOS_1.OBJ` | not referenced by the linked code; place unknown |

Converted as ELF `COMMON` symbols, they give way to a definition elsewhere, here an absolute symbol in the linker script. (`psyq-obj-parser` without `-c` makes them weak symbols inside the member's `.bss`, which moves that `.bss`.)

## In the build

The matching build links the 63 members from the original libraries; `decomp/tools/psyq.py` converts them, and `exe.yaml` and `build.py` place them.

### The libraries

Copy these files into `decomp/psyq/` (ignored by git), under the version's folder. The 4.6 ones are in `Psy-Q_46.zip`'s `LIB` folder, and `LIBPAD.LIB` in `Psy-Q_47.zip`'s ([psx.arthus.net](https://psx.arthus.net/sdk/Psy-Q/)). The build checks their SHA-1s:

| File | SHA-1 |
|---|---|
| `psyq/4.6/LIBAPI.LIB` | `38d8cb98d26da3c7028e20a9e122d9945be7cdfb` |
| `psyq/4.6/LIBC.LIB` | `720ef685dd1cb726274f1915f3df7e7d7c03e786` |
| `psyq/4.6/LIBGTE.LIB` | `909ec172f2627c7ec2f35aeb584a42a1377785fa` |
| `psyq/4.6/LIBETC.LIB` | `87d42ae9c7317aac96245d0d664d446921ea918e` |
| `psyq/4.6/LIBCD.LIB` | `d5624294464b067b6bd1e0c29109f16e84468534` |
| `psyq/4.6/LIBCARD.LIB` | `e62ec20500d18c85015b5d066338e5c1bb7e3654` |
| `psyq/4.6/LIBPRESS.LIB` | `7cc56853a6627b7278512197c9587eca1fa59ef0` |
| `psyq/4.7/LIBPAD.LIB` | `aa8cf942a08c3ce963ec155ea13cb2857c0f9a18` |

Only the EXE needs them; the overlays link against its symbols.

### Conversion

`tools/psyq.py` reads the members from the `.LIB` files and converts each one from LNK to an ELF relocatable object, then archives them per library: `build/psyq/libcd.a` holds `BIOS_1.o`, `EVENT.o`, and so on. `build.py` runs it for the members the yaml lists, on every EXE build.

- Sections keep their LNK names (`.text`, `.rdata`, `.data`, `.bss`) and alignment.
- Relocations become `R_MIPS_32`, `R_MIPS_26`, `R_MIPS_HI16` and `R_MIPS_LO16`, with the addend in the field (REL). GNU ld takes a HI16's addend from the next LO16 against the same symbol, while each LNK relocation has its own expression, so each HI16 is put right before a LO16 with the same target.
- Uninitialized symbols (op 48) become `COMMON` symbols, so the definitions below take their place.
- `PATCHES` sets the libpad stamp to the game's: `PDRESRES` `.data` + 6, `47 00` becomes `46 20`.

The converter does the same as pcsx-redux's `psyq-obj-parser -n -c`, which the first test link used. It is in Python so the build needs no other tool, and it supports only what these 63 members use: it stops on any other LNK opcode, expression or relocation type.

### Placement

- `exe.yaml` sets `lib_path: build/psyq` and has a `lib` subsegment for each member's `.text`, `.rdata` and `.data`, at its address: `[0x76e58, lib, libpress, LIBPRESS, .text]` links `build/psyq/libpress.a:LIBPRESS.o(.text)`. The game's own library between libapi and libc is the `syslib` asm subsegment. The resident data is split around the libraries' `.rdata` (`resident_data`, `resident_data2`) and `.data` (`resident_data3`, which is `data` so that it follows them).
- splat can't place a `lib` subsegment's `.bss` in a NOLOAD section, and the EXE's yaml has no BSS. `build.py` writes `build/sim/exe.psyq.ld`, splat's script with a `psyq_bss` NOLOAD section at `0x801c9598` holding the members' `.bss` in `.text` order, and the two uninitialized symbols (`PSYQ_BSS` and `PSYQ_SYMBOLS` in `build.py`). It goes before splat's `/DISCARD/`, which would otherwise drop those sections.
- The archives are linked with `--whole-archive`, so every member is loaded and placed by the script. `StMode`, which nothing references, stays a `COMMON` symbol and is discarded.
- The members import one game function, `setjmp` (`8007ad58`, from libetc's `INTR.OBJ`); everything else they import is among the 63.

### Names

The game's code references 41 library functions, by the names in `exe.txt`. They are the members' names, so the archives define them; splat also writes them to `exe.undefined_funcs_auto.txt` at the same addresses. libapi's `Krom2RawAdd` stub (`8008c978`) used to be `Krom2RawAdd_8008C978` there, because a game function at `8008051c` had the same name in Ghidra; that one is now `KanjiFont_Krom2RawAdd`. It is the game's own kanji ROM lookup (772 bytes), not libgpu's `Krom2RawAdd2` (`KT00.OBJ`, 176 bytes).

No game code or overlay references the libraries' data or their internal functions, so the names Ghidra gave those (`g_Vcount`, `g_CdSyncCallback`, `Pad_Sio…` for libpad's `_padSioRW` and others) don't matter to the build.

### Not PsyQ

The C++ runtime and `__divdi3` (`80085974–80086658`, with their `.rodata` at `80090a24` and `.data` at `800a70cc`) are from GCC 2.8.1's `libgcc`. The build compiles them from GCC 2.8.1's `libgcc2.c` and `cp/tinfo.cc` with the game's compiler and links them the same way, from `build/psyq/libgcc.a` ([runtime-libs.md](runtime-libs.md#in-the-build)).
