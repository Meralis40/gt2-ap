// OVL2 80019518-80019e60 (carspecpanel2): Car spec panel.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carspecpanel2", CarSpecPanel_Show);

// Advances every widget of both slots.
void CarSpecPanel_Update(void *panel)
{
    ArcStatBar *b;
    ArcTextItem *item;
    int iVar1;
    int iVar2;
    int iVar3;
    int iVar4;
    int iVar5;
    int iVar6;

    iVar6 = 0;
    iVar5 = 0x250;
    iVar4 = 0x6f0;
    iVar3 = 0x118;
    iVar2 = 0x88;
    iVar1 = 0x58;
    item = panel;
    do {
        TextItem_Update_OVL2(item);
        IconItem_Update((ArcIconItem *)((int)panel + iVar1));
        PowerGraphView_Update_OVL2((short *)((int)panel + iVar2));
        b = (ArcStatBar *)((int)panel + iVar3);
        StatBar_Update(b);
        StatBar_Update(b + 1);
        StatBar_Update(b + 2);
        ColorChips_Update((ArcColorChips *)((int)panel + iVar4));
        SpecTable_Update((ArcSpecTable *)((int)panel + iVar5));
        iVar5 = iVar5 + 0x250;
        iVar4 = iVar4 + 0x86;
        iVar3 = iVar3 + 0x9c;
        iVar2 = iVar2 + 0x44;
        iVar1 = iVar1 + 0x18;
        iVar6 = iVar6 + 1;
        item = item + 1;
    } while (iVar6 < 2);
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carspecpanel2", CarSpecPanel_Draw);
}
