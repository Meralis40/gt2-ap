// OVL0 80048bd8-80049a14 (resultwidgets): Result screen widgets.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", TitleBanner_Init);

// Title text centred at x, wipe bar from x - 0x80.
void TitleBanner_Draw(TitleBanner *b, unsigned int *ot, int x, int y, TextCtx *ctx)
{
    int iVar1;

    iVar1 = TextItem_MeasureWidth(&b->text, ctx);
    TextItem_Draw(&b->text, ot, x - (iVar1 >> 1), y + 8, ctx);
    WipeBar_Draw(&b->bar, ot + 1, x + -0x80, y - ((int)((unsigned int)(unsigned short)(b->bar).height << 0x10) >> 0x11));
    Prim_AddDrawMode((OtTag *)(ot + 1), 0x220);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", InfoBar_Init);

// Updates the label and bar, and fades the value in over 16 frames (or out when anim <
// -1).
void InfoBar_Update(InfoBar *b)
{
    short sVar1;

    TextItem_Update(&b->label);
    WipeBar_Update(&b->bar);
    if (b->anim < 0) {
        if (b->anim < -1) {
            b->anim = b->anim + 1;
        }
    }
    else {
        sVar1 = b->anim + 1;
        b->anim = sVar1;
        if (0x10 < sVar1) {
            b->anim = 0x10;
        }
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", InfoBar_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", LapTime_Draw);

// c[0] = max, c[1] = counter: counts up to max, or from a negative (closing) value up
// to -1. Copy of the resident FadeCounter.
void FadeCounter_Update(short *c)
{
    short sVar1;

    if (c[1] < 0) {
        if (c[1] < -1) {
            c[1] = c[1] + 1;
            return;
        }
    }
    else {
        sVar1 = c[1] + 1;
        c[1] = sVar1;
        if (*c <= sVar1) {
            c[1] = *c;
        }
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", AnimatedTime_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", RecordRow_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", RecordRow_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", ResultCarView_Init);

// Turns the car 0x10 per frame; after frame 0xfb90 also pulls the camera back.
void ResultCarView_Spin(ResultCarView *view, int frame)
{
    view->rotY = view->rotY + 0x10U & 0x3fff;
    if (0xfb90 < frame) {
        view->pos[2] = view->pos[2] + 0x8000;
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", Kanji_DrawFaded);
}
