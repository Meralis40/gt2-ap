// OVL0 80049a14-8004a718 (resultmenu): Result menu pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern QuickMenu *g_QuickMenu;
extern void *g_OVL0_EndFontInfo;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultmenu", ResultMenu_ItemCallback);

// Root page after an arcade race, a 2P battle or a championship race: stores back,
// waits 20 frames, sets up the card UI (QuickMenu_SetupCardUi on work+0x6e8) and plays
// music 8.
void ResultRoot_Enter(int back)
{
    void *pvVar1;
    QuickMenuPage *pQVar2;

    pvVar1 = g_OVL0_EndFontInfo;
    pQVar2 = g_QuickMenu->activePage;
    pQVar2->sel = (short)back;
    pQVar2->timer = 0x14;
    QuickMenu_SetupCardUi((void *)((int)pvVar1 + 0x6e8));
    QuickMenu_PlayMusic(8);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultmenu", ResultRoot_Update);

// Empty.
void ResultRoot_Draw(DrawLayer *layer)
{
}

// g_Page_Quit, pushed by most result pages once a choice is made (QuickMenu.unk7c
// holds it): 20-frame timer and music stop.
void QuitPage_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x14;
    QuickMenu_StopMusic();
}

// Returns 4 (leave the QuickMenu) when the timer runs out.
int QuitPage_Update(int active)
{
    unsigned int uVar1;

    uVar1 = (unsigned short)g_QuickMenu->activePage->timer - 1;
    g_QuickMenu->activePage->timer = (short)uVar1;
    return uVar1 >> 0xd & 4;
}

// Empty.
void QuitPage_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultmenu", ResultMenu_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultmenu", ResultMenu_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultmenu", ResultMenu_Draw);
}
