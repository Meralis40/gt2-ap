// OVL2 80011ebc-800121e0 (gradientbar): Gradient bar.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


// bar: +0 counter (-1 hidden), +6 w, +8 h, +0xa length, +0xc flags (bit 0 grows
// leftwards), +0x10/+0x14 start colours, +0x18 end colour. desc: w, h, colours, flags,
// length.
void GradientBar_Init(short *bar, short *desc)
{
    short sVar1;

    bar[3] = *desc;
    bar[4] = desc[1];
    *(unsigned int *)(bar + 8) = *(unsigned int *)(desc + 2);
    *(unsigned int *)(bar + 10) = *(unsigned int *)(desc + 4);
    *(unsigned int *)(bar + 0xc) = *(unsigned int *)(desc + 6);
    bar[6] = desc[9];
    sVar1 = desc[8];
    *bar = -1;
    bar[5] = sVar1;
}

// counter = 0 (open animation).
void GradientBar_Start(short *bar)
{
    *bar = 0;
}

// counter = ~length (close animation).
void GradientBar_Close(short *bar)
{
    *bar = ~bar[5];
}

// Same as FadeCounter_Update: counts up to the length, or from a negative value up to -1.
void GradientBar_Update(short *bar)
{
    short sVar1;

    if (*bar < 0) {
        if (*bar < -1) {
            *bar = *bar + 1;
            return;
        }
    }
    else {
        sVar1 = *bar + 1;
        *bar = sVar1;
        if (bar[5] <= sVar1) {
            *bar = bar[5];
        }
    }
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/gradientbar", GradientBar_Draw);
}
