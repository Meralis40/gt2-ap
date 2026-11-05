// OVL2 8001824c-80018548 (colorchips): Colour chips.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
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

// A column of swatches 10 apart, each lighting up in turn, the selected one with a
// brighter outline.
void ColorChips_Draw(ArcColorChips *c, void *ot, int x, int y)
{
    if (c->anim >= 0) {
        int i = 0;
        unsigned short *colors;

        y -= c->count * 10;
        colors = (unsigned short *)c;
        while (i < c->count) {
            int t = c->anim - i * 24 / c->count;

            if (t >= 0) {
                int fade = 8 - t;
                GouraudQuad quad;
                unsigned int col;
                int r;
                int g;
                int b;
                RECT *tile;
                PrimLineF4 *line;
                int left;

                if (fade < 0) {
                    fade = 0;
                }
                col = colors[2];
                r = (col & 0x1f) << 3;
                g = (col & 0x3e0) >> 2;
                b = (col & 0xf800) >> 7;
                r += (0xff - r) * fade / 8;
                g += (0xff - g) * fade / 8;
                b += (0xff - b) * fade / 8;
                quad.rect.x = x;
                quad.rect.y = y;
                quad.rect.w = 8;
                quad.rect.h = 6;
                quad.top = r | g << 8 | b << 16;
                quad.bottom = r >> 2 | (g >> 2) << 8 | (b >> 2) << 16;
                Menu_AddGouraudQuad((uint *)ot, &quad);
                tile = Prim_AddTile(ot, 0);
                tile->w = 8;
                tile->x = x;
                tile->y = y;
                tile->h = 6;
                fade = 0x28;
                if (i == c->selected) {
                    fade = 0x80;
                }
                line = Prim_AddLineF4(ot, fade | fade << 8 | fade << 16);
                left = x - 1;
                line->x2 = x + 8;
                line->x1 = x + 8;
                line->y4 = y - 1;
                line->y1 = y - 1;
                line->y0 = y - 1;
                line->x4 = left;
                line->x3 = left;
                line->x0 = left;
                line->y3 = y + 6;
                line->y2 = y + 6;
            }
            y += 10;
            colors++;
            i++;
        }
        Prim_AddDrawMode(ot, 0x20);
    }
}
}
