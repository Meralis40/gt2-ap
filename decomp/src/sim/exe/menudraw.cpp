// Resident code 8006b548-8006bcb8 (menudraw). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
PrimG4 *Prim_AddPolyG4(uint *ot);
PrimF3 *Prim_AddPolyF3(uint *ot, uint rgb);
RECT *Prim_AddTile(uint *ot, uint rgb);
void *Prim_AddDrawMode(uint *ot, uint mode);

unsigned int Rgb_Lerp(unsigned char *from, unsigned char *to, int t, int range)
{
    if (range < t) {
        t = range;
    }
    if (t < 0) {
        t = 0;
    }
    return (unsigned int)from[3] << 0x18 |
                  ((unsigned int)*from + (int)(((unsigned int)*to - (unsigned int)*from) * t) / range |
                    ((unsigned int)from[1] + (int)(((unsigned int)to[1] - (unsigned int)from[1]) * t) / range) * 0x100 |
                  ((unsigned int)from[2] + (int)(((unsigned int)to[2] - (unsigned int)from[2]) * t) / range) * 0x10000) & 0xffffff;
}

// Menu_AddGouraudQuad with grey levels (0-255) for colours.
void Menu_AddGreyBand(uint *ot, GouraudQuad *q)
{
    uint top = q->top;
    uint bottom = q->bottom;
    PrimG4 *p = Prim_AddPolyG4(ot);
    uint c;

    c = top | top << 8 | top << 16 | 0x3a000000;
    p->c2 = c;
    p->c0 = c;
    c = bottom | bottom << 8 | bottom << 16;
    p->c3 = c;
    p->c1 = c;
    p->x0 = p->x2 = q->rect.x;
    p->x1 = p->x3 = q->rect.x + q->rect.w;
    p->y0 = p->y1 = q->rect.y;
    p->y2 = p->y3 = q->rect.y + q->rect.h;
}

// Adds a semi-transparent (additive) gradient quad.
void Menu_AddGouraudQuad(uint *ot, GouraudQuad *q)
{
    PrimG4 *p = Prim_AddPolyG4(ot);

    p->c0 = p->c2 = q->top | 0x3a000000;
    p->c1 = p->c3 = q->bottom;
    p->x0 = p->x2 = q->rect.x;
    p->x1 = p->x3 = q->rect.x + q->rect.w;
    p->y0 = p->y1 = q->rect.y;
    p->y2 = p->y3 = q->rect.y + q->rect.h;
}

void Menu_AddGouraudQuadOpaque(uint *ot, GouraudQuad *q)
{
    PrimG4 *p = Prim_AddPolyG4(ot);

    p->c0 = p->c2 = q->top | 0x38000000;
    p->c1 = p->c3 = q->bottom;
    p->x0 = p->x2 = q->rect.x;
    p->x1 = p->x3 = q->rect.x + q->rect.w;
    p->y0 = p->y1 = q->rect.y;
    p->y2 = p->y3 = q->rect.y + q->rect.h;
}

// Draws the cursor box over box->rect, centred on its x: it grows out from the middle
// over the first 16 frames (t), and its colour fades from box->bottom back to
// box->top between frames 16 and 64; it is drawn additive until then.
#ifdef NON_MATCHING
// The game keeps one more value in a saved register ($fp holds full) and gives w - part
// box's register; the code is otherwise the game's.
void Menu_DrawCursorBox(uint *ot, GouraudQuad *box, int t)
{
    int w = box->rect.w;
    int x = box->rect.x - (w >> 1);
    int full = 1;
    int n = t;
    int part;
    int k;
    uint mode;
    uint c;
    uint from;
    GouraudQuad q;

    if (n > 16) {
        full = 0;
        n = 16;
    }
    part = w * n / 16;
    q.rect.x = x;
    q.rect.w = part;
    q.rect.y = box->rect.y;
    mode = 0x200;
    k = 64 - t;
    q.rect.h = box->rect.h;
    if (k < 0) {
        k = 0;
        mode = k;
    }
    if (k > 48) {
        k = 48;
    }
    c = Rgb_Lerp((unsigned char *)&box->top, (unsigned char *)&box->bottom, k, 48);
    from = box->top;
    q.bottom = c;
    q.top = from;
    Menu_AddGouraudQuad(ot, &q);
    q.rect.x = x + (w - part);
    q.top = c;
    q.bottom = from;
    Menu_AddGouraudQuad(ot, &q);
    if (full) {
        q.rect.x = x + part;
        q.rect.w = w - part;
        Menu_AddGouraudQuad(ot, &q);
        q.rect.x = x;
        q.top = from;
        q.bottom = c;
        Menu_AddGouraudQuad(ot, &q);
    }
    Prim_AddDrawMode(ot, mode | 0x20);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/menudraw", Menu_DrawCursorBox);
#endif

// Draws an up arrow of the given half width and height (negative for down), fading in
// as anim goes from 40 to 30.
void Menu_DrawScrollArrow(void *self, uint *ot, int x, int y, int halfWidth, int height, int anim)
{
    int t = 40 - anim;
    PrimF3 *tri;

    if (t > 10) {
        t = 10;
    }
    if (t < 0) {
        t = 0;
    }
    t = t * 255 / 10;
    tri = Prim_AddPolyF3(ot, t | (t >> 1) << 8 | 0x2000000);
    tri->x0 = x;
    tri->y0 = y + height;
    tri->x1 = x + halfWidth;
    tri->y1 = y;
    tri->x2 = x - halfWidth;
    tri->y2 = y;
}

// Menu_DrawScrollArrow pointing sideways.
void Menu_DrawSideArrow(void *self, uint *ot, int x, int y, int width, int halfHeight, int anim)
{
    int t = 40 - anim;
    PrimF3 *tri;

    if (t > 10) {
        t = 10;
    }
    if (t < 0) {
        t = 0;
    }
    t = t * 255 / 10;
    tri = Prim_AddPolyF3(ot, t | (t >> 1) << 8 | 0x2000000);
    tri->x0 = x + width;
    tri->y0 = y;
    tri->x1 = x;
    tri->y1 = y + halfHeight;
    tri->x2 = x;
    tri->y2 = y - halfHeight;
}

// Draws a swatch of the RGB555 colour q->top scaled by scale/16, shaded from grey on
// the left, inside a black frame.
void Menu_DrawColorSwatch(uint *ot, GouraudQuad *q, int scale)
{
    uint c = q->top;
    int r = (c & 0x1f) * scale >> 4;
    int g = (c & 0x3e0) * scale >> 9;
    int b = (c & 0x7c00) * scale >> 14;
    int grey = scale * 244 >> 7;
    PrimG4 *p;
    RECT *tile;

    if (grey > 255) {
        grey = 255;
    }
    if (r > 255) {
        r = 255;
    }
    if (g > 255) {
        g = 255;
    }
    if (b > 255) {
        b = 255;
    }
    p = Prim_AddPolyG4(ot);
    p->c0 = grey | grey << 8 | grey << 16 | 0x38000000;
    p->c1 = r | g << 8 | b << 16;
    p->c2 = 0;
    p->c3 = 0;
    p->x0 = p->x2 = q->rect.x + 1;
    p->x1 = p->x3 = q->rect.x + q->rect.w - 1;
    p->y0 = p->y1 = q->rect.y + 1;
    p->y2 = p->y3 = q->rect.y + q->rect.h - 1;
    tile = Prim_AddTile(ot, 0);
    tile->x = q->rect.x;
    tile->w = q->rect.w;
    tile->y = q->rect.y;
    tile->h = q->rect.h;
    Prim_AddDrawMode(ot, 0x200);
}
}
