// Resident code 8006bcb8-8006c460 (widgets): fading bars, the memory card progress
// bar and a disc drawn flat on the ground.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern short g_SinTable[];

void GrowBar_Update(unsigned char *bar)
{
    short sVar1;

    if (*(short *)(bar + 0x12) < 0) {
        if (*(short *)(bar + 0x12) < -1) {
            *(short *)(bar + 0x12) = *(short *)(bar + 0x12) + 1;
            return;
        }
    }
    else {
        sVar1 = *(short *)(bar + 0x12) + 1;
        *(short *)(bar + 0x12) = sVar1;
        if (*(short *)(bar + 0x10) < sVar1) {
            *(unsigned short *)(bar + 0x12) = *(unsigned short *)(bar + 0x10);
        }
    }
}

#ifdef NON_MATCHING
// Only registers differ: the game adds duration + 1 first, then t; GCC adds t + 1
// first. Writing it (duration + 1) + t, t + duration + 1, duration - ~t or as an
// if/else didn't change that.
void GrowBar_Draw(GrowBar *bar, uint *ot)
{
    int t = bar->t;
    int duration = bar->duration;
    int width;
    int k;
    RECT *r;

    if (t == -1) {
        return;
    }
    width = bar->width * t / duration;
    k = duration - t;
    if (t < 0) {
        width = bar->width * ~t / duration;
        k = duration + 1 + t;
    }
    r = Prim_AddTile(ot, Rgb_Lerp((uchar *)&bar->rgbFrom, (uchar *)&bar->rgbTo, k, duration));
    r->x = bar->x - (width >> 1);
    r->y = bar->y;
    r->w = width;
    r->h = bar->height;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", GrowBar_Draw);
#endif

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
        if (*c < sVar1) {
            c[1] = *c;
        }
    }
}

int FadeCounter_GetLevel(short *c)
{
    int iVar1;
    unsigned int uVar2;

    uVar2 = (unsigned int)c[1];
    iVar1 = 0;
    if (-1 < (int)uVar2) {
        iVar1 = (int)(uVar2 << 7) / (int)*c;
    }
    if ((int)uVar2 < -1) {
        iVar1 = (int)(~uVar2 << 7) / (int)*c;
    }
    return iVar1;
}

void WipeBar_Update(WipeBar *bar)
{
    short sVar1;

    if (bar->t < 0) {
        if (bar->t < -1) {
            bar->t = bar->t + 1;
            return;
        }
    }
    else {
        sVar1 = bar->t + 1;
        bar->t = sVar1;
        if (bar->duration < sVar1) {
            bar->t = bar->duration;
        }
    }
}

int WipeBar_GetLevel(WipeBar *bar)
{
    int t = bar->t;
    int level = 0;

    if (t < 0) {
        if (t < -1) {
            level = ~t;
        }
    } else {
        level = t;
    }
    return (level << 7) / bar->duration;
}

// A horizontal gradient from rgbTop (left) to rgbBottom (right), each mixed towards
// rgbIn while t counts up and rgbOut while it counts down; flags bit 0 puts it left
// of x. It widens by width * k / duration on both sides.
#ifdef NON_MATCHING
// The game keeps bar, left, right, k and duration in s2 to s6; GCC picks other saved
// registers and copies x. Locals in other orders, short locals and reading the
// fields directly didn't help.
void WipeBar_Draw(WipeBar *bar, uint *ot, int x, int y)
{
    uint rgb = bar->rgbIn;
    int width = bar->width;
    int height = bar->height;
    int t = bar->t;
    int duration = bar->duration;
    int k;
    int left;
    int right;
    int d;
    PrimG4 *p;

    if (t == -1) {
        return;
    }
    k = duration;
    if (t < 0) {
        k += 1 + t;
        rgb = bar->rgbOut;
    } else {
        k -= t;
    }
    if (bar->flags & 1) {
        left = x - width;
        right = x;
    } else {
        left = x;
        right = x + width;
    }
    d = width * k / duration;
    left -= d;
    right += d;
    p = Prim_AddPolyG4(ot);
    p->c0 = p->c2 = Rgb_Lerp((uchar *)&bar->rgbTop, (uchar *)&rgb, k, duration) | 0x38000000;
    p->c1 = p->c3 = Rgb_Lerp((uchar *)&bar->rgbBottom, (uchar *)&rgb, k, duration);
    p->x0 = p->x2 = left;
    p->x1 = p->x3 = right;
    p->y0 = p->y1 = y;
    p->y2 = p->y3 = y + height;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", WipeBar_Draw);
#endif

#ifdef NON_MATCHING
// The game walks a pointer down from bar + 31 next to the counter; GCC recomputes
// bar + i each time. A do-while, a while, a pointer, and short or char counters
// didn't change it.
void CardProgress_Reset(CardProgressBar *bar)
{
    int i;

    bar->visible = 0;
    for (i = 31; i >= 0; i--) {
        bar->segFade[i] = -1;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", CardProgress_Reset);
#endif

// remaining counts down to 0; the segments light up as the work done (squared,
// so they speed up) passes them, then fade in over 24 frames.
void CardProgress_Update(CardProgressBar *bar, int remaining)
{
    uint total;
    int done;
    int i;

    if (bar->visible < 0) {
        return;
    }
    total = bar->total;
    done = total - remaining;
    if (done < 0) {
        done = 0;
    }
    if ((int)total < done) {
        done = total;
    }
    if (total < 40000) {
        done = (uint)(done * done) / total;
    } else {
        done = (uint)(done * (done / 256)) / (total >> 8);
    }
    for (i = 0; i < 32; i++) {
        if (bar->segFade[i] < 0) {
            if (done >= (int)(i * bar->total >> 5)) {
                bar->segFade[i] = 0;
            }
        } else {
            bar->segFade[i]++;
            if (bar->segFade[i] > 24) {
                bar->segFade[i] = 24;
            }
        }
    }
}

// Unlit segments are the color darkened by 3/4; lit ones start white and fade to the
// color over 24 frames.
#ifdef NON_MATCHING
// The game keeps the 24 of 24 - fade in a saved register, set before the loop. GCC
// loads it in the loop; only an artificial local set after the early return (as
// int n = 24) got it out, one instruction away from the game's place.
void CardProgress_Draw(CardProgressBar *bar, uint *ot)
{
    uint white = 0xd4d4d4;
    uint black = 0;
    int x = bar->x;
    int step = bar->segStep;
    int y = bar->y;
    int i;
    int fade;
    uint rgb;
    RECT *r;

    if (bar->visible < 0) {
        return;
    }
    for (i = 0; i < 32; i++) {
        fade = bar->segFade[i];
        rgb = Rgb_Lerp((uchar *)&bar->color, (uchar *)&black, 0x60, 0x80);
        if (fade >= 0) {
            rgb = Rgb_Lerp((uchar *)&bar->color, (uchar *)&white, 24 - fade, 24);
        }
        r = Prim_AddTile(ot, rgb);
        r->x = x;
        r->y = y;
        r->w = bar->segWidth;
        r->h = bar->segHeight;
        x += step;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", CardProgress_Draw);
#endif

// Puts the rim points on a circle of radius 0x40000 and the centre at 0.
void GroundDisc_Init(GroundDisc *disc, int unused)
{
    int radius = 0x400;
    int x;
    int z;
    int angle;
    int i;

    for (i = 0; i < 25; i++) {
        angle = (i << 12) / 24 & 0xfff;
        x = radius * g_SinTable[angle + 0x400];
        z = radius * g_SinTable[angle];
        disc->rim[i].y = 0;
        disc->rim[i].x = (x >> 12) << 8;
        disc->rim[i].z = (z >> 12) << 8;
    }
    disc->centre.z = 0;
    disc->centre.y = 0;
    disc->centre.x = 0;
}

// Draws the disc as Gouraud triangles from the centre's rgb to black, semi-transparent
// if blend is set, each at the deepest of its three points.
void GroundDisc_Draw(GroundDisc *disc, uint *ot, uchar *camera, int blend, uint rgb)
{
    ScreenPoint *pts = (ScreenPoint *)0x1f800000;
    ScreenPoint *a = pts;
    ScreenPoint *centre = &pts[25];
    uint code = 0x30;
    int i;
    int z;
    uint *entry;
    PrimG3 *p;

    if (blend) {
        code = 0x32;
    }
    Points_Project(camera + 8, pts, disc, 26);
    for (i = 0; i < 24; i++) {
        z = centre->z;
        if (z < a[0].z) {
            z = a[0].z;
        }
        if (z < a[1].z) {
            z = a[1].z;
        }
        entry = ot + z;
        p = Prim_AddPolyG3(entry);
        p->c0 = code << 24 | rgb;
        p->x0 = centre->x;
        p->y0 = centre->y;
        p->c1 = 0;
        p->x1 = a->x;
        p->y1 = a->y;
        a++;
        p->c2 = 0;
        p->x2 = a->x;
        p->y2 = a->y;
        Prim_AddDrawMode(entry, 0x200);
    }
}
}
