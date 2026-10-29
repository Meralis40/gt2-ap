// Resident code 8007da44-8007e0b0 (font). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int Printf_Format(void *putc, char *fmt, void *args);
void Text_SetDefaultTexturePage(int tpage);
unsigned int VolId_GetSector(VolBootId id);
int VolId_GetSize(VolBootId id);
void CdAudio_SetupRead(void *dst, unsigned int start, unsigned int size, int reportErrors);
int CdAudio_WaitRead(void (*onRetry)(void));
int CdGetSector(void *dst, int words);
int Gpu_TransferImage(RECT *rect, void *src, int arg);
void Text_DrawGlyph(TextCtx *ctx, int code, int x, int y);
int Text_GetAdvance(TextCtx *ctx, int left, int right);
int FUN_8007da9c(int c);
int FUN_8007daac(TextCtx *ctx, int c);
void RaceFont_InitUpload(void);
int RaceFont_OnSector(unsigned int sector, void *header);

extern CdAudio g_CdAudio;
extern unsigned int g_VolLba;
extern char *g_PacketPtr;
extern unsigned char D_80093148;
extern TextCtx *D_8009314C;

// Upload of the race font (VOL entry 0xc2) to VRAM, eight lines per sector.
struct RaceFontUpload {
    void *buf;      // sector buffer
    RECT rect;      // VRAM rectangle of the next sector
    ushort tpageId; // texture page the font goes to
    short unkE;
    short step;     // lines per sector
    ushort remaining; // sectors left
};
extern RaceFontUpload g_RaceFont_Buf;

// The pen position, in TextCtx.unk18.
#define TEXT_PEN_X(ctx) (((short *)&(ctx)->unk18)[0])
#define TEXT_PEN_Y(ctx) (((short *)&(ctx)->unk18)[1])

struct OtTag {
    unsigned addr : 24;
    unsigned len : 8;
};

// Links a draw-mode packet (E1 command mode) from g_PacketPtr in after ot.
// The 24-bit links are copied with lwl and swl, one asm statement each.
void *Prim_AddDrawMode(OtTag *ot, unsigned int mode)
{
    unsigned int *p;
    unsigned int link;

    __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
    p = (unsigned int *)g_PacketPtr;
    ((unsigned char *)p)[3] = 1;
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(p) : "memory");
    __asm__ volatile("swl\t%0,2(%1)" : : "r"((unsigned int)p << 8), "r"(ot) : "memory");
    p[1] = mode | 0xe1000000;
    g_PacketPtr = (char *)(p + 2);
    return p + 1;
}

void Text_SetFont(TextCtx *ctx, Font *font)
{
    ctx->font = *font;
}

// Printf_Format's output function for FUN_8007db30: draws c in D_8009314C.
#ifdef NON_MATCHING
// The game has no return of its own: it falls through into FUN_8007daac.
int FUN_8007da9c(int c)
{
    return FUN_8007daac(D_8009314C, c);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/font", FUN_8007da9c);
#endif

// Draws c at the pen in a fixed-width digit cell and moves the pen one cell right.
int FUN_8007daac(TextCtx *ctx, int c)
{
    int x = TEXT_PEN_X(ctx);
    int w = ctx->font.digitWidth;

    TEXT_PEN_X(ctx) = x + w;
    Text_DrawGlyph(ctx, (c & 0xff) | 0x100, x, TEXT_PEN_Y(ctx));
    return 0;
}

// Draws s with FUN_8007daac.
int FUN_8007dae4(TextCtx *ctx, signed char *s)
{
    int c;

    while ((c = *s++) != 0) {
        FUN_8007daac(ctx, c);
    }
    return 0;
}

void FUN_8007db30(TextCtx *ctx, char *fmt, void *args)
{
    TextCtx *prev;

    prev = D_8009314C;
    D_8009314C = ctx;
    Printf_Format((void *)FUN_8007da9c, fmt, args);
    D_8009314C = prev;
}

void FUN_8007db70(TextCtx *ctx, char *fmt, ...)
{
    FUN_8007db30(ctx, fmt, __builtin_next_arg(fmt));
}

// Draws s at the pen with kerning; the pen doesn't move.
#ifdef NON_MATCHING
// The game keeps s in $s2 and the current character in $s1, copied from the next
// one at the top of the loop; GCC swaps them (permuter didn't find a form).
int FUN_8007db9c(TextCtx *ctx, signed char *s)
{
    int next = *s++;
    int x = TEXT_PEN_X(ctx);
    int y = TEXT_PEN_Y(ctx);

    while (next) {
        int c = next;
        Text_DrawGlyph(ctx, c & 0xff, x, y);
        next = *s++;
        if (!next) break;
        x += Text_GetAdvance(ctx, c, next);
    }
    return 0;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/font", FUN_8007db9c);
#endif

void Text_SetTexturePage(TextCtx *ctx, int tpageId)
{
    unsigned short uVar1;

    if (tpageId < 0) {
        tpageId = (int)D_80093148;
    }
    uVar1 = (unsigned short)tpageId;
    ctx->clut = (uVar1 & 0xf) * 4 + (uVar1 & 0x10) * 0x400;
    ctx->tpage = uVar1;
    *(unsigned char *)((int)&ctx->rgb + 3) = 0;
}

// Advance from left to right: left's advance plus the spacing, less the kerning.
#ifdef NON_MATCHING
// The kerning word's lwr and lwl are an asm statement each, which gives the game's
// order (GCC's ulw is lwl first). The game adds the rows offset to a separate rows
// pointer, multiplies with multu, and schedules the loads at the start differently.
int Text_GetAdvance(TextCtx *ctx, int left, int right)
{
    FontKerning *kern = ctx->font.kerning;
    FontGlyphTable *glyphs = ctx->font.glyphs;
    int header = *(int *)kern;
    int spacing = (unsigned char)ctx->font.spacing;
    int rowBytes = header & 0xff;
    int l = ((int *)&glyphs->glyphs[left & 0xff])[0];
    int r = ((int *)&glyphs->glyphs[right & 0xff])[1];
    int advance = spacing + (l & 0x3f);
    int k = l >> 23;

    if (l < 0) {
        k &= 0x3f;
    } else {
        int rowOff = (k & 0xff) * rowBytes;
        k = r >> 23;
        if (r < 0) {
            k &= 0x3f;
        } else {
            unsigned char col = k;
            int bits = header >> 16;
            int n = bits & 0xff;
            int bit = col * 4;
            if (bits & 1) bit += col;
            if (bits & 2) bit += col * 2;
            unsigned char *q = kern->rows + (bit >> 3) + rowOff;
            int w;

            __asm__("lwr\t%0,0(%1)" : "=r"(w) : "r"(q));
            __asm__("lwl\t%0,3(%1)" : "+r"(w) : "r"(q));
            k = (w >> (bit & 7)) & ((1 << n) - 1);
        }
    }
    return advance - k;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/font", Text_GetAdvance);
#endif

// Adds the SPRT for glyph code & 0xff at (x, y) and its accent, if any. Bit 0x100
// moves it by the glyph's fixed-width offset and bit 0x200 centers it on x.
#ifdef NON_MATCHING
// The links are lwl and swl asm statements, as in Prim_AddDrawMode. The game computes
// every field before storing any, reads the CLUT and texture page as one word, and its
// registers differ.
void Text_DrawGlyph(TextCtx *ctx, int code, int x, int y)
{
    FontGlyphTable *tbl = ctx->font.glyphs;
    unsigned int g0 = ((unsigned int *)&tbl->glyphs[code & 0xff])[0];
    int g1 = ((int *)&tbl->glyphs[code & 0xff])[1];
    int sprite = (g0 & 0x3fc0) >> 6;
    int center;
    unsigned int s;
    unsigned int *p;
    int top;
    int hw;
    OtTag *ot;
    unsigned int link;

    if (!sprite) return;
    center = 0;
    if (code & 0xff00) {
        if (code & 0x100) x += (g0 >> 17) & 0x3f;
        if (code & 0x200) center = -1;
    }
    p = (unsigned int *)g_PacketPtr;
    s = ((unsigned int *)tbl->sprites)[sprite];
    top = y - ((g1 >> 6) & 0x3f);
    hw = (s >> 16) & 0x1f;
    x -= center & hw;
    p[1] = 0xe1000000 | (ctx->tpage + (s >> 29));
    p[2] = ctx->rgb ^ 0x64000000;
    p[3] = (top << 16) | ((x + (g1 & 0x3f)) & 0xffff);
    p[5] = (hw << 1) | (((s >> 21) & 0x3f) << 16);
    ((ushort *)p)[9] = ctx->clut + ((s >> 27) & 3);
    ((ushort *)p)[8] = s;
    ot = (OtTag *)ctx->ot;
    __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(p) : "memory");
    ((unsigned char *)p)[3] = 5;
    __asm__ volatile("swl\t%0,2(%1)" : : "r"((unsigned int)p << 8), "r"(ot) : "memory");
    if ((g0 >> 14) & 7) {
        s = ((unsigned int *)tbl->accents)[((g0 >> 14) & 7) - 1];
        p += 6;
        p[1] = (ctx->tpage + (s >> 29)) ^ 0xe1000000;
        ((short *)p)[6] = x + ((g1 >> 12) & 0x3f);
        ((short *)p)[7] = top - ((g1 >> 18) & 0x1f);
        ((ushort *)p)[8] = s;
        p[2] = ctx->rgb ^ 0x64000000;
        ((short *)p)[10] = (s >> 15) & 0x3e;
        ((short *)p)[11] = (s >> 21) & 0x3f;
        ((ushort *)p)[9] = ctx->clut + ((s >> 27) & 3);
        ot = (OtTag *)ctx->ot;
        __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
        ((unsigned char *)p)[3] = 5;
        __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(p) : "memory");
        __asm__ volatile("swl\t%0,2(%1)" : : "r"((unsigned int)p << 8), "r"(ot) : "memory");
    }
    g_PacketPtr = (char *)(p + 6);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/font", Text_DrawGlyph);
#endif

// Sector function of the race font read: uploads each sector to VRAM.
#ifdef NON_MATCHING
// The game passes its arguments on to an inlined function of g_CdAudio: it moves
// them up a register and loads &g_CdAudio into $a0 before the prologue. An inline
// helper and a register variable didn't give it.
int RaceFont_OnSector(unsigned int sector, void *header)
{
    CdAudio *cd = &g_CdAudio;
    RaceFontUpload *u = &g_RaceFont_Buf;
    int ret = 8;

    cd->curSector = cd->nextSector;
    if (sector == cd->nextSector) {
        cd->nextSector = sector + 1;
        ret = 0;
        if (u->remaining) {
            CdGetSector(g_RaceFont_Buf.buf, 0x200);
            Gpu_TransferImage(&u->rect, g_RaceFont_Buf.buf, ret);
            u->rect.y += u->step;
            if (--u->remaining == 0) {
                cd->unk167 = 0;
            }
        }
    }
    return ret;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/font", RaceFont_OnSector);
#endif

// Sets up the upload: 32 sectors of 128x8 halfwords to the font's texture page.
void RaceFont_InitUpload(void)
{
    RaceFontUpload *u = &g_RaceFont_Buf;
    ushort id = u->tpageId;

    u->rect.w = 0x80;
    u->rect.h = 8;
    u->step = 8;
    u->unkE = 0x20;
    u->remaining = 0x20;
    u->rect.x = (id & 0xf) << 6;
    u->rect.y = (id << 4) & 0x100;
}

// Reads the race font into texture page tpageId.
void RaceFont_Load(int tpageId)
{
    char buf[0x800];
    unsigned int start;

    D_80093148 = tpageId;
    Text_SetDefaultTexturePage(tpageId);
    g_RaceFont_Buf.buf = buf;
    g_RaceFont_Buf.tpageId = tpageId;
    RaceFont_InitUpload();
    start = g_VolLba + VolId_GetSector(0xc2);
    CdAudio_SetupRead(0, start, VolId_GetSize(0xc2), 0);
    g_CdAudio.onSector = (void *)RaceFont_OnSector;
    CdAudio_WaitRead(RaceFont_InitUpload);
}
}
