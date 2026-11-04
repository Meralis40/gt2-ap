// OVL2 80019fe4-8001aab4 (textitem): Text items: another version of the resident
// TextItem, with another layout.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "copies.h"
#include "common.h"

#define TEXTITEM_ABR 0x3        // blend mode of the letters
#define TEXTITEM_TILE 0x4       // a tile of the letter's colour behind each letter
#define TEXTITEM_WAVE 0x8       // once shown, the letters fade again one after another
#define TEXTITEM_MONO 0x10      // fixed-width glyphs (code | 0x100) for every letter
#define TEXTITEM_SEMI 0x20      // semi-transparent letters
#define TEXTITEM_GLOW 0x40      // two additive copies spread apart while a letter appears
#define TEXTITEM_CENTER 0x80
#define TEXTITEM_RIGHT 0x100

extern "C" {
extern Font D_80093124;
extern char D_80027128[]; // an empty string


#ifdef NON_MATCHING
// Copies the template (defaults without one), with no text and nothing shown yet.
void TextItem_Init_OVL2(ArcTextItem *item, void *arg)
{
    int value;

    if (arg == 0) {
        item->charsPerStep = 0x80;
        item->echoDiv = 2;
        item->revealSteps = 0x1e;
        item->blinkSteps = 0x1e;
        item->spacing = 1;
        item->zoomWidth = 0x10;
        item->fadeSteps = 0x10;
        item->height = 0x10;
        item->flags = 0;
        item->font = &D_80093124;
        *(unsigned int *)item->rgbFrom = 0xffffff;
        *(unsigned int *)item->rgbTo = 0xffffff;
    } else {
        ArcTextItemTmpl *tmpl = (ArcTextItemTmpl *)arg;

        item->charsPerStep = tmpl->charsPerStep;
        item->echoDiv = tmpl->echoDiv;
        value = tmpl->revealSteps;
        item->revealSteps = value;
        value = tmpl->zoomWidth;
        item->zoomWidth = value;
        value = tmpl->blinkSteps;
        item->blinkSteps = value;
        value = tmpl->fadeSteps;
        item->fadeSteps = value;
        value = tmpl->spacing;
        item->spacing = value;
        value = tmpl->height;
        item->height = value;
        item->flags = tmpl->flags;
        item->font = tmpl->font;
        *(unsigned int *)item->rgbFrom = tmpl->rgbFrom;
        *(unsigned int *)item->rgbTo = tmpl->rgbTo;
    }
    item->text = D_80027128;
    item->brightness = 0x80;
    item->anim = -1;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/textitem", TextItem_Init_OVL2);
#endif

// Copy of TextItem_Show: starts the per-character reveal; endFrame < 0 = derived from
// the length.
void TextItem_Show_OVL2(ArcTextItem *item, int endFrame)
{
    size_t sVar1;

    item->anim = 0;
    sVar1 = strlen(item->text);
    item->length = (short)sVar1;
    item->animEnd = item->revealSteps + (short)((int)(short)sVar1 / (int)(unsigned int)item->charsPerStep);
    if (-1 < endFrame) {
        item->animEnd = (short)endFrame;
    }
}

// Copy of TextItem_ShowNow: shown with the reveal finished.
void TextItem_ShowNow_OVL2(ArcTextItem *item)
{
    TextItem_Show_OVL2(item, -1);
    item->anim = item->animEnd;
}

// Copy of TextItem_Hide: starts the close animation.
void TextItem_Hide_OVL2(ArcTextItem *item)
{
    size_t sVar1;

    item->anim = ~item->revealSteps;
    sVar1 = strlen(item->text);
    item->length = (short)sVar1;
}

// Once shown, the animation loops back to animEnd every blinkSteps frames. -1 is hidden.
void TextItem_Update_OVL2(ArcTextItem *item)
{
    if (item->anim >= 0) {
        item->anim++;
        if (item->anim >= item->animEnd + item->blinkSteps) {
            item->anim = item->animEnd;
        }
    } else if (item->anim < -1) {
        item->anim++;
    }
}

#ifdef NON_MATCHING
void TextItem_Draw_OVL2(ArcTextItem *item, void *arg, TextCtx *ctx)
{
    uint *ot = (uint *)arg;
    int x = item->x;
    int y = item->y;
    int shown = item->anim * item->charsPerStep;
    uint flags = item->flags;
    signed char *s = (signed char *)item->text;
    int i;
    int h;
    int d;
    uint rgb;
    uint semi;
    uint abr;
    int t;
    int r, g, b;
    int w;
    int a;
    GouraudQuad quad;

    Text_SetFont(ctx, item->font);
    switch (flags & (TEXTITEM_CENTER | TEXTITEM_RIGHT)) {
    case TEXTITEM_RIGHT:
        x -= TextItem_MeasureWidth_OVL2(item, ctx);
        break;
    case TEXTITEM_CENTER:
        x -= TextItem_MeasureWidth_OVL2(item, ctx) >> 1;
        break;
    }
    h = 1;
    if (ctx->unk1c != 0) {
        h = 2;
    }

    if (item->anim < 0) {
        if (item->anim < -1) {
            t = item->anim + item->revealSteps + 1;
            w = TextItem_MeasureWidth_OVL2(item, ctx);
            a = w * t / item->revealSteps;
            d = item->zoomWidth * t;
            h = item->height >> 1;
            int dy = h * t;
            t = item->revealSteps - t;
            r = item->rgbFrom[0] * t / item->revealSteps;
            g = item->rgbFrom[1] * t / item->revealSteps;
            b = item->rgbFrom[2] * t / item->revealSteps;
            dy /= item->revealSteps;
            d /= item->revealSteps;
            quad.top = 0;
            quad.rect.x = x + a;
            rgb = r | g << 8 | b << 16;
            dy = h - dy;
            quad.rect.w = d + w - a;
            quad.rect.y = y - h - dy;
            quad.rect.h = dy * 2;
            quad.bottom = rgb;
            Menu_AddGouraudQuad(ot, &quad);
            quad.rect.x = x - d;
            quad.bottom = 0;
            quad.top = rgb;
            Menu_AddGouraudQuad(ot, &quad);
            Prim_AddDrawMode(ot, 0x20);
        }
        return;
    }

    d = ctx->font.digitWidth;
    semi = ((flags >> 4) & 2) << 24;
    abr = (flags & TEXTITEM_ABR) << 21;
    for (i = 0; i < item->length; i++) {
        int f;
        int range;
        a = (flags >> 6) & 1;
        if (item->anim >= item->animEnd) {
            shown = 1;
            t = item->revealSteps;
            f = item->fadeSteps;
            a = 0;
            if (flags & TEXTITEM_WAVE) {
                t = item->anim - item->animEnd - i / item->echoDiv;
                f = t;
            }
        } else {
            t = shown / item->charsPerStep;
            f = t;
        }
        if (shown <= 0) {
            break;
        }
        if (item->revealSteps < t) {
            a = 0;
            t = item->revealSteps;
        }
        if (t < 0) {
            t = item->revealSteps;
        }
        t = item->revealSteps - t;
        if (item->anim >= item->animEnd) {
            if (item->fadeSteps < f) {
                f = item->fadeSteps;
            }
            if (f < 0) {
                f = item->fadeSteps;
            }
            f = item->fadeSteps - f;
            range = item->fadeSteps;
        } else {
            f = t;
            range = item->revealSteps;
        }
        r = item->rgbFrom[0] + (item->rgbTo[0] - item->rgbFrom[0]) * f / range;
        g = item->rgbFrom[1] + (item->rgbTo[1] - item->rgbFrom[1]) * f / range;
        b = item->rgbFrom[2] + (item->rgbTo[2] - item->rgbFrom[2]) * f / range;
        r = r * item->brightness >> 8;
        g = g * item->brightness >> 8;
        b = b * item->brightness >> 8;
        rgb = r | g << 8 | b << 16;

        signed char c = *s;
        if (c == ' ') {
            w = d + item->spacing;
        } else {
            ctx->ot = ot + 1;
            ctx->rgb = rgb | semi;
            *(uint *)&ctx->clut = (*(uint *)&ctx->clut & 0xff9fffff) | abr;
            if (flags & TEXTITEM_MONO) {
                Text_DrawGlyph(ctx, (c & 0xff) | 0x100, x, y);
                w = d;
            } else if ((uchar)(c - '0') < 10) {
                w = d;
                Text_DrawGlyph(ctx, (c & 0xff) | 0x200, x + ((w + item->spacing) >> 1), y);
            } else {
                Text_DrawGlyph(ctx, c & 0xff, x, y);
                w = Text_GetAdvance(ctx, c, s[1]);
            }
            w += item->spacing;
            if (a) {
                r = r * t / item->revealSteps;
                g = g * t / item->revealSteps;
                b = b * t / item->revealSteps;
                a = item->zoomWidth * t / item->revealSteps;
                ctx->ot = ot;
                *(uint *)&ctx->clut = (*(uint *)&ctx->clut & 0xff9fffff) | 0x200000;
                ctx->rgb = r | g << 8 | b << 16 | 0x2000000;
                Text_DrawGlyph(ctx, c & 0xff, x - a, y);
                Text_DrawGlyph(ctx, c & 0xff, x + a, y);
            }
        }
        if (flags & TEXTITEM_TILE) {
            RECT *tile = Prim_AddTile(ot, rgb << 1);
            tile->x = x;
            tile->y = y;
            tile->w = w;
            tile->h = h;
        }
        s++;
        x += w;
        shown--;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/textitem", TextItem_Draw_OVL2);
#endif

#ifdef NON_MATCHING
int TextItem_MeasureWidth_OVL2(ArcTextItem *item, TextCtx *ctx)
{
    int w = 0;
    signed char *s = (signed char *)item->text;
    int digitWidth;
    signed char c;

    Text_SetFont(ctx, item->font);
    digitWidth = ctx->font.digitWidth;
    while (*s != 0) {
        c = *s++;
        if ((uchar)c - '0' < 10U || c == ' ') {
            w += digitWidth;
        } else {
            w += Text_GetAdvance(ctx, c, *s);
        }
        w += item->spacing;
    }
    return w;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/textitem", TextItem_MeasureWidth_OVL2);
#endif
}
