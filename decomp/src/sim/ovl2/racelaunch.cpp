// OVL2 8002344c-80023690 (racelaunch): Race launch and guest garage pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


// 24-frame delay.
void RaceLaunch_Enter(void *menu, int back)
{
    *(unsigned int *)(*(int *)((int)menu + 0x1c8) + 0x14) = 0x18;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/racelaunch", RaceLaunch_Update);

// Empty.
void RaceLaunch_Draw(void *menu, DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/racelaunch", LoadGuest_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/racelaunch", LoadGuest_Update);

// GuestLoad_Draw.
void LoadGuest_Draw(void *menu, DrawLayer *layer)
{
    GuestLoad_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// Always 0: LEVEL SELECTION shows 3 levels (the 4-level layout is unused).
int ArcLevel_HasFourthLevel(void)
{
    return 0;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/racelaunch", License_IsEarned_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/racelaunch", ArcBonus_AllCoursesWon);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/racelaunch", ArcBonus_IsStat349Set);
}
