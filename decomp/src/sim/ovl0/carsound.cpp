// OVL0 80017e18-80018a68 (carsound): Wheels and car sounds.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", Tyre_GetHubOffset);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", RaceVehicle_SetupModelDims);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", RaceVehicle_SetupWheels);

// Turns reverb on for both layer voices of es.
void EngineSound_EnableReverb(EngineSound *es)
{
    SfxVoice_SetReverb(&es->voices[0].voice, 1);
    SfxVoice_SetReverb(&es->voices[1].voice, 1);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", CarSound_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", CarSound_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", CarSound_Stop);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", CarSound_PlayOneShot);

// Once per race context (flag +0x11d702): loads the race sound bank (boot file bootId,
// 0xed from Race_Entry) into the SpuBank at +0xd4394 with its header at +0xd43a0
// (0x500 bytes).
void Race_LoadSfxBank(void *race, int bootId)
{
    if (*(char *)((int)race + 0x11d702) == '\0') {
        *(unsigned char *)((int)race + 0x11d702) = 1;
        SpuBank_Init((SpuBank *)((int)race + 0xd4394), (SeqBank *)((int)race + 0xd43a0), 0x500);
        SpuBank_Load((SpuBank *)((int)race + 0xd4394), bootId, 0, 1);
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", Race_LoadEngineBanks);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carsound", RaceSfx_PlayStereo);
}
