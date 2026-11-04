// OVL2 8001c118-8001c7ac (iconitem): Icon items.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

static inline int Half(unsigned short v)
{
    return (short)v >> 1;
}

extern "C" {


// Defaults without a template.
void IconItem_Init(ArcIconItem *it, ArcIconItemTmpl *t)
{
    if (t == 0) {
        it->pulseSteps = 2;
        it->revealSteps = 0x1e;
        it->zoomWidth = 0x20;
        it->brightness = 0x80;
        it->pulseBrightness = 0x40;
        it->flags = 0;
        it->scale = 0x80;
    } else {
        int zoomWidth;

        it->pulseSteps = t->pulseSteps;
        it->revealSteps = t->revealSteps;
        it->flags = t->flags;
        zoomWidth = t->zoomWidth;
        it->zoomWidth = zoomWidth;
        it->brightness = t->brightness;
        it->pulseBrightness = t->pulseBrightness;
        it->scale = t->scale;
    }
    it->anim = -1;
}

// Starts the zoom-in reveal (anim 0).
void IconItem_Show(ArcIconItem *it)
{
    it->anim = 0;
}

// Shown with the reveal finished.
void IconItem_ShowNow(ArcIconItem *it)
{
    it->anim = it->revealSteps;
}

// Starts the close animation.
void IconItem_Hide(ArcIconItem *it)
{
    it->anim = ~it->revealSteps;
}

// The closing animation counts up to -1. Once open, anim cycles through the pulse.
void IconItem_Update(ArcIconItem *it)
{
    if (it->anim >= 0) {
        if (++it->anim >= it->revealSteps + it->pulseSteps) {
            it->anim = it->revealSteps;
        }
    } else if (it->anim < -1) {
        it->anim++;
    }
}

#ifdef NON_MATCHING
// Closing collapses to two grey bands. Opening fades in with two ghosts sliding together
// when flag 4 is set, and open it pulses when flag 1 is set.
void IconItem_Draw(ArcIconItem *it, void *ot)
{
    int scale = it->scale;
    int anim = it->anim;
    int steps = it->revealSteps;
    int gap = 1;
    int ghostGrey = 0;
    int ghost = 0;
    int halfW;
    int x;
    int y;
    int grey;
    int zoom;
    int rise;
    int fade;
    int left;
    int mode;
    unsigned short abr;
    ArcSprite *sprite;
    GouraudQuad quad;
    unsigned int rgb;
    PrimSprite *p;

    if (anim == -1) {
        return;
    }
    halfW = Half(((ArcSprite *)it->sprite)->w);
    if (scale != 0x80) {
        halfW = halfW * scale / 128;
    }
    x = it->x;
    y = it->y;
    switch (it->flags & 0x60) {
    case 0x20:
        x += halfW;
        break;
    case 0x40:
        x -= halfW;
        break;
    }
    anim = it->anim;
    if (anim < 0) {
        if (anim >= -1) {
            return;
        }
        anim = steps + 1 + anim;
        ghostGrey = ((ArcSprite *)it->sprite)->w;
        if (scale != 0x80) {
            ghostGrey = ghostGrey * scale / 128;
        }
        gap = ghostGrey * anim / steps;
        zoom = it->zoomWidth * anim / steps;
        grey = Half(((ArcSprite *)it->sprite)->h);
        if (scale != 0x80) {
            grey = grey * scale / 128;
        }
        rise = grey * anim / steps;
        fade = it->brightness;
        rise = grey - rise;
        grey = (fade + -fade * anim / steps) * 2;
        if (grey > 0xc0) {
            grey = 0xc0;
        }
        left = x - halfW;
        quad.rect.x = left + gap;
        quad.rect.w = halfW * 2 + zoom - gap;
        quad.rect.y = y - rise;
        quad.rect.h = rise * 2;
        quad.top = 0;
        quad.bottom = grey;
        Menu_AddGreyBand_OVL2((uint *)ot, &quad);
        quad.rect.x = left - zoom;
        quad.top = grey;
        quad.bottom = 0;
        Menu_AddGreyBand_OVL2((uint *)ot, &quad);
        Prim_AddDrawMode(ot, 0x20);
        return;
    }
    if (anim < steps) {
        grey = it->brightness * anim / steps;
        anim = steps - anim;
        gap = it->zoomWidth * anim / steps;
        ghostGrey = it->brightness * anim / steps;
        ghost = 1;
    } else {
        grey = it->brightness;
        if (it->flags & 1) {
            anim -= steps;
            if (anim > steps) {
                anim = steps;
            }
            anim = steps - anim;
            grey += (it->pulseBrightness - grey) * anim / steps;
        }
    }
    if (!(it->flags & 4)) {
        ghost = 0;
    }
    mode = (it->flags & 0x18) << 2;
    abr = mode;
    sprite = (ArcSprite *)it->sprite;
    if (scale != 0x80) {
        if (ghost) {
            Sprite_AddScaledSemi(ot, sprite, x - gap, y, ghostGrey, scale, 0x20);
            Sprite_AddScaledSemi(ot, sprite, x + gap, y, ghostGrey, scale, 0x20);
        }
        if (it->flags & 2) {
            Sprite_AddScaledSemi(ot, sprite, x, y, grey, scale, abr);
        } else {
            Sprite_AddScaled(ot, sprite, x, y, grey, scale);
        }
        return;
    }
    if (ghost) {
        rgb = ghostGrey | ghostGrey << 8 | ghostGrey << 16 | 0x2000000;
        p = Prim_AddSprite(ot, rgb);
        *(int *)p = x - gap - Half(sprite->w) + ((y - Half(sprite->h)) << 16);
        *(int *)&p->u = *(int *)sprite;
        *(int *)&p->w = *(int *)&sprite->w;
        p = Prim_AddSprite(ot, rgb);
        *(int *)p = x + gap - Half(sprite->w) + ((y - Half(sprite->h)) << 16);
        *(int *)&p->u = *(int *)sprite;
        *(int *)&p->w = *(int *)&sprite->w;
        Prim_AddDrawMode(ot, sprite->tpage | mode);
    }
    if (it->flags & 2) {
        rgb = grey | grey << 8 | grey << 16 | 0x2000000;
    } else {
        rgb = grey | grey << 8 | grey << 16;
    }
    p = Prim_AddSprite(ot, rgb);
    *(int *)p = x - Half(sprite->w) + ((y - Half(sprite->h)) << 16);
    *(int *)&p->u = *(int *)sprite;
    *(int *)&p->w = *(int *)&sprite->w;
    Prim_AddDrawMode(ot, sprite->tpage | abr);
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/iconitem", IconItem_Draw);
#endif
}
