// OVL0 800122c4-80012cd4 (timetrial): Time trial ghosts and lap logs.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char g_TT_GhostState;
extern unsigned char g_TT_BestLapLog;
extern unsigned char g_TT_LapLog;
extern RaceWorld g_RaceWorld;

// &g_TT_GhostState (0xc4 bytes, cleared by GhostPlayer_Clear).
void *TimeTrial_GetGhostState(void)
{
    return &g_TT_GhostState;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_GetLapSlot);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_GetShownLap);

// &g_TT_BestLapLog (0x342 bytes; +1 state, +2 frame count).
void *TimeTrial_GetBestLapLog(void)
{
    return &g_TT_BestLapLog;
}

// &g_TT_LapLog (0x342 bytes) of the lap being driven.
void *TimeTrial_GetLapLog(void)
{
    return &g_TT_LapLog;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", Race_IsTimeTrial);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_CopyPlayerPhys);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_InitBuffers);

// Rewinds the ghost car's input log for playback and the best lap log frame count.
void TimeTrial_RestartGhost(void)
{
    void *pvVar1;

    InputLog_Reset(g_RaceWorld.vehicles[1].inputLog, 1, 0x1000);
    pvVar1 = TimeTrial_GetBestLapLog();
    *(unsigned short *)((int)pvVar1 + 2) = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_StepGhost);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_GhostEnded);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_NextLapSlot);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_EndSession);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_OnLap);
}
