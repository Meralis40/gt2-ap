// OVL0 8005a474-8005a77c (ending): Ending.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern QuickMenu *g_QuickMenu;
extern short g_Ending_Fade;

// Root page after winning the GT World League: 16-frame delay, music 0.
void EndingRoot_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x10;
    QuickMenu_PlayMusic(0);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ending", EndingRoot_Update);

// Empty.
void EndingRoot_Draw(DrawLayer *layer)
{
}

// Resets the fade counter.
void Ending_Enter(int back)
{
    g_Ending_Fade = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ending", Ending_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ending", Ending_Draw);
}
