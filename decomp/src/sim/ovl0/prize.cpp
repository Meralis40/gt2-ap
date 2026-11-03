// OVL0 800593f4-8005a474 (prize): Prize pages and money.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", PrizeCarView_Approach);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", PrizeRoot_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", ChampRacePrizeRoot_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", ChampWonPrizeRoot_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", EventPrizeRoot_Update);

// Empty.
void PrizeRoot_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Race_ShowPrizeDialog);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Race_AwardPrizeMoney);

// Random number below n, seeded from the vsync counter.
unsigned int Rand_Below(unsigned int n)
{
    unsigned int uVar1;
    unsigned int local_10 [2];

    local_10[0] = Irq_WaitVSyncs(0);
    uVar1 = Rand_Next(local_10);
    return uVar1 % n;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Race_AwardChampionshipPrize);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Race_AwardEventPrize);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Prize_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Prize_DrawCredits);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Prize_Draw);
}
