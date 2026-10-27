// Resident code 8006e1cc-8006ebf0 (carddialog). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void TextItem_Hide(TextItem *item);
void TextItem_Show(TextItem *item, int endFrame);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carddialog", CardDialog_Init);

void CardDialog_Open(CardDialog *dlg)
{
    TextItem_Show(&dlg->message, 0x3c);
    TextItem_Show(&dlg->option0, -1);
    TextItem_Show(&dlg->option1, -1);
    dlg->anim = 0;
    dlg->flash = 0;
    dlg->flags = dlg->flags | 0x80;
}

void FUN_8006e3e0(int param_1)
{
    *(short *)(param_1 + 0x82) = 0xffff;
    *(short *)(param_1 + 0x84) = 0;
    *(short *)(param_1 + 0x26) = 0xffff;
    *(short *)(param_1 + 0x4e) = 0xffff;
    *(short *)(param_1 + 0x76) = 0xffff;
}

void CardDialog_Close(CardDialog *dlg)
{
    dlg->anim = -0x11;
    TextItem_Hide(&dlg->message);
    TextItem_Hide(&dlg->option0);
    TextItem_Hide(&dlg->option1);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carddialog", CardDialog_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carddialog", CardDialog_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carddialog", FUN_8006ea38);

void CardSlots_Init(short *status)
{
    *status = 2;
    status[1] = 2;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carddialog", CardSlots_Update);
}
