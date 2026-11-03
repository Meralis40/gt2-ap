// OVL0 80014e28-800157a8 (raceworld): Race world and the lap callbacks.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
// Clears the world, stores isReplay and the course record of the course
// (CourseRecord_GetCurrent).
void RaceWorld_Init(RaceWorld *w, unsigned char isReplay)
{
    CourseRecord *pCVar1;

    memset(w, 0, 0x5d30);
    w->isReplay = isReplay;
    pCVar1 = CourseRecord_GetCurrent();
    w->courseRecord = pCVar1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_UpdateSounds);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_UpdateSounds2P);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_MuteSounds);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_UnmuteSounds);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_SetupCars);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_Start);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_Shutdown);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_DrawCars);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_OnLapCb);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_OnLapDoneCb);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_ShowGap);

// Empty race logic callback.
void Race_Nop(void)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_ShowMessage);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_OnFinish);
}
