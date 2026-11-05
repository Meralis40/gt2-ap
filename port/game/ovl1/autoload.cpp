// OVL1 800169b0-8001721c (autoload): Kanji strings and the auto-load page.
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern char *g_AutoLoad_Line1;
extern char *g_AutoLoad_Line2;
extern int g_AutoLoad_State;
extern int g_AutoLoad_Port;
extern int g_AutoLoad_Timer;
extern unsigned char g_AutoLoad_EmptyLine;
extern CardProgressBar g_AutoLoad_Progress;
extern GameSaveFile g_SaveFileBuf;
extern KanjiFont *g_OVL1_KanjiFont;
extern short g_CardSlotStatus[];
extern short D_8004B860;
extern int g_McOp_Remaining;
extern Font g_OVL1_FontLarge;
extern BaseMenu *g_CurrentMenu;
extern char sB_MemoryCard1[];
extern char sB_Loading[];
extern char sB_LoadingComplete[];
extern char sB_LoadingFailed[];
extern char sB_LoadingDataFailed[];
extern signed char s_LoadingSaveData[];
extern signed char s_AutoLoadingComplete[];
extern signed char s_AutoLoadingFailed[];
extern signed char s_SaveDataIsCorrupt[];
void Kanji_DrawString(void *ot, char *sjis, int x, int y, int center, unsigned int rgb);

// Japanese only: resets the kanji glyph cache and caches the two slot names.
void Kanji_CacheSlotNames(void)
{
    KanjiFont *font = g_OVL1_KanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_Reset(font);
        KanjiFont_CacheString(font, sB_MemoryCard1);
        KanjiFont_CacheString(font, sB_MemoryCard1 + 0x11);
    }
}

// Japanese only: caches the glyphs of a string.
void Kanji_CacheString(char *sjis)
{
    KanjiFont *font = g_OVL1_KanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_CacheString(font, sjis);
    }
}

// Japanese only: resets the kanji glyph cache and caches the glyphs of the slot names
// and of the two current message lines.
void AutoLoad_CacheKanjiMessages(void)
{
    if (g_GameState.language == 0) {
        Kanji_CacheSlotNames();
        Kanji_CacheString(g_AutoLoad_Line1);
        Kanji_CacheString(g_AutoLoad_Line2);
    }
}

// Japanese only: draws SJIS text from the kanji cache, 12 pixels a character
// (centred on x if center is set).
void Kanji_DrawString(void *ot, char *sjis, int x, int y, int center, unsigned int rgb)
{
    KanjiFont *font = g_OVL1_KanjiFont;
    unsigned char *p;
    unsigned int semi;
    int w = 0;

    if (g_GameState.language != 0) {
        return;
    }
    if (center) {
        unsigned char *s;

        for (s = (unsigned char *)sjis; *s != 0; s += 2) {
            w += 12;
        }
        x -= w >> 1;
    }
    p = (unsigned char *)sjis;
    semi = 0x2000000;
    while (*p != 0) {
        KanjiGlyph *glyph = KanjiFont_FindGlyph(font, p[0] << 8 | p[1]);

        if (glyph != 0) {
            PrimFT4 *q = (PrimFT4 *)KanjiFont_AddGlyphQuad(glyph, (unsigned int *)ot, rgb | semi);

            q->x0 = x;
            q->y0 = y;
            q->x1 = x + 15;
            q->y1 = y;
            q->x2 = x;
            q->y2 = y + 31;
            q->x3 = x + 15;
            q->y3 = y + 31;
            q->tpage = (q->tpage & ~0x60) | 0x20;
        }
        x += 12;
        p += 2;
    }
    Prim_AddDrawMode(ot, 0x220);
}

// g_Page_AutoLoad enter: resets the state machine, shows the "Memory Card 1" header
// and builds the save-file header template in g_SaveFileBuf.
void AutoLoad_Enter(int back)
{
    g_AutoLoad_State = 0;
    g_AutoLoad_Port = 0;
    g_AutoLoad_Line1 = (char *)&g_AutoLoad_EmptyLine;
    g_AutoLoad_Line2 = (char *)&g_AutoLoad_EmptyLine;
    AutoLoad_CacheKanjiMessages();
    SaveFile_InitHeader(&g_SaveFileBuf);
}

// Looks for a save on both cards and loads it. States: 0 find it on the port, 1 no
// save, 2 start the read, 3 reading, 4 check it, 5 loaded, 6 corrupt, 7 read failed,
// 8 its message, 9 and 10 a short wait before leaving the menu (4). A read that
// doesn't start sets state 7, which the store of state 3 overwrites.
int AutoLoad_Update(int arg)
{
    int status = g_CardSlotStatus[g_AutoLoad_Port];
    PadState *pad = (PadState *)((char *)g_CurrentMenu + 0x1a4);
    int result = 0;
    int r;

    switch (g_AutoLoad_State) {
    case 0:
        switch (status) {
        case 0:
            if (SaveFile_Find(&g_SaveFileBuf, g_AutoLoad_Port) != 0) {
                g_AutoLoad_State = 2;
            } else {
                g_AutoLoad_State = 1;
                g_AutoLoad_Port++;
            }
            break;
        case 1:
            break;
        default:
            g_AutoLoad_State = 1;
            g_AutoLoad_Port++;
            break;
        }
        break;
    case 2:
        if (status != 0) {
            g_AutoLoad_State = 7;
            break;
        }
        if (SaveFile_StartRead(&g_SaveFileBuf, g_AutoLoad_Port, &g_SaveFileBuf) != 0) {
            g_AutoLoad_State = 7;
        } else {
            g_AutoLoad_Line1 = sB_Loading;
            g_AutoLoad_Line2 = sB_Loading - 0x151; // sB_PleaseDoNotRemoveMemoryCard
            AutoLoad_CacheKanjiMessages();
            g_AutoLoad_Progress.total = SaveFile_GetFileSize(&g_SaveFileBuf);
            CardProgress_Reset(&g_AutoLoad_Progress);
        }
        g_AutoLoad_State = 3;
        break;
    case 3:
        r = McCard_GetResult();
        switch (r) {
        case 1:
            CardProgress_Update(&g_AutoLoad_Progress, g_McOp_Remaining);
            return result;
        case 0:
            D_8004B860 = -1;
            g_AutoLoad_State = 4;
            break;
        default:
            D_8004B860 = -1;
            g_AutoLoad_State = 7;
            break;
        }
        break;
    case 4:
        if (SaveFile_CheckCrc(&g_SaveFileBuf, &g_SaveFileBuf) != 0) {
            GameState_ReadSave((unsigned int)&g_SaveFileBuf, (int)&g_SaveFileBuf);
            g_AutoLoad_Timer = 0xb4;
            g_AutoLoad_State = 5;
            g_AutoLoad_Line1 = (char *)&g_AutoLoad_EmptyLine;
            g_AutoLoad_Line2 = sB_LoadingComplete;
        } else {
            g_AutoLoad_Timer = 0xf0;
            g_AutoLoad_State = 6;
            g_AutoLoad_Line1 = (char *)&g_AutoLoad_EmptyLine;
            g_AutoLoad_Line2 = sB_LoadingFailed;
        }
        AutoLoad_CacheKanjiMessages();
        break;
    case 7:
        Sfx_PlayMenu(0);
        g_AutoLoad_Timer = 0xf0;
        g_AutoLoad_State = 8;
        g_AutoLoad_Line1 = (char *)&g_AutoLoad_EmptyLine;
        g_AutoLoad_Line2 = sB_LoadingDataFailed;
        AutoLoad_CacheKanjiMessages();
        break;
    case 5:
    case 6:
    case 8:
        g_AutoLoad_Timer--;
        if ((pad->pressed & 0xf00) != 0) {
            g_AutoLoad_Timer = 0;
            Sfx_PlayMenu(1);
        }
        if (g_AutoLoad_Timer != 0) {
            break;
        }
    case 1:
        g_AutoLoad_State = 9;
        break;
    case 9:
        g_AutoLoad_Timer = 6;
        g_AutoLoad_State = 10;
        break;
    case 10:
        if (--g_AutoLoad_Timer == 0) {
            result = 4;
        }
        break;
    }
    return result;
}

// The current message, Japanese from the kanji cache, then the card's name.
void AutoLoad_Draw(DrawLayer *layer)
{
    TextCtx ctx;
    TextCtx *c;
    unsigned int rgb = 0x604610;
    int show = 0;
    uint *ot = (uint *)layer->ot;
    char *name;

    Text_Init(&ctx, 0x1e);
    c = &ctx;
    c->rgb = rgb;
    c->ot = ot;
    Text_SetFont(c, &g_OVL1_FontLarge);

    switch (g_AutoLoad_State) {
    case 3:
        if (g_GameState.language == 0) {
            rgb = 0x26e5a50;
            Kanji_DrawString(ot, g_AutoLoad_Line1, 0xb0, 0xd0, 1, rgb);
            Kanji_DrawString(ot, g_AutoLoad_Line2, 0xb0, 0xf0, 1, rgb);
            Prim_AddDrawMode(ot, 0x220);
        } else {
            Text_DrawCentered(c, s_LoadingSaveData, 0xb0, 0xf0, 1);
        }
        CardProgress_Draw(&g_AutoLoad_Progress, ot);
        show = 1;
        break;
    case 5:
        if (g_GameState.language == 0) {
            rgb = 0x26e5a50;
            Kanji_DrawString(ot, g_AutoLoad_Line1, 0xb0, 0xd0, 1, rgb);
            Kanji_DrawString(ot, g_AutoLoad_Line2, 0xb0, 0xf0, 1, rgb);
        } else {
            Text_DrawCentered(c, s_AutoLoadingComplete, 0xb0, 0xf0, 1);
        }
        show = 1;
        break;
    case 8:
        rgb = 0x20a50f0;
        if (g_GameState.language == 0) {
            Kanji_DrawString(ot, g_AutoLoad_Line1, 0xb0, 0xd0, 1, rgb);
            Kanji_DrawString(ot, g_AutoLoad_Line2, 0xb0, 0xf0, 1, rgb);
        } else {
            c->rgb = rgb;
            Text_DrawCentered(c, s_AutoLoadingFailed, 0xb0, 0xf0, 1);
        }
        show = 1;
        break;
    case 6:
        rgb = 0x20a50f0;
        if (g_GameState.language == 0) {
            Kanji_DrawString(ot, g_AutoLoad_Line1, 0xb0, 0xd0, 1, rgb);
            Kanji_DrawString(ot, g_AutoLoad_Line2, 0xb0, 0xf0, 1, rgb);
        } else {
            c->rgb = rgb;
            Text_DrawCentered(c, s_SaveDataIsCorrupt, 0xb0, 0xf0, 1);
        }
        show = 1;
        break;
    }
    if (show) {
        rgb = 0x26e6460;
        name = sB_MemoryCard1;
        if (g_AutoLoad_Port == 1) {
            name += 0x11;
        }
        if (g_GameState.language == 0) {
            Kanji_DrawString(ot, name, 0xb0, 0x94, 1, rgb);
        } else {
            Text_DrawCentered(&ctx, (signed char *)name, 0xb0, 0xb4, 1);
        }
    }
}
}
