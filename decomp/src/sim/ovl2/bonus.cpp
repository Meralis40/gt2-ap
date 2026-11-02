// OVL2 80023690-8002445c (bonus): Bonus items pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_DrawRow);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_ListCallback);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusSelect_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusSelect_Draw);

// 24-frame delay.
void BonusLaunch_Enter(void *menu, int back)
{
    *(unsigned int *)(*(int *)((int)menu + 0x1c8) + 0x14) = 0x18;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusLaunch_Update);

// Empty.
void BonusLaunch_Draw(void *menu, DrawLayer *layer)
{
}
}
