// OVL0 8003c360-8003d848 (lapprogress): Lap progress, race clock and license tests.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char g_Race_IsEndurance;
extern int g_RaceClock;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", Math_ApproxDist2);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", Math_ApproxDist3);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", SplitTimes_Reset);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", SplitTimes_Record);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", Car_UpdateLapProgress);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", Car_UpdateTrackPosition);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", Cars_UpdateTrackPositions);

// Endurance race (g_Race_IsEndurance) and g_RaceClock past 21600000 (2 hours).
unsigned char Race_IsEnduranceTimeUp(void)
{
    return g_Race_IsEndurance != '\0' && 21600000 < (unsigned int)g_RaceClock;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", RaceClock_Tick);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", Car_GetDisplayLap);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", LicenseTest_ResetResult);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", LicenseTest_Fail);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", LicenseTest_Succeed);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", LicenseTest_OnSector);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", LicenseTest_OnLap);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", CarPhys_CountWheelsOffRoad);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", CarPhys_GetTrackDistRange);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", LicenseTest_CheckEnd);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/lapprogress", PackedTime_ToMs);
}
