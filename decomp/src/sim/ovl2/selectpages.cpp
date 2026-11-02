// OVL2 8001d568-8001e53c (selectpages): Root, mode, level and class pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", ArcadeRoot_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", ArcadeRoot_Update);

// Empty.
void ArcadeRoot_Draw(void *menu, DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", ModeSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", ModeSelect_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", ModeSelect_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", Game1PSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", Game1PSelect_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", Game1PSelect_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", Game2PSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", Game2PSelect_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", Game2PSelect_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", LevelSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", LevelSelect_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", LevelSelect_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", ClassSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", ClassSelect_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", ClassSelect_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", RallyClassSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", RallyClassSelect_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/selectpages", RallyClassSelect_Draw);
}
