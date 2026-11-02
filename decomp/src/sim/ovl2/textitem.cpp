// OVL2 80019fe4-8001aab4 (textitem): Text items: another version of the resident
// TextItem, with another layout.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/textitem", TextItem_Init_OVL2);

// Copy of TextItem_Show: starts the per-character reveal; endFrame < 0 = derived from
// the length.
void TextItem_Show_OVL2(ArcTextItem *item, int endFrame)
{
    size_t sVar1;

    item->anim = 0;
    sVar1 = strlen(item->text);
    item->length = (short)sVar1;
    item->animEnd = item->revealSteps + (short)((int)(short)sVar1 / (int)(unsigned int)item->charsPerStep);
    if (-1 < endFrame) {
        item->animEnd = (short)endFrame;
    }
}

// Copy of TextItem_ShowNow: shown with the reveal finished.
void TextItem_ShowNow_OVL2(ArcTextItem *item)
{
    TextItem_Show_OVL2(item, -1);
    item->anim = item->animEnd;
}

// Copy of TextItem_Hide: starts the close animation.
void TextItem_Hide_OVL2(ArcTextItem *item)
{
    size_t sVar1;

    item->anim = ~item->revealSteps;
    sVar1 = strlen(item->text);
    item->length = (short)sVar1;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/textitem", TextItem_Update_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/textitem", TextItem_Draw_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/textitem", TextItem_MeasureWidth_OVL2);
}
