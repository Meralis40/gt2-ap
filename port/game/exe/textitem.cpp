// Resident code 8006c460-8006cdcc (textitem): a line of text that appears letter by
// letter, each letter fading from one colour to another, and disappears by shrinking
// into a fading bar.
#include "common.h"

// TextItem.flags
#define TEXTITEM_ABR 0x3        // blend mode of the letters
#define TEXTITEM_TILE 0x4       // a tile of the letter's colour behind each letter
#define TEXTITEM_WAVE 0x8       // once shown, the letters fade again one after another
#define TEXTITEM_MONO 0x10      // fixed-width glyphs (code | 0x100) for every letter
#define TEXTITEM_SEMI 0x20      // semi-transparent letters
#define TEXTITEM_GLOW 0x40      // two additive copies spread apart while a letter appears
#define TEXTITEM_CENTER 0x80
#define TEXTITEM_RIGHT 0x100

extern "C" {

// Copies the style and shows nothing yet.
void TextItem_Init(TextItem *item, TextStyle *style, char *text)
{
    *(TextStyle *)item = *style;
    // Some menus leave a row without a label: the PlayStation can read address 0. The
    // text is a guest pointer, so the empty string stands in guest memory.
    static char *const empty = rt::guestString("");
    item->text = text != 0 ? text : empty;
    item->brightness = 0x80;
    item->anim = -1;
}

// Starts revealing the text, charsPerStep letters a step. The reveal ends at endFrame,
// or when the last letter has finished appearing if endFrame is negative.
void TextItem_Show(TextItem *item, int endFrame)
{
    item->anim = 0;
    item->length = strlen(item->text);
    item->animEnd = item->revealSteps + item->length / item->charsPerStep;
    if (endFrame >= 0) {
        item->animEnd = endFrame;
    }
}

void TextItem_ShowNow(TextItem *item)
{
    TextItem_Show(item, -1);
    item->anim = item->animEnd;
}

// Starts closing the text: a bar that shrinks to the middle and fades out over
// revealSteps frames.
void TextItem_Hide(TextItem *item)
{
    item->anim = ~item->revealSteps;
    item->length = strlen(item->text);
}

// Once shown, the animation loops back to animEnd every closeSteps frames (for the
// TEXTITEM_WAVE fade). -1 is hidden.
void TextItem_Update(TextItem *item)
{
    if (item->anim >= 0) {
        item->anim++;
        if (item->anim >= item->animEnd + item->closeSteps) {
            item->anim = item->animEnd;
        }
    } else if (item->anim < -1) {
        item->anim++;
    }
}

// Draws the text at (x, y), aligned by its flags, at its current animation step.
// The code is the game's, but its registers aren't: the game keeps the letter's
// unsigned copy in $fp and the colour's range in $s1 (with b), where GCC gives
// $fp to a and spills the letter, which takes 8 more bytes of frame. Reusing b as
// the range gets closer, but isn't the game's code; the order of the declarations
// changes nothing. As in the game, the variables serve both the closing bar and the
// letters: a is the bar's inset and the glow, d its spread and the digit width, h
// its half height and the tiles' height.
void TextItem_Draw(TextItem *item, uint *ot, int x, int y, TextCtx *ctx)
{
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
        x -= TextItem_MeasureWidth(item, ctx);
        break;
    case TEXTITEM_CENTER:
        x -= TextItem_MeasureWidth(item, ctx) >> 1;
        break;
    }
    h = 1;
    if (ctx->unk1c != 0) {
        h = 2;
    }

    if (item->anim < 0) {
        if (item->anim < -1) {
            t = item->anim + item->revealSteps + 1;
            w = TextItem_MeasureWidth(item, ctx);
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
                t = item->anim - item->animEnd - i / item->unk01;
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

// The width of the item's text: fixed-width digits and spaces, and the spacing after
// every letter, the last one included.
// The game increments s with the loop's test, where GCC does it in the body, and
// gives digitWidth $s2 before item and ctx. while ((c = *s++) != 0) increments it
// there but tests c instead of reading *s again; s[-1], a comma in the test,
// other types for digitWidth and other declaration orders didn't do both.
int TextItem_MeasureWidth(TextItem *item, TextCtx *ctx)
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
}
