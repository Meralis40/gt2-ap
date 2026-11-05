// OVL2 80011bb0-80011ebc (sprite): Number to text and scaled sprites.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {

extern int g_OVL2_Pow10[8];

// The decimal digits of a value from 0 to 99999999.
void IntToStr_OVL2(char *out, int value)
{
    int i;
    int j;

    for (i = 7; i > 0; i--) {
        if (value >= g_OVL2_Pow10[i]) {
            break;
        }
    }
    if (i > 0) {
        j = i;
        do {
            int pow = g_OVL2_Pow10[j];
            int digit = value / pow;
            *out++ = digit + '0';
            value -= digit * pow;
        } while (--j > 0);
    }

    out[0] = value + '0';
    out[1] = 0;
}

// The sprite centred on (cx, cy), scaled by scale / 256 (0x80: its size), at grey level
// grey.
void Sprite_AddScaled(void *ot, ArcSprite *sprite, short cx, short cy, unsigned int grey, int scale)
{
    int w = sprite->w * scale;
    int h = sprite->h * scale;
    PrimFT4 *p = Prim_AddPolyFT4((uint *)ot, grey | grey << 8 | grey << 16);
    short x0 = cx - (w >> 8);
    short x1 = cx + (w >> 8);
    short y0;
    short y1;
    unsigned char u;
    unsigned char v;

    p->x2 = x0;
    p->x0 = x0;
    y0 = cy - (h >> 8);
    y1 = cy + (h >> 8);
    p->x3 = x1;
    p->x1 = x1;
    p->y1 = y0;
    p->y0 = y0;
    p->y3 = y1;
    p->y2 = y1;
    u = sprite->u;
    p->u2 = u;
    p->u0 = u;
    u = sprite->u + sprite->w - 1;
    p->u3 = u;
    p->u1 = u;
    v = sprite->v;
    p->v1 = v;
    p->v0 = v;
    v = sprite->v + sprite->h - 1;
    p->v3 = v;
    p->v2 = v;
    p->clut = sprite->clut;
    p->tpage = sprite->tpage;
}

// Sprite_AddScaled semi-transparent, with the blending mode abr in the tpage.
void Sprite_AddScaledSemi(void *ot, ArcSprite *sprite, short cx, short cy, unsigned int grey, int scale, unsigned short abr)
{
    int w = sprite->w * scale;
    int h = sprite->h * scale;
    PrimFT4 *p = Prim_AddPolyFT4((uint *)ot, grey | grey << 8 | grey << 16 | 0x2000000);
    short x0 = cx - (w >> 8);
    short x1 = cx + (w >> 8);
    short y0;
    short y1;
    unsigned char u;
    unsigned char v;

    p->x2 = x0;
    p->x0 = x0;
    y0 = cy - (h >> 8);
    y1 = cy + (h >> 8);
    p->x3 = x1;
    p->x1 = x1;
    p->y1 = y0;
    p->y0 = y0;
    p->y3 = y1;
    p->y2 = y1;
    u = sprite->u;
    p->u2 = u;
    p->u0 = u;
    u = sprite->u + sprite->w - 1;
    p->u3 = u;
    p->u1 = u;
    v = sprite->v;
    p->v1 = v;
    p->v0 = v;
    v = sprite->v + sprite->h - 1;
    p->v3 = v;
    p->v2 = v;
    p->clut = sprite->clut;
    p->tpage = sprite->tpage | abr;
}
}
