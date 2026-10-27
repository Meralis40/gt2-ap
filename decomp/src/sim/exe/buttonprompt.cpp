// Resident code 8006dcb8-8006e1cc (buttonprompt). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void TextItem_Hide(TextItem *item);
void TextItem_Show(TextItem *item, int endFrame);

INCLUDE_ASM("asm/sim/exe/nonmatchings/buttonprompt", ButtonPrompt_Init);

void ButtonPrompt_Open(void *p)
{
    TextItem_Show((TextItem *)((int)p + 4), -1);
    *(unsigned short *)((int)p + 0x32) = 0;
    *(unsigned short *)((int)p + 0x2c) = *(unsigned short *)((int)p + 0x2c) | 0x80;
}

void FUN_8006de0c(int param_1)
{
    *(short *)(param_1 + 0x32) = 0xffff;
    *(short *)(param_1 + 0x26) = 0xffff;
}

void ButtonPrompt_Close(void *p)
{
    *(short *)((int)p + 0x32) = 0xfff3;
    TextItem_Hide((TextItem *)((int)p + 4));
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/buttonprompt", ButtonPrompt_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/buttonprompt", ButtonPrompt_Draw);
}
