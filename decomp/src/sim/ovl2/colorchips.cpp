// OVL2 8001824c-80018548 (colorchips): Colour chips.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


// Hidden (anim -1), selected = 0.
void ColorChips_Init(ArcColorChips *c)
{
    c->anim = -1;
    c->selected = 0;
}

// Sets the colours, and starts the open animation.
void ColorChips_Set(ArcColorChips *c, int count, unsigned short *colors)
{
    int i = 0;

    if ((count <= 0) | (colors == 0)) {
        return;
    }
    c->anim = 0;
    c->count = count;
    if (count > 0) {
        do {
            c->colors[i] = *colors++;
            i++;
        } while (i < count);
    }
}

// Starts the close animation (anim -17).
void ColorChips_Close(ArcColorChips *c)
{
    c->anim = -0x11;
}

// Opening: anim up to 0x20; closing: up to -1.
void ColorChips_Update(ArcColorChips *c)
{
    short sVar1;

    if (c->anim < 0) {
        if (c->anim < -1) {
            c->anim = c->anim + 1;
            return;
        }
    }
    else {
        sVar1 = c->anim + 1;
        c->anim = sVar1;
        if (0x20 < sVar1) {

            c->anim = 0x20;
        }
    }
}

// Sets the highlighted colour.
void ColorChips_Select(ArcColorChips *c, int index)
{
    c->selected = index;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/colorchips", ColorChips_Draw);
}
