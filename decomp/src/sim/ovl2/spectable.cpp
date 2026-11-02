// OVL2 8001894c-80019064 (spectable): Spec table.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/spectable", SpecTable_Init);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/spectable", SpecTable_Set);

// Hides all items immediately.
void SpecTable_Reset(ArcSpecTable *t)
{
    (t->weightLabel).anim = -1;
    (t->weightValue).anim = -1;
    (t->powerLabel).anim = -1;
    (t->powerValue).anim = -1;
    (t->torqueLabel).anim = -1;
    (t->torqueValue).anim = -1;
    t->anim = -1;
}

// Starts the close animation of the 6 items (if shown).
void SpecTable_Close(ArcSpecTable *t)
{
    if (-1 < t->anim) {
        TextItem_Hide_OVL2(&t->weightLabel);
        TextItem_Hide_OVL2(&t->weightValue);
        TextItem_Hide_OVL2(&t->powerLabel);
        TextItem_Hide_OVL2(&t->powerValue);
        TextItem_Hide_OVL2(&t->torqueLabel);
        TextItem_Hide_OVL2(&t->torqueValue);
        t->anim = -1;
    }
}

// Advances the 6 TextItems.
void SpecTable_Update(ArcSpecTable *t)
{
    TextItem_Update_OVL2(&t->weightLabel);
    TextItem_Update_OVL2(&t->weightValue);
    TextItem_Update_OVL2(&t->powerLabel);
    TextItem_Update_OVL2(&t->powerValue);
    TextItem_Update_OVL2(&t->torqueLabel);
    TextItem_Update_OVL2(&t->torqueValue);
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/spectable", SpecTable_Draw);
}
