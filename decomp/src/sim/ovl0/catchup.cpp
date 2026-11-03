// OVL0 80041e4c-80043388 (catchup): Catch-up, race order and car bodies.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern CatchUpParams g_CatchUp;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", CatchUp_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", CatchUp_Apply2PSide);

// Catch-up between the two cars of raceType 0: the one behind gets the behind factor
// (only when a behind % is set).
void CatchUp_Apply2P(CarPhys *bodyA, CarPhys *bodyB)
{
    int local_18;
    int local_14;

    if (g_CatchUp.behindPct != 0) {
        Body_GetGap(bodyA, bodyB, &local_18, &local_14);
        CatchUp_Apply2PSide(bodyA, bodyB, local_18, local_14);
        CatchUp_Apply2PSide(bodyB, bodyA, -local_18, -local_14);
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", CatchUp_BehindFactor);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", CatchUp_AheadFactor);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", CatchUp_ApplyAI);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", Body_ApplyTimeScale);

// Lap counter (body+0x608), minus 1 while body+0x78d bit 1 is set (start line not
// crossed yet).
int Body_GetLap(CarPhys *body)
{
    int iVar1;

    iVar1 = (int)body->lap;
    if ((body->flags & 2) != 0) {
        iVar1 = iVar1 + -1;
    }
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", Body_GetGap);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", RaceOrder_Compare);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", RaceOrder_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", RaceOrder_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", Body_UpdateTyreFx);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", Color_HeatRamp);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", Body_UpdateWheelHeights);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/catchup", Body_UpdateWheelOffsets);

// Body_UpdateWheelHeights then Body_UpdateWheelOffsets.
void Body_UpdateWheelPositions(CarPhys *body)
{
    Body_UpdateWheelHeights(body);
    Body_UpdateWheelOffsets(body);
}
}
