// Resident code 8006c460-8006cdcc (textitem). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void TextItem_Show(TextItem *item, int endFrame);
size_t strlen(char *__s);

INCLUDE_ASM("asm/sim/exe/nonmatchings/textitem", TextItem_Init);

void TextItem_Show(TextItem *item, int endFrame)
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

void TextItem_ShowNow(TextItem *t)
{
    TextItem_Show(t, -1);
    t->anim = t->animEnd;
}

void TextItem_Hide(TextItem *item)
{
    size_t sVar1;

    item->anim = ~item->revealSteps;
    sVar1 = strlen(item->text);
    item->length = (short)sVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/textitem", TextItem_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/textitem", TextItem_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/textitem", TextItem_MeasureWidth);
}
