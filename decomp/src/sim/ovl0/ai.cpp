// OVL0 80035714-800392d8 (ai): CPU drivers, racing lines and pits.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char g_Race_IsDemo;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Car_HasCompletedLaps);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", PitStop_GetLift);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiParam_LookaheadForSpeed);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_PickForSlot);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_CornerSpeed);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_UpdateNextCorner);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_FindSegment);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_PointAt);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine5_TestAt);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", PitLine_PointAt);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_SyncToLine);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_SpeedControl);

// aiLine = line, Ai_SyncToLine.
void Ai_SetLine(CarPhys *phys, unsigned char line)
{
    phys->aiLine = line;
    Ai_SyncToLine(phys);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_SwapToLeft);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_SwapToRight);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_PickPassLineA);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_PickPassLineB);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_DefaultUnless6);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_PitEntry);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_PitLane);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_SetMode);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_EnterDefaultMode);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Pit_TyreWearActive);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Pit_IsInPitZone);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Pit_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_StartLineRev);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_ThrottleFromSpeedError);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_BrakeForTarget);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_FindLookaheadPoint);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_SteerTowards);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_StopCar);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_ReleaseControls);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", Ai_Drive);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", CarAi_Drive);

// Track segment containing (x, 0, z) (World_LocateSection on g_Course), searching from
// hint.
int AiLine_FindSegmentXZ(int x, int z, int hint)
{
    int iVar1;
    int local_18 [4];

    local_18[0] = x << 4;
    local_18[2] = z << 4;
    local_18[1] = 0;
    iVar1 = World_LocateSection(&g_Race_IsDemo, hint, local_18);
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_TraceSegment);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_ProbeGround);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_FindSegmentFromLine6);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLines_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_GetPointAtDistance);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ai", AiLine_GetPitBox);
}
