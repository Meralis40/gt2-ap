// Resident code 8006d9c8-8006dcb8 (numberpicker): a number chosen with left and right,
// drawn between two arrows.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern char D_8008FAAC[]; // "%d"

void NumberPicker_Init(NumberPicker *p)
{
    p->anim = -1;
    p->value = p->min;
}

// Steps the value with left and right; returns it on cross or circle, -1 on triangle or
// square, and -2 otherwise.
int NumberPicker_Update(NumberPicker *p, PadState *pad)
{
    int result = -2;
    int v;
    int old;
    PadButton buttons;

    if (p->anim >= 0) {
        if (++p->anim >= 46) {
            p->anim = 0;
        }
        if (pad != 0) {
            v = p->value;
            old = v;
            buttons = pad->pressed;
            if (buttons & (PAD_TRIANGLE | PAD_SQUARE)) {
                result = -1;
            } else if (buttons & (PAD_CROSS | PAD_CIRCLE)) {
                result = v;
            } else {
                buttons |= pad->repeat;
                if (buttons & PAD_LEFT) {
                    v--;
                }
                if (buttons & PAD_RIGHT) {
                    v++;
                }
                if (v < p->min) {
                    v = p->min;
                }
                if (v > p->max) {
                    v = p->max;
                }
                if (v != old && p->sfx >= 0) {
                    Sfx_PlayMenu(p->sfx);
                }
                p->value = v;
            }
        }
    }
    return result;
}

// Draws the value with an arrow on each side it can still move to, fading in over the
// first frames of the animation.
void NumberPicker_Draw(NumberPicker *p, uint *ot, TextCtx *ctx)
{
    char text[64];
    int x;
    int y;
    short w;
    int t;
    int c;
    short ax;
    PrimF3 *tri;

    if (p->anim < 0) {
        return;
    }
    sprintf(text, D_8008FAAC, p->value);
    Text_SetFont(ctx, p->font);
    ctx->ot = ot;
    ctx->rgb = p->rgb;
    Text_DrawCenteredMonoDigits(ctx, text, p->x, p->y + (p->unk14 >> 1), p->spacing, p->unk18, 0);
    t = 40 - p->anim;
    x = p->x;
    y = p->y;
    w = ctx->font.digitWidth * 2;
    if (t > 10) {
        t = 10;
    }
    if (t < 0) {
        t = 0;
    }
    c = t * 255 / 10;
    if (p->value != p->max) {
        tri = Prim_AddPolyF3(ot, c | (c >> 1) << 8 | 0x2000000);
        ax = x; // in two steps, for the operand order of the addu
        ax += w;
        tri->x0 = ax;
        tri->x1 = ax + 6;
        tri->x2 = ax;
        tri->y0 = y - 10;
        tri->y1 = y;
        tri->y2 = y + 10;
    }
    if (p->value != p->min) {
        tri = Prim_AddPolyF3(ot, c | (c >> 1) << 8 | 0x2000000);
        ax = x;
        ax -= w;
        tri->x0 = ax;
        tri->x1 = ax - 6;
        tri->x2 = ax;
        tri->y0 = y - 10;
        tri->y1 = y;
        tri->y2 = y + 10;
    }
}
}
