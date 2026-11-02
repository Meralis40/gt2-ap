// Resident code 8006ac68-8006b548 (text): drawing and measuring strings with a
// TextCtx's font, left-aligned, centered or right-aligned, with proportional or
// fixed-width digits.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

void Text_Init(TextCtx *ctx, int tpageId)
{
    Text_SetTexturePage(ctx, tpageId);
    ctx->unk1c = '\0';
}

// Draws s at (x, y), with spacing added to every advance. Returns its width.
int Text_DrawString(TextCtx *ctx, signed char *s, int x, int y, int spacing)
{
    int x0 = x;
    signed char c;

    while ((c = *s++) != 0) {
        Text_DrawGlyph(ctx, c & 0xff, x, y);
        int advance = Text_GetAdvance(ctx, c, *s);
        x += advance + spacing;
    }
    return x - x0 - spacing;
}

// The width Text_DrawString would draw s with.
int Text_MeasureString(TextCtx *ctx, signed char *s, int spacing)
{
    int w = 0;
    signed char c;

    while ((c = *s++) != 0) {
        w += Text_GetAdvance(ctx, c, *s) + spacing;
    }
    return w - spacing;
}

void Text_DrawCentered(TextCtx *ctx, signed char *s, int x, int y, int spacing)
{
    x -= Text_MeasureString(ctx, s, spacing) >> 1;
    Text_DrawString(ctx, s, x, y, spacing);
}

void Text_DrawStringRight(TextCtx *ctx, signed char *s, int x, int y, int spacing)
{
    x -= Text_MeasureString(ctx, s, spacing);
    Text_DrawString(ctx, s, x, y, spacing);
}

// Cuts s at the first character that would end past maxWidth. Returns 1 if it did.
int Text_TruncateToWidth(TextCtx *ctx, signed char *s, int spacing, int maxWidth)
{
    int w = 0;
    signed char c;

    while ((c = *s) != 0) {
        w += Text_GetAdvance(ctx, c, s[1]) + spacing;
        if (w > maxWidth) {
            *s = 0;
            return 1;
        }
        s++;
    }
    return 0;
}

// Text_DrawString with the digits in cells of the font's digit width plus digitExtra,
// each moved by digitShift and its glyph's fixed-width offset.
int Text_DrawStringMonoDigits(TextCtx *ctx, signed char *s, int x, int y, int spacing, int digitShift,
                              int digitExtra)
{
    int x0 = x;
    int cell = ctx->font.digitWidth;
    int half = cell >> 1;
    signed char c;

    while ((c = *s++) != 0) {
        if ((uchar)c - '0' < 10U) {
            Text_DrawGlyph(ctx, (c & 0xff) | 0x100, x + digitShift - half, y);
            x = x + spacing + cell + digitExtra;
        } else {
            Text_DrawGlyph(ctx, c & 0xff, x, y);
            int advance = Text_GetAdvance(ctx, c, *s);
            x += advance + spacing;
        }
    }
    return x - x0 - spacing;
}

int Text_MeasureStringMonoDigits(TextCtx *ctx, signed char *s, int spacing, int digitExtra)
{
    int w = 0;
    signed char c;

    while ((c = *s++) != 0) {
        if ((uchar)c - '0' < 10U) {
            w += spacing + ctx->font.digitWidth + digitExtra;
        } else {
            int advance = Text_GetAdvance(ctx, c, *s);
            w += advance + spacing;
        }
    }
    return w - spacing;
}

void Text_DrawCenteredMonoDigits(TextCtx *ctx, signed char *s, int x, int y, int spacing, int digitShift,
                                 int digitExtra)
{
    x -= Text_MeasureStringMonoDigits(ctx, s, spacing, digitExtra) >> 1;
    Text_DrawStringMonoDigits(ctx, s, x, y, spacing, digitShift, digitExtra);
}

void Text_DrawStringMonoDigitsRight(TextCtx *ctx, signed char *s, int x, int y, int spacing, int digitShift,
                                    int digitExtra)
{
    x -= Text_MeasureStringMonoDigits(ctx, s, spacing, digitExtra);
    Text_DrawStringMonoDigits(ctx, s, x, y, spacing, digitShift, digitExtra);
}

// Draws a number: every character in a cell of width, centered on the font's digit
// width, the one before and after a ':' or '.' in a cell of narrowWidth, and a space
// as an empty cell. A '.' is moved by dotX. With signs, '+', '-' and 'O' take one more
// pixel. Returns the width.
#ifdef NON_MATCHING
// Only registers differ: the game keeps c in $s0 and x in $s1 (GCC gives c the lower
// priority), and loads the ' ' for its test into $t0, a reload register, where GCC
// loads it into $v0. Other declaration orders, a pen variable apart from x, a
// switch, an int c and the ternary for gx didn't change that.
int Text_DrawDigits(TextCtx *ctx, signed char *s, int x, int y, int width, int narrowWidth, int signs,
                    int dotX)
{
    int x0 = x;
    int narrow = 0;
    int cell = ctx->font.digitWidth;
    signed char c;

    x -= cell >> 1;
    while ((c = *s++) != 0) {
        if (c == ' ') {
            x += width;
            continue;
        }
        int gx;
        if (c == '.') {
            gx = x + dotX;
        } else {
            gx = x;
        }
        Text_DrawGlyph(ctx, (c & 0xff) | 0x100, gx, y);
        int prev = narrow;
        narrow = (*s == ':') | (*s == '.');
        x += width;
        if (prev | narrow) {
            x = x - width + narrowWidth;
        }
        if (signs != 0 && ((c == '+') | (c == '-') | (c == 'O'))) {
            x++;
        }
    }
    return x - x0 + (width - narrowWidth);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawDigits);
#endif

// The width Text_DrawDigits would draw s with, less the extra pixel of an 'O'.
int Text_MeasureDigits(TextCtx *ctx, signed char *s, int width, int narrowWidth, int signs)
{
    int w = 0;
    int narrow = 0;
    int prev;
    signed char c;

    while ((c = *s++) != 0) {
        prev = narrow;
        narrow = (*s == ':') | (*s == '.');
        w += width;
        if (prev | narrow) {
            w = w - width + narrowWidth;
        }
        if (signs != 0 && ((c == '+') | (c == '-'))) {
            w++;
        }
    }
    return w;
}

void Text_DrawDigitsRight(TextCtx *ctx, signed char *s, int x, int y, int width, int narrowWidth, int signs,
                          int dotX)
{
    x -= Text_MeasureDigits(ctx, s, width, narrowWidth, signs);
    Text_DrawDigits(ctx, s, x, y, width, narrowWidth, signs, dotX);
}

void Text_DrawDigitsCentered(TextCtx *ctx, signed char *s, int x, int y, int width, int narrowWidth, int signs,
                             int dotX)
{
    x -= Text_MeasureDigits(ctx, s, width, narrowWidth, signs) >> 1;
    Text_DrawDigits(ctx, s, x, y, width, narrowWidth, signs, dotX);
}
}
