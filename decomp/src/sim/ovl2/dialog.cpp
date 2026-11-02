// OVL2 8001aab4-8001b37c (dialog): Two-option dialog.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/dialog", Dialog_Init_OVL2);

// Starts the open animation and the text reveals.
void Dialog_Open_OVL2(ArcDialog *dlg)
{
    TextItem_Show_OVL2(&dlg->message, 0x3c);
    TextItem_Show_OVL2(&dlg->option0, -1);
    TextItem_Show_OVL2(&dlg->option1, -1);
    dlg->anim = 0;
    dlg->flash = 0;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/dialog", FUN_OVL2__8001ac40);

// Starts the close animation (anim -17).
void Dialog_Close_OVL2(ArcDialog *dlg)
{
    dlg->anim = -0x11;
    TextItem_Hide_OVL2(&dlg->message);
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/dialog", Dialog_Update_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/dialog", Dialog_Draw_OVL2);
}
