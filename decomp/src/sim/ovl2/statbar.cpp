// OVL2 80018548-8001894c (statbar): Stat bars.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char D_8004F67C[];
extern Font D_80129840;
extern char D_800F8661[]; // shown for no value


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

#ifdef NON_MATCHING
// The label, then ten cells filled up to value, the last two red. A value below zero
// shows the text for no value instead.
void StatBar_Draw(ArcStatBar *b, void *ot, int x, int y, TextCtx *ctx, int value)
{
    ArcTextItem *label = &b->label;
    int r;
    int g;
    int bl;
    int i;
    int t;
    int anim;

    label->x = x - 4;
    label->y = y;
    TextItem_Draw_OVL2(label, ot, ctx);
    anim = b->anim;
    if (anim >= 0) {
        if (value < 0) {
            int fade = 8 - anim;

            if (fade < 0) {
                fade = 0;
            }
            r = 0x6f;
            g = 0x4b;
            bl = 0x2d;
            r += (0xff - r) * fade / 8;
            g += (0xff - g) * fade / 8;
            bl += (0xff - bl) * fade / 8;
            Text_SetFont(ctx, &D_80129840);
            ctx->rgb = r | g << 8 | bl << 16 | 0x2000000;
            ctx->ot = (uint *)ot;
            *(uint *)&ctx->clut = (*(uint *)&ctx->clut & 0xff9fffff) | 0x200000;
            Text_DrawString(ctx, (signed char *)D_800F8661, x, y, 1);
        }
        i = 0;
        do {
            int cellX = x + i * 5;
            int cellY = y - 12;
            int wait;
            wait = i * 24 / 10;
            t = b->anim - wait;

            if (t >= 0) {
                RECT *tile;

                t = 8 - t;
                if (t < 0) {
                    t = 0;
                }
                if (i < 8) {
                    r = 0xde;
                    g = 0x6f;
                    bl = 0x10;
                } else {
                    r = 0xf2;
                    g = 0xb;
                    bl = 0xb;
                }
                if (i >= value) {
                    r = 0;
                    g = 0;
                    bl = 0;
                    t = 0;
                }
                r += (0xff - r) * t / 8;
                g += (0xff - g) * t / 8;
                bl += (0xff - bl) * t / 8;
                tile = Prim_AddTile(ot, r | g << 8 | bl << 16 | 0x2000000);
                tile->w = 4;
                tile->x = cellX;
                tile->y = cellY;
                tile->h = 12;
                Prim_AddDrawMode(ot, 0x20);
            }
            {
                RECT *tile = Prim_AddTile(ot, 0x2081020);

                tile->w = 4;
                tile->x = cellX;
                tile->y = cellY;
                tile->h = 12;
                Prim_AddDrawMode(ot, 0);
            }
            i++;
        } while (i < 10);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/statbar", StatBar_Draw);
#endif
}
