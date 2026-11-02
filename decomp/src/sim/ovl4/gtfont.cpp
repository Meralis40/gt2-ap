// OVL4 8001f210-8001fcdc (gtfont): GT mode font and number drawing.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GTFontGlyph g_GTFont_Glyphs[0x10b];
extern SpriteDesc g_GTFont_Sprites[0x10b];
extern GTFontGlyph g_GTFontB_Glyphs[0x18];
extern SpriteDesc g_GTNumFont_Glyphs[0x18];
extern short g_GTFont_Index[0x100];
extern short g_GTFontB_Index[0x100];

// Fills the glyph index of the codes below 0x100, for both fonts.
void GTFont_BuildIndex(void)
{
    int i;
    int code;

    for (i = 0; i < 0x100; i++) {
        g_GTFont_Index[i] = 0;
        g_GTFontB_Index[i] = 0;
    }
    for (i = 0; i < 0x10b; i++) {
        code = g_GTFont_Glyphs[i].code;
        if (code < 0x100)
            g_GTFont_Index[code] = i;
    }
    for (i = 0; i < 0x18; i++) {
        code = g_GTFontB_Glyphs[i].code;
        if (code < 0x100)
            g_GTFontB_Index[code] = i;
    }
}

// The glyph of code: from the index below 0x100, else by binary search
// (0 if missing).
int GTFont_FindGlyph(unsigned int code)
{
    int lo = 0;
    int hi = 0x10b;
    int n;
    int mid;

    if (code < 0x100)
        return g_GTFont_Index[code];
    for (n = 0; n < 16; n++) {
        mid = (hi + lo) >> 1;
        if (g_GTFont_Glyphs[mid].code == code)
            return mid;
        if (code < g_GTFont_Glyphs[mid].code) {
            if (lo == mid)
                goto last;
            hi = mid;
        } else {
            if (lo == mid)
                goto last;
            lo = mid;
        }
    }
    return 0;
last:
    if (g_GTFont_Glyphs[mid].code == code)
        return mid;
    return 0;
}

// Draws a wide string at x, y; returns its width.
int GTFont_DrawString(void *ot, unsigned short *text, int x, int y, unsigned int rgb)
{
    unsigned short c;
    int width = 0;
    int g;
    unsigned int *sprite;

    while ((c = *text++) != 0) {
        g = GTFont_FindGlyph(c);
        sprite = (unsigned int *)Prim_AddSprite(ot, rgb);
        sprite[0] = x + g_GTFont_Glyphs[g].xoff + (y + g_GTFont_Glyphs[g].yoff) * 0x10000;
        sprite[1] = g_GTFont_Sprites[g].uvClut;
        sprite[2] = g_GTFont_Sprites[g].wh;
        Prim_AddDrawMode(ot, g_GTFont_Sprites[g].tpage | 0x40);
        x += g_GTFont_Glyphs[g].advance;
        width += g_GTFont_Glyphs[g].advance;
    }
    return width;
}

// GTText_DrawW with the right edge at x.
int GTText_DrawWRight(void *ot, unsigned short *str, int x, int y, unsigned int rgb)
{
    return GTFont_DrawString(ot, str, x - GTText_WidthW(str), y, rgb);
}

// Draws a string at x, y; returns its width.
int GTText_Draw(void *ot, char *str, int x, int y, unsigned int rgb)
{
    unsigned short c;
    int width = 0;
    int g;
    unsigned int *sprite;

    while ((c = *(signed char *)str++) != 0) {
        g = GTFont_FindGlyph(c);
        sprite = (unsigned int *)Prim_AddSprite(ot, rgb);
        sprite[0] = x + g_GTFont_Glyphs[g].xoff + (y + g_GTFont_Glyphs[g].yoff) * 0x10000;
        sprite[1] = g_GTFont_Sprites[g].uvClut;
        sprite[2] = g_GTFont_Sprites[g].wh;
        Prim_AddDrawMode(ot, g_GTFont_Sprites[g].tpage | 0x40);
        x += g_GTFont_Glyphs[g].advance;
        width += g_GTFont_Glyphs[g].advance;
    }
    return width;
}

// GTText_Draw with the right edge at x.
int GTText_DrawRight(void *ot, char *str, int x, int y, unsigned int rgb)
{
    return GTText_Draw(ot, str, x - GTText_Width(str), y, rgb);
}

// The width of a wide string.
int GTText_WidthW(unsigned short *str)
{
    unsigned short c;
    int width = 0;

    while ((c = *str++) != 0)
        width += g_GTFont_Glyphs[GTFont_FindGlyph(c)].advance;
    return width;
}

// The width of a string.
int GTText_Width(char *str)
{
    unsigned short c;
    int width = 0;

    while ((c = *(signed char *)str++) != 0)
        width += g_GTFont_Glyphs[GTFont_FindGlyph(c)].advance;
    return width;
}

// Draws a wide string with the number font at x, y; returns its width.
int GTNum_DrawW(void *ot, unsigned short *str, int x, int y, unsigned int rgb)
{
    unsigned short c;
    int width = 0;
    int g;
    unsigned int *sprite;

    while ((c = *str++) != 0) {
        g = g_GTFontB_Index[c & 0xff];
        sprite = (unsigned int *)Prim_AddSprite(ot, rgb);
        sprite[0] = x + g_GTFontB_Glyphs[g].xoff + (y + g_GTFontB_Glyphs[g].yoff) * 0x10000;
        sprite[1] = g_GTNumFont_Glyphs[g].uvClut;
        sprite[2] = g_GTNumFont_Glyphs[g].wh;
        Prim_AddDrawMode(ot, g_GTNumFont_Glyphs[g].tpage | 0x40);
        x += g_GTFontB_Glyphs[g].advance;
        width += g_GTFontB_Glyphs[g].advance;
    }
    return width;
}

// GTNum_DrawW with the right edge at x.
int GTNum_DrawWRight(void *ot, unsigned short *str, int x, int y, unsigned int rgb)
{
    return GTNum_DrawW(ot, str, x - GTNum_WidthW(str), y, rgb);
}

// Draws a string with the number font at x, y; returns its width.
int GTNum_Draw(void *ot, char *str, int x, int y, unsigned int rgb)
{
    unsigned short c;
    int width = 0;
    int g;
    unsigned int *sprite;

    while ((c = *(signed char *)str++) != 0) {
        g = g_GTFontB_Index[c & 0xff];
        sprite = (unsigned int *)Prim_AddSprite(ot, rgb);
        sprite[0] = x + g_GTFontB_Glyphs[g].xoff + (y + g_GTFontB_Glyphs[g].yoff) * 0x10000;
        sprite[1] = g_GTNumFont_Glyphs[g].uvClut;
        sprite[2] = g_GTNumFont_Glyphs[g].wh;
        Prim_AddDrawMode(ot, g_GTNumFont_Glyphs[g].tpage | 0x40);
        x += g_GTFontB_Glyphs[g].advance;
        width += g_GTFontB_Glyphs[g].advance;
    }
    return width;
}

// GTNum_Draw with the right edge at x.
int GTNum_DrawRight(void *ot, char *str, int x, int y, unsigned int rgb)
{
    return GTNum_Draw(ot, str, x - GTNum_Width(str), y, rgb);
}

// The width of a wide string in the number font.
int GTNum_WidthW(unsigned short *str)
{
    unsigned short c;
    int width = 0;

    while ((c = *str++) != 0)
        width += (g_GTFontB_Glyphs + g_GTFontB_Index[c & 0xff])->advance;
    return width;
}

// The width of a string in the number font.
int GTNum_Width(char *str)
{
    unsigned short c;
    int width = 0;

    while ((c = *(signed char *)str++) != 0)
        width += (g_GTFontB_Glyphs + g_GTFontB_Index[c & 0xff])->advance;
    return width;
}

// Draws a wide string with the text font, or the number font if numFont.
int GTFont_DrawW(void *ot, unsigned short *str, int x, int y, unsigned int rgb, int numFont)
{
    if (numFont == 0)
        return GTFont_DrawString(ot, str, x, y, rgb);
    return GTNum_DrawW(ot, str, x, y, rgb);
}

// GTFont_DrawW with the right edge at x.
int GTFont_DrawWRight(void *ot, unsigned short *str, int x, int y, unsigned int rgb, int numFont)
{
    if (numFont == 0)
        return GTText_DrawWRight(ot, str, x, y, rgb);
    return GTNum_DrawWRight(ot, str, x, y, rgb);
}

// Draws a string with the text font, or the number font if numFont.
int GTFont_Draw(void *ot, char *str, int x, int y, unsigned int rgb, int numFont)
{
    if (numFont == 0)
        return GTText_Draw(ot, str, x, y, rgb);
    return GTNum_Draw(ot, str, x, y, rgb);
}

// GTFont_Draw with the right edge at x.
int GTFont_DrawRight(void *ot, char *str, int x, int y, unsigned int rgb, int numFont)
{
    if (numFont == 0)
        return GTText_DrawRight(ot, str, x, y, rgb);
    return GTNum_DrawRight(ot, str, x, y, rgb);
}
}
