// OVL1 80013230-80016254 (replaycopy): Replay copy between memory cards.
// The resident card menu (exe/cardmenu.cpp) with its own states. The functions that
// don't match yet are the original assembly, with their C++ under NON_MATCHING.
#include "gptr.h"
#include "common.h"
#include "prim.h"
#include "mccard.h"

typedef int ReplayCopyStateFn(int event, void *arg);

// A row of the replay list.
struct ReplayCopyRow {
    short kind; // 0 a replay, 2 the row that starts the copy
    short ok;   // can be chosen
    short why;  // the hint: 0 copy, 1 too large, 2 too many, 3 the last row
};

// The copy's work area, in ReplayCopyConfig's heap. Like the start of CardMenu.
struct ReplayCopyWork {
    short state;
    short pad02;
    game_function<ReplayCopyStateFn> handler;
    game_pointer<KanjiFont> kanjiFont;
    game_pointer<short> slotStatus;
    short listDelay;
    short pad12;
    PadButton pressed;
    PadButton held;
    PadButton repeat;
    game_pointer<char> errorMsg;
    game_pointer<char> line1;
    game_pointer<char> line2;
    short textPri;
    short timer;
    WipeBar slotBar;
    GrowBar growBar;
    CardDialog dlgError;
    CardDialog dlgNoReplays;
    CardDialog dlgSlots;
    CardDialog dlgFormat;
    CardDialog dlgComplete;     // the last two bytes are blockResult
    short listResult;
    short pad35a;
    CardProgressBar progress;
    int cardsOk;                // cleared when a card goes away during the copy
    ReplayCopyRow rows[33];
    unsigned char pad45e[0x48a - 0x45e];
    short listRow;
    ReplayDir dir2;             // the destination, card 2
    unsigned char pad1a0c[12];
    short usage[2];             // ReplayDir_DrawUsageBar's total and used sectors
    ReplayDir dir1;             // the source, card 1
    unsigned char pad2f9c[12];
    signed char selected[33];
    unsigned char pad2fc9[7];
    unsigned char data[0x20000]; // the replays read
    short current;              // the replay being read
    short count;                // of replays to copy
    int total;                  // their bytes
    short indices[32];
    unsigned char pad23018[8];
    int offsets[36];            // into data
    int padded[36];             // the sizes, in 0x80-byte sectors
    int sizes[36];
    int remaining[36];          // the bytes after this replay
    ReplayDirEntry entries[32];
    unsigned char pad23de0[0x170];
    short chain[961];           // a SectorChain
    short writeIndex;           // the replay being written
};

#define blockResult dlgComplete.pad96

extern "C" {
extern GameState (&g_GameState);
extern GameSaveFile (&g_SaveFileBuf);
extern game_pointer<ReplayCopyWork> (&g_ReplayCopy);
extern Font (&g_ReplayCopy_Fonts); // medium
extern Font (&g_ReplayCopy_FontLarge);
extern Font (&g_ReplayCopy_FontSmall);
extern NumberPicker (&g_ReplayCopy_BlockSpinner);
extern MenuList (&g_ReplayCopy_List);
extern game_function<ReplayCopyStateFn> (&g_ReplayCopy_States)[];
extern ReplayEntryStyle (&D_8004B7C0);
extern int (&g_McOp_Remaining);
extern char (&D_800210E4)[]; // an empty string
extern WipeBar (&D_8004B614);
extern GrowBar (&D_8004B630);
extern CardDialogDesc (&D_8004B644);
extern CardDialogDesc (&D_8004B674);
extern CardDialogDesc (&D_8004B6A4);
extern CardDialogDesc (&D_8004B6D4);
extern CardDialogDesc (&D_8004B758);
extern CardProgressBar (&D_8004B788);
extern char (&sB_DataIsTooLarge)[];
extern char (&sB_DoYouWantToFormat)[];
extern char (&sB_Max32Files)[];
extern char (&sB_MemoryCard1)[];
extern char (&sB_PleaseDoNotRemoveMemoryCard)[];
extern char (&sB_TotalNReplay)[];
extern char (&s_CannotDetectMemoryCard1)[];
extern char (&s_CannotDetectMemoryCard2)[];
extern char (&s_CannotFindReplayDataOnMemoryCard1)[];
extern char (&s_CannotFindReplayFileToCopy)[];
extern char (&s_CheckingSlot1)[];
extern char (&s_CheckingSlot2)[];
extern char (&s_Copy)[];
extern char (&s_CopyComplete)[];
extern char (&s_CopyNReplay)[];
extern char (&s_CopyToMemoryCard2)[];
extern char (&s_Copy_2)[];
extern char (&s_CreatingReplayFileOnMemoryCard2)[];
extern char (&s_FailedToCreateReplayFileOnMemoryCard2)[];
extern char (&s_FailedToFormatMemoryCard2)[];
extern char (&s_FailedToLoadMemoryCard1)[];
extern char (&s_FailedToLoadMemoryCard2)[];
extern char (&s_FailedToSaveOnMemoryCard2)[];
extern char (&s_FormattingMemoryCard2)[];
extern char (&s_LoadingFromMemoryCard1)[];
extern char (&s_MemoryCard1IsCorrupt)[];
extern char (&s_MemoryCard2IsCorrupt)[];
extern char (&s_MemoryCard2IsNotFormatted)[];
extern char (&s_MemoryCard2IsNotFormatted_2)[];
extern char (&s_NotEnoughEmptyBlocksOnMemoryCard2)[];
extern char (&s_ReplayFileOnMemoryCard1IsCorrupt)[];
extern char (&s_ReplayFileOnMemoryCard2IsCorrupt)[];
extern char (&s_SavingOnMemoryCard2)[];
extern char (&s_SelectNoOfBlocksOnMemoryCard2)[];
extern char (&s_WhileCopyingData)[];
int sprintf(char *buf, const char *format, ...);
void ReplayCopy_DrawKanji(uint *ot, char *sjis, int x, int y, uint rgb);
int ReplayCopy_ListCallback(MenuListEventId event, MenuListEvent *ev, int row);
int ReplayCopy_GetSelectedSectors(void);
int ReplayCopy_CountSelected(void);
int ReplayCopy_GetCountAfterCopy(void);
int ReplayCopy_GetFreeAfterCopy(void);
int ReplayCopy_CanToggle(int i, int *why);
void ReplayCopy_ToggleSelected(int i);
void ReplayCopy_InitSpaceGauge(void);
void ReplayCopy_DrawCard2Space(uint *ot, int current);
void ReplayCopy_DrawSelectedCount(uint *ot);
int ReplayCopy_CardsReady(int strict);
int ReplayCopy_GetSlotMessages(char **msg1, char **msg2, int checkFormat);

// Japanese only: empties the kanji cache and adds the list's hints.
void ReplayCopy_CacheKanjiCommon(void)
{
    KanjiFont *font = g_ReplayCopy->kanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_Reset(font);
        KanjiFont_CacheString(font, s_Copy_2);
        KanjiFont_CacheString(font, sB_DataIsTooLarge);
        KanjiFont_CacheString(font, sB_DataIsTooLarge - 0x11);
        KanjiFont_CacheString(font, s_Copy_2 + 0xf);
        KanjiFont_CacheString(font, s_Copy_2 + 0x24);
    }
}

// Japanese only: adds a string to the kanji cache.
void ReplayCopy_CacheKanji(char *sjis)
{
    KanjiFont *font = g_ReplayCopy->kanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_CacheString(font, sjis);
    }
}

// Draws SJIS text from the kanji cache, centred on x, 12 pixels a character, in Japanese
// only.
void ReplayCopy_DrawKanji(uint *ot, char *sjis, int x, int y, uint rgb)
{
    KanjiFont *font = g_ReplayCopy->kanjiFont;
    unsigned char *p;
    unsigned char *s;
    int w = 0;

    if (g_GameState.language != 0) {
        return;
    }
    for (s = (unsigned char *)sjis; *s != 0; s += 2) {
        w += 12;
    }
    x -= w >> 1;
    p = (unsigned char *)sjis;
    while (*p != 0) {
        KanjiGlyph *glyph = KanjiFont_FindGlyph(font, p[0] << 8 | p[1]);

        if (glyph != 0) {
            PrimFT4 *q = (PrimFT4 *)KanjiFont_AddGlyphQuad(glyph, ot, rgb);

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
}

// Japanese only: caches the common strings and both message lines.
void ReplayCopy_CacheMessages(void)
{
    void *pvVar1;

    pvVar1 = g_ReplayCopy;
    if (g_GameState.language == '\0') {
        ReplayCopy_CacheKanjiCommon();
        ReplayCopy_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x24)));
        ReplayCopy_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x28)));
    }
}

// Japanese only: caches the common strings and the error text.
void ReplayCopy_CacheErrorText(void)
{
    void *pvVar1;

    pvVar1 = g_ReplayCopy;
    if (g_GameState.language == '\0') {
        ReplayCopy_CacheKanjiCommon();
        ReplayCopy_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x20)));
    }
}

// The sectors of the selected replays of card 1.
int ReplayCopy_GetSelectedSectors(void)
{
    int total = 0;
    int i = 0;
    ReplayCopyWork *m = g_ReplayCopy;
    ReplayDir *dir = &m->dir1;

    for (; i < 0x20; i++) {
        int selected = m->selected[i];

        if (selected) {
            total += dir->entries[i].sectorCount;
        }
    }
    return total;
}

// The number of selected replays.
int ReplayCopy_CountSelected(void)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int n = 0;
    int i;

    for (i = 0; i < 0x20; i++) {
        if (m->selected[i]) {
            n++;
        }
    }
    return n;
}

// The replays card 2 will have after the copy.
int ReplayCopy_GetCountAfterCopy(void)
{
    int count = g_ReplayCopy->dir2.count;

    return count + ReplayCopy_CountSelected();
}

// The free sectors card 2 will have after the copy.
int ReplayCopy_GetFreeAfterCopy(void)
{
    int free = g_ReplayCopy->dir2.totalSectors;

    free -= SectorMap_CountUsed(&g_ReplayCopy->dir2.sectorMap);
    return free - ReplayCopy_GetSelectedSectors();
}

// Capacity of the card 2 replay file.
int ReplayCopy_GetCapacity(void)
{
    return (int)*(short *)rt::from_guest(((int)g_ReplayCopy + 0x68e));
}

// Whether row i can be selected; why is 1 when the data is too large, 2 when card 2 would
// have too many replays.
int ReplayCopy_CanToggle(int i, int *why)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int free = ReplayCopy_GetFreeAfterCopy();

    *why = 0;
    if (m->selected[i] == 1) {
        return 1;
    }
    *why = 1;
    free -= m->dir1.entries[i].sectorCount;
    if (free < 0) {
        return 0;
    }
    *why = 2;
    if (ReplayCopy_GetCountAfterCopy() == 0x20) {
        return 0;
    }
    *why = 0;
    return 1;
}

// Selects or deselects row i.
void ReplayCopy_ToggleSelected(int i)
{
    ReplayCopyWork *m = g_ReplayCopy;

    if (m->selected[i] == 1) {
        m->selected[i] = 0;
    } else {
        m->selected[i] = 1;
    }
}

void ReplayCopy_ClearSelection(void)
{
    int iVar1;
    int iVar2;

    iVar2 = 0x20;
    iVar1 = (int)g_ReplayCopy + 0x20;
    do {
        *(unsigned char *)rt::from_guest((iVar1 + 0x2fa8)) = 0;
        iVar2 = iVar2 + -1;
        iVar1 = iVar1 + -1;
    } while (-1 < iVar2);
}

// Card 2's total and used sectors, for the usage bar.
void ReplayCopy_InitSpaceGauge(void)
{
    ReplayCopyWork *m = g_ReplayCopy;
    short *usage = m->usage;
    int total = m->dir2.totalSectors;

    m->usage[0] = total;
    usage[1] = SectorMap_CountUsed(&m->dir2.sectorMap);
}

// Card 2's replays and free sectors: as they are, or as they'll be after the copy.
void ReplayCopy_DrawCard2Space(uint *ot, int current)
{
    ReplayCopyWork *m = g_ReplayCopy;
    TextCtx ctx;
    char buf[0x100];
    TextCtx *t;
    char *p;
    char *format;
    int one;
    int right;
    int spacing;
    int w;
    int free;

    Text_Init(&ctx, m->textPri);
    t = &ctx;
    *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
    t->ot = ot;
    Text_SetFont(t, &g_ReplayCopy_Fonts);
    one = 1;
    t->unk1c = one;
    t->rgb = 0x24a4136;
    if (current) {
        ReplayDir *dir = &m->dir2;
        SectorMap *map;

        p = buf;
        format = sB_TotalNReplay;
        sprintf(p, format, dir->count);
        w = Text_MeasureStringMonoDigits(t, (signed char *)p, one, 0);
        right = 0x140;
        spacing = -2;
        Text_DrawStringMonoDigits(t, (signed char *)p, right - w, 0x8a, one, spacing, 0);
        map = &m->dir2.sectorMap;
        free = dir->totalSectors;
        free -= SectorMap_CountUsed(map);
        sprintf(p, format + 0x19, free, ReplayCopy_GetCapacity());
        w = Text_MeasureStringMonoDigits(t, (signed char *)p, one, 0);
        Text_DrawStringMonoDigits(t, (signed char *)p, right - w, 0x9e, one, spacing, 0);
        m->usage[1] = SectorMap_CountUsed(map);
        ReplayDir_DrawUsageBar(m->usage, ot, 0xb0, 0xaa, 0x80, 0, 0);
    } else {
        int count = ReplayCopy_GetCountAfterCopy();

        p = buf;
        format = sB_TotalNReplay;
        sprintf(p, format, count);
        w = Text_MeasureStringMonoDigits(t, (signed char *)p, 1, 0);
        right = 0x140;
        spacing = -2;
        Text_DrawStringMonoDigits(t, (signed char *)p, right - w, 0x8a, 1, spacing, 0);
        free = ReplayCopy_GetFreeAfterCopy();
        sprintf(p, format + 0x19, free, ReplayCopy_GetCapacity());
        w = Text_MeasureStringMonoDigits(t, (signed char *)p, 1, 0);
        Text_DrawStringMonoDigits(t, (signed char *)p, right - w, 0x9e, 1, spacing, 0);
        ReplayDir_DrawUsageBar(m->usage, ot, 0xb0, 0xaa, 0x80, 0, ReplayCopy_GetSelectedSectors());
    }
}

// "Copy N replays".
void ReplayCopy_DrawSelectedCount(uint *ot)
{
    TextCtx ctx;
    char buf[0x100];
    TextCtx *t;
    char *p;
    int one;
    int w;

    Text_Init(&ctx, g_ReplayCopy->textPri);
    t = &ctx;
    *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
    t->ot = ot;
    Text_SetFont(t, &g_ReplayCopy_Fonts);
    one = 1;
    t->unk1c = one;
    t->rgb = 0x24a4136;
    p = buf;
    sprintf(p, s_CopyNReplay, ReplayCopy_CountSelected());
    w = Text_MeasureStringMonoDigits(t, (signed char *)p, one, 0);
    Text_DrawStringMonoDigits(t, (signed char *)p, 0x140 - w, 0xe0, one, -2, 0);
}

// A row of the replay list: the entry, marked when selected, dimmed when it can't be.
int ReplayCopy_ListCallback(MenuListEventId event, MenuListEvent *ev, int row)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int result = 0;
    ReplayDirEntry *entry = 0;
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
        int enabled = m->rows[row].ok == 1;
        int state = list->state;
        int open = state >= 0;
        int closing = state < -1;
        int alpha;
        int slide;
        uint rgb;

        if (!(open | closing)) {
            break;
        }
        alpha = ((list->fade << 7) / list->fadeMax * brightness) >> 7;
        if (alpha == 0) {
            break;
        }
        rgb = 0x5a4a3e;
        if (!enabled) {
            rgb = 0x8082a;
        }
        ((uint *)&D_8004B7C0)[4] = rgb;
        slide = 128 - (list->fade << 7) / list->fadeMax;
        if (slide < 0) {
            slide = 0;
        }
        slide = slide * 176 >> 7;
        if (m->selected[row]) {
            GouraudQuad q;
            uint from = 0;
            uint to = 0x5a8c32;
            int left;
            int width;

            Text_SetFont(t, &g_ReplayCopy_FontLarge);
            *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
            t->ot = ot;
            t->rgb = 0x20a5a5a;
            left = x + slide;
            width = Text_DrawString(t, (signed char *)s_Copy, left - 0x98, y + 0x18, 1);
            q.rect.x = left - 0x9c;
            q.rect.y = y - 4;
            q.rect.w = width + 8;
            q.rect.h = 0x22;
            to = Rgb_Lerp((uchar *)&from, (uchar *)&to, alpha, 0x80);
            q.top = to;
            q.bottom = Rgb_Lerp((uchar *)&to, (uchar *)&from, 0x20, 0x80);
            Menu_AddGouraudQuad(ot, &q);
            Prim_AddDrawMode(ot, 0x220);
            Menu_AddGouraudQuad(ot, &q);
            Prim_AddDrawMode(ot, 0x240);
        }
        if (m->rows[row].kind == 0) {
            entry = &m->dir1.entries[row];
        }
        ReplayEntry_Draw(&D_8004B7C0, ot, t, entry, x + slide, y, alpha, m->rows[row].kind);
        break;
    }
    case 6:
        break;
    case 8:
        result = m->rows[row].ok == 1;
        break;
    }
    return result;
}

// Card 1 has a replay file and card 2 is there; unless strict, card 2 may be unformatted.
int ReplayCopy_CardsReady(int strict)
{
    ReplayCopyWork *m = g_ReplayCopy;
    short *status = m->slotStatus;
    int ready = status[0] == 0;

    if (strict) {
        if (status[1] == 0) {
            ready &= 1;
        } else {
            ready = 0;
        }
    } else {
        int st = status[1];

        ready &= (st == 0) | (st == 3);
    }
    if (ready) {
        if (ReplayFile_Find(&m->dir1, 0) == 0) {
            ready = 0;
        } else {
            ready &= 1;
        }
    }
    return ready;
}

// What's wrong with each card, if anything; returns whether both are fine.
int ReplayCopy_GetSlotMessages(char **msg1, char **msg2, int checkFormat)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int fine = 0;
    int busy = 0;

    *msg1 = D_800210E4;
    *msg2 = D_800210E4;
    switch (m->slotStatus[0]) {
    case 0:
        if (ReplayFile_Find(&m->dir1, 0) != 0) {
            break;
        }
        // fall through
    case 3:
        *msg1 = s_CannotFindReplayDataOnMemoryCard1;
        busy = 1;
        break;
    case 1:
        *msg1 = s_CheckingSlot1;
        busy = 1;
        break;
    case 2:
        *msg1 = s_CannotDetectMemoryCard1;
        busy = 1;
        break;
    case 4:
        *msg1 = s_MemoryCard1IsCorrupt;
        busy = 1;
        break;
    }
    switch (m->slotStatus[1]) {
    case 1:
        *msg2 = s_CheckingSlot2;
        busy = 1;
        break;
    case 2:
        *msg2 = s_CannotDetectMemoryCard2;
        busy = 1;
        break;
    case 3:
        if (checkFormat) {
            *msg2 = s_MemoryCard2IsNotFormatted;
            busy = 1;
        }
        break;
    case 4:
        *msg2 = s_MemoryCard2IsCorrupt;
        busy = 1;
        break;
    }
    if (!busy) {
        fine = 1;
    }
    return fine;
}

// Waits 24 frames, then goes to state 2.
int ReplayCopy_State0_Delay(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
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

// State 1: the error dialog, with errorMsg drawn above it; back to the slots (2), or leaves.
int ReplayCopy_State1_Error(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
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
        m->line1 = D_800210E4;
        m->line2 = D_800210E4;
        ReplayCopy_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = 0x16;
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
        Text_SetFont(&ctx, &g_ReplayCopy_FontLarge);
        {
            TextCtx *t = &ctx;

            *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
            t->rgb = 0x22a485c;
            t->unk1c = 1;
            t->ot = ot;
        }
        if (g_GameState.language == 0) {
            ReplayCopy_DrawKanji(ot, m->errorMsg, 0xb0, 0x112, 0x2606060);
        } else {
            Text_DrawCentered(&ctx, (signed char *)m->errorMsg, 0xb0, 0x122, 1);
        }
        break;
    }
    }
    return next;
}

// State 2: says what's wrong with the cards until the player starts: on to 9, to format
// card 2 (3), to create its file (5), or leaves.
int ReplayCopy_State2_CheckSlots(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next = -1;
    int fine;
    int r = m->dlgSlots.result;
    char *msg1;
    char *msg2;

    switch (event) {
    case 0: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            GrowBar *grow;

            bar->t = ~bar->duration;
            grow = &m->growBar;
            grow->t = ~grow->duration;
        }
        if (m->slotStatus[1] == 3) {
            next = 3;
            break;
        }
        CardDialog_Open(&m->dlgSlots);
    }
        // fall through
    case 1: {
        CardDialog *dlg;

        fine = ReplayCopy_GetSlotMessages(&msg1, &msg2, 1);
        if (fine == 0) {
            m->line1 = msg1;
            m->line2 = msg2;
        } else {
            m->line1 = s_WhileCopyingData;
            m->line2 = s_WhileCopyingData + 0x19;
        }
        ReplayCopy_CacheMessages();
        dlg = &m->dlgSlots;
        dlg->color = 0x280400c;
        if (fine == 0 && dlg->choice == 0) {
            dlg->color = 0x2080830;
        }
        switch (m->slotStatus[1]) {
        case 0:
            if (ReplayFile_Find(&m->dir2, 1) == 0) {
                CardDialog_Close(&m->dlgSlots);
                next = 5;
                return next;
            }
            break;
        case 3:
            Sfx_PlayMenu(7);
            CardDialog_Close(&m->dlgSlots);
            next = 3;
            return next;
        }
        switch (r) {
        case -1:
        case 1:
            CardDialog_Close(&m->dlgSlots);
            next = 0x16;
            break;
        case 0:
            if (ReplayCopy_CardsReady(1)) {
                Sfx_PlayMenu(1);
                CardDialog_Close(&m->dlgSlots);
                ReplayCopy_ClearSelection();
                next = 9;
                if (ReplayFile_Find(&m->dir2, 1) == 0) {
                    next = 5;
                }
            } else {
                Sfx_PlayMenu(0);
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

// State 3: card 2 isn't formatted, format it? On to 4, or leaves.
int ReplayCopy_State3_AskFormat(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int r = m->dlgFormat.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgFormat;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = s_MemoryCard2IsNotFormatted_2;
        m->line2 = sB_DoYouWantToFormat;
        ReplayCopy_CacheMessages();
        m->slotBar.t = 0;
        m->growBar.t = 0;
    }
        // fall through
    case 1:
        if (m->slotStatus[1] != 3) {
            CardDialog_Close(&m->dlgFormat);
            next = 2;
            break;
        }
        switch (r) {
        case -1:
        case 1:
            next = 0x16;
            break;
        case 0:
            Sfx_PlayMenu(1);
            next = 4;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 4: formats card 2, then on to 5.
int ReplayCopy_State4_Format(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next = -1;

    switch (event) {
    case 0:
        if (McCard_StartFormat(1) != 0) {
            next = 1;
        } else {
            m->line1 = s_FormattingMemoryCard2;
            m->line2 = sB_PleaseDoNotRemoveMemoryCard;
            ReplayCopy_CacheMessages();
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            break;
        case 0:
            next = 5;
            break;
        default:
            m->errorMsg = s_FailedToFormatMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 5: picks the replay file's size on card 2, then on to 7; 6 without 3 free blocks.
int ReplayCopy_State5_SelectBlocks(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int r = m->blockResult;
    int next = -1;

    switch (event) {
    case 0: {
        int free = g_Card.ports[1].freeBlocks;
        NumberPicker *p;

        if (free < 3) {
            next = 6;
            break;
        }
        p = &g_ReplayCopy_BlockSpinner;
        p->min = 3;
        p->max = free;
        p->value = 3;
        p->anim = 0;
        m->line1 = s_SelectNoOfBlocksOnMemoryCard2;
        m->line2 = D_800210E4;
        ReplayCopy_CacheMessages();
        m->slotBar.t = 0;
        m->growBar.t = 0;
        break;
    }
    case 1:
        if (m->slotStatus[1] != 0) {
            g_ReplayCopy_BlockSpinner.anim = -1;
            next = 2;
            break;
        }
        switch (r) {
        case -1:
            g_ReplayCopy_BlockSpinner.anim = -1;
            next = 0x16;
            break;
        case -2:
        case -3:
            break;
        default:
            g_ReplayCopy_BlockSpinner.anim = -1;
            ReplayDir_Init(&m->dir2, r);
            next = 7;
            break;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 6: not enough free blocks on card 2; back to the slots (2), or leaves.
int ReplayCopy_State6_NoSpace(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int r = m->dlgError.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg;

        Sfx_PlayMenu(0);
        dlg = &m->dlgError;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        m->line1 = s_NotEnoughEmptyBlocksOnMemoryCard2;
        m->line2 = D_800210E4;
        ReplayCopy_CacheMessages();
        break;
    }
    case 1:
        if (m->slotStatus[1] != 0) {
            CardDialog_Close(&m->dlgError);
            next = 2;
            break;
        }
        switch (r) {
        case -1:
        case 1:
            next = 0x16;
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

// State 7: creates the replay file on card 2, then on to 8.
int ReplayCopy_State7_CreateFile(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next = -1;

    switch (event) {
    case 0:
        if (ReplayFile_Create(&m->dir2, 1) != 0) {
            next = 1;
        }
        m->line1 = s_CreatingReplayFileOnMemoryCard2;
        m->line2 = sB_PleaseDoNotRemoveMemoryCard;
        ReplayCopy_CacheMessages();
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            break;
        case 0:
            next = 8;
            break;
        default:
            m->errorMsg = s_FailedToCreateReplayFileOnMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        break;
    }
    return next;
}

// State 8: writes the new file's directory, then back to the slots (2).
int ReplayCopy_State8_WriteEmptyDir(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next = -1;

    switch (event) {
    case 0: {
        ReplayDir *dir = &m->dir2;

        ReplayDir_UpdateCrc(dir);
        if (ReplayFile_StartWriteDir(dir, 1) != 0) {
            m->errorMsg = s_FailedToSaveOnMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = 0x150c;
            bar->color = 0xc5090;
            CardProgress_Reset(bar);
            m->line1 = s_SavingOnMemoryCard2;
            m->line2 = sB_PleaseDoNotRemoveMemoryCard;
            ReplayCopy_CacheMessages();
        }
        break;
    }
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0:
            m->progress.visible = -1;
            next = 2;
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = s_FailedToSaveOnMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// State 9: reads card 2's directory, then on to 10; back to 2 if a card was removed.
int ReplayCopy_State9_LoadCard2Dir(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next = -1;

    switch (event) {
    case 0:
        if (ReplayFile_StartReadDir(&m->dir2, 1) != 0) {
            m->errorMsg = s_FailedToLoadMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        } else {
            CardProgressBar *bar = &m->progress;

            m->slotBar.t = 0;
            m->growBar.t = 0;
            bar->total = 0x150c;
            bar->color = 0x90500c;
            CardProgress_Reset(bar);
            m->line1 = s_CheckingSlot2;
            m->line2 = D_800210E4;
            ReplayCopy_CacheMessages();
            m->cardsOk = 1;
            if (ReplayCopy_CardsReady(1) ^ 1) {
                m->cardsOk = 0;
            }
        }
        break;
    case 1:
        if (ReplayCopy_CardsReady(1) ^ 1) {
            m->cardsOk = 0;
        }
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0:
            m->progress.visible = -1;
            if (ReplayDir_IsValid(&m->dir2) ^ 1) {
                m->errorMsg = s_ReplayFileOnMemoryCard2IsCorrupt;
                ReplayCopy_CacheErrorText();
                next = 1;
            } else if (m->cardsOk) {
                next = 0xa;
            } else {
                next = 2;
            }
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = s_FailedToLoadMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// State 10: reads card 1's directory, then on to the list (11), or 20 without replays.
int ReplayCopy_State10_LoadCard1Dir(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next = -1;

    switch (event) {
    case 0:
        if (m->cardsOk == 0) {
            next = 2;
            break;
        }
        if (ReplayFile_StartReadDir(&m->dir1, 0) != 0) {
            m->errorMsg = s_FailedToLoadMemoryCard1;
            ReplayCopy_CacheErrorText();
            next = 1;
            break;
        }
        ReplayCopy_InitSpaceGauge();
        {
            CardProgressBar *bar = &m->progress;

            bar->total = 0x150c;
            bar->color = 0x90500c;
            CardProgress_Reset(bar);
        }
        m->line1 = s_CheckingSlot1;
        m->line2 = D_800210E4;
        ReplayCopy_CacheMessages();
        if (ReplayCopy_CardsReady(1) ^ 1) {
            m->cardsOk = 0;
        }
        // fall through
    case 1:
        if (ReplayCopy_CardsReady(1) ^ 1) {
            m->cardsOk = 0;
        }
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0: {
            ReplayDir *dir;

            m->progress.visible = -1;
            dir = &m->dir1;
            if (ReplayDir_IsValid(dir) ^ 1) {
                m->errorMsg = s_ReplayFileOnMemoryCard1IsCorrupt;
                ReplayCopy_CacheErrorText();
                next = 1;
            } else {
                m->listDelay = 1;
                next = 0xb;
                if (dir->count == 0) {
                    next = 0x14;
                }
                m->listRow = 0;
                if (m->cardsOk == 0) {
                    next = 2;
                }
            }
            break;
        }
        default:
            m->progress.visible = -1;
            m->errorMsg = s_FailedToLoadMemoryCard1;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        ReplayCopy_DrawCard2Space((uint *)arg, 0);
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// State 11: the list of card 1's replays, with an extra row that starts the copy (12).
int ReplayCopy_State11_SelectReplays(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int r = m->listResult;
    int count = m->dir1.count;
    int next = -1;
    uint *ot = (uint *)arg;

    switch (event) {
    case 0: {
        int i;

        m->line1 = D_800210E4;
        m->line2 = D_800210E4;
        ReplayCopy_CacheMessages();
        g_ReplayCopy_List.count = count + 1;
        g_ReplayCopy_List.selected = m->listRow;
        for (i = 0; i < count; i++) {
            int why;
            int ok;

            m->rows[i].kind = 0;
            ok = ReplayCopy_CanToggle(i, &why);
            m->rows[i].ok = ok;
            m->rows[i].why = why;
        }
        m->rows[i].kind = 2;
        m->rows[i].ok = ReplayCopy_CountSelected() > 0;
        m->rows[i].why = 3;
        if (ReplayCopy_CardsReady(1) ^ 1) {
            m->cardsOk = 0;
        }
        break;
    }
    case 1:
        if (ReplayCopy_CardsReady(1) ^ 1) {
            m->cardsOk = 0;
        }
        if (m->cardsOk == 0) {
            MenuList_Close(&g_ReplayCopy_List);
            next = 2;
            break;
        }
        if (m->listDelay > 0) {
            if (--m->listDelay == 0) {
                Sfx_PlayMenu(7);
                MenuList_Open(&g_ReplayCopy_List);
            }
        }
        switch (r) {
        case -1:
            MenuList_Close(&g_ReplayCopy_List);
            Sfx_PlayMenu(2);
            next = 2;
            break;
        case -3:
            Sfx_PlayMenu(5);
            break;
        case -4:
            Sfx_PlayMenu(0);
            break;
        case -2:
            break;
        default: {
            int row;

            Sfx_PlayMenu(1);
            row = -1;
            if (m->rows[r].kind == 0) {
                row = r;
            }
            if (row >= 0) {
                Sfx_PlayMenu(1);
                ReplayCopy_ToggleSelected(r);
                next = 0xb;
                m->listDelay = 0;
                m->listRow = r;
            } else {
                Sfx_PlayMenu(1);
                MenuList_Close(&g_ReplayCopy_List);
                next = 0xc;
            }
            break;
        }
        }
        break;
    case 2: {
        char *text;
        uint rgb;

        ReplayCopy_DrawCard2Space(ot, 0);
        ReplayCopy_DrawSelectedCount(ot);
        text = D_800210E4;
        rgb = 0x646464;
        switch (m->rows[g_ReplayCopy_List.selected].why) {
        case 0:
            text = s_Copy_2;
            break;
        case 1:
            text = sB_DataIsTooLarge;
            break;
        case 2:
            text = sB_Max32Files;
            break;
        case 3:
            text = s_CannotFindReplayFileToCopy;
            if (ReplayCopy_CountSelected() > 0) {
                text -= 0x15;
            }
            break;
        }
        if (g_GameState.language == 0) {
            ReplayCopy_DrawKanji(ot, text, 0xb0, 0x1a8, rgb);
        } else {
            TextCtx ctx;
            TextCtx *t;

            Text_Init(&ctx, m->textPri);
            Text_SetFont(&ctx, &g_ReplayCopy_Fonts);
            t = &ctx;
            t->unk1c = 1;
            t->ot = ot;
            t->rgb = rgb;
            *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
            Text_DrawCentered(t, (signed char *)text, 0xb0, 0x1b8, 1);
        }
        break;
    }
    }
    return next;
}

// State 12: lists the selected replays and where they go in the buffer; on to 13, or back
// to 2 with none.
int ReplayCopy_State12_PrepareCopy(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int total = 0;
    int sum = 0;
    int n = 0;
    int i;

    m->current = 0;
    for (i = 0; i < 0x20; i++) {
        if (m->selected[i]) {
            int padded;

            m->indices[n] = i;
            m->entries[n] = m->dir1.entries[i];
            m->offsets[n] = total;
            m->sizes[n] = m->dir1.entries[i].size;
            padded = (m->dir1.entries[i].size + 0x7f) & ~0x7f;
            total += padded;
            m->padded[n] = padded;
            n++;
        }
    }
    m->count = n;
    m->total = total;
    if (n == 0) {
        return 2;
    }
    for (i = 0; i < n; i++) {
        sum += m->padded[i];
        m->remaining[i] = total - sum;
    }
    {
        CardProgressBar *bar = &m->progress;

        bar->total = total;
        bar->color = 0x50782c;
        CardProgress_Reset(bar);
    }
    m->line1 = s_LoadingFromMemoryCard1;
    m->line2 = sB_PleaseDoNotRemoveMemoryCard;
    ReplayCopy_CacheMessages();
    return 0xd;
}

// State 13: reads the current replay from card 1 and checks it, then on to 14.
int ReplayCopy_State13_ReadReplay(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int current = m->current;
    int index = m->indices[current];
    unsigned char *buf = m->data + m->offsets[current];
    int next = -1;

    switch (event) {
    case 0:
        if (m->cardsOk == 0) {
            m->progress.visible = -1;
            next = 2;
            break;
        }
        if (ReplayFile_StartReadEntry(&m->dir1, 0, index, buf, (SectorChain *)m->chain) != 0) {
            m->progress.visible = -1;
            m->errorMsg = s_FailedToLoadMemoryCard1;
            ReplayCopy_CacheErrorText();
            next = 1;
            break;
        }
        if (ReplayCopy_CardsReady(1) ^ 1) {
            m->cardsOk = 0;
        }
        break;
    case 1:
        if (ReplayCopy_CardsReady(1) ^ 1) {
            m->cardsOk = 0;
        }
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining + m->remaining[current]);
            break;
        case 0:
            next = 0xe;
            if (ReplayDir_CheckEntryCrc(&m->dir1, index, buf) == 0) {
                m->errorMsg = s_ReplayFileOnMemoryCard1IsCorrupt;
                ReplayCopy_CacheErrorText();
                next = 1;
            }
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = s_FailedToLoadMemoryCard1;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        ReplayCopy_DrawCard2Space((uint *)arg, 0);
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// State 14: the next replay to read (13), or on to the writing (15).
int ReplayCopy_State14_NextRead(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next;

    next = 0xf;
    if (m->cardsOk == 0) {
        m->progress.visible = -1;
        next = 2;
    } else if (++m->current < m->count) {
        next = 0xd;
    }
    return next;
}

// State 15: starts writing, then on to 16.
int ReplayCopy_State15_StartWrite(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    CardProgressBar *bar;
    int total;

    m->writeIndex = 0;
    total = m->total;
    bar = &m->progress;
    bar->color = 0xc5090;
    bar->total = total + 0x150c;
    CardProgress_Reset(bar);
    m->line1 = s_SavingOnMemoryCard2;
    m->line2 = sB_PleaseDoNotRemoveMemoryCard;
    ReplayCopy_CacheMessages();
    return 0x10;
}

// State 16: adds the current replay to card 2's directory and writes it, then on to 17.
int ReplayCopy_State16_WriteReplay(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int i = m->writeIndex;
    int size = m->sizes[i];
    unsigned char *buf = m->data + m->offsets[i];
    int next = -1;

    switch (event) {
    case 0: {
        ReplayDir *dir = &m->dir2;
        SectorChain *chain = (SectorChain *)m->chain;

        if (ReplayDir_AddEntry(dir, -1, &m->entries[i], buf, size, chain) == 0) {
            m->errorMsg = s_ReplayFileOnMemoryCard2IsCorrupt;
        } else if (ReplayDir_IsValid(dir) ^ 1) {
            m->errorMsg = s_ReplayFileOnMemoryCard2IsCorrupt;
        } else if (ReplayFile_StartWriteData(dir, 1, buf, size, (short *)chain) != 0) {
            m->errorMsg = s_FailedToSaveOnMemoryCard2;
        } else {
            break;
        }
        ReplayCopy_CacheErrorText();
        next = 1;
        break;
    }
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining + m->remaining[i] + 0x150c);
            break;
        case 0:
            next = 0x11;
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = s_FailedToSaveOnMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        ReplayCopy_DrawCard2Space((uint *)arg, 1);
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// State 17: the next replay to write (16), or on to the directory (18).
int ReplayCopy_State17_NextWrite(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next;

    next = 0x12;
    if (m->slotStatus[1] != 0) {
        m->errorMsg = s_FailedToSaveOnMemoryCard2;
        ReplayCopy_CacheErrorText();
        next = 1;
    } else if (++m->writeIndex < m->count) {
        next = 0x10;
    }
    return next;
}

// State 18: writes card 2's directory, then on to 19.
int ReplayCopy_State18_WriteDir(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int next = -1;

    switch (event) {
    case 0:
        if (ReplayFile_StartWriteDir(&m->dir2, 1) != 0) {
            m->errorMsg = s_FailedToSaveOnMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        m->line1 = s_SavingOnMemoryCard2;
        m->line2 = sB_PleaseDoNotRemoveMemoryCard;
        ReplayCopy_CacheMessages();
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0:
            m->progress.visible = -1;
            next = 0x13;
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = s_FailedToSaveOnMemoryCard2;
            ReplayCopy_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        ReplayCopy_DrawCard2Space((uint *)arg, 1);
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// State 19: "Copy complete"; back to the slots (2), or leaves.
int ReplayCopy_State19_Complete(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int r = m->dlgComplete.result;
    int next = -1;

    switch (event) {
    case 0: {
        WipeBar *bar = &m->slotBar;
        GrowBar *grow;
        CardDialog *dlg;

        bar->t = ~bar->duration;
        grow = &m->growBar;
        grow->t = ~grow->duration;
        Sfx_PlayMenu(7);
        dlg = &m->dlgComplete;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        m->line1 = s_CopyComplete;
        m->line2 = D_800210E4;
        ReplayCopy_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = 0x16;
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

// State 20: card 1 has no replays; back to the slots (2), or leaves.
int ReplayCopy_State20_NoReplays(int event, void *arg)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int r = m->dlgNoReplays.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg;

        Sfx_PlayMenu(0);
        dlg = &m->dlgNoReplays;
        CardDialog_Open(dlg);
        dlg->choice = 0;
        m->line1 = sB_MemoryCard1;
        m->line2 = sB_MemoryCard1 - 0x117;
        ReplayCopy_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = 0x16;
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

// Returns -1.
int ReplayCopy_StateIdle(int event, void *ot)
{
    return -1;
}

// Runs the new state's event 0; a negative state is idle.
int ReplayCopy_SetState(int state)
{
    ReplayCopyWork *m = g_ReplayCopy;

    if (state < 0) {
        m->handler = ReplayCopy_StateIdle;
    } else {
        m->handler = g_ReplayCopy_States[state];
    }
    m->state = state;
    return m->handler(0, 0);
}

// Copies the bars and builds the dialogs, the block picker and the list.
void ReplayCopy_InitWidgets(void)
{
    ReplayCopyWork *m = g_ReplayCopy;

    m->slotBar = D_8004B614;
    m->slotBar.t = -1;
    m->growBar = D_8004B630;
    m->growBar.t = -1;
    CardDialog_Init(&m->dlgError, &D_8004B644, 0);
    CardDialog_Init(&m->dlgNoReplays, &D_8004B674, 0);
    CardDialog_Init(&m->dlgSlots, &D_8004B6A4, 0);
    CardDialog_Init(&m->dlgFormat, &D_8004B6D4, 0);
    NumberPicker_Init(&g_ReplayCopy_BlockSpinner);
    MenuList_Init(&g_ReplayCopy_List, ReplayCopy_ListCallback, 0);
    CardDialog_Init(&m->dlgComplete, &D_8004B758, 0);
    m->progress = D_8004B788;
    m->line1 = D_800210E4;
    m->line2 = D_800210E4;
    ReplayCopy_CacheMessages();
}

// Sets up the copy in config's work area; returns the end of the area.
void *ReplayCopy_Setup(ReplayCopyConfig *config)
{
    ReplayCopyWork *m = (ReplayCopyWork *)rt::from_guest(config->heap);

    g_ReplayCopy_FontSmall = *config->fontSmall;
    g_ReplayCopy_Fonts = *config->fontMedium;
    g_ReplayCopy_FontLarge = *config->fontLarge;
    m->kanjiFont = config->font;
    m->slotStatus = config->cardSlots;
    g_ReplayCopy = m;
    m->textPri = config->textPri;
    return m + 1;
}

// Builds the widgets and enters state 0.
void ReplayCopy_Start(void)
{
    ReplayCopy_InitWidgets();
    ReplayCopy_SetState(0);
}

// Each frame: takes the pad, updates the widgets, then runs the state and the states it
// moves to. Returns 1 when the page is left.
int ReplayCopy_Update(PadState *pad)
{
    ReplayCopyWork *m = g_ReplayCopy;
    int result = 0;
    int r;

    m->repeat = 0;
    m->pressed = 0;
    m->held = 0;
    if (pad != 0) {
        m->held = pad->held;
        m->pressed = pad->pressed;
        m->repeat = pad->repeat;
    }
    WipeBar_Update(&m->slotBar);
    GrowBar_Update((unsigned char *)&m->growBar);
    m->dlgError.result = CardDialog_Update(&m->dlgError, pad);
    m->dlgNoReplays.result = CardDialog_Update(&m->dlgNoReplays, pad);
    m->dlgSlots.result = CardDialog_Update(&m->dlgSlots, pad);
    m->dlgFormat.result = CardDialog_Update(&m->dlgFormat, pad);
    m->blockResult = NumberPicker_Update(&g_ReplayCopy_BlockSpinner, pad);
    m->listResult = MenuList_Update(&g_ReplayCopy_List, pad);
    m->dlgComplete.result = CardDialog_Update(&m->dlgComplete, pad);
    r = m->handler(1, 0);
next:
    switch (r) {
    case -1:
        break;
    case 0x16: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            GrowBar *grow;

            bar->t = ~bar->duration;
            grow = &m->growBar;
            grow->t = ~grow->duration;
        }
        m->line1 = D_800210E4;
        m->line2 = D_800210E4;
        ReplayCopy_CacheMessages();
        ReplayCopy_SetState(-1);
        result = 1;
        break;
    }
    default:
        r = ReplayCopy_SetState(r);
        goto next;
    }
    return result;
}

// Each frame: the two lines, the destination header, the widgets, then the state's own
// drawing (event 2).
void ReplayCopy_Draw(uint *ot)
{
    ReplayCopyWork *m = g_ReplayCopy;
    TextCtx ctx;
    GouraudQuad q;
    uint from = 0x2000000;
    uint to = 0x242362a;
    uint bandTop;
    uint bandBottom;
    uint *ot4 = ot + 4;
    WipeBar *bar;
    int level;

    Text_Init(&ctx, m->textPri);
    Text_SetFont(&ctx, &g_ReplayCopy_FontLarge);
    {
        TextCtx *t = &ctx;
        int center;

        *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
        t->unk1c = 1;
        t->ot = ot;
        t->rgb = 0x25c5248;

        center = 1;
        if (g_GameState.language == 0) {
            ReplayCopy_DrawKanji(ot, m->line1, 0xb0, 0x102, 0x2606060);
            ReplayCopy_DrawKanji(ot, m->line2, 0xb0, 0x126, 0x2606060);
        } else {
            Text_DrawCentered(t, (signed char *)m->line1, 0xb0, 0x112, center);
            Text_DrawCentered(t, (signed char *)m->line2, 0xb0, 0x136, center);
        }
    }
    bar = &m->slotBar;
    level = WipeBar_GetLevel(bar);
    if (level != 0) {
        GrowBar *grow;
        TextCtx *t = &ctx;

        t->rgb = Rgb_Lerp((uchar *)&from, (uchar *)&to, level, 0x80);
        Text_DrawString(t, (signed char *)s_CopyToMemoryCard2, 0x14, 0x8a, 0);
        WipeBar_Draw(bar, ot, 0x10, 0x7a);
        Prim_AddDrawMode(ot, 0x220);
        grow = &m->growBar;
        grow->x = 0xb0;
        grow->y = 0xc6;
        GrowBar_Draw(grow, ot);
        grow->x = 0xb0;
        grow->y = 0x6e;
        GrowBar_Draw(grow, ot);
        Prim_AddDrawMode(ot, 0x220);
        bandTop = 0x25a5a5a;
        bandBottom = 0x2141414;
        q.rect.x = 0;
        q.rect.y = 0x6e;
        q.rect.w = 0x160;
        q.rect.h = 0x58;
        q.top = Rgb_Lerp((uchar *)&from, (uchar *)&bandTop, level, 0x80);
        q.bottom = Rgb_Lerp((uchar *)&from, (uchar *)&bandBottom, level, 0x80);
        Menu_AddGouraudQuad(ot4, &q);
        Prim_AddDrawMode(ot4, 0x200);
    }
    CardDialog_Draw(&m->dlgError, ot, &ctx);
    CardDialog_Draw(&m->dlgNoReplays, ot, &ctx);
    CardDialog_Draw(&m->dlgSlots, ot, &ctx);
    CardDialog_Draw(&m->dlgFormat, ot, &ctx);
    NumberPicker_Draw(&g_ReplayCopy_BlockSpinner, ot, &ctx);
    Prim_AddDrawMode(ot, 0x20);
    MenuList_Draw(&g_ReplayCopy_List, ot4);
    CardDialog_Draw(&m->dlgComplete, ot, &ctx);
    m->handler(2, ot);
}
}
