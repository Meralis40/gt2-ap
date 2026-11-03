// OVL1 8001191c-800124a4 (basemenu): BaseMenu, the main menu screen loop, and the menu
// music.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__DrawPage);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__BaseMenu);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__BaseMenu_dtor);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__SetRootPage);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__GetTransitionRects);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__Draw);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", PushPage);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__PopPage);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__Leave);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__BeginFrame);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", FUN_OVL1__800123a4);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", FUN_OVL1__800123d0);

// Music_PlayTrack(track, loop).
void MainMenu_PlayMusic(int track)
{
    Music_PlayTrack(track, 1);
}

// Music_Stop.
void MainMenu_StopMusic(void)
{
    Music_Stop();
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu____tf);
}
