// OVL2 8002445c-80025614 (guestload): Guest garage loading.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"

typedef int GuestLoadStateFn(int event, void *arg);

// The guest load's work area, in ArcGuestLoadConfig's buffer. DataXfer's (ovl1/dataxfer.cpp)
// with a dialog less and a smaller buffer.
struct GuestLoadWork {
    short state;
    short pad02;
    game_function<GuestLoadStateFn> handler;
    game_pointer<KanjiFont> kanjiFont;
    game_pointer<short> slotStatus;
    short slot;
    unsigned char pad12[0x12];
    game_pointer<char> errorMsg;
    game_pointer<char> line1;
    game_pointer<char> line2;
    short textPri;
    short timer;
    WipeBar slotBar;
    CardDialog dlgError;
    CardDialog dlgSelectSlot;
    CardDialog dlgNoCard;
    CardDialog dlgComplete;
    CardDialog dlgConfirm;
    CardProgressBar progress;
    game_pointer<GameSaveFile> file;
    unsigned char data[0x8000];
};

extern "C" {
extern GameState (&g_GameState);
extern game_pointer<GuestLoadWork> (&g_GuestLoad);
extern Font (&D_800F39F0); // medium
extern Font (&D_800F3A00); // large
extern Font (&D_800F3A10); // small
extern game_pointer<char> (&D_800529DC)[]; // the slots' names
extern game_function<GuestLoadStateFn> (&g_GuestLoad_States)[];
extern int (&g_McOp_Remaining);
extern char (&D_8002746C)[]; // an empty string
extern char (&D_800F879E)[]; // the file has no cars
extern WipeBar (&D_80052898);
extern CardDialogDesc (&D_800528B4);
extern CardDialogDesc (&D_800528E4);
extern CardDialogDesc (&D_80052914);
extern CardDialogDesc (&D_80052944);
extern CardDialogDesc (&D_80052974);
extern CardProgressBar (&D_800529A4);
extern char (&sB_MemoryCard1)[];
extern char (&sB_SelectASlot)[];
extern char (&sB_CheckingSlot)[];
extern char (&sB_NoMemoryCardDetected)[];
extern char (&sB_FailedToReadMemoryCard)[];
extern char (&sB_StartLoading)[];
extern char (&sB_Loading)[];
extern char (&sB_LoadingFailed)[];
extern char (&sB_LoadingDataFailed)[];
extern char (&sB_LoadingComplete)[];
extern char (&sB_NoGameFileFound)[];
void GuestLoad_DrawKanji(uint *ot, char *sjis, int x, int y, int center, uint rgb);

// Japanese only: empties the kanji cache and adds both slot names.
void GuestLoad_CacheKanjiCommon(void)
{
    KanjiFont *font = g_GuestLoad->kanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_Reset(font);
        KanjiFont_CacheString(font, sB_MemoryCard1);
        KanjiFont_CacheString(font, sB_MemoryCard1 + 0x11);
    }
}

// Japanese only: adds a string to the kanji cache.
void GuestLoad_CacheKanji(char *sjis)
{
    KanjiFont *font = g_GuestLoad->kanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_CacheString(font, sjis);
    }
}

// Draws SJIS text from the kanji cache, 12 pixels a character (centred on x if center is
// set), in Japanese only. CardMenu_DrawKanji without its draw mode.
void GuestLoad_DrawKanji(uint *ot, char *sjis, int x, int y, int center, uint rgb)
{
    KanjiFont *font = g_GuestLoad->kanjiFont;
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
        KanjiGlyph *glyph = KanjiFont_FindGlyph(font, p[0] << 8 | p[1]);

        if (glyph != 0) {
            PrimFT4 *q = (PrimFT4 *)KanjiFont_AddGlyphQuad(glyph, ot, rgb | semi);

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

// Copy of DataXfer_CacheMessages.
void GuestLoad_CacheMessages(void)
{
    void *pvVar1;

    pvVar1 = g_GuestLoad;
    if (g_GameState.language == '\0') {
        GuestLoad_CacheKanjiCommon();
        GuestLoad_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x28)));
        GuestLoad_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x2c)));
    }
}

// Copy of DataXfer_CacheErrorText.
void GuestLoad_CacheErrorText(void)
{
    void *pvVar1;

    pvVar1 = g_GuestLoad;
    if (g_GameState.language == '\0') {
        GuestLoad_CacheKanjiCommon();
        GuestLoad_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x24)));
    }
}

// State 6: "Start loading?"; loads the file (7), or back to the slot choice (2).
int GuestLoad_State6_Confirm(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
    int r = m->dlgConfirm.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgConfirm;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = sB_StartLoading;
        m->line2 = D_8002746C;
        GuestLoad_CacheMessages();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        if (*status != 0) {
            CardDialog_Close(&m->dlgConfirm);
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
            next = 7;
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

// State 7: reads the file, with the progress bar. If its CRC is right, moves its garage
// to the second one and goes on to 8.
int GuestLoad_State7_Read(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
    int next = -1;

    switch (event) {
    case 0:
        if (SaveFile_StartRead(m->file, m->slot, m->file) != 0) {
            m->errorMsg = sB_LoadingDataFailed;
            GuestLoad_CacheErrorText();
            next = 1;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = SaveFile_GetFileSize(m->file);
            bar->color = 0x90500c;
            CardProgress_Reset(bar);
            m->line1 = sB_Loading;
            m->line2 = sB_Loading - 0x151; // sB_PleaseDoNotRemoveMemoryCard
            GuestLoad_CacheMessages();
        }
        break;
    case 1:
        switch (McCard_GetResult()) {
        case 1:
            CardProgress_Update(&m->progress, g_McOp_Remaining);
            break;
        case 0:
            m->progress.visible = -1;
            if (SaveFile_CheckCrc(m->file, m->file) != 0) {
                next = 8;
                SaveFile_CopyGarageToPlayer2(m->file, m->file);
            } else {
                m->errorMsg = sB_LoadingFailed;
                GuestLoad_CacheErrorText();
                next = 1;
            }
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = sB_LoadingDataFailed;
            GuestLoad_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// State 8: says if the file has cars; back to the slot choice (2), or leaves.
int GuestLoad_State8_Complete(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
    int r = m->dlgComplete.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgComplete;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        if (*(short *)SaveFile_GetGarages(m->file, m->file) > 0) {
            m->line1 = sB_LoadingComplete;
        } else {
            m->line1 = D_800F879E;
        }
        m->line2 = D_8002746C;
        GuestLoad_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
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

// Waits 24 frames, then goes to state 2.
int GuestLoad_State0_Delay(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
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

// State 1: the error dialog, with errorMsg drawn above it; back to the slot choice (2),
// or leaves.
int GuestLoad_State1_Error(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
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
        m->line1 = D_8002746C;
        m->line2 = D_8002746C;
        GuestLoad_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = 10;
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
        Text_SetFont(&ctx, &D_800F3A00);
        {
            TextCtx *t = &ctx;

            t->unk1c = 1;
            t->ot = ot;
            t->rgb = 0x6f6f6f;
        }
        if (g_GameState.language == 0) {
            GuestLoad_DrawKanji(ot, m->errorMsg, 0xb0, 0xea, 1, 0x606060);
        } else {
            Text_DrawCentered(&ctx, (signed char *)m->errorMsg, 0xb0, 0xfa, 1);
        }
        break;
    }
    }
    return next;
}

// State 2: "Select a slot"; goes on to check the slot (3), or leaves.
int GuestLoad_State2_SelectSlot(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
    int r = m->dlgSelectSlot.result;
    int next = -1;

    switch (event) {
    case 0: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            bar->t = ~bar->duration;
        }
        CardDialog_Open(&m->dlgSelectSlot);
        m->line1 = sB_SelectASlot;
        m->line2 = D_8002746C;
        GuestLoad_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
            next = 10;
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

// State 3: checks the card in the slot, then goes on by slot status (0 a card, 1 checking,
// 2 no card, 4 read error): to the confirmation, or to 5 without a file.
int GuestLoad_State3_CheckSlot(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
    int next = -1;

    switch (event) {
    case 0:
        m->line1 = sB_CheckingSlot;
        m->slotBar.t = 0;
        m->line2 = D_8002746C;
        GuestLoad_CacheMessages();
        // fall through
    case 1: {
        short *status = m->slotStatus + m->slot;

        switch (*status) {
        case 1:
            break;
        case 2:
            next = 4;
            break;
        case 4:
            m->errorMsg = sB_FailedToReadMemoryCard;
            GuestLoad_CacheErrorText();
            next = 1;
            break;
        case 0:
            next = 6;
            if (SaveFile_Find(m->file, m->slot) == 0) {
                next = 5;
            }
            break;
        default:
            next = 5;
            break;
        }
        break;
    }
    case 2:
        break;
    }
    return next;
}

// "No memory card detected", until the slot's status changes (3).
int GuestLoad_State4_NoCard(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
    int r = m->dlgNoCard.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0:
        CardDialog_Open(&m->dlgNoCard);
        m->line1 = sB_NoMemoryCardDetected;
        m->line2 = D_8002746C;
        GuestLoad_CacheMessages();
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

// "No file found", until a card with no error is out of the slot (3).
int GuestLoad_State5_NoFile(int event, void *arg)
{
    GuestLoadWork *m = g_GuestLoad;
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
        m->line1 = sB_NoGameFileFound;
        m->line2 = D_8002746C;
        GuestLoad_CacheMessages();
        break;
    }
    case 1:
        status = m->slotStatus + m->slot;
        st = *status;
        if (st == 2 || st == 4) {
            CardDialog_Close(&m->dlgError);
            next = 3;
            break;
        }
        switch (r) {
        case -1:
        case 1:
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

// Returns -1.
int GuestLoad_StateIdle(int event, void *ot)
{
    return -1;
}

// Runs the new state's event 0; a negative state is idle.
int GuestLoad_SetState(int state)
{
    GuestLoadWork *m = g_GuestLoad;

    if (state < 0) {
        m->handler = GuestLoad_StateIdle;
    } else {
        m->handler = g_GuestLoad_States[state];
    }
    m->state = state;
    return m->handler(0, 0);
}

// Copies the slot bar and the progress bar and builds the dialogs.
void GuestLoad_InitWidgets(void)
{
    GuestLoadWork *m = g_GuestLoad;
    CardDialog *dlg;

    m->slotBar = D_80052898;
    m->slotBar.t = -1;
    CardDialog_Init(&m->dlgError, &D_800528B4, 0);
    dlg = &m->dlgSelectSlot;
    CardDialog_Init(dlg, &D_800528E4, 0);
    dlg->choice = 1;
    CardDialog_Init(&m->dlgNoCard, &D_80052914, 0);
    CardDialog_Init(&m->dlgComplete, &D_80052944, 0);
    m->progress = D_800529A4;
    CardDialog_Init(&m->dlgConfirm, &D_80052974, 0);
    m->line1 = D_8002746C;
    m->line2 = D_8002746C;
    GuestLoad_CacheMessages();
}

// Sets up the transfer in config's work area; returns the end of the area.
void *GuestLoad_Setup(ArcGuestLoadConfig *config)
{
    GuestLoadWork *m = (GuestLoadWork *)config->buffer;

    D_800F3A10 = *config->fontSmall;
    D_800F39F0 = *config->fontMedium;
    D_800F3A00 = *config->fontLarge;
    m->kanjiFont = (KanjiFont *)config->font;
    m->slotStatus = config->slots;
    g_GuestLoad = m;
    m->textPri = config->textPri;
    return m + 1;
}

// Prepares the save file buffer (work +0x384), the widgets, then state 0.
void GuestLoad_Start(void)
{
    GameSaveFile *file;

    file = (GameSaveFile *)rt::from_guest(((int)g_GuestLoad + 900));
    *(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_GuestLoad + 0x380)) = file;
    SaveFile_InitHeader(file);
    GuestLoad_InitWidgets();
    GuestLoad_SetState(0);
}

// Each frame: updates the widgets, then runs the state and the states it moves to.
// Returns 3 when the file is read (state 8), 1 when the player left (state 10), 0
// otherwise.
int GuestLoad_Update(PadState *pad)
{
    GuestLoadWork *m = g_GuestLoad;
    int result = 0;
    int r;

    WipeBar_Update(&m->slotBar);
    m->dlgError.result = CardDialog_Update(&m->dlgError, pad);
    m->dlgSelectSlot.result = CardDialog_Update(&m->dlgSelectSlot, pad);
    m->dlgNoCard.result = CardDialog_Update(&m->dlgNoCard, pad);
    m->dlgComplete.result = CardDialog_Update(&m->dlgComplete, pad);
    m->dlgConfirm.result = CardDialog_Update(&m->dlgConfirm, pad);
    r = m->handler(1, 0);
next:
    switch (r) {
    case 8:
        GuestLoad_SetState(8);
        result = 3;
        break;
    case 10: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            bar->t = ~bar->duration;
        }
        m->line1 = D_8002746C;
        m->line2 = D_8002746C;
        GuestLoad_CacheMessages();
        GuestLoad_SetState(-1);
        result = 1;
        break;
    }
    case -1:
        break;
    default:
        r = GuestLoad_SetState(r);
        goto next;
    }
    return result;
}

// Each frame: the two text lines, the slot bar with its slot's name, the dialogs, then the
// state's own drawing (event 2).
void GuestLoad_Draw(unsigned int *ot)
{
    GuestLoadWork *m = g_GuestLoad;
    TextCtx ctx;
    uint from;
    uint to;
    int level;

    from = 0x2000000;
    to = 0x242362a;
    Text_Init(&ctx, m->textPri);
    Text_SetFont(&ctx, &D_800F3A00);
    {
        TextCtx *t = &ctx;
        int center;

        *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
        t->unk1c = 1;
        t->ot = ot;
        t->rgb = 0x25c5248;

        center = 1;
        if (g_GameState.language == 0) {
            GuestLoad_DrawKanji(ot, m->line1, 0xb0, 0xc6, center, 0x606060);
            GuestLoad_DrawKanji(ot, m->line2, 0xb0, 0xea, center, 0x606060);
        } else {
            Text_DrawCentered(t, (signed char *)m->line1, 0xb0, 0xd6, center);
            Text_DrawCentered(t, (signed char *)m->line2, 0xb0, 0xfa, center);
        }
    }
    level = WipeBar_GetLevel(&m->slotBar);
    if (level != 0) {
        if (g_GameState.language == 0) {
            GuestLoad_DrawKanji(ot, D_800529DC[m->slot], 0x1c, 0x66, 0, level | level << 8 | level << 16);
        } else {
            TextCtx *t = &ctx;

            t->rgb = Rgb_Lerp((uchar *)&from, (uchar *)&to, level, 0x80);
            Text_DrawString(t, (signed char *)D_800529DC[m->slot], 0x1c, 0x7e, 1);
        }
        WipeBar_Draw(&m->slotBar, ot, 0x18, 0x6e);
        Prim_AddDrawMode(ot, 0x220);
    }
    CardDialog_Draw(&m->dlgError, ot, &ctx);
    CardDialog_Draw(&m->dlgSelectSlot, ot, &ctx);
    CardDialog_Draw(&m->dlgNoCard, ot, &ctx);
    CardDialog_Draw(&m->dlgComplete, ot, &ctx);
    CardDialog_Draw(&m->dlgConfirm, ot, &ctx);
    m->handler(2, ot);
}
}
