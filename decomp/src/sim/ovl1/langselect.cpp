// OVL1 8001721c-80017720 (langselect): Language selection pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern short g_LangSelectFadeOut_Timer;

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/langselect", LangSelect_ItemCallback);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/langselect", LangSelect_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/langselect", LangSelect_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/langselect", LangSelect_Draw);

// Sets a 12-frame timer.
void LangSelectFadeOut_Enter(int back)
{
    g_LangSelectFadeOut_Timer = 0xc;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/langselect", LangSelectFadeOut_Update);

// Empty page draw callback.
void Page_DrawNothing(DrawLayer *layer)
{
}
}
