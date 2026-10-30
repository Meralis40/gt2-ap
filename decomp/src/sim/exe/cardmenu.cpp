// Resident code 8006ebf0-8007306c (cardmenu). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CardMenu_CacheKanji(char *sjis);
void CardMenu_CacheKanjiSlotNames(void);
void CardMenu_InitWidgets(void);
void CardMenu_PrepareGhostEntry(void);
void CardMenu_PrepareReplayEntry(void);
int CardMenu_SetState(int state);
int CardState_Idle(int event, void *arg);
void KanjiFont_Reset(void *font);
void KanjiFont_CacheString(void *font, char *sjis);
void GameState_WriteSave(void *dst, void *file, int size);
void KanjiFont_SetBuffers(void *font, void *pixels, void *nodes, unsigned char wideMode);
void KanjiFont_SetVram(void *font, int x, int y, int w, int h);
void *KanjiFont_FindGlyph(void *font, unsigned int code);
void KanjiFont_RenderGlyph(void *font, void *glyph);
typedef void (*KanjiLoadFn)(void *font, void *glyph);
int ReplayData_BuildRace(void *dst);
int ReplayData_BuildGhost(void *dst);
void ReplayEntry_SetTitle(char *dst, char *title);
void ReplayEntry_SetCarName(void *entry, char *name);
void Sfx_PlayMenu(int sfx);
void CardDialog_Open(CardDialog *dlg);
int McCard_GetResult(void);
void McCard_StartFormat(int port);
int ReplayFile_Create(ReplayDir *dir, int port);
void CardMenu_CacheKanjiLines(void);
void CardMenu_CacheKanjiError(void);
void CardDialog_Close(CardDialog *dlg);
int SaveFile_Create(GameSaveFile *file, int port);
int ReplayFile_StartWriteDir(ReplayDir *dir, int port);
void CardProgress_Update(CardProgressBar *bar, int remaining);
void CardProgress_Draw(CardProgressBar *bar);
extern int g_McOp_Remaining;
int SaveFile_StartWrite(GameSaveFile *file, int port, void *data);
int SaveFile_StartRead(GameSaveFile *file, int port, void *data);
int SaveFile_GetFileSize(GameSaveFile *file);
int SaveFile_CheckCrc(GameSaveFile *file, void *data);
void GameState_ReadSave(void *file, void *data);
void CardProgress_Reset(CardProgressBar *bar);
void Text_Init(TextCtx *ctx, int tpageId);
void Text_SetFont(TextCtx *ctx, Font *font);
void Text_DrawCentered(TextCtx *ctx, char *s, int x, int y, int a4);
void CardMenu_DrawKanji(uint *ot, char *sjis, int x, int y, int a4, uint rgb);
void ReplayDir_Init(ReplayDir *dir, int blocks);
int ReplayFile_StartWriteData(ReplayDir *dir, int port, void *data, int size, short *sectors);
void ReplayDir_UpdateCrc(ReplayDir *dir);
int ReplayFile_StartReadEntry(ReplayDir *dir, int port, int entry, void *data, short *sectors);
int ReplayDir_CheckEntryCrc(ReplayDir *dir, int entry, void *data);
void Replay_Unpack(void *data, int arg);
void GhostReplay_Unpack(void *data);
struct PolyFT4;
PolyFT4 *KanjiFont_AddGlyphQuad(void *glyph, uint *ot, uint rgb);
void *Prim_AddDrawMode(uint *ot, uint tpage);
int ReplayFile_StartReadDir(ReplayDir *dir, int port);
int ReplayDir_IsValid(ReplayDir *dir);
// Through an inline function, as the game tests the result with xori and beqz.
static inline int ReplayDir_IsOk(ReplayDir *dir)
{
    return ReplayDir_IsValid(dir) == 1;
}
void MenuList_Open(MenuList *list);
void MenuList_Close(MenuList *list);
unsigned char ReplayDir_IsPlayable(ReplayDir *dir, int entry);
int SaveFile_Find(GameSaveFile *file, int port);
int SaveFile_GetBlockCount(GameSaveFile *file);
int ReplayFile_Find(ReplayDir *dir, int port);
void WipeBar_Update(WipeBar *bar);
int CardDialog_Update(CardDialog *dlg, PadState *pad);
int NumberPicker_Update(NumberPicker *p, PadState *pad);
int MenuList_Update(MenuList *list, PadState *pad);
int TextEntry_Update(TextEntry *entry, PadState *pad);
struct CardDialogDesc;
void CardDialog_Init(CardDialog *dlg, CardDialogDesc *desc, int choice);
void NumberPicker_Init(NumberPicker *p);
void MenuList_Init(MenuList *list, void *callback, int arg);
void TextEntry_Init(TextEntry *entry, TextEntry *desc);
int CardMenu_EntryListCallback(int event, MenuListEvent *ev, int row);
extern WipeBar D_80091EE8;          // the slot bar
extern CardDialogDesc D_80091F04;   // the dialogs, in CardMenu order
extern CardDialogDesc D_80091F34;
extern CardDialogDesc D_80091F64;
extern CardDialogDesc D_80091F94;
extern CardDialogDesc D_80092018;
extern CardDialogDesc D_80092048;
extern CardDialogDesc D_80092078;
extern CardDialogDesc D_800920A8;
extern CardDialogDesc D_800920D8;
extern CardDialogDesc D_80092108;
extern CardDialogDesc D_80092138;
extern CardProgressBar D_80092168;  // the progress bar
extern TextEntry D_800921A0;        // the title entry
void ReplayEntry_Draw(uint *style, uint *ot, TextCtx *ctx, ReplayDirEntry *entry, int x, int y, int alpha, int empty);
extern uint D_800921BC[]; // the entry list's style; [4] is the row colour
int WipeBar_GetLevel(WipeBar *bar);
void WipeBar_Draw(WipeBar *bar, uint *ot, int x, int y);
uint Rgb_Lerp(uchar *from, uchar *to, int t, int range);
void Text_DrawString(TextCtx *ctx, char *s, int x, int y, int a4);
void CardDialog_Draw(CardDialog *dlg, uint *ot, TextCtx *ctx);
void NumberPicker_Draw(NumberPicker *p, uint *ot, TextCtx *ctx);
void MenuList_Draw(MenuList *list, uint *ot);
void TextEntry_Draw(TextEntry *entry, uint *ot);
extern char *D_800921B4[2]; // the slot names
char *strcpy(char *dst, const char *src);
int strlen(const char *s);
void TextEntry_Open(TextEntry *entry);
void TextEntry_Close(TextEntry *entry);
int ReplayDir_AddEntry(ReplayDir *dir, int at, ReplayDirEntry *entry, void *data, int size, short *sectors);
int Text_MeasureString(TextCtx *ctx, char *s, int a2);
int sprintf(char *buf, char *fmt, ...);
int SectorMap_CountUsed(SectorMap *map);
int SectorMap_CountFree(SectorMap *map);
int ReplayDir_CheckFits(ReplayDir *dir, int at, int size);
void ReplayDir_DeleteEntry(ReplayDir *dir, int entry);
void ReplayDir_DrawUsageBar(short *usage, uint *ot, int x, int y, int alpha, int need, int needNew);
typedef int (*CardStateFn)(int event, void *arg);
extern CardStateFn g_CardMenu_States[];
// The menu text, one block of strings (sB_LbFt is the first). The game addresses them
// from one base, so the second of two strings is often an offset from the first.
extern char sB_LbFt[];
enum {
    T_LbFt = 0x0,
    T_Hp = 0x6,
    T_1000rpm = 0x9,
    T_Nlb = 0x11,
    T_Nhp = 0x16,
    T_SelectNumberOfBlocks = 0x1b,
    T_NotEnoughEmptyBlocks = 0x3a,
    T_SelectASlot = 0x59,
    T_CheckingSlot = 0x76,
    T_PleaseDoNotRemoveMemoryCard = 0x94,
    T_NoMemoryCardDetected = 0xbf,
    T_FailedToReadMemoryCard = 0xdc,
    T_MemoryCardIsNotFormatted = 0x105,
    T_DoYouWantToFormat = 0x126,
    T_Formatting = 0x13e,
    T_FormattingFailed = 0x157,
    T_CheckingReplayFile = 0x174,
    T_CreatingFile = 0x197,
    T_CreatingFileFailed = 0x1b0,
    T_StartLoading = 0x1d0,
    T_Loading = 0x1e5,
    T_LoadingComplete = 0x1f8,
    T_LoadingDataFailed = 0x20f,
    T_StartSaving = 0x22c,
    T_OverwriteFile = 0x241,
    T_Saving = 0x258,
    T_SavingComplete = 0x26b,
    T_TrySavingAgain = 0x283,
    T_SavingDataFailed = 0x2a1,
    T_GameFileAlreadyExists = 0x2ba,
    T_NoGameFileFound = 0x2e7,
    T_LoadingFailed = 0x30f,
    T_FailedToCreateGameFile = 0x326,
    T_NoReplayFilesFound = 0x350,
    T_NoReplayDataInFile = 0x372,
    T_FailedToCreateReplayFile = 0x39d,
    T_LoadingFailed_2 = 0x3cc,
    T_NoMatchingDataFound = 0x3e3,
    T_RenameTitle = 0x40a,
    T_SaveChanges = 0x421,
    T_CreateNewFile = 0x43a,
    T_OverwriteFile_2 = 0x450,
    T_DeleteFile = 0x465,
    T_SaveReplay = 0x475,
    T_MemoryCard1 = 0x489,
    T_MemoryCard2 = 0x49a,
    T_TotalNReplay = 0x4ab,
    T_NNSectorFree = 0x4c4,
    T_NeedNSector = 0x4db,
    T_PressL1R1ForDeleteMode = 0x4f2,
    T_EditReplayDataFile = 0x512,
    T_Ok = 0x534,
    T_Cancel = 0x537,
    T_MemoryCardSlot = 0x541,
    T_Slot1 = 0x559,
    T_Slot2 = 0x566,
    T_NoMemoryCardDetected_2 = 0x573,
    T_ChangeSlot = 0x590,
    T_Load = 0x5a2,
    T_Save = 0x5ac,
    T_Start = 0x5ba,
    T_DoYouWantToFormat_2 = 0x5c4,
    T_Yes = 0x5db,
    T_No = 0x5df,
    T_Error = 0x5e4,
    T_DataError = 0x5ee,
    T_DoYouWantToOverwrite = 0x603,
    T_NSector = 0x61d,
    T_NewFile = 0x62c,
    T_Exit = 0x640,
    T_Ok_2 = 0x64c,
    T_Exit_2 = 0x653,
    T_Retry = 0x65b,
    T_Ghost = 0x666,
    T_Race = 0x676,
    T_TimeTrial = 0x67e,
    T_License = 0x68f,
    T_2p = 0x69c,
    T_Max32Files = 0x6a0,
    T_DataIsTooLarge = 0x6b1,
    T_Cr = 0x6ca,
    T_ChangesAreNotSaved = 0x6ce,
    T_Cancel_2 = 0x6f2,
    T_Play = 0x707,
    T_CannotPlayGhost = 0x714,
    T_TestRun = 0x73e,
    T_Championship = 0x750,
    T_0400m = 0x75e,
    T_01000m = 0x768,
    T_MaxSpeed = 0x773,
    T_SN = 0x788,
    T_IaN = 0x78d,
    T_IbN = 0x793,
    T_IcN = 0x799,
    T_AN = 0x79f,
    T_BN = 0x7a4,
};
#define TEXT(name) (sB_LbFt + T_##name)

// m->dir.entries + i, added up as the game does in CardState_EditTitle: the entry's offset
// and m first, then the offset of the entries.
#define DIR_ENTRY(m, i)     ((ReplayDirEntry *)((char *)(m) + (i) * sizeof(ReplayDirEntry) + (int)((CardMenu *)0)->dir.entries))

// This file's .rodata, in the game's order: the empty line, CardState_CheckSlot's three
// jump tables, the default replay title and CardMenu_Update's jump table.
extern const char D_8008FAC8[8];
const char D_8008FAC8[8] = { 0 }; // braces: a string initializer is emitted again as a literal
#define EMPTY ((char *)D_8008FAC8)
enum { CARD_EXIT = 0x27, CARD_REPLAY_LOADED = 0x28 }; // what a state returns to leave
void KanjiFont_UploadClut(void *font);
void KanjiFont_UploadTexture(void *font);
void SaveFile_InitHeader(GameSaveFile *file);
extern CardMenu *g_CardMenu;
extern signed char g_CardMenu_GhostLoaded;
extern GameState g_GameState;
extern MenuList g_CardMenu_EntryList;      // the replay files
extern NumberPicker g_CardMenu_BlockPicker; // the replay file's size, in blocks
extern Font g_CardMenu_FontSmall;
extern Font g_CardMenu_FontMedium;
extern Font g_CardMenu_FontLarge;

// A memory card port of the card driver (0x264 bytes).
struct McPort {
    uchar unk000[0x25a];
    uchar freeBlocks;
    uchar unk25b[2];
    uchar state;
    uchar unk25e[6];
};

// The card driver state.
struct CardSystem {
    uchar unk00[0x44];
    McPort ports[2];
};

extern CardSystem g_Card;

// The textured quad of a glyph (PsyQ POLY_FT4 after its tag).
struct PolyFT4 {
    uint rgb;
    short x0, y0;
    uchar u0, v0;
    ushort clut;
    short x1, y1;
    uchar u1, v1;
    ushort tpage;
    short x2, y2;
    uchar u2, v2;
    ushort pad1;
    short x3, y3;
    uchar u3, v3;
    ushort pad2;
};

// What an overlay hands CardMenu_Setup.
struct CardMenuConfig {
    CardMenu *work; // becomes g_CardMenu
    void *kanjiFont;
    short *slotStatus;
    Font *fontSmall;
    Font *fontMedium;
    Font *fontLarge;
    short textPri;
};

// Sets up a kanji glyph cache: the pixel buffer at +0x1824, 256 glyph nodes at +0x24,
// its texture and CLUT in VRAM, and the glyph loader at +0x9824.
void KanjiFont_Init(void *font, int vramX, int vramY, int texWidth, int texHeight, int clutX, int clutY)
{
    KanjiFont_SetBuffers(font, (char *)font + 0x1824, (char *)font + 0x24, 0);
    KanjiFont_SetVram(font, vramX, vramY, texWidth, texHeight);
    *(short *)((char *)font + 0xe) = (clutX >> 4) + clutY * 64;
    KanjiFont_Reset(font);
    *(KanjiLoadFn *)((char *)font + 0x9824) = KanjiFont_RenderGlyph;
}

void KanjiFont_Upload(void *font)
{
    KanjiFont_UploadClut(font);
    KanjiFont_UploadTexture(font);
}

// Finds (or adds) the glyph of each SJIS character and has the font's loader draw it.
void KanjiFont_CacheString(void *font, char *sjis)
{
    unsigned char *p = (unsigned char *)sjis;

    for (; *p != 0; p += 2) {
        (*(KanjiLoadFn *)((char *)font + 0x9824))(font, KanjiFont_FindGlyph(font, p[0] << 8 | p[1]));
    }
}

void CardMenu_CacheKanjiSlotNames(void)
{
    void *font = g_CardMenu->kanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_Reset(font);
        KanjiFont_CacheString(font, TEXT(MemoryCard1));
        KanjiFont_CacheString(font, TEXT(MemoryCard2));
    }
}

void CardMenu_CacheKanji(char *sjis)
{
    void *font = g_CardMenu->kanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_CacheString(font, sjis);
    }
}

// Draws SJIS text from the kanji cache, 12 pixels a character (centred on x if center is
// set), in Japanese only.
void CardMenu_DrawKanji(uint *ot, char *sjis, int x, int y, int center, uint rgb)
{
    void *font = g_CardMenu->kanjiFont;
    unsigned char *p;
    uint semi;
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
        void *glyph = KanjiFont_FindGlyph(font, p[0] << 8 | p[1]);

        if (glyph != 0) {
            PolyFT4 *q = KanjiFont_AddGlyphQuad(glyph, ot, rgb | semi);

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

void CardMenu_CacheKanjiLines(void)
{
    CardMenu *pCVar1;

    pCVar1 = g_CardMenu;
    if (g_GameState.language == '\0') {
        CardMenu_CacheKanjiSlotNames();
        CardMenu_CacheKanji(pCVar1->line1);
        CardMenu_CacheKanji(pCVar1->line2);
    }
}

void CardMenu_CacheKanjiError(void)
{
    CardMenu *pCVar1;

    pCVar1 = g_CardMenu;
    if (g_GameState.language == '\0') {
        CardMenu_CacheKanjiSlotNames();
        CardMenu_CacheKanji(pCVar1->errorMsg);
    }
}

// The replay list's hints, in Japanese only.
void CardMenu_CacheKanjiListHints(void)
{
    if (g_GameState.language == 0) {
        CardMenu_CacheKanjiSlotNames();
        CardMenu_CacheKanji(TEXT(RenameTitle));
        CardMenu_CacheKanji(TEXT(SaveChanges));
        CardMenu_CacheKanji(TEXT(CreateNewFile));
        CardMenu_CacheKanji(TEXT(OverwriteFile_2));
        CardMenu_CacheKanji(TEXT(DeleteFile));
        CardMenu_CacheKanji(TEXT(DataIsTooLarge));
        CardMenu_CacheKanji(TEXT(Max32Files));
        CardMenu_CacheKanji(TEXT(Play));
        CardMenu_CacheKanji(TEXT(CannotPlayGhost));
    }
}

// The replay list's callback: draws a row (4), and says whether a row can be chosen (8).
// Not matching: GCC swaps the registers of m and list (the game has m in s0, list in s1).
#ifdef NON_MATCHING
int CardMenu_EntryListCallback(int event, MenuListEvent *ev, int row)
{
    CardMenu *m = g_CardMenu;
    int result = 0;
    ReplayDirEntry *entry = 0;
    uint rgb = 0x5a4a3e;
    uint *ot = (uint *)ev->ot + 1;
    MenuList *list = ev->list;
    int x = ev->x;
    int y = ev->y;
    int brightness = ev->brightness;
    TextCtx ctx;
    TextCtx *t;

    Text_Init(&ctx, m->textPri);
    t = &ctx;
    t->unk1c = 1;
    switch (event) {
    case 3:
        break;
    case 4: {
        int alpha;
        int slide;
        int empty;

        int open = list->state >= 0;
        int closing = list->state < -1;

        if (!(open | closing)) {
            break;
        }
        alpha = ((list->fade << 7) / list->fadeMax * brightness) >> 7;
        if (alpha == 0) {
            break;
        }
        if ((signed char)m->deleteMode != 0) {
            rgb = 0x324052;
        }
        D_800921BC[4] = rgb;
        slide = 128 - (list->fade << 7) / list->fadeMax;
        if (slide < 0) {
            slide = 0;
        }
        slide = slide * 176 >> 7;
        empty = (signed char)m->rows[row][1];
        if (empty == 0) {
            entry = DIR_ENTRY(m, (signed char)m->rows[row][0]);
        }
        ReplayEntry_Draw(D_800921BC, ot, t, entry, x + slide, y, alpha, empty);
        break;
    }
    case 6:
        break;
    case 8:
        if ((signed char)m->deleteMode != 0) {
            result = (signed char)m->rows[row][1] == 0;
        } else {
            result = (signed char)m->rows[row][2] == 1;
        }
        break;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_EntryListCallback);
#endif

// State 9 of a new replay card: picks the replay file's size in blocks (3 to the free
// blocks), then sets up the directory (10, after 12 frames); "No space" (9) under 3.
int CardState_PickReplayFileSize(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->blockPickResult;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        int free = g_Card.ports[m->slot].freeBlocks;
        NumberPicker *p;

        if (free < 3) {
            next = 9;
            break;
        }
        p = &g_CardMenu_BlockPicker;
        p->min = 3;
        p->max = free;
        p->value = 3;
        p->anim = 0;
        m->line1 = TEXT(SelectNumberOfBlocks);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            g_CardMenu_BlockPicker.anim = -1;
            next = 3;
            break;
        }
        switch (r) {
        case -3:
        case -2:
            break;
        case -1:
            g_CardMenu_BlockPicker.anim = -1;
            Sfx_PlayMenu(2);
            next = 2;
            break;
        default:
            g_CardMenu_BlockPicker.anim = -1;
            ReplayDir_Init(&m->dir, r);
            next = 10;
            m->delay = 12;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 8: reads the replay directory, with the progress bar; then goes on by mode, or
// "No replay data" (20) when it is empty.
int CardState_ReadReplayDir(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        if (ReplayFile_StartReadDir(&m->dir, m->slot) != 0) {
            m->errorMsg = TEXT(LoadingDataFailed);
            CardMenu_CacheKanjiError();
            next = 1;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = 0x150c;
            bar->color = 0x90500c;
            CardProgress_Reset(bar);
            m->line1 = TEXT(CheckingReplayFile);
            m->line2 = EMPTY;
            CardMenu_CacheKanjiLines();
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0:
            m->progress.visible = -1;
            switch ((short)m->mode) {
            case CARDMODE_SAVE_REPLAY:
                m->delay = 1;
                next = 10;
                break;
            case CARDMODE_EDIT_REPLAY:
                next = 23;
                m->delay = 1;
                m->editEntry = 0;
                if ((signed char)m->dir.count == 0) {
                    m->line1 = TEXT(NoReplayDataInFile);
                    m->line2 = EMPTY;
                    CardMenu_CacheKanjiLines();
                    next = 24;
                }
                break;
            case CARDMODE_PLAY_REPLAY:
                next = 18;
                if ((signed char)m->dir.count == 0) {
                    m->line1 = TEXT(NoReplayDataInFile);
                    m->line2 = EMPTY;
                    CardMenu_CacheKanjiLines();
                    next = 20;
                }
                break;
            case CARDMODE_LOAD_GHOST:
                next = 26;
                if ((signed char)m->dir.count == 0) {
                    m->line1 = TEXT(NoReplayDataInFile);
                    m->line2 = EMPTY;
                    CardMenu_CacheKanjiLines();
                    next = 20;
                }
                break;
            }
            if (!ReplayDir_IsOk(&m->dir)) {
                m->errorMsg = TEXT(LoadingFailed_2);
                CardMenu_CacheKanjiError();
                next = 1;
            }
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = TEXT(LoadingDataFailed);
            CardMenu_CacheKanjiError();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress);
        break;
    }
    return next;
}

// State 21: "Changes are not saved"; leaves the list without saving (10, after a
// frame), or back to the slot choice (2).
int CardState_DiscardChanges(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgDiscard.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgDiscard;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = TEXT(ChangesAreNotSaved);
        m->line2 = TEXT(Cancel_2);
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            CardDialog_Close(&m->dlgDiscard);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
        case 1:
            Sfx_PlayMenu(1);
            m->delay = 1;
            next = 10;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// m->rows[i], added up the game's way: the row's offset first, then m. A row is the
// directory entry (-1 for the last row), what it is (0 an entry, 1 a new file, 2 save the
// changes) and, when saving, whether the replay fits and why not.
#define ROW(m, i) ((signed char *)((i) * 4 + (char *)(m) + (int)((CardMenu *)0)->rows))

// Through an inline function, as the game tests the result with xori and beqz.
static inline int Row_IsOk(CardMenu *m, int row)
{
    return ROW(m, row)[2] == 1;
}

// States 10 and 23: the replay files, to save the new replay to (a new file or over one)
// or to edit. A chosen file goes on to its title (11), "Save changes" to writing them (25);
// in delete mode the file is deleted instead (10 again, after a frame). Leaving goes back to
// the slot choice (2), or to "Changes are not saved" (21) when editing.
// Not matching: as in CardState_PlayList, the game indexes the rows with i on each pass
// (and stores each field of a row through its own copy of the row's address); GCC here
// strength-reduces both loops, and shares a row's address between blocks in the drawing.
#ifdef NON_MATCHING
int CardState_ReplayEntryList(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    ReplayDir *dir = &m->dir;
    int r = m->listResult;
    int count = (signed char)dir->count;
    int next = -1;
    int i;

    switch (event) {
    case 0: {
        MenuList *list;
        short *usage;

        m->line1 = EMPTY;
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        CardMenu_CacheKanjiListHints();
        m->usedSectors = dir->totalSectors;
        usage = &m->usedSectors;
        usage[1] = SectorMap_CountUsed(&m->dir.sectorMap);
        switch ((short)m->mode) {
        case CARDMODE_SAVE_REPLAY: {
            int fits;

            list = &g_CardMenu_EntryList;
            list->count = count + 1;
            list->selected = count;
            for (i = 0; i < count; i++) {
                ROW(m, i)[0] = i;
                ROW(m, i)[1] = 0;
                fits = ReplayDir_CheckFits(&m->dir, i, m->replaySize);
                ROW(m, i)[2] = fits == 0;
                ROW(m, i)[3] = fits;
            }
            ROW(m, i)[0] = -1;
            ROW(m, i)[1] = 1;
            fits = ReplayDir_CheckFits(&m->dir, -1, m->replaySize);
            ROW(m, i)[2] = fits == 0;
            ROW(m, i)[3] = fits;
            break;
        }
        case CARDMODE_EDIT_REPLAY:
            list = &g_CardMenu_EntryList;
            list->count = count + 1;
            list->selected = m->editEntry;
            for (i = 0; i < count; i++) {
                ROW(m, i)[0] = i;
                ROW(m, i)[1] = 0;
                ROW(m, i)[2] = 1;
            }
            ROW(m, i)[0] = -1;
            ROW(m, i)[1] = 2;
            ROW(m, i)[2] = 1;
            break;
        }
        break;
    }
    case 1: {
        short *status;
        int sel;

        if (m->delay > 0 && --m->delay == 0) {
            Sfx_PlayMenu(7);
            MenuList_Open(&g_CardMenu_EntryList);
        }
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            MenuList_Close(&g_CardMenu_EntryList);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
            MenuList_Close(&g_CardMenu_EntryList);
            Sfx_PlayMenu(2);
            switch ((short)m->mode) {
            case CARDMODE_SAVE_REPLAY:
                next = 2;
                break;
            case CARDMODE_EDIT_REPLAY:
                next = 0x15;
                break;
            }
            break;
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -4:
            Sfx_PlayMenu(0);
            break;
        case -2:
            break;
        default:
            Sfx_PlayMenu(1);
            sel = -1;
            switch ((short)m->mode) {
            case CARDMODE_SAVE_REPLAY:
                if (ROW(m, r)[1] == 0) {
                    sel = r;
                }
                break;
            case CARDMODE_EDIT_REPLAY: {
                int kind;

                m->editEntry = r;
                kind = ROW(m, r)[1];
                if (kind == 0) {
                    sel = r;
                } else if (kind == 2) {
                    MenuList_Close(&g_CardMenu_EntryList);
                    next = 0x19;
                    goto done;
                }
                break;
            }
            }
            MenuList_Close(&g_CardMenu_EntryList);
            if ((signed char)m->deleteMode != 0) {
                ReplayDir_DeleteEntry(&m->dir, sel);
                m->delay = 1;
                next = 10;
            } else {
                next = 11;
                m->selEntry = sel;
                m->delay = 12;
            }
            break;
        }
        break;
    }
    case 2: {
        char buf[256];
        TextCtx ctx;
        TextCtx *t;
        TextCtx *p;
        char *text;
        uint rgb;
        int row;
        char *hint;

        Text_Init(&ctx, m->textPri);
        t = &ctx;
        ctx.ot = (uint *)arg;
        *(uint *)&ctx.clut = (*(uint *)&ctx.clut & 0xff9fffff) | 0x200000;
        t->unk1c = 1;
        text = EMPTY;
        rgb = 0x606060;
        row = g_CardMenu_EntryList.selected;
        switch ((short)m->mode) {
        case CARDMODE_SAVE_REPLAY:
            switch (ROW(m, row)[1]) {
            case 0:
                text = TEXT(OverwriteFile_2);
                if (!Row_IsOk(m, row)) {
                    text = TEXT(DataIsTooLarge);
                    rgb = 0xb2460;
                }
                if ((signed char)m->deleteMode != 0) {
                    text = TEXT(DeleteFile);
                }
                break;
            case 1:
                text = TEXT(CreateNewFile);
                if (!Row_IsOk(m, row)) {
                    rgb = 0xb2460;
                    if (ROW(m, row)[3] == 1) {
                        text = TEXT(Max32Files);
                    } else {
                        text = TEXT(DataIsTooLarge);
                    }
                }
                if ((signed char)m->deleteMode != 0) {
                    text = EMPTY;
                }
                break;
            }
            break;
        case CARDMODE_EDIT_REPLAY:
            switch (ROW(m, row)[1]) {
            case 0:
                text = TEXT(RenameTitle);
                if ((signed char)m->deleteMode != 0) {
                    text = TEXT(DeleteFile);
                }
                break;
            case 2:
                text = TEXT(SaveChanges);
                if ((signed char)m->deleteMode != 0) {
                    text = EMPTY;
                }
                break;
            }
            break;
        }
        CardMenu_DrawKanji((uint *)arg, text, 0x18, 0x19a, 0, rgb);
        p = &ctx;
        Text_SetFont(p, &g_CardMenu_FontMedium);
        p->rgb = 0x20c3060;
        hint = TEXT(PressL1R1ForDeleteMode);
        Text_DrawString(p, hint, 0x148 - Text_MeasureString(p, hint, 0), 0x7c, 0);
        switch ((short)m->mode) {
        case CARDMODE_SAVE_REPLAY: {
            MenuList *list = &g_CardMenu_EntryList;
            int need = 0;
            ReplayDir *d;
            int w;
            int x;

            row = list->selected;
            if (ROW(m, row)[1] == 0) {
                need = m->dir.entries[ROW(m, row)[0]].sectorCount;
            }
            ReplayDir_DrawUsageBar(&m->usedSectors, (uint *)arg, 0xb0, 0x1c2, (list->fade << 7) / list->fadeMax,
                                   need, (uint)(m->replaySize + 0x7f) >> 7);
            Text_SetFont(p, &g_CardMenu_FontSmall);
            p->rgb = 0x24a4136;
            d = &m->dir;
            sprintf(buf, TEXT(TotalNReplay), (signed char)d->count);
            Text_DrawString(p, buf, 0x148 - Text_MeasureString(p, buf, 1), 0x1a4, 1);
            sprintf(buf, TEXT(NNSectorFree), SectorMap_CountFree(&m->dir.sectorMap), d->totalSectors);
            w = Text_MeasureString(p, buf, 1);
            Text_DrawString(p, buf, 0x148 - w, 0x1b4, 1);
            sprintf(buf, TEXT(NeedNSector), (uint)(m->replaySize + 0x7f) >> 7);
            x = 0x148 - Text_MeasureString(p, buf, 1);
            Text_DrawString(p, buf, x - w - 8, 0x1b4, 1);
            break;
        }
        case CARDMODE_EDIT_REPLAY: {
            MenuList *list = &g_CardMenu_EntryList;
            int need = 0;
            ReplayDir *d;

            row = list->selected;
            if (ROW(m, row)[1] == 0) {
                need = m->dir.entries[ROW(m, row)[0]].sectorCount;
            }
            ReplayDir_DrawUsageBar(&m->usedSectors, (uint *)arg, 0xb0, 0x1c2, (list->fade << 7) / list->fadeMax,
                                   need, need);
            Text_SetFont(p, &g_CardMenu_FontSmall);
            p->rgb = 0x24a4136;
            d = &m->dir;
            sprintf(buf, TEXT(TotalNReplay), (signed char)d->count);
            Text_DrawString(p, buf, 0x148 - Text_MeasureString(p, buf, 1), 0x1a4, 1);
            sprintf(buf, TEXT(NNSectorFree), SectorMap_CountFree(&m->dir.sectorMap), d->totalSectors);
            Text_DrawString(p, buf, 0x148 - Text_MeasureString(p, buf, 1), 0x1b4, 1);
            break;
        }
        }
        break;
    }
    }
done:
    return next;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_ReplayEntryList);
#endif

// State 11: edits the replay's title (after `delay` frames); saving a new replay adds its
// entry and creates the file (12) or writes it (13), editing renames the entry. Both then
// leave through state 10.
int CardState_EditTitle(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int sel = m->selEntry;
    int next = -1;

    switch (event) {
    case 0:
        break;
    case 1: {
        short *status;

        if (m->delay > 0 && --m->delay == 0) {
            TextEntry *entry;

            m->titleBuf[0] = 0;
            if (sel >= 0) {
                strcpy(m->titleBuf, DIR_ENTRY(m, sel)->title);
            }
            entry = &m->titleEntry;
            TextEntry_Open(entry);
            entry->cursorPos = strlen(entry->text);
        }
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            TextEntry_Close(&m->titleEntry);
            next = 3;
            goto done;
        }
        switch (m->titleResult) {
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            TextEntry_Close(&m->titleEntry);
            m->delay = 12;
            next = 10;
            break;
        case 0:
            Sfx_PlayMenu(1);
            TextEntry_Close(&m->titleEntry);
            switch ((short)m->mode) {
            case CARDMODE_EDIT_REPLAY:
                ReplayEntry_SetTitle(DIR_ENTRY(m, sel)->title, m->titleBuf);
                m->delay = 12;
                next = 10;
                break;
            case CARDMODE_SAVE_REPLAY: {
                ReplayDirEntry *e = &m->newEntry;
                ReplayDir *dir;

                ReplayEntry_SetTitle(e->title, m->titleBuf);
                dir = &m->dir;
                if (ReplayDir_AddEntry(dir, sel, e, m->replayData, m->replaySize, m->newSectors) == 0) {
                    goto failed;
                }
                if (!ReplayDir_IsOk(dir)) {
                failed:
                    m->errorMsg = TEXT(LoadingFailed_2);
                    CardMenu_CacheKanjiError();
                    next = 1;
                    goto done;
                }
                next = 13;
                if (ReplayFile_Find(dir, m->slot) == 0) {
                    next = 12;
                }
                break;
            }
            }
            break;
        }
    }
        // fall through
    case 2: {
        TextCtx ctx;
        TextCtx *t;
        char *title;

        Text_Init(&ctx, m->textPri);
        t = &ctx;
        *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
        Text_SetFont(t, &g_CardMenu_FontMedium);
        title = TEXT(EditReplayDataFile);
        t->unk1c = 1;
        t->ot = (uint *)arg;
        t->rgb = 0x20c3060;
        Text_DrawString(t, title, 0x148 - Text_MeasureString(t, title, 0), 0x7c, 0);
        break;
    }
    }
done:
    return next;
}

// State 12: creates the replay file; then writes its data (13).
int CardState_CreateReplayFile(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        if (ReplayFile_Create(&m->dir, m->slot) != 0) {
            m->errorMsg = TEXT(FailedToCreateReplayFile);
            CardMenu_CacheKanjiError();
            next = 1;
        } else {
            m->line1 = TEXT(Saving);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            break;
        case 0:
            next = 13;
            break;
        default:
            m->errorMsg = TEXT(FailedToCreateReplayFile);
            CardMenu_CacheKanjiError();
            next = 1;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 13: writes the new replay's data (or, when editing, only the directory), with the
// progress bar; then the directory (14).
int CardState_WriteReplayData(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        switch ((short)m->mode) {
        case CARDMODE_SAVE_REPLAY: {
            CardProgressBar *bar;

            if (ReplayFile_StartWriteData(&m->dir, m->slot, m->replayData, m->replaySize, m->newSectors) != 0) {
                m->errorMsg = TEXT(SavingDataFailed);
                CardMenu_CacheKanjiError();
                next = 15;
            }
            bar = &m->progress;
            bar->total = m->replaySize + 0x150c;
            bar->color = 0xc5090;
            CardProgress_Reset(bar);
            m->line1 = TEXT(Saving);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
            break;
        }
        case CARDMODE_EDIT_REPLAY: {
            CardProgressBar *bar;

            ReplayDir_UpdateCrc(&m->dir);
            bar = &m->progress;
            bar->total = 0x150c;
            bar->color = 0xc5090;
            CardProgress_Reset(bar);
            m->line1 = TEXT(Saving);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
            next = 14;
            break;
        }
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining + 0x150c);
            break;
        case 0:
            next = 14;
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = TEXT(SavingDataFailed);
            CardMenu_CacheKanjiError();
            next = 15;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress);
        break;
    }
    return next;
}

// State 14: writes the replay directory, with the progress bar; then "Saving complete" (16).
int CardState_WriteReplayDir(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        if (ReplayFile_StartWriteDir(&m->dir, m->slot) != 0) {
            m->errorMsg = TEXT(SavingDataFailed);
            CardMenu_CacheKanjiError();
            next = 15;
        } else {
            m->line1 = TEXT(Saving);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0:
            m->progress.visible = -1;
            next = 16;
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = TEXT(SavingDataFailed);
            CardMenu_CacheKanjiError();
            next = 15;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress);
        break;
    }
    return next;
}

// State 15: "Saving data failed. Try saving again?", while the card stays in the slot;
// retries the write (13), or back to the slot choice (2). A changed slot goes to 4, 5 or
// the read error.
int CardState_SaveFailed(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgSaveFailed.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg;
        short *status = m->slotStatus + m->slot;

        if (*status == 2) {
            return 33;
        }
        Sfx_PlayMenu(0);
        dlg = &m->dlgSaveFailed;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        m->line1 = TEXT(SavingDataFailed);
        m->line2 = TEXT(TrySavingAgain);
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1: {
        short *status = m->slotStatus + m->slot;
        int st = *status;

        switch (st) {
        case 2:
            Sfx_PlayMenu(0);
            CardDialog_Close(&m->dlgSaveFailed);
            next = 4;
            break;
        case 3:
            Sfx_PlayMenu(0);
            CardDialog_Close(&m->dlgSaveFailed);
            next = 5;
            break;
        case 4:
            Sfx_PlayMenu(0);
            CardDialog_Close(&m->dlgSaveFailed);
            m->errorMsg = TEXT(FailedToReadMemoryCard);
            CardMenu_CacheKanjiError();
            next = 1;
            break;
        case 0:
            switch (r) {
            case -1:
                Sfx_PlayMenu(0);
                break;
            case 1:
                CardDialog_Close(&m->dlgSaveFailed);
                Sfx_PlayMenu(2);
                next = 2;
                break;
            case 0:
                CardDialog_Close(&m->dlgSaveFailed);
                Sfx_PlayMenu(1);
                next = 13;
                break;
            }
            break;
        }
        break;
    }
    case 2:
        break;
    }
    return next;
}

// State 16: "Saving complete"; back to the slot choice (2), or leaves.
int CardState_SaveComplete(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgComplete.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgComplete;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = TEXT(SavingComplete);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = CARD_EXIT;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 18: the replays to play (ghosts can't be); loads the chosen one (19).
// Not matching: the game doesn't strength-reduce the rows loop (it indexes rows with i
// on each pass, through four copies of the row address); GCC here keeps a row pointer
// in its own register.
#ifdef NON_MATCHING
int CardState_PlayList(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->listResult;
    int count = (signed char)m->dir.count;
    int next = -1;
    short *status;
    int i;

    switch (event) {
    case 0: {
        MenuList *list;

        m->line1 = EMPTY;
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        CardMenu_CacheKanjiListHints();
        list = &g_CardMenu_EntryList;
        list->count = count;
        list->selected = 0;
        MenuList_Open(list);
        Sfx_PlayMenu(7);
        for (i = 0; i < count; i++) {
            unsigned char ok = ReplayDir_IsPlayable(&m->dir, i);

            m->rows[i][0] = i;
            m->rows[i][1] = 0;
            m->rows[i][2] = ok;
            m->rows[i][3] = ok;
        }
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            MenuList_Close(&g_CardMenu_EntryList);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
            MenuList_Close(&g_CardMenu_EntryList);
            Sfx_PlayMenu(2);
            next = 2;
            break;
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -4:
            Sfx_PlayMenu(0);
            break;
        case -2:
            break;
        default:
            Sfx_PlayMenu(1);
            m->selEntry = r;
            MenuList_Close(&g_CardMenu_EntryList);
            next = 19;
            break;
        }
        break;
    case 2: {
        char *text = TEXT(Play);

        if (m->rows[g_CardMenu_EntryList.selected][3] == 0) {
            text = TEXT(CannotPlayGhost);
        }
        CardMenu_DrawKanji((uint *)arg, text, 0xb0, 0x1a4, 1, 0x808080);
        break;
    }
    }
    return next;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_PlayList);
#endif

// State 20: reads the selected replay, with the progress bar, and unpacks it if its CRC is
// right; then leaves with CARD_REPLAY_LOADED.
int CardState_LoadReplay(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int sel = m->selEntry;
    int next = -1;

    switch (event) {
    case 0: {
        ReplayDir *dir = &m->dir;

        if (ReplayFile_StartReadEntry(dir, m->slot, sel, m->replayData, m->newSectors) != 0) {
            m->errorMsg = TEXT(LoadingDataFailed);
            CardMenu_CacheKanjiError();
            next = 1;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = ((dir->entries + sel)->size + 0x7f) & ~0x7f;
            bar->color = 0x50782c;
            CardProgress_Reset(bar);
            m->line1 = TEXT(Loading);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
        }
        break;
    }
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0: {
            WipeBar *bar = &m->slotBar;

            m->progress.visible = -1;
            bar->t = ~bar->duration;
            if (ReplayDir_CheckEntryCrc(&m->dir, sel, m->replayData) != 0) {
                Replay_Unpack(m->replayData, 0);
                next = CARD_REPLAY_LOADED;
            } else {
                m->errorMsg = TEXT(LoadingFailed_2);
                CardMenu_CacheKanjiError();
                next = 1;
            }
            break;
        }
        default:
            m->progress.visible = -1;
            m->errorMsg = TEXT(LoadingDataFailed);
            CardMenu_CacheKanjiError();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress);
        break;
    }
    return next;
}

// State 26: the ghosts recorded on the current race's course; loads the chosen one (27),
// or "No matching data found" (20).
// Not matching: as in CardState_PlayList, GCC strength-reduces the rows loop.
#ifdef NON_MATCHING
int CardState_GhostList(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    uint course = setup->courseHash;
    ReplayDir *dir = &m->dir;
    int r = m->listResult;
    int count = (signed char)dir->count;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        int n;
        int i;

        m->line1 = EMPTY;
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        n = 0;
        for (i = 0; i < count; i++) {
            ReplayDirEntry *e = &dir->entries[i];

            if ((e->kind == 1) & (e->courseId == course)) {
                m->rows[n][0] = i;
                m->rows[n][1] = 0;
                m->rows[n][2] = 1;
                n++;
            }
        }
        if (n > 0) {
            MenuList *list = &g_CardMenu_EntryList;

            list->count = n;
            list->selected = 0;
            MenuList_Open(list);
            Sfx_PlayMenu(7);
            next = -1;
        } else {
            m->line1 = TEXT(NoMatchingDataFound);
            m->line2 = EMPTY;
            CardMenu_CacheKanjiLines();
            next = 20;
        }
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            MenuList_Close(&g_CardMenu_EntryList);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
            MenuList_Close(&g_CardMenu_EntryList);
            Sfx_PlayMenu(2);
            next = 2;
            break;
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -4:
            Sfx_PlayMenu(0);
            break;
        case -2:
            break;
        default:
            Sfx_PlayMenu(1);
            m->selEntry = (signed char)m->rows[r][0];
            next = 27;
            MenuList_Close(&g_CardMenu_EntryList);
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_GhostList);
#endif

// State 27: reads the selected ghost, with the progress bar, and unpacks it if its CRC is
// right; then 28.
int CardState_LoadGhost(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int sel = m->selEntry;
    int next = -1;

    switch (event) {
    case 0: {
        ReplayDir *dir = &m->dir;

        if (ReplayFile_StartReadEntry(dir, m->slot, sel, m->replayData, m->newSectors) != 0) {
            m->errorMsg = TEXT(LoadingDataFailed);
            CardMenu_CacheKanjiError();
            next = 15;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = ((dir->entries + sel)->size + 0x7f) & ~0x7f;
            bar->color = 0x50782c;
            CardProgress_Reset(bar);
            m->line1 = TEXT(Loading);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
        }
        break;
    }
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0: {
            WipeBar *bar = &m->slotBar;

            m->progress.visible = -1;
            bar->t = ~bar->duration;
            if (ReplayDir_CheckEntryCrc(&m->dir, sel, m->replayData) != 0) {
                GhostReplay_Unpack(m->replayData);
                g_CardMenu_GhostLoaded = 1;
                next = 28;
            } else {
                m->errorMsg = TEXT(LoadingFailed_2);
                CardMenu_CacheKanjiError();
                next = 1;
            }
            break;
        }
        default:
            m->progress.visible = -1;
            m->errorMsg = TEXT(LoadingDataFailed);
            CardMenu_CacheKanjiError();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress);
        break;
    }
    return next;
}

// State 29: "Start saving?"; creates the game file (31), or back to the slot choice (2).
int CardState_ConfirmSave(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgConfirmSave.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgConfirmSave;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = TEXT(StartSaving);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            CardDialog_Close(&m->dlgConfirmSave);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
            Sfx_PlayMenu(2);
            next = 2;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 31;
            break;
        case 1:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 31: creates the game file; then writes the save (30), or reports the failure (33).
int CardState_CreateGameFile(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        if (SaveFile_Create(m->saveFile, m->slot) != 0) {
            m->errorMsg = TEXT(FailedToCreateGameFile);
            CardMenu_CacheKanjiError();
            next = 33;
        } else {
            m->line1 = TEXT(Saving);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            break;
        case 0:
            next = 30;
            break;
        default:
            m->errorMsg = TEXT(FailedToCreateGameFile);
            CardMenu_CacheKanjiError();
            next = 33;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 32: "A game file already exists. Overwrite it?"; writes the save (30), or back to
// the slot choice (2).
int CardState_ConfirmOverwrite(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgOverwrite.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgOverwrite;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = TEXT(GameFileAlreadyExists);
        m->line2 = TEXT(OverwriteFile);
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            CardDialog_Close(&m->dlgOverwrite);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
            Sfx_PlayMenu(2);
            next = 2;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 30;
            break;
        case 1:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 30: writes the game save, with the progress bar; then "Saving complete" (16).
int CardState_WriteGameSave(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        if (SaveFile_StartWrite(m->saveFile, m->slot, m->saveFile) != 0) {
            m->errorMsg = TEXT(SavingDataFailed);
            CardMenu_CacheKanjiError();
            next = 33;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = SaveFile_GetFileSize(m->saveFile);
            bar->color = 0xc5090;
            CardProgress_Reset(bar);
            m->line1 = TEXT(Saving);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0:
            m->progress.visible = -1;
            next = 16;
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = TEXT(SavingDataFailed);
            CardMenu_CacheKanjiError();
            next = 33;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress);
        break;
    }
    return next;
}

// State 34: "Saving data failed"; back to the slot choice (2), or leaves.
int CardState_SaveError(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgSaveError.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg;

        Sfx_PlayMenu(0);
        dlg = &m->dlgSaveError;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        m->line1 = TEXT(SavingDataFailed);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        switch (r) {
        case -1:
            Sfx_PlayMenu(0);
            break;
        case 1:
            CardDialog_Close(&m->dlgSaveError);
            Sfx_PlayMenu(1);
            next = CARD_EXIT;
            break;
        case 0:
            CardDialog_Close(&m->dlgSaveError);
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 37: "Start loading?"; reads the save (35), or back to the slot choice (2).
int CardState_ConfirmLoad(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgConfirmLoad.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgConfirmLoad;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = TEXT(StartLoading);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            CardDialog_Close(&m->dlgConfirmLoad);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
            Sfx_PlayMenu(2);
            next = 2;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 35;
            break;
        case 1:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 35: reads the game save, with the progress bar, and loads it if its CRC is right;
// then "Loading complete" (37).
int CardState_ReadGameSave(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        if (SaveFile_StartRead(m->saveFile, m->slot, m->saveFile) != 0) {
            m->errorMsg = TEXT(LoadingDataFailed);
            CardMenu_CacheKanjiError();
            next = 1;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = SaveFile_GetFileSize(m->saveFile);
            bar->color = 0x90500c;
            CardProgress_Reset(bar);
            m->line1 = TEXT(Loading);
            m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
            CardMenu_CacheKanjiLines();
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0:
            m->progress.visible = -1;
            if (SaveFile_CheckCrc(m->saveFile, m->saveFile) != 0) {
                next = 37;
                GameState_ReadSave(m->saveFile, m->saveFile);
            } else {
                m->errorMsg = TEXT(LoadingFailed);
                CardMenu_CacheKanjiError();
                next = 1;
            }
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = TEXT(LoadingDataFailed);
            CardMenu_CacheKanjiError();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress);
        break;
    }
    return next;
}

// State 38: "Loading complete"; back to the slot choice (2), or leaves.
int CardState_LoadComplete(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgComplete.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgComplete;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = TEXT(LoadingComplete);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = CARD_EXIT;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// Waits 24 frames, then goes to state 2.
int CardState_Start(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        m->timer = 24;
        break;
    case 1:
        if (--m->timer < 0) {
            next = 2;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 1: the error dialog, with errorMsg drawn above it; back to the slot choice (2), or
// leaves.
int CardState_Error(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    uint *ot = (uint *)arg;
    int r = m->dlgError.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg;

        Sfx_PlayMenu(0);
        dlg = &m->dlgError;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        m->line1 = EMPTY;
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = CARD_EXIT;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2: {
        TextCtx ctx;

        Text_Init(&ctx, m->textPri);
        Text_SetFont(&ctx, &g_CardMenu_FontLarge);
        {
            TextCtx *t = &ctx;

            t->unk1c = 1;
            t->ot = ot;
            t->rgb = 0x6f6f6f;
        }
        if (g_GameState.language == 0) {
            CardMenu_DrawKanji(ot, m->errorMsg, 0xb0, 0xea, 1, 0x808080);
        } else {
            Text_DrawCentered(&ctx, m->errorMsg, 0xb0, 0xfa, 1);
        }
        break;
    }
    }
    return next;
}

// State 2: "Select a slot"; goes on to check the slot (3), or leaves on cancel.
int CardState_SelectSlot(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgSelectSlot.result;
    int next = -1;

    switch (event) {
    case 0: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            bar->t = ~bar->duration;
        }
        CardDialog_Open(&m->dlgSelectSlot);
        m->line1 = TEXT(SelectASlot);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        switch (r) {
        case -1:
            next = CARD_EXIT;
            break;
        case 0:
        case 1:
            Sfx_PlayMenu(1);
            m->slot = r;
            next = 3;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 3: checks the card in the slot, then goes on by mode and slot status (0 a card,
// 1 checking, 2 no card, 3 not formatted, 4 read error).
int CardState_CheckSlot(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        m->line1 = TEXT(CheckingSlot);
        m->slotBar.t = 0;
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        // fall through
    case 1:
        switch ((short)m->mode) {
        case CARDMODE_SAVE_GAME: {
            short *status;
            int free;

            status = m->slotStatus + m->slot;
            switch (*status) {
            case 0:
                next = 29;
                if (SaveFile_Find(m->saveFile, m->slot) != 0) {
                    break;
                }
                free = g_Card.ports[m->slot].freeBlocks;
                next = 32;
                if (free < SaveFile_GetBlockCount(m->saveFile)) {
                    next = 9;
                }
                break;
            case 1:
                break;
            case 2:
                goto noCard;
            case 3:
                goto notFormatted;
            case 4:
                goto readError;
            }
            break;
        }
        case CARDMODE_LOAD_GAME: {
            short *status;
            int st;

            status = m->slotStatus + m->slot;
            switch (st = *status) {
            case 4:
                goto readError;
            case 0:
                next = 34;
                if (SaveFile_Find(m->saveFile, m->slot) == 0) {
                    next = 36;
                }
                break;
            case 2:
                goto noCard;
            case 1:
                break;
            default:
                next = 36;
                break;
            }
            break;
        }
        case CARDMODE_SAVE_REPLAY: {
            short *status;

            status = m->slotStatus + m->slot;
            switch (*status) {
            case 3:
            notFormatted:
                next = 5;
                break;
            case 0:
                if (ReplayFile_Find(&m->dir, m->slot) == 0) {
                    next = 8;
                } else {
                    next = 7;
                }
                break;
            case 1:
                break;
            case 2:
                goto noCard;
            case 4:
                goto readError;
            }
            break;
        }
        case CARDMODE_PLAY_REPLAY:
        case CARDMODE_LOAD_GHOST: {
            short *status;
            int st;

            status = m->slotStatus + m->slot;
            switch (st = *status) {
            case 4:
                goto readError;
            case 0:
                if (ReplayFile_Find(&m->dir, m->slot) == 0) {
                    next = 17;
                } else {
                    next = 7;
                }
                break;
            case 2:
                goto noCard;
            case 1:
                break;
            default:
                next = 17;
                break;
            }
            break;
        }
        case CARDMODE_EDIT_REPLAY: {
            short *status;
            int st;

            status = m->slotStatus + m->slot;
            switch (st = *status) {
            case 2:
            noCard:
                next = 4;
                break;
            case 4:
            readError:
                m->errorMsg = TEXT(FailedToReadMemoryCard);
                CardMenu_CacheKanjiError();
                next = 1;
                break;
            case 0:
                if (ReplayFile_Find(&m->dir, m->slot) == 0) {
                    next = 22;
                } else {
                    next = 7;
                }
                break;
            case 1:
                break;
            default:
                next = 22;
                break;
            }
            break;
        }
        }
        break;
    case 2:
        break;
    }
    return next;
}

// "No memory card detected", until the slot's status changes (3).
int CardState_NoCard(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgNoCard.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0:
        CardDialog_Open(&m->dlgNoCard);
        m->line1 = TEXT(NoMemoryCardDetected);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 2) {
            CardDialog_Close(&m->dlgNoCard);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
        case 1:
            next = CARD_EXIT;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 5: "The memory card is not formatted. Format it?"; formats (6), or back to the
// slot choice (2).
int CardState_NotFormatted(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgFormat.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgFormat;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = TEXT(MemoryCardIsNotFormatted);
        m->line2 = TEXT(DoYouWantToFormat);
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 3) {
            CardDialog_Close(&m->dlgFormat);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
        case 1:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 6;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 6: formats the card; then checks it again (3), or creates the game file (31)
// when saving a game.
int CardState_Format(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int next = -1;

    switch (event) {
    case 0:
        McCard_StartFormat(m->slot);
        m->line1 = TEXT(Formatting);
        m->line2 = TEXT(PleaseDoNotRemoveMemoryCard);
        CardMenu_CacheKanjiLines();
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            break;
        case 0:
            next = 3;
            if ((short)m->mode == CARDMODE_SAVE_GAME) {
                next = 31;
            }
            break;
        default:
            m->errorMsg = TEXT(FormattingFailed);
            CardMenu_CacheKanjiError();
            next = 1;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// "No replay files" (replay modes) or "Not enough empty blocks" (saves), until the slot's
// status changes (3).
int CardState_NoSpace(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgError.result;
    int next = -1;
    short *status;
    int st;

    switch (event) {
    case 0: {
        CardDialog *dlg;

        Sfx_PlayMenu(0);
        dlg = &m->dlgError;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        switch ((short)m->mode) {
        case CARDMODE_SAVE_REPLAY:
        case CARDMODE_SAVE_GAME:
            m->line1 = TEXT(NotEnoughEmptyBlocks);
            break;
        case CARDMODE_PLAY_REPLAY:
        case CARDMODE_EDIT_REPLAY:
        case CARDMODE_LOAD_GHOST:
            m->line1 = TEXT(NoReplayFilesFound);
            break;
        default:
            return next;
        }
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        st = *status;
        switch ((short)m->mode) {
        case CARDMODE_SAVE_REPLAY:
        case CARDMODE_SAVE_GAME:
            if (st == 0) {
                goto ask;
            }
            break;
        case CARDMODE_PLAY_REPLAY:
        case CARDMODE_EDIT_REPLAY:
        case CARDMODE_LOAD_GHOST:
            if (st == 0 || st == 3) {
                goto ask;
            }
            break;
        default:
            goto ask;
        }
        CardDialog_Close(&m->dlgError);
        next = 3;
        break;
    ask:
        switch (r) {
        case -1:
        case 1:
            next = CARD_EXIT;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// "No replay data": the error dialog, until a card shows up in the slot (3).
int CardState_NoReplayData(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgError.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg;

        Sfx_PlayMenu(0);
        dlg = &m->dlgError;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            CardDialog_Close(&m->dlgError);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
        case 1:
            next = CARD_EXIT;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// "No game file found", until the slot has no card or another card (3).
int CardState_NoGameFile(int event, void *arg)
{
    CardMenu *m = g_CardMenu;
    int r = m->dlgError.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg;

        Sfx_PlayMenu(0);
        dlg = &m->dlgError;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        m->line1 = TEXT(NoGameFileFound);
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status == 2 || *status == 4) {
            CardDialog_Close(&m->dlgError);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
        case 1:
            next = CARD_EXIT;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 2;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

int CardState_Idle(int event, void *arg)
{
    return -1;
}

// Enters state (Idle when negative) by calling its handler with event 0, and returns what
// it returns.
int CardMenu_SetState(int state)
{
    CardMenu *m = g_CardMenu;

    if (state < 0) {
        m->handler = (void *)CardState_Idle;
    } else {
        m->handler = (void *)g_CardMenu_States[state];
    }
    m->state = state;
    return ((CardStateFn)m->handler)(0, 0);
}

extern const char D_8008FB14[12];
const char D_8008FB14[12] = { 'N', 'o', ' ', 'N', 'a', 'm', 'e' };

// Builds the race's replay data and its directory entry.
void CardMenu_PrepareReplayEntry(void)
{
    CardMenu *m = g_CardMenu;
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    ReplayDirEntry *e;

    m->replaySize = ReplayData_BuildRace(m->replayData);
    e = &m->newEntry;
    ReplayEntry_SetTitle(e->title, (char *)D_8008FB14);
    e->unk40 = setup->game;
    e->raceMode = setup->raceType;
    e->kind = 0;
    e->courseId = setup->courseHash;
    *(uint *)e->unk48 = setup->cars[0].carId;
    ReplayEntry_SetCarName(e, setup->cars[0].name);
}

// Builds the ghost's replay data and its directory entry.
void CardMenu_PrepareGhostEntry(void)
{
    CardMenu *m = g_CardMenu;
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    ReplayDirEntry *e;

    m->replaySize = ReplayData_BuildGhost(m->replayData);
    e = &m->newEntry;
    ReplayEntry_SetTitle(e->title, (char *)D_8008FB14);
    e->unk40 = setup->game;
    e->raceMode = setup->raceType;
    e->kind = 1;
    e->courseId = setup->courseHash;
    *(uint *)e->unk48 = setup->cars[0].carId;
    ReplayEntry_SetCarName(e, setup->cars[0].name);
}

// Sets up the menu's widgets from their templates.
void CardMenu_InitWidgets(void)
{
    CardMenu *m = g_CardMenu;
    TextEntry *entry;

    m->slotBar = D_80091EE8;
    m->slotBar.t = -1;
    CardDialog_Init(&m->dlgError, &D_80091F04, 0);
    CardDialog_Init(&m->dlgSelectSlot, &D_80091F34, 0);
    CardDialog_Init(&m->dlgNoCard, &D_80091F64, 0);
    CardDialog_Init(&m->dlgFormat, &D_80091F94, 0);
    NumberPicker_Init(&g_CardMenu_BlockPicker);
    MenuList_Init(&g_CardMenu_EntryList, (void *)CardMenu_EntryListCallback, 0);
    CardDialog_Init(&m->dlgSaveFailed, &D_80092018, 0);
    CardDialog_Init(&m->dlgSaveError, &D_80092048, 0);
    CardDialog_Init(&m->dlgComplete, &D_80092078, 0);
    m->progress = D_80092168;
    CardDialog_Init(&m->dlgOverwrite, &D_800920A8, 0);
    CardDialog_Init(&m->dlgConfirmSave, &D_800920D8, 0);
    CardDialog_Init(&m->dlgConfirmLoad, &D_80092108, 0);
    CardDialog_Init(&m->dlgDiscard, &D_80092138, 0);
    entry = &m->titleEntry;
    D_800921A0.textPri = m->textPri;
    TextEntry_Init(entry, &D_800921A0);
    entry->text = m->titleBuf;
    entry->maxLen = 31;
    entry->maxWidth = 0x100;
    m->titleBuf[0] = 0;
    m->line1 = EMPTY;
    m->line2 = EMPTY;
    CardMenu_CacheKanjiLines();
}

// Sets up the card menu in config's work buffer; returns the end of the buffer.
CardMenu *CardMenu_Setup(CardMenuConfig *config)
{
    CardMenu *m = config->work;

    g_CardMenu_FontSmall = *config->fontSmall;
    g_CardMenu_FontMedium = *config->fontMedium;
    g_CardMenu_FontLarge = *config->fontLarge;
    m->kanjiFont = config->kanjiFont;
    m->slotStatus = config->slotStatus;
    g_CardMenu = m;
    m->textPri = config->textPri;
    return m + 1;
}

// Each frame: takes the pad, updates the widgets, then runs the state and the states it
// moves to. Returns 1 when the menu is left, 2 when a replay was loaded, 0 otherwise.
int CardMenu_Update(PadState *pad)
{
    CardMenu *m = g_CardMenu;
    int result = 0;
    int r;

    m->repeat = 0;
    m->pressed = 0;
    m->held = 0;
    m->deleteMode = 0;
    if (pad != 0) {
        PadButton held = pad->held;

        m->held = held;
        m->pressed = pad->pressed;
        m->repeat = pad->repeat;
        switch ((short)m->mode) {
        case CARDMODE_SAVE_REPLAY:
        case CARDMODE_EDIT_REPLAY:
            m->deleteMode = (held & (PAD_L1 | PAD_R1)) == (PAD_L1 | PAD_R1);
            break;
        case CARDMODE_PLAY_REPLAY:
        case CARDMODE_LOAD_GHOST:
        case CARDMODE_SAVE_GAME:
        case CARDMODE_LOAD_GAME:
            break;
        }
    }
    WipeBar_Update(&m->slotBar);
    m->dlgError.result = CardDialog_Update(&m->dlgError, pad);
    m->dlgSelectSlot.result = CardDialog_Update(&m->dlgSelectSlot, pad);
    m->dlgNoCard.result = CardDialog_Update(&m->dlgNoCard, pad);
    m->dlgFormat.result = CardDialog_Update(&m->dlgFormat, pad);
    m->blockPickResult = NumberPicker_Update(&g_CardMenu_BlockPicker, pad);
    m->listResult = MenuList_Update(&g_CardMenu_EntryList, pad);
    m->dlgSaveFailed.result = CardDialog_Update(&m->dlgSaveFailed, pad);
    m->dlgSaveError.result = CardDialog_Update(&m->dlgSaveError, pad);
    m->dlgComplete.result = CardDialog_Update(&m->dlgComplete, pad);
    m->titleResult = TextEntry_Update(&m->titleEntry, pad);
    m->dlgOverwrite.result = CardDialog_Update(&m->dlgOverwrite, pad);
    m->dlgConfirmSave.result = CardDialog_Update(&m->dlgConfirmSave, pad);
    m->dlgConfirmLoad.result = CardDialog_Update(&m->dlgConfirmLoad, pad);
    m->dlgDiscard.result = CardDialog_Update(&m->dlgDiscard, pad);
    r = ((CardStateFn)m->handler)(1, 0);
next:
    switch (r) {
    case CARD_EXIT: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            bar->t = ~bar->duration;
        }
        m->line1 = EMPTY;
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        CardMenu_SetState(-1);
        result = 1;
        break;
    }
    case CARD_REPLAY_LOADED: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            bar->t = ~bar->duration;
        }
        m->line1 = EMPTY;
        m->line2 = EMPTY;
        CardMenu_CacheKanjiLines();
        CardMenu_SetState(-1);
        result = 2;
        break;
    }
    case -1:
        break;
    default:
        r = CardMenu_SetState(r);
        goto next;
    }
    return result;
}

// Each frame: the two text lines, the slot bar with its slot's name, the widgets, then the
// state's own drawing (event 2).
void CardMenu_Draw(uint *ot)
{
    CardMenu *m = g_CardMenu;
    TextCtx ctx;
    uint from;
    uint to;
    int level;

    from = 0x2000000;
    to = 0x242362a;
    Text_Init(&ctx, m->textPri);
    Text_SetFont(&ctx, &g_CardMenu_FontLarge);
    {
        TextCtx *t = &ctx;
        int center;

        *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
        t->unk1c = 1;
        t->ot = ot;
        t->rgb = 0x25c5248;

        center = 1;
        if (g_GameState.language == 0) {
            CardMenu_DrawKanji(ot, m->line1, 0xb0, 0xc6, center, 0x808080);
            CardMenu_DrawKanji(ot, m->line2, 0xb0, 0xea, center, 0x808080);
        } else {
            Text_DrawCentered(t, m->line1, 0xb0, 0xd6, center);
            Text_DrawCentered(t, m->line2, 0xb0, 0xfa, center);
        }
    }
    level = WipeBar_GetLevel(&m->slotBar);
    if (level != 0) {
        if (g_GameState.language == 0) {
            CardMenu_DrawKanji(ot, D_800921B4[m->slot], 0x1c, 0x66, 0, level | level << 8 | level << 16);
        } else {
            TextCtx *t = &ctx;

            t->rgb = Rgb_Lerp((uchar *)&from, (uchar *)&to, level, 0x80);
            Text_DrawString(t, D_800921B4[m->slot], 0x1c, 0x7e, 1);
        }
        WipeBar_Draw(&m->slotBar, ot, 0x18, 0x6e);
        Prim_AddDrawMode(ot, 0x220);
    }
    CardDialog_Draw(&m->dlgError, ot, &ctx);
    CardDialog_Draw(&m->dlgSelectSlot, ot, &ctx);
    CardDialog_Draw(&m->dlgNoCard, ot, &ctx);
    CardDialog_Draw(&m->dlgFormat, ot, &ctx);
    NumberPicker_Draw(&g_CardMenu_BlockPicker, ot, &ctx);
    Prim_AddDrawMode(ot, 0x20);
    MenuList_Draw(&g_CardMenu_EntryList, ot);
    CardDialog_Draw(&m->dlgSaveFailed, ot, &ctx);
    CardDialog_Draw(&m->dlgSaveError, ot, &ctx);
    CardDialog_Draw(&m->dlgComplete, ot, &ctx);
    CardDialog_Draw(&m->dlgOverwrite, ot, &ctx);
    CardDialog_Draw(&m->dlgConfirmSave, ot, &ctx);
    CardDialog_Draw(&m->dlgConfirmLoad, ot, &ctx);
    CardDialog_Draw(&m->dlgDiscard, ot, &ctx);
    TextEntry_Draw(&m->titleEntry, ot);
    ((CardStateFn)m->handler)(2, ot);
}

void CardMenu_StartSaveReplay(void)
{
    g_CardMenu->mode = CARDMODE_SAVE_REPLAY;
    CardMenu_PrepareReplayEntry();
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartSaveGhost(void)
{
    g_CardMenu->mode = CARDMODE_SAVE_REPLAY;
    CardMenu_PrepareGhostEntry();
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartPlayReplay(void)
{
    g_CardMenu->mode = CARDMODE_PLAY_REPLAY;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartEditReplay(void)
{
    g_CardMenu->mode = CARDMODE_EDIT_REPLAY;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartLoadGhost(void)
{
    g_CardMenu->mode = CARDMODE_LOAD_GHOST;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
    g_CardMenu_GhostLoaded = '\0';
}

int CardMenu_GhostLoaded(void)
{
    return g_CardMenu_GhostLoaded != 0;
}

void CardMenu_StartSave(GameSaveFile *file)
{
    CardMenu *m = g_CardMenu;

    m->saveFile = (GameSaveFile *)m->replayData;
    if (file != 0) {
        m->saveFile = file;
    }
    SaveFile_InitHeader(m->saveFile);
    GameState_WriteSave(m->saveFile, m->saveFile, 0x8000);
    m->mode = CARDMODE_SAVE_GAME;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartLoad(GameSaveFile *file)
{
    CardMenu *pCVar1;

    pCVar1 = g_CardMenu;
    g_CardMenu->saveFile = (GameSaveFile *)g_CardMenu->replayData;
    if (file != 0) {
        pCVar1->saveFile = file;
    }
    SaveFile_InitHeader(pCVar1->saveFile);
    pCVar1->mode = CARDMODE_LOAD_GAME;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}
}
