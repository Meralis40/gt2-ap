// OVL0 80057a70-800593f4 (gtracemenu): GT race menu and machine test.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern QuickMenu *g_QuickMenu;
extern void *g_OVL0_EndFontInfo;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", GtRaceMenu_ItemCallback);

// Root page of the GT race menu: stores back, 20-frame delay, card UI on the work area
// after the settings work, music 8.
void GtRaceRoot_Enter(int back)
{
    void *pvVar1;
    QuickMenuPage *pQVar2;

    pvVar1 = g_OVL0_EndFontInfo;
    pQVar2 = g_QuickMenu->activePage;
    pQVar2->sel = (short)back;
    pQVar2->timer = 0x14;
    pvVar1 = Settings_SetWork((void *)((int)pvVar1 + 0x5ec));
    QuickMenu_SetupCardUi(pvVar1);
    QuickMenu_PlayMusic(8);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", GtRaceRoot_Update);

// Empty.
void GtRaceRoot_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", GtRaceMenu_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", GhostType_Next);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", GtRaceMenu_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", GtRaceMenu_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", MachineTestMenu_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", MachineTestMenu_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", MachineTestMenu_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", MachineTestRecord_DrawRow);

// Machine test ranking page: resets the slide-in counter.
void MachineTestRecords_Enter(int back)
{
    QuickMenuPage *pQVar1;

    pQVar1 = g_QuickMenu->activePage;
    pQVar1->timer = 0;
    pQVar1->sel = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", MachineTestRecords_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtracemenu", MachineTestRecords_Draw);
}
