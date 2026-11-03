// OVL1 80017d74-80019218 (options): Options pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern BaseMenu *g_CurrentMenu;

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", Option_Get);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", Option_Set);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", Option_Step);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", Option_Draw);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionList_ItemCallback);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionsMenu_TabCallback);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionsMenu_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionsMenu_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionsMenu_Draw);

// Root page of Options: stores back, waits 16 frames (no music change).
void OptionsRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].enter = (MenuPageEnterFn *)0x10;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionsRoot_Update);

// Empty.
void OptionsRoot_Draw(DrawLayer *layer)
{
}

// Unreferenced page: 16-frame delay, MainMenu_StopMusic.
void OptionsUnusedWait_Enter(int back)
{
    g_CurrentMenu->activePage[1].enter = (MenuPageEnterFn *)0x10;
    MainMenu_StopMusic();
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionsUnusedWait_Update);

// Empty.
void OptionsUnusedWait_Draw(DrawLayer *layer)
{
}
}
