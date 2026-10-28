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
void CardMenu_SetState(int state);
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
extern char D_8008FB14[]; // "No Name"
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
enum { CARD_EXIT = 0x27, CARD_REPLAY_LOADED = 0x28 }; // what a state returns to leave
extern char D_8008FAC8[]; // ""
void KanjiFont_UploadClut(void *font);
void KanjiFont_UploadTexture(void *font);
void SaveFile_InitHeader(GameSaveFile *file);
extern CardMenu *g_CardMenu;
extern signed char g_CardMenu_GhostLoaded;
extern GameState g_GameState;
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

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_EntryListCallback);

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
        m->line2 = D_8008FAC8;
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
            m->line2 = D_8008FAC8;
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
                    m->line2 = D_8008FAC8;
                    CardMenu_CacheKanjiLines();
                    next = 24;
                }
                break;
            case CARDMODE_PLAY_REPLAY:
                next = 18;
                if ((signed char)m->dir.count == 0) {
                    m->line1 = TEXT(NoReplayDataInFile);
                    m->line2 = D_8008FAC8;
                    CardMenu_CacheKanjiLines();
                    next = 20;
                }
                break;
            case CARDMODE_LOAD_GHOST:
                next = 26;
                if ((signed char)m->dir.count == 0) {
                    m->line1 = TEXT(NoReplayDataInFile);
                    m->line2 = D_8008FAC8;
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

// State 11: "Changes are not saved"; leaves the list without saving (10, after a
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

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_ReplayEntryList);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_EditTitle);

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
        m->line2 = D_8008FAC8;
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

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_PlayList);

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

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_GhostList);

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
        m->line2 = D_8008FAC8;
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
        m->line2 = D_8008FAC8;
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
        m->line2 = D_8008FAC8;
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
        m->line2 = D_8008FAC8;
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
        m->line1 = D_8008FAC8;
        m->line2 = D_8008FAC8;
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
        m->line2 = D_8008FAC8;
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

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_CheckSlot);

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
        m->line2 = D_8008FAC8;
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
        m->line2 = D_8008FAC8;
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
        m->line2 = D_8008FAC8;
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

// Enters state (Idle when negative) by calling its handler with event 0.
void CardMenu_SetState(int state)
{
    CardMenu *m = g_CardMenu;

    if (state < 0) {
        m->handler = (void *)CardState_Idle;
    } else {
        m->handler = (void *)g_CardMenu_States[state];
    }
    m->state = state;
    ((CardStateFn)m->handler)(0, 0);
}

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
    ReplayEntry_SetTitle(e->title, D_8008FB14);
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
    ReplayEntry_SetTitle(e->title, D_8008FB14);
    e->unk40 = setup->game;
    e->raceMode = setup->raceType;
    e->kind = 1;
    e->courseId = setup->courseHash;
    *(uint *)e->unk48 = setup->cars[0].carId;
    ReplayEntry_SetCarName(e, setup->cars[0].name);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_InitWidgets);

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

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_Draw);

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
