// OVL1 80016394-800169b0 (menusprite): Menu sprites, the main menu's row images.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

// The SPRT word of a sprite centred on (x, y).
static inline void MenuSprite_Place(PrimSprite *p, SpriteDesc *spr, int x, int y)
{
    *(int *)p = (x - (((short *)&spr->wh)[0] >> 1)) + ((y - (((short *)&spr->wh)[1] >> 1)) << 16);
    ((unsigned int *)p)[1] = spr->uvClut;
    ((unsigned int *)p)[2] = spr->wh;
}

extern "C" {
// Copies the style and hides the sprite (anim -1, brightness 0x80).
void MenuSprite_Init(MenuSprite *item, MenuSpriteStyle *style)
{
    int z;
    unsigned char b;

    item->flags = style->flags;
    z = style->zoomWidth;
    item->zoomWidth = z;
    b = style->brightness;
    item->anim = -1;
    item->brightness = 0x80;
    item->maxBrightness = b;
}

// Advances the open animation (0 to 12) or the close animation (negative, towards -1).
void MenuSprite_Update(MenuSprite *item)
{
    if (item->anim >= 0) {
        item->anim++;
        if (item->anim >= 12) {
            item->anim = 12;
        }
    } else if (item->anim < -1) {
        item->anim++;
    }
}

// Draws a menu row image. flags: 1 additive glow when selected, 2 semi-transparent,
// 4 zoom trail while opening, 0x18 blend mode, 0x60 anchor (0x20 left edge at x,
// 0x40 right edge). While opening it fades in from two copies zoomWidth apart; while
// closing it collapses as two gouraud bands.
// The game keeps the brightness and the trail flag in other registers, and its two
// spilled values in each other's stack slots.
#ifdef NON_MATCHING
void MenuSprite_Draw(MenuSprite *item, void *ot, int selected)
{
    int zoom = 1;
    int trail = 0;
    unsigned int fade = trail;
    SpriteDesc *spr = item->sprite;
    int x;
    int y;
    int halfW;
    int anim;
    unsigned int mode;

    if (item->anim == -1) {
        return;
    }
    halfW = ((short *)&spr->wh)[0] >> 1;
    x = item->x;
    y = item->y;
    switch (item->flags & 0x60) {
    case 0x20:
        x += halfW;
        break;
    case 0x40:
        x -= halfW;
        break;
    }
    anim = item->anim;
    if (anim < 0) {
        int t;
        int z;
        int hh;
        int h;
        int b;
        int left;
        GouraudQuad band;

        if (anim >= -1) {
            return;
        }
        t = anim + 13;
        zoom = ((short *)&spr->wh)[0] * t / 12;
        z = item->zoomWidth * t / 12;
        hh = ((short *)&item->sprite->wh)[1] >> 1;
        h = hh - hh * t / 12;
        b = (item->maxBrightness + -item->maxBrightness * t / 12) * 2;
        if (b > 0xc0) {
            b = 0xc0;
        }
        band.rect.w = halfW * 2 + z - zoom;
        band.rect.y = y - h;
        band.rect.h = h * 2;
        left = x - halfW;
        band.rect.x = left + zoom;
        band.top = 0;
        band.bottom = b;
        Menu_AddGreyBand((uint *)ot, &band);
        band.rect.x = left - z;
        band.top = b;
        band.bottom = 0;
        Menu_AddGreyBand((uint *)ot, &band);
        mode = 0x20;
    } else {
        int bright = item->maxBrightness;
        unsigned short abr;
        int level;
        unsigned int rgb;
        PrimSprite *p;

        if (anim < 12) {
            int b = bright * anim / 12;

            fade = bright - b;
            bright = b;
            zoom = item->zoomWidth * (12 - anim) / 12;
            trail = 1;
        }
        if (!(item->flags & 4)) {
            trail = 0;
        }
        level = bright * item->brightness;
        abr = (item->flags & 0x18) << 2;
        rgb = level >> 7;
        if (trail) {
            unsigned int c = fade | fade << 8 | fade << 16 | 0x2000000;

            p = Prim_AddSprite(ot, c);
            MenuSprite_Place(p, spr, x - zoom, y);
            p = Prim_AddSprite(ot, c);
            MenuSprite_Place(p, spr, x + zoom, y);
            Prim_AddDrawMode(ot, spr->tpage | abr);
        }
        if (item->flags & 2) {
            p = Prim_AddSprite(ot, rgb | rgb << 8 | rgb << 16 | 0x2000000);
            MenuSprite_Place(p, spr, x, y);
            Prim_AddDrawMode(ot, spr->tpage | abr);
            if (item->flags & 1) {
                if (selected) {
                    p = Prim_AddSprite(ot, 0x2000000);
                    MenuSprite_Place(p, spr, x, y);
                    Prim_AddDrawMode(ot, spr->tpage);
                } else {
                    rgb = level >> 8;
                }
                p = Prim_AddSprite(ot, rgb | rgb << 8 | rgb << 16 | 0x2000000);
                MenuSprite_Place(p, spr, x, y);
                Prim_AddDrawMode(ot, spr->tpage | 0x40);
            }
        } else {
            p = Prim_AddSprite(ot, rgb | rgb << 8 | rgb << 16);
            MenuSprite_Place(p, spr, x, y);
        }
        mode = spr->tpage | abr;
    }
    Prim_AddDrawMode(ot, mode);
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/menusprite", MenuSprite_Draw);
#endif
}
