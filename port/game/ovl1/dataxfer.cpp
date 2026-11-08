// OVL1 8001f508-80020dcc (dataxfer): GT1 and GT2 save data transfer.
// The resident card menu (exe/cardmenu.cpp) with fewer pages, and most of its states.
#include "gptr.h"
#include "common.h"

typedef int DataXferStateFn(int event, void *arg);

// The transfer's work area, in DataXferConfig's heap. Its start is the same as CardMenu's.
struct DataXferWork {
    short mode; // 0 trade, 1 mix records, 2 convert GT1
    short state;
    game_function<DataXferStateFn> handler;
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
    CardDialog dlgNoCars;
    CardProgressBar progress;
    game_pointer<GameSaveFile> file;
    unsigned char data[0x10000]; // the save read; a GT1 save starts at +0x200
};

extern "C" {
extern GameState (&g_GameState);
extern GameSaveFile (&g_SaveFileBuf);
extern game_pointer<DataXferWork> (&g_DataXfer);
extern Font (&g_DataXfer_Fonts); // medium
extern Font (&g_DataXfer_FontLarge);
extern Font (&g_DataXfer_FontSmall);
extern game_pointer<char> (&g_DataXfer_SlotNames)[];
extern game_function<DataXferStateFn> (&g_DataXfer_States)[];
extern int (&g_McOp_Remaining);
extern char (&D_80023D98)[]; // an empty string
extern char (&D_8004C6EC)[]; // BASCUS-94194GT, GT1's save file
extern WipeBar (&D_8004C6FC);
extern CardDialogDesc (&D_8004C718);
extern CardDialogDesc (&D_8004C748);
extern CardDialogDesc (&D_8004C778);
extern CardDialogDesc (&D_8004C7A8);
extern CardDialogDesc (&D_8004C7D8);
extern CardDialogDesc (&D_8004C808);
extern CardProgressBar (&D_8004C838);
extern char (&sB_MemoryCard1)[];
extern char (&sB_SelectASlot)[];
extern char (&sB_CheckingSlot)[];
extern char (&sB_NoMemoryCardDetected)[];
extern char (&sB_FailedToReadMemoryCard)[];
extern char (&sB_StartLoading)[];
extern char (&sB_Loading)[];
extern char (&sB_LoadingFailed)[];
extern char (&sB_LoadingDataFailed)[];
extern char (&sB_NoGameFileFound)[];
extern char (&s_PurchaseCarsFromSavedGameFiles)[];
extern char (&s_MixCourseLicenseRecords)[];
extern char (&s_ConvertGranTurismoGameData)[];
extern char (&s_CannotFindGranTurismoGameData)[];
extern char (&s_CannotFindCarsInGameFile)[];
extern char (&s_CombiningRecordsComplete)[];
extern char (&s_ConvertingDataComplete)[];
extern char (&s_LoadingDataFailed)[];
void DataXfer_DrawKanji(uint *ot, char *sjis, int x, int y, int center, uint rgb);
void *DataXfer_FindGt1File(void);

// Japanese only: empties the kanji cache and adds both slot names.
void DataXfer_CacheKanjiCommon(void)
{
    KanjiFont *font = g_DataXfer->kanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_Reset(font);
        KanjiFont_CacheString(font, sB_MemoryCard1);
        KanjiFont_CacheString(font, sB_MemoryCard1 + 0x11);
    }
}

// Japanese only: adds a string to the kanji cache.
void DataXfer_CacheKanji(char *sjis)
{
    KanjiFont *font = g_DataXfer->kanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_CacheString(font, sjis);
    }
}

// Draws SJIS text from the kanji cache, 12 pixels a character (centred on x if center is
// set), in Japanese only. CardMenu_DrawKanji without its draw mode.
void DataXfer_DrawKanji(uint *ot, char *sjis, int x, int y, int center, uint rgb)
{
    KanjiFont *font = g_DataXfer->kanjiFont;
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

// Japanese only: common strings and both message lines.
void DataXfer_CacheMessages(void)
{
    void *pvVar1;

    pvVar1 = g_DataXfer;
    if (g_GameState.language == '\0') {
        DataXfer_CacheKanjiCommon();
        DataXfer_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x28)));
        DataXfer_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x2c)));
    }
}

// Japanese only: common strings and the error text.
void DataXfer_CacheErrorText(void)
{
    void *pvVar1;

    pvVar1 = g_DataXfer;
    if (g_GameState.language == '\0') {
        DataXfer_CacheKanjiCommon();
        DataXfer_CacheKanji(*(game_pointer<char> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x24)));
    }
}

// State 6: "Start loading?"; loads the GT2 file (7), or back to the slot choice (2).
int DataXfer_State6_ConfirmGt2(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int r = m->dlgConfirm.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgConfirm;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = sB_StartLoading;
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
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

// State 7: reads the GT2 file, with the progress bar. If its CRC is right goes on to 8,
// or to 9 when trading and it has no cars.
int DataXfer_State7_LoadGt2(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int next = -1;

    switch (event) {
    case 0:
        if (SaveFile_StartRead(m->file, m->slot, m->file) != 0) {
            m->errorMsg = sB_LoadingDataFailed;
            DataXfer_CacheErrorText();
            next = 1;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = SaveFile_GetFileSize(m->file);
            bar->color = 0x90500c;
            CardProgress_Reset(bar);
            m->line1 = sB_Loading;
            m->line2 = sB_Loading - 0x151; // sB_PleaseDoNotRemoveMemoryCard
            DataXfer_CacheMessages();
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
                short *cars = (short *)SaveFile_GetGarages(m->file, m->file);

                switch (m->mode) {
                case 1:
                    next = 8;
                    break;
                case 0:
                    next = 9;
                    if (*cars > 0) {
                        next = 8;
                    }
                    break;
                }
            } else {
                m->errorMsg = sB_LoadingFailed;
                DataXfer_CacheErrorText();
                next = 1;
            }
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = sB_LoadingDataFailed;
            DataXfer_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// State 8: the result of mixing or converting; back to the slot choice (2), or leaves.
int DataXfer_State8_Complete(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int r = m->dlgComplete.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgComplete;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        switch (m->mode) {
        case 1:
            m->line1 = s_CombiningRecordsComplete;
            break;
        case 2:
            m->line1 = s_ConvertingDataComplete;
            break;
        default:
            return next;
        }
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = 13;
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

// State 9: the file has no cars; back to the slot choice (2), or leaves.
int DataXfer_State9_NoCars(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int r = m->dlgNoCars.result;
    int next = -1;

    switch (event) {
    case 0: {
        CardDialog *dlg;

        Sfx_PlayMenu(0);
        dlg = &m->dlgNoCars;
        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = s_CannotFindCarsInGameFile;
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = 13;
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

// A running sum in the high half and a CRC with polynomial 0x11021 in the low half,
// with each byte's bits shifted in.
unsigned int Gt1Save_Checksum(unsigned char *data, unsigned int size)
{
    unsigned int sum = 0xaaaa;
    unsigned int crc = 0x3770;
    unsigned int i;
    unsigned int byte;
    unsigned int bit;

    for (i = 0; i < size; i++) {
        byte = data[i];
        sum += byte;
        sum ^= byte << 8;
        for (bit = 0; bit < 8; bit++) {
            crc <<= 1;
            if (crc & 0x10000) {
                crc ^= 0x11021;
            }
            crc |= (byte >> 7) & 1;
            byte <<= 1;
        }
    }
    return (crc & 0xffff) | (sum << 16);
}

// Checksum of the first 0x6ba4 bytes matches the stored one.
int Gt1Save_Check(unsigned char *data)
{
    unsigned int uVar1;

    uVar1 = Gt1Save_Checksum(data, 0x6ba4);
    return *(unsigned int *)(data + 0x6ba4) == uVar1;
}

// State 10: "Start loading?"; loads the GT1 file (11), or back to the slot choice (2).
int DataXfer_State10_ConfirmGt1(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int r = m->dlgConfirm.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0: {
        CardDialog *dlg = &m->dlgConfirm;

        CardDialog_Open(dlg);
        dlg->choice = 1;
        m->line1 = sB_StartLoading;
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
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
            next = 11;
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

// State 11: reads 0xa000 bytes of the GT1 file, with the progress bar, and checks them.
int DataXfer_State11_LoadGt1(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int next = -1;

    switch (event) {
    case 0: {
        int size = 0xa000;

        if (McCard_StartTransfer(6, m->slot, D_8004C6EC, m->data, 0, size, 0) != 0) {
            m->errorMsg = sB_LoadingDataFailed;
            DataXfer_CacheErrorText();
            next = 1;
        } else {
            CardProgressBar *bar = &m->progress;

            bar->total = size;
            bar->color = 0x90500c;
            CardProgress_Reset(bar);
            m->line1 = sB_Loading;
            m->line2 = sB_Loading - 0x151; // sB_PleaseDoNotRemoveMemoryCard
            DataXfer_CacheMessages();
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
            if (Gt1Save_Check(m->data + 0x200)) {
                next = 8;
            } else {
                m->errorMsg = s_LoadingDataFailed;
                DataXfer_CacheErrorText();
                next = 1;
            }
            break;
        default:
            m->progress.visible = -1;
            m->errorMsg = sB_LoadingDataFailed;
            DataXfer_CacheErrorText();
            next = 1;
        }
        break;
    case 2:
        CardProgress_Draw(&m->progress, (uint *)arg);
        break;
    }
    return next;
}

// Waits 24 frames, then goes to state 2.
int DataXfer_State0_Delay(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
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
int DataXfer_State1_Error(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
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
        m->line1 = D_80023D98;
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
        case 1:
            next = 13;
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
        Text_SetFont(&ctx, &g_DataXfer_FontLarge);
        {
            TextCtx *t = &ctx;

            t->unk1c = 1;
            t->ot = ot;
            t->rgb = 0x6f6f6f;
        }
        if (g_GameState.language == 0) {
            DataXfer_DrawKanji(ot, m->errorMsg, 0xb0, 0xea, 1, 0x747474);
        } else {
            Text_DrawCentered(&ctx, (signed char *)m->errorMsg, 0xb0, 0xfa, 1);
        }
        break;
    }
    }
    return next;
}

// State 2: the mode's title and "Select a slot"; goes on to check the slot (3), or leaves.
int DataXfer_State2_SelectSlot(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int r = m->dlgSelectSlot.result;
    int next = -1;

    switch (event) {
    case 0: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            bar->t = ~bar->duration;
        }
        CardDialog_Open(&m->dlgSelectSlot);
        switch (m->mode) {
        case 0:
            m->line1 = s_PurchaseCarsFromSavedGameFiles;
            m->line2 = sB_SelectASlot;
            break;
        case 1:
            m->line1 = s_MixCourseLicenseRecords;
            m->line2 = sB_SelectASlot;
            break;
        case 2:
            m->line1 = s_ConvertGranTurismoGameData;
            m->line2 = sB_SelectASlot;
            break;
        default:
            m->line1 = sB_SelectASlot;
            m->line2 = D_80023D98;
            break;
        }
        DataXfer_CacheMessages();
        break;
    }
    case 1:
        switch (r) {
        case -1:
            next = 13;
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

// The GT1 save file on the chosen slot's card.
void *DataXfer_FindGt1File(void)
{
    return McCard_FindFile(g_DataXfer->slot, D_8004C6EC);
}

// State 3: checks the card in the slot, then goes on by slot status (0 a card, 1 checking,
// 2 no card, 4 read error): to the confirmation of the mode's file, or to 5 without one.
int DataXfer_State3_CheckSlot(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int next = -1;

    switch (event) {
    case 0:
        m->line1 = sB_CheckingSlot;
        m->slotBar.t = 0;
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
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
            DataXfer_CacheErrorText();
            next = 1;
            break;
        case 0:
            switch (m->mode) {
            case 0:
            case 1:
                next = 6;
                if (SaveFile_Find(m->file, m->slot) == 0) {
                    next = 5;
                }
                break;
            case 2:
                next = 10;
                if (DataXfer_FindGt1File() == 0) {
                    next = 5;
                }
                break;
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
int DataXfer_State4_NoCard(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
    int r = m->dlgNoCard.result;
    int next = -1;
    short *status;

    switch (event) {
    case 0:
        CardDialog_Open(&m->dlgNoCard);
        m->line1 = sB_NoMemoryCardDetected;
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
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
            next = 13;
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
int DataXfer_State5_NoFile(int event, void *arg)
{
    DataXferWork *m = g_DataXfer;
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
        if (m->mode == 2) {
            m->line1 = s_CannotFindGranTurismoGameData;
        } else {
            m->line1 = sB_NoGameFileFound;
        }
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
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
            next = 13;
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
int DataXfer_StateIdle(int event, void *ot)
{
    return -1;
}

// Runs the new state's event 0; a negative state is idle.
int DataXfer_SetState(int state)
{
    DataXferWork *m = g_DataXfer;

    if (state < 0) {
        m->handler = DataXfer_StateIdle;
    } else {
        m->handler = g_DataXfer_States[state];
    }
    m->state = state;
    return m->handler(0, 0);
}

// Copies the slot bar and the progress bar and builds the dialogs.
void DataXfer_InitWidgets(void)
{
    DataXferWork *m = g_DataXfer;
    CardDialog *dlg;

    m->slotBar = D_8004C6FC;
    m->slotBar.t = -1;
    CardDialog_Init(&m->dlgError, &D_8004C718, 0);
    dlg = &m->dlgSelectSlot;
    CardDialog_Init(dlg, &D_8004C748, 0);
    dlg->choice = 1;
    CardDialog_Init(&m->dlgNoCard, &D_8004C778, 0);
    CardDialog_Init(&m->dlgComplete, &D_8004C7A8, 0);
    CardDialog_Init(&m->dlgNoCars, &D_8004C7D8, 0);
    m->progress = D_8004C838;
    CardDialog_Init(&m->dlgConfirm, &D_8004C808, 0);
    m->line1 = D_80023D98;
    m->line2 = D_80023D98;
    DataXfer_CacheMessages();
}

// Sets up the transfer in config's work area; returns the end of the area.
void *DataXfer_Setup(DataXferConfig *config)
{
    DataXferWork *m = (DataXferWork *)rt::from_guest(config->heap);

    g_DataXfer_FontSmall = *config->fontSmall;
    g_DataXfer_Fonts = *config->fontMedium;
    g_DataXfer_FontLarge = *config->fontLarge;
    m->kanjiFont = config->font;
    m->slotStatus = config->cardSlots;
    g_DataXfer = m;
    m->textPri = config->textPri;
    return m + 1;
}

// mode 0 trade, 1 mix records, 2 convert GT1. Prepares the save header template, the
// widgets, then state 0.
void DataXfer_Start(short mode)
{
    void *pvVar1;
    GameSaveFile *file;

    pvVar1 = g_DataXfer;
    *(short *)g_DataXfer = mode;
    file = (GameSaveFile *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x41c));
    *(game_pointer<GameSaveFile> *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x418)) = file;
    SaveFile_InitHeader(file);
    DataXfer_InitWidgets();
    DataXfer_SetState(0);
}

// Each frame: updates the widgets, then runs the state and the states it moves to.
// Returns 2 when the transfer's file is ready (state 8; mode 0 leaves here, to pick the
// cars), 1 when the player left (state 13), 0 otherwise.
int DataXfer_Update(PadState *pad)
{
    DataXferWork *m = g_DataXfer;
    int result = 0;
    int r;

    WipeBar_Update(&m->slotBar);
    m->dlgError.result = CardDialog_Update(&m->dlgError, pad);
    m->dlgSelectSlot.result = CardDialog_Update(&m->dlgSelectSlot, pad);
    m->dlgNoCard.result = CardDialog_Update(&m->dlgNoCard, pad);
    m->dlgComplete.result = CardDialog_Update(&m->dlgComplete, pad);
    m->dlgNoCars.result = CardDialog_Update(&m->dlgNoCars, pad);
    m->dlgConfirm.result = CardDialog_Update(&m->dlgConfirm, pad);
    r = m->handler(1, 0);
next:
    switch (r) {
    case 8:
        switch (m->mode) {
        case 0: {
            WipeBar *bar = &m->slotBar;

            if (bar->t >= 0) {
                bar->t = ~bar->duration;
            }
            m->line1 = D_80023D98;
            m->line2 = D_80023D98;
            DataXfer_CacheMessages();
            DataXfer_SetState(-1);
            result = 2;
            break;
        }
        case 1:
            DataXfer_SetState(8);
            result = 2;
            break;
        case 2:
            DataXfer_SetState(8);
            result = 2;
            break;
        }
        break;
    case 13: {
        WipeBar *bar = &m->slotBar;

        if (bar->t >= 0) {
            bar->t = ~bar->duration;
        }
        m->line1 = D_80023D98;
        m->line2 = D_80023D98;
        DataXfer_CacheMessages();
        DataXfer_SetState(-1);
        result = 1;
        break;
    }
    case -1:
        break;
    default:
        r = DataXfer_SetState(r);
        goto next;
    }
    return result;
}

// Each frame: the two text lines, the slot bar with its slot's name, the dialogs, then the
// state's own drawing (event 2).
void DataXfer_Draw(unsigned int *ot)
{
    DataXferWork *m = g_DataXfer;
    TextCtx ctx;
    uint from;
    uint to;
    int level;

    from = 0x2000000;
    to = 0x242362a;
    Text_Init(&ctx, m->textPri);
    Text_SetFont(&ctx, &g_DataXfer_FontLarge);
    {
        TextCtx *t = &ctx;
        int center;

        *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
        t->unk1c = 1;
        t->ot = ot;
        t->rgb = 0x25c5248;

        center = 1;
        if (g_GameState.language == 0) {
            DataXfer_DrawKanji(ot, m->line1, 0xb0, 0xc6, center, 0x747474);
            DataXfer_DrawKanji(ot, m->line2, 0xb0, 0xea, center, 0x747474);
        } else {
            Text_DrawCentered(t, (signed char *)m->line1, 0xb0, 0xd6, center);
            Text_DrawCentered(t, (signed char *)m->line2, 0xb0, 0xfa, center);
        }
    }
    level = WipeBar_GetLevel(&m->slotBar);
    if (level != 0) {
        if (g_GameState.language == 0) {
            DataXfer_DrawKanji(ot, g_DataXfer_SlotNames[m->slot], 0x1c, 0x66, 0, level | level << 8 | level << 16);
        } else {
            TextCtx *t = &ctx;

            t->rgb = Rgb_Lerp((uchar *)&from, (uchar *)&to, level, 0x80);
            Text_DrawString(t, (signed char *)g_DataXfer_SlotNames[m->slot], 0x1c, 0x7e, 1);
        }
        WipeBar_Draw(&m->slotBar, ot, 0x18, 0x6e);
        Prim_AddDrawMode(ot, 0x220);
    }
    CardDialog_Draw(&m->dlgError, ot, &ctx);
    CardDialog_Draw(&m->dlgSelectSlot, ot, &ctx);
    CardDialog_Draw(&m->dlgNoCard, ot, &ctx);
    CardDialog_Draw(&m->dlgComplete, ot, &ctx);
    CardDialog_Draw(&m->dlgNoCars, ot, &ctx);
    CardDialog_Draw(&m->dlgConfirm, ot, &ctx);
    m->handler(2, ot);
}

// Cars of the loaded GT2 file (count, then Car records of 0xa4 from +4), via
// SaveFile_GetGarages.
void *DataXfer_GetCarList(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetGarages(*(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)),
                                                              *(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)));
    return pvVar1;
}

// Course records of the loaded file (0x80 x 0x24).
void *DataXfer_GetCourseRecords(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetCourseRecords
                                          (*(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)),
                                            *(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)));
    return pvVar1;
}

// License tests of the loaded file (6 x 10 x 0xa4).
void *DataXfer_GetLicenseTests(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetLicenses
                                          (*(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)),
                                            *(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)));
    return pvVar1;
}

void *DataXfer_GetMachineTest0(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetMachineTest0
                                          (*(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)),
                                            *(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)));
    return pvVar1;
}

void *DataXfer_GetMachineTest1(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetMachineTest1
                                          (*(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)),
                                            *(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)));
    return pvVar1;
}

void *DataXfer_GetMachineTest2(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetMachineTest2
                                          (*(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)),
                                            *(game_pointer<GameSaveFile> *)rt::from_guest(((int)g_DataXfer + 0x418)));
    return pvVar1;
}

// GT1 save data (+0x61c of the work area, after the card header).
unsigned char *DataXfer_GetGt1Data(void)
{
    return (unsigned char *)rt::from_guest(((int)g_DataXfer + 0x61c));
}
}
