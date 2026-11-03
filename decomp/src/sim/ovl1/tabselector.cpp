// OVL1 8001d4b0-8001da08 (tabselector): Tab selector.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
// Horizontal tab selector (Left/Right change tab). callback(event, ev, tab): 0 update,
// 1 draw, 3 confirm, 4 changed.
void TabSelector_Init(TabSelector *tabs, void *callback, void *userData)
{
    tabs->selected = '\0';
    tabs->anim = -1;
    tabs->hadInput = '\0';
    tabs->active = '\0';
    tabs->callback = callback;
    tabs->userData = userData;
    tabs->skipInput = '\0';
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/tabselector", TabSelector_Open);

// Starts the close animation.
void TabSelector_Close(TabSelector *tabs)
{
    tabs->anim = -0x11;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/tabselector", TabSelector_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/tabselector", TabSelector_GetFade);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/tabselector", TabSelector_Draw);
}
