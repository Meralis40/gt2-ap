// OVL2 80018548-8001894c (statbar): Stat bars.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char D_8004F67C[];


// Hidden, with the label text.
void StatBar_Init(ArcStatBar *b, void *text)
{
    ArcTextItem *label = &b->label;

    b->anim = -1;
    TextItem_Init_OVL2(label, D_8004F67C);
    label->text = (char *)text;
    b->text = (char *)text;
}

// Starts the open animation and the label reveal.
void StatBar_Show(ArcStatBar *b)
{
    b->anim = 0;
    TextItem_Show_OVL2(&b->label, -1);
}

// Hides the bar and starts the label close animation.
void StatBar_Hide(ArcStatBar *b)
{
    b->anim = -1;
    TextItem_Hide_OVL2(&b->label);
}

// Advances the label and the bar animation (up to 0x20, or back to -1).
void StatBar_Update(ArcStatBar *b)
{
    short sVar1;

    TextItem_Update_OVL2(&b->label);
    if (b->anim < 0) {
        if (b->anim < -1) {
            b->anim = b->anim + 1;
        }
    }
    else {
        sVar1 = b->anim + 1;
        b->anim = sVar1;
        if (0x20 < sVar1) {

            b->anim = 0x20;
        }
    }
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/statbar", StatBar_Draw);
}
