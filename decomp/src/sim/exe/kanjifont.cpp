// Resident code 8007f01c-8007f5a8 (kanjifont). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

// A cached glyph: a node of the binary trie KanjiFont_FindGlyph walks with the code's
// bits, and the texture coordinates of its cell.
struct KanjiGlyph {
    KanjiGlyph *left;  // code bit clear
    KanjiGlyph *right; // code bit set
    short x, y;        // cell position in the pixel buffer
    unsigned short code;
    unsigned char rendered;
    unsigned char wide; // 24 rows tall instead of 16
    unsigned short clut;
    unsigned short tpage;
    unsigned short uv;
};

// A glyph cache: a 4-bit texture of 16-pixel-wide cells and the glyph nodes that own them.
struct KanjiFont {
    unsigned char *pixels;
    RECT texRect; // in VRAM (w in 16-bit units)
    unsigned short tpage;
    unsigned short clut;
    KanjiGlyph *nodes;
    KanjiGlyph *root;
    KanjiGlyph *next; // the next free node
    short col, row;   // the next free cell
    short cols;       // cells a row
    unsigned char pad22;
    unsigned char wideMode;
};

// The textured quad of a glyph (PsyQ POLY_FT4 after its tag).
struct KanjiQuad {
    unsigned int rgb;
    short x0, y0;
    unsigned short uv0, clut;
    short x1, y1;
    unsigned short uv1, tpage;
    short x2, y2;
    unsigned short uv2, pad1;
    short x3, y3;
    unsigned short uv3, pad2;
};

struct KanjiPrim {
    unsigned char addr[3]; // the OT link
    unsigned char len;
    KanjiQuad quad;
};

extern "C" {
int Gpu_TransferImage(RECT *rect, unsigned int *data, int store);
unsigned short *KanjiFont_Krom2RawAdd(unsigned int code);
unsigned char *KanjiFont_GetPixelPtr(KanjiFont *font, int x, int y);
extern unsigned char *g_PacketPtr;
extern unsigned int D_800A0A68[];    // the glyphs' CLUT
extern unsigned short D_80095950[]; // a ROM row nibble, spread into three 5-bit fields

// Adds a quad of the glyph's cell, linked in after ot, and returns it for the caller to
// place (the game copies the 24-bit links with lwl and swl).
KanjiQuad *KanjiFont_AddGlyphQuad(KanjiGlyph *glyph, unsigned int *ot, unsigned int rgb)
{
    KanjiPrim *p;
    unsigned int link;
    int h;
    unsigned short uv;

    __asm__ volatile("lwl	%0,2(%1)" : "=r"(link) : "r"(ot));
    p = (KanjiPrim *)g_PacketPtr;
    p->len = 9;
    __asm__ volatile("swl	%0,2(%1)" : : "r"(link), "r"(p) : "memory");
    __asm__ volatile("swl	%0,2(%1)" : : "r"((unsigned int)p << 8), "r"(ot) : "memory");
    h = 0x1700;
    if (!glyph->wide) {
        h = 0xf00;
    }
    uv = glyph->uv;
    p->quad.rgb = rgb ^ 0x2c000000;
    p->quad.uv0 = uv;
    p->quad.clut = glyph->clut;
    g_PacketPtr = (unsigned char *)(p + 1);
    p->quad.tpage = glyph->tpage;
    p->quad.uv1 = uv + 0xf;
    p->quad.uv2 = uv + h;
    p->quad.uv3 = uv + (h | 0xf);
    return &p->quad;
}

void KanjiFont_SetBuffers(KanjiFont *font, unsigned char *pixels, KanjiGlyph *nodes, unsigned char wideMode)
{
    font->pixels = pixels;
    font->nodes = nodes;
    font->wideMode = wideMode;
}

// Places the texture at (x, y) in VRAM, w 4-bit pixels wide and h tall.
void KanjiFont_SetVram(KanjiFont *font, int x, int y, int w, int h)
{
    font->texRect.x = x;
    font->texRect.y = y;
    font->texRect.w = w >> 2;
    font->cols = w >> 4;
    font->tpage = (x >> 6) | ((y >> 8) << 4);
    font->texRect.h = h;
}

void KanjiFont_UploadClut(KanjiFont *font)
{
    RECT rect;

    rect.x = (font->clut & 0x3f) << 4;
    rect.y = (font->clut & 0x7fc0) >> 6;
    rect.w = 16;
    rect.h = 1;
    Gpu_TransferImage(&rect, D_800A0A68, 0);
}

void KanjiFont_UploadTexture(KanjiFont *font)
{
    Gpu_TransferImage(&font->texRect, (unsigned int *)font->pixels, 0);
}

void KanjiFont_Reset(KanjiFont *font)
{
    font->root = 0;
    font->row = 0;
    font->col = 0;
    font->next = font->nodes;
}

// Returns the glyph of an SJIS code, adding it (unrendered) in the next free cell if it
// isn't cached.
KanjiGlyph *KanjiFont_FindGlyph(KanjiFont *font, unsigned int code)
{
    KanjiGlyph *node = font->root;
    KanjiGlyph *parent = 0;
    KanjiGlyph **link;
    unsigned int bit = 0;
    unsigned int mask = 1;
    int col;
    int row;
    int cols;
    int tpage;
    int clut;
    int x;
    int y;
    int u;
    int v;

    if (node != 0) {
        do {
            unsigned short c;
            KanjiGlyph *next;

            bit = code & mask;
            c = node->code;
            link = bit ? &node->right : &node->left;
            next = *link;
            parent = node;
            if (code == c) {
                return node;
            }
            node = next;
            mask <<= 1;
        } while (node != 0);
    }

    node = font->next;
    link = &font->root;
    font->next = node + 1;
    if (parent != 0) {
        link = &parent->left;
        if (bit) {
            link = &parent->right;
        }
    }
    *link = node;

    col = font->col;
    row = font->row;
    u = font->texRect.x;
    v = font->texRect.y;
    cols = font->cols;
    tpage = (short)font->tpage;
    clut = (short)font->clut;
    x = col << 4;
    if (!font->wideMode) {
        y = row << 4;
    } else if (row < 10) {
        y = row * 24;
    } else {
        y = (row - 10) * 24 + 0x100;
    }
    u = (u & 0x3f) * 4 + x;
    v = (v & 0xff) + y;
    col++;
    node->y = y;
    node->left = 0;
    node->right = 0;
    node->code = code;
    node->x = x;
    node->rendered = 0;
    node->wide = font->wideMode;
    node->clut = clut;
    node->tpage = tpage + (u >> 8) + ((v >> 4) & 0x10);
    ((unsigned char *)&node->uv)[0] = u;
    ((unsigned char *)&node->uv)[1] = v;
    if (col >= cols) {
        col = 0;
        row++;
    }
    font->col = col;
    font->row = row;
    return node;
}

unsigned char *KanjiFont_GetPixelPtr(KanjiFont *font, int x, int y)
{
    return font->pixels + y * (font->cols * 8) + (x >> 1);
}

// Draws the glyph's 16x16 ROM bitmap into its cell scaled to 12x12, as 4-bit coverage
// (each output pixel sums a 4x4 block of weights).
#ifdef NON_MATCHING
// Same instructions as the game; font and glyph swap saved registers, and the row bits
// and the first sum swap $a1 and $a2.
// Two 4-bit pixels from two 5-bit sums (each saturated at 15).
static inline unsigned char Kanji_Pair(unsigned int s)
{
    unsigned int lo = s & 0x1f;
    unsigned int hi;

    if (lo == 16) {
        lo = 15;
    }
    hi = s >> 1 & 0x1f0;
    if (hi == 0x100) {
        hi = 0xf0;
    }
    return lo | hi;
}

void KanjiFont_RenderGlyph(KanjiFont *font, KanjiGlyph *glyph)
{
    int code = glyph->code & 0x7f7f;
    unsigned short *src;
    unsigned short *dst;
    unsigned int prevLo;
    unsigned int prevHi;
    unsigned int lo;
    unsigned int hi;
    unsigned int a;
    unsigned int b;
    unsigned int bits;
    int phase;
    int rest;
    int row;
    int stride;
    int n;

    if (!glyph->rendered) {
        src = KanjiFont_Krom2RawAdd(code);
        dst = (unsigned short *)KanjiFont_GetPixelPtr(font, (unsigned short)glyph->x, (unsigned short)glyph->y);
        prevHi = 0;
        prevLo = 0;
        phase = 2;
        row = 15;
        stride = font->cols;
        stride <<= phase;
        do {
            bits = *src++;
            rest = 4 - phase;
            if (row == 0) {
                bits = 0;
            }
            a = D_80095950[bits & 0xf];
            b = D_80095950[bits >> 4 & 0xf];
            lo = a << 15 | b;
            a = D_80095950[bits >> 8 & 0xf];
            b = D_80095950[bits >> 12];
            hi = a << 15 | b;
            if (phase != 0) {
                // The previous row's weight is phase quarters, this one's the rest.
                b = 0;
                n = phase - 1;
                a = prevLo;
                if (n != 0) {
                    a += prevLo;
                    b += prevHi;
                    n = phase - 2;
                }
                b += prevHi;
                if (n != 0) {
                    a += prevLo;
                    b += prevHi;
                }
                a += lo;
                n = rest - 1;
                if (n != 0) {
                    a += lo;
                    b += hi;
                    n = rest - 2;
                }
                b += hi;
                if (n != 0) {
                    a += lo;
                    b += hi;
                }
                ((unsigned char *)dst)[0] = Kanji_Pair(a);
                ((unsigned char *)dst)[1] = Kanji_Pair(a >> 10);
                ((unsigned char *)dst)[2] = Kanji_Pair(a >> 20);
                ((unsigned char *)dst)[3] = Kanji_Pair(b);
                ((unsigned char *)dst)[4] = Kanji_Pair(b >> 10);
                ((unsigned char *)dst)[5] = Kanji_Pair(b >> 20);
                dst[3] = 0;
                dst += stride;
            }
            phase = (phase - 1) & 3;
            prevLo = lo;
            prevHi = hi;
        } while (--row >= 0);

        // The four rows under the 12.
        ((unsigned int *)dst)[0] = 0;
        ((unsigned int *)dst)[1] = 0;
        dst += stride;
        ((unsigned int *)dst)[0] = 0;
        ((unsigned int *)dst)[1] = 0;
        dst += stride;
        ((unsigned int *)dst)[0] = 0;
        ((unsigned int *)dst)[1] = 0;
        dst += stride;
        ((unsigned int *)dst)[0] = 0;
        ((unsigned int *)dst)[1] = 0;
        glyph->rendered = 1;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/kanjifont", KanjiFont_RenderGlyph);
#endif
}
