// OVL2 8001c118-8001c7ac (iconitem): Icon items.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/iconitem", IconItem_Init);

// Starts the zoom-in reveal (anim 0).
void IconItem_Show(ArcIconItem *it)
{
    it->anim = 0;
}

// Shown with the reveal finished.
void IconItem_ShowNow(ArcIconItem *it)
{
    it->anim = it->revealSteps;
}

// Starts the close animation.
void IconItem_Hide(ArcIconItem *it)
{
    it->anim = ~it->revealSteps;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/iconitem", IconItem_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/iconitem", IconItem_Draw);
}
