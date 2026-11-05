// OVL2 80011ebc-800121e0 (gradientbar): Gradient bar.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
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

// The colours fade to the end colour and the bar widens as the counter runs out.
void GradientBar_Draw(ArcGradientBar *bar, void *ot, int x, int y)
{
    int w = bar->w;
    int h = bar->h;
    int counter = bar->counter;
    int length = bar->length;
    int t;
    int left;
    int right;
    int inset;
    int endR;
    int endG;
    int endB;
    int r0;
    int g0;
    int b0;
    int r1;
    int g1;
    int b1;
    int dr0;
    int dg0;
    int db0;
    int dr1;
    int dg1;
    int db1;
    int code;
    PrimG4 *p;
    unsigned int c0;
    unsigned int c1;

    if (counter != -1) {
        t = length - counter;
        if (counter < 0) {
            int last = length + 1;

            t = last + counter;
        }
        if (bar->flags & 1) {
            left = x - w;
            right = x;
        } else {
            left = x;
            right = left + w;
        }
        inset = w * t / length;
        r0 = bar->start[0];
        endR = bar->end[0];
        dr0 = (endR - r0) * t / length;
        g0 = bar->start[1];
        endG = bar->end[1];
        dg0 = (endG - g0) * t / length;
        b0 = bar->start[2];
        endB = bar->end[2];
        db0 = (endB - b0) * t / length;
        r1 = bar->start2[0];
        dr1 = (endR - r1) * t / length;
        g1 = bar->start2[1];
        dg1 = (endG - g1) * t / length;
        b1 = bar->start2[2];
        db1 = (endB - b1) * t / length;
        left -= inset;
        right += inset;
        r0 += dr0;
        g0 += dg0;
        code = bar->start[3];
        b0 += db0;
        r1 += dr1;
        g1 += dg1;
        b1 += db1;
        p = Prim_AddPolyG4((uint *)ot);
        c0 = (code | 0x38) << 24 | (r0 | g0 << 8 | b0 << 16);
        c1 = r1 | g1 << 8 | b1 << 16;
        p->c2 = c0;
        p->c0 = c0;
        p->c3 = c1;
        p->c1 = c1;
        p->x2 = left;
        p->x0 = left;
        p->x3 = right;
        p->x1 = right;
        p->y1 = y;
        p->y0 = y;
        p->y3 = y + h;
        p->y2 = y + h;
    }
}
}
