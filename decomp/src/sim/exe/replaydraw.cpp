// Resident code 8006a3fc-8006ac68 (replaydraw). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

// A rectangle filled with a vertical gradient (Menu_AddGouraudQuad).
struct GouraudQuad {
    RECT rect;
    uint top;
    uint bottom;
};

// How the card menu's replay list draws its rows (D_800921BC).
struct ReplayEntryStyle {
    Font *titleFont;
    short unk4;
    short titleArg; // Text_DrawString's last argument
    Font *fieldFont;
    short unkc;
    short spacing; // the fields' letter spacing
    uint rgb;      // the row's background
};

// What ReplayEntry_DrawField draws with.
struct ReplayField {
    uint *ot;
    TextCtx *text;
    short spacing;
    short fade; // 0-128, towards black
    uint rgb;   // the text's colour
    uint tickRgb;
};

// The Course_Get entry's first field.
struct CourseEntry {
    char *name;
};

extern "C" {
void *Prim_AddDrawMode(uint *ot, uint tpage);
RECT *Prim_AddTile(uint *ot, uint rgb);
uint Rgb_Lerp(uchar *from, uchar *to, int t, int range);
void Menu_AddGouraudQuad(uint *ot, GouraudQuad *quad);
void Menu_AddGouraudQuadOpaque(uint *ot, GouraudQuad *quad);
void Text_SetFont(TextCtx *ctx, Font *font);
void Text_DrawString(TextCtx *ctx, char *s, int x, int y, int a4);
void Text_DrawCentered(TextCtx *ctx, char *s, int x, int y, int a4);
int Text_MeasureStringMonoDigits(TextCtx *ctx, char *s, int spacing, int a3);
int Text_DrawStringMonoDigits(TextCtx *ctx, char *str, int x, int y, int spacing, int digitShift, int digitExtra);
int Text_TruncateToWidth(TextCtx *ctx, char *str, int spacing, int maxWidth);
char *strcat(char *dst, char *src);
char *strcpy(char *dst, const char *src);
int sprintf(char *buf, char *fmt, ...);
unsigned int Course_FindByHash(unsigned int hash);
CourseEntry *Course_Get(int index);
void ReplayEntry_GetInfo(ReplayDirEntry *entry, char *buf);
extern unsigned char D_8008FA74;
extern uint D_80091E64; // ReplayEntry_Draw's colours
extern uint D_80091E68;
extern uint D_80091E6C;
extern uint D_80091E70;
extern uint D_80091E74;
extern uint D_80091E78;
extern uint D_80091E7C; // black, what everything fades to
extern char *D_80091E80[6]; // the license names' formats
extern uint D_80091E98; // the usage bar's colours
extern uint D_80091E9C;
extern uint D_80091EA0;
extern uint D_80091EA4;
extern char sB_NSector[]; // the card menu's strings start here
extern char sB_NewFile[];
extern char sB_Exit[];
extern char sB_Ok_2[];
extern char sB_TimeTrial[];
extern char sB_License[];
extern char sB_2p[];
extern char sB_TestRun[];
extern char sB_0400m[];
extern char sB_01000m[];
extern char sB_MaxSpeed[];
#define sB_Ghost (sB_NSector + 0x49)
#define sB_Race (sB_NSector + 0x59)

// Draws str at x, y, cut to maxWidth with "..." after it, and a tick to its left.
void ReplayEntry_DrawField(ReplayField *f, char *str, int maxWidth, int x, int y)
{
    RECT *r;

    if (Text_TruncateToWidth(f->text, str, f->spacing, maxWidth) != 0) {
        strcat(str, (char *)&D_8008FA74);
    }
    f->text->rgb = Rgb_Lerp((uchar *)&f->rgb, (uchar *)&D_80091E7C, f->fade, 0x80);
    Text_DrawStringMonoDigits(f->text, str, x + 5, y, f->spacing, -2, 0);
    r = Prim_AddTile(f->ot, f->tickRgb);
    r->w = 4;
    r->h = 6;
    r->x = x;
    r->y = y - 12;
    Prim_AddDrawMode(f->ot, 0x220);
}

// TextCtx's tpage as the GPU's bits.
struct TextTpage {
    uint clut : 16;
    uint page : 5;
    uint abr : 2; // semi-transparency mode
    uint rest : 9;
};

#ifndef NON_MATCHING
INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydraw", ReplayEntry_Draw);
#else
// A row of the replay list centred on x, y: entry's title, size, race type, course and
// info, or for empty 1-3 "New file", "OK" or "Exit". It fades in with alpha (0-128).
// The race type's string starts as "Race" before the switch, with an empty case 11, which
// gives the game's 12-entry table and the string in the range test's delay slot. Doesn't
// match (about 50 words): GCC keeps ot in $fp and spills line, where the game keeps line in
// $fp and reloads ot from its argument slot for each call. No declaration or store order
// tried changes that, and decomp-permuter (20 minutes) only got closer by moving code into
// another case.
void ReplayEntry_Draw(ReplayEntryStyle *style, uint *ot, TextCtx *ctx, ReplayDirEntry *entry, int x, int y,
                      int alpha, int empty)
{
    GouraudQuad q;
    char buf[256];
    ReplayField f;
    uint bg;
    uint line;
    int t = 0x80 - alpha;

    if (t < 0) {
        t = 0;
    }
    ((TextTpage *)&ctx->clut)->abr = 1;
    ctx->ot = ot;
    line = Rgb_Lerp((uchar *)&D_80091E64, (uchar *)&D_80091E7C, t, 0x80);
    f.text = ctx;
    f.ot = ot;
    f.spacing = style->spacing;
    f.fade = t;
    f.tickRgb = f.rgb = Rgb_Lerp((uchar *)&D_80091E78, (uchar *)&D_80091E7C, t, 0x80);
    switch (empty) {
    case 0: {
        char *s;
        int w;

        Text_SetFont(ctx, style->titleFont);
        ctx->rgb = Rgb_Lerp((uchar *)&D_80091E6C, (uchar *)&D_80091E7C, t, 0x80);
        Text_DrawString(ctx, entry->title, x - 0x80, y - 10, style->titleArg);
        q.rect.x = x - 0x80;
        q.rect.y = y - 0x12;
        q.rect.w = 0x100;
        q.rect.h = 8;
        q.top = Rgb_Lerp((uchar *)&D_80091E70, (uchar *)&D_80091E7C, t, 0x80);
        q.bottom = D_80091E7C;
        Menu_AddGouraudQuadOpaque(ot, &q);
        Prim_AddDrawMode(ot, 0x200);
        q.rect.x = x - 0x80;
        q.rect.y = y - 6;
        q.rect.w = 0x100;
        q.rect.h = 1;
        q.top = q.bottom = line;
        Menu_AddGouraudQuad(ot, &q);
        q.rect.y = y + 0xe;
        Menu_AddGouraudQuad(ot, &q);
        q.rect.y = y + 0x26;
        Menu_AddGouraudQuad(ot, &q);
        Prim_AddDrawMode(ot, 0x200);
        Text_SetFont(ctx, style->fieldFont);
        ctx->rgb = Rgb_Lerp((uchar *)&D_80091E74, (uchar *)&D_80091E7C, t, 0x80);
        sprintf(buf, sB_NSector, entry->sectorCount);
        w = Text_MeasureStringMonoDigits(ctx, buf, style->spacing, 0) - 0x7c;
        Text_DrawStringMonoDigits(ctx, buf, x - w, y + 0x26, style->spacing, -2, 0);
        if (entry->kind == 1) {
            s = sB_Ghost;
        } else {
            s = sB_Race;
            switch (entry->raceMode) {
            case 6:
                s = sB_TimeTrial;
                break;
            case 3:
                s = sB_License;
                break;
            case 0:
                s = sB_2p;
                break;
            case 1:
            case 10:
                s = sB_TestRun;
                break;
            case 7:
                s = sB_0400m;
                break;
            case 8:
                s = sB_01000m;
                break;
            case 9:
                s = sB_MaxSpeed;
                break;
            case 11:
                break;
            }
        }
        ReplayEntry_DrawField(&f, s, 0x48, x - 0x7a, y + 0xe);
        if (entry->raceMode == 3) {
            uint id = entry->courseId;
            uint license = id >> 16;
            uint test = id & 0xff;

            if (license >= 6) {
                license = 0;
            }
            sprintf(buf, D_80091E80[license], test);
        } else {
            strcpy(buf, Course_Get(Course_FindByHash(entry->courseId))->name);
        }
        ReplayEntry_DrawField(&f, buf, 0xac, x - 0x7a, y + 0x26);
        ReplayEntry_GetInfo(entry, buf);
        ReplayEntry_DrawField(&f, buf, 0x94, x - 0x20, y + 0xe);
        break;
    }
    case 1:
        Text_SetFont(ctx, style->titleFont);
        ctx->rgb = Rgb_Lerp((uchar *)&D_80091E68, (uchar *)&D_80091E7C, t, 0x80);
        Text_DrawCentered(ctx, sB_NewFile, x, y - 10, style->titleArg);
        break;
    case 3:
        Text_SetFont(ctx, style->titleFont);
        ctx->rgb = Rgb_Lerp((uchar *)&D_80091E68, (uchar *)&D_80091E7C, t, 0x80);
        Text_DrawCentered(ctx, sB_Exit, x, y - 10, style->titleArg);
        break;
    case 2:
        Text_SetFont(ctx, style->titleFont);
        ctx->rgb = Rgb_Lerp((uchar *)&D_80091E68, (uchar *)&D_80091E7C, t, 0x80);
        Text_DrawCentered(ctx, sB_Ok_2, x, y - 10, style->titleArg);
        break;
    }
    bg = Rgb_Lerp((uchar *)&style->rgb, (uchar *)&D_80091E7C, t, 0x80);
    q.bottom = Rgb_Lerp((uchar *)&bg, (uchar *)&D_80091E7C, 0x60, 0x80);
    q.top = bg;
    q.rect.x = x - 0x90;
    q.rect.y = y - 0x2a;
    q.rect.w = 0x120;
    q.rect.h = 0x54;
    Menu_AddGouraudQuadOpaque(ot, &q);
    Prim_AddDrawMode(ot, 0x220);
}
#endif

// The card's usage bar, 256 wide and centred on x: usage[0] sectors in all and usage[1]
// used. The used part leaves out need (the sectors of the entry being replaced), then
// needNew more sectors follow in another colour. It fades in with alpha (0-128).
void ReplayDir_DrawUsageBar(short *usage, uint *ot, int x, int y, int alpha, int need, int needNew)
{
    GouraudQuad q;
    RECT *r;
    int w;
    int left = x - 0x80;
    int top = y - 4;
    int t = 0x80 - alpha;

    if (t < 0) {
        t = 0;
    }
    w = ((usage[1] - need) << 8) / usage[0];
    q.rect.x = left;
    q.rect.y = top;
    q.rect.w = w;
    q.rect.h = 8;
    q.top = Rgb_Lerp((uchar *)&D_80091E98, (uchar *)&D_80091E7C, t, 0x80);
    q.bottom = Rgb_Lerp((uchar *)&D_80091EA0, (uchar *)&D_80091E7C, t, 0x80);
    Menu_AddGouraudQuad(ot, &q);
    q.rect.x = left + w;
    q.rect.w = ((usage[1] - need + needNew) << 8) / usage[0] - w;
    q.top = Rgb_Lerp((uchar *)&D_80091E9C, (uchar *)&D_80091E7C, t, 0x80);
    q.bottom = Rgb_Lerp((uchar *)&D_80091E98, (uchar *)&D_80091E7C, t, 0x80);
    Menu_AddGouraudQuad(ot, &q);
    Prim_AddDrawMode(ot, 0x220);
    r = Prim_AddTile(ot, 0);
    r->x = left;
    r->w = 0x100;
    r->y = top;
    r->h = 8;
    r = Prim_AddTile(ot, Rgb_Lerp((uchar *)&D_80091EA4, (uchar *)&D_80091E7C, t, 0x80));
    r->x = x - 0x81;
    r->w = 0x102;
    r->y = y - 6;
    r->h = 0xc;
}
}
