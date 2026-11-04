// OVL0 8004a718-8004c728 (timetrialmenu): Time trial menu pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline char *GameState_Base(void)
{
    return (char *)&g_GameState;
}

static inline char *RaceSetup_CourseName(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf9c;

    return base + offset;
}

// The ghost's flags in g_GameState (0xbcc4): byte 6 is set when one is loaded.
static inline signed char *GhostState_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbcc4;

    return (signed char *)(base + offset);
}

// The race's outcome in g_GameState (0xc508).
struct SessionOutcome {
    signed char unk0;
    signed char isRecord;   // 1: the session's best is a new course record
};

static inline RaceRecord *Session_Record(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xc5a8;

    return (RaceRecord *)(base + offset);
}

// How InfoBar_Init sets up a bar.
struct InfoBarDesc {
    uint unk00;
    uint unk04;
    char *label;
    uint labelRgb;
    uint value;
    uint valueRgb;
    ushort kind;
    ushort pad1a;
    ushort width;
    ushort height;
};

// A row of the time trial menu.
struct TimeTrialOption {
    uint unk00;
    char *label;
    signed char enabled;
    signed char next;
    unsigned char pad0a[2];
};

// The pages' working memory.
struct TimeTrialWork {
    int fromResults;                // came by the results: leaving goes on to the menu
    TitleBanner sessionBanner;      // 0x04
    unsigned char pad48[0xf0];
    short rowCount;                 // 0x138
    short pad13a;
    unsigned char rows[11][0x20];   // 0x13c: a RecordRow per lap, then the session's best
    WipeBar totalBar;               // 0x29c
    InfoBar totalInfo;              // 0x2b8
    unsigned char bestRow[0x20];    // 0x308: the course record
    WipeBar bestBar;                // 0x328
    InfoBar bestInfo;               // 0x344
    short nameX;                    // 0x394
    short pad396;
    InfoBar carInfo;                // 0x398
    InfoBar driverInfo;             // 0x3e8
    GrowBar grow;                   // 0x438
    TextEntry entry;                // 0x44c
    ResultCar car;                  // 0x4c4
    ResultCarView view;             // 0x608
    TitleBanner banner;             // 0x6e4
    TextItem items[16];             // 0x728
    WipeBar bars[16];               // 0x9a8
    unsigned char cardWork[1];      // 0xb68
};

extern "C" {
extern QuickMenu *g_QuickMenu;
extern TimeTrialWork *g_OVL0_EndFontInfo;
extern signed char g_TimeTrial_FreshResult;
extern signed char g_TimeTrial_SettingsUsed;
extern void *g_TimeTrial_CardWork;
extern short g_GhostTypeList_Blink;
extern unsigned char g_TT_GhostReady;
extern unsigned char D_801C9995;    // the ghost setting
extern MenuList g_SessionResults_List;
extern MenuList g_TimeTrialMenu_List;
extern MenuList g_GhostTypeList;
extern TimeTrialOption g_TimeTrialMenu_Options[];
extern char *g_GhostTypeNames[];
extern QuickMenuPage g_Page_TimeTrialName;
extern QuickMenuPage g_Page_TimeTrialMenu;
extern QuickMenuPage g_Page_TimeTrialQuit;
extern QuickMenuPage g_Page_SessionResults;
extern QuickMenuPage g_Page_ChangeParts;
extern QuickMenuPage g_Page_SaveReplay;
extern QuickMenuPage g_Page_SaveGhost;
extern QuickMenuPage g_Page_LoadGhost;
extern TextEntry D_8005AF68;
extern WipeBar D_8005AECC;
extern InfoBarDesc D_8005AEE8;
extern InfoBarDesc D_8005AF08;
extern InfoBarDesc D_8005AF28;
extern InfoBarDesc D_8005AF48;
extern GrowBar D_8005AEB8;
extern TextStyle D_8005AFE4;
extern WipeBar D_8005B000;
extern unsigned char D_8005AE78[];
extern unsigned char D_8005AE7C[];
extern unsigned char D_8005AE80[];
extern void *D_800A9F00;
extern Font g_OVL0_FontB;
extern Font g_OVL0_FontD;
extern char sR_CAR[];
extern char sR_SectorN[];
extern char sR_Lap[];
extern char sR_Total[];
extern char sR_Mph[];
void QuickMenu_PushPage(QuickMenu *menu, QuickMenuPage *page);
void QuickMenu_SetupCardUi2(void *work);
int SessionResults_ItemCallback(MenuListEventId event, MenuListEvent *e, int i);
int GhostTypeList_ItemCallback(MenuListEventId event, MenuListEvent *e, int i);
void InfoBar_Init(InfoBar *bar, InfoBarDesc *desc);
void InfoBar_Draw(InfoBar *bar, uint *ot, int x, int y, TextCtx *ctx);
void TitleBanner_Init(TitleBanner *banner, char *text, TextCtx *ctx);
void ResultCarView_Init(ResultCarView *view, int w, int h);

static inline void WipeBar_Hide(WipeBar *bar)
{
    bar->t = ~bar->duration;
}

static inline void InfoBar_Show(InfoBar *bar)
{
    bar->bar.t = 0;
    TextItem_Show(&bar->label, -1);
    bar->anim = 0;
}

static inline void InfoBar_Hide(InfoBar *bar, int anim)
{
    bar->bar.t = ~bar->bar.duration;
    TextItem_Hide(&bar->label);
    bar->anim = anim;
}

// Whether a new result waits to be shown; a new session has no ghost loaded.
void TimeTrial_SetFreshResult(unsigned char fresh)
{
    g_TimeTrial_FreshResult = fresh;
    GhostState_Get()[6] = 0;
}

int TimeTrial_IsGhostLoaded(void)
{
    return GhostState_Get()[6] != 0;
}

// Root page of the time trial: stores back, 20-frame delay, card UI on the work area
// after the settings work.
void TimeTrialRoot_Enter(int back)
{
    QuickMenuPage *page = g_QuickMenu->activePage;
    TimeTrialWork *work = g_OVL0_EndFontInfo;
    void *card;

    page->sel = back;
    page->timer = 0x14;
    card = Settings_SetWork(work->cardWork);
    g_TimeTrial_CardWork = card;
    QuickMenu_SetupCardUi(card);
    QuickMenu_SetupCardUi2(card);
}

// After the wait: leaves on sel 1, or the new result (with the name page for a
// record), or the menu.
int TimeTrialRoot_Update(int active)
{
    QuickMenuPage *page = g_QuickMenu->activePage;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            int one = 1;

            if (page->sel == one) {
                result = 4;
            } else {
                if (g_TimeTrial_FreshResult != 0) {
                    QuickMenu_PlayMusic(8);
                    g_OVL0_EndFontInfo->fromResults = one;
                    {
                        char *base = (char *)&g_GameState;
                        int offset = 0xc508;
                        SessionOutcome *outcome = (SessionOutcome *)(base + offset);

                        int isRecord = outcome->isRecord;

                        if ((isRecord ^ 1) == 0) {
                            QuickMenu_PushPage(g_QuickMenu, &g_Page_TimeTrialName);
                        } else {
                            QuickMenu_PushPage(g_QuickMenu, &g_Page_SessionResults);
                        }
                    }
                } else {
                    QuickMenu_PlayMusic(8);
                    QuickMenu_PushPage(g_QuickMenu, &g_Page_TimeTrialMenu);
                }
                result = 1;
            }
        }
    }
    return result;
}

// Empty.
void TimeTrialRoot_Draw(DrawLayer *layer)
{
}

// Same as QuitPage_Enter, for the time trial menu.
void TimeTrialQuit_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x14;
    QuickMenu_StopMusic();
}

// Returns 4 when the timer runs out.
int TimeTrialQuit_Update(int active)
{
    unsigned int uVar1;

    uVar1 = (unsigned short)g_QuickMenu->activePage->timer - 1;
    g_QuickMenu->activePage->timer = (short)uVar1;
    return uVar1 >> 0xd & 4;
}

// Empty.
void TimeTrialQuit_Draw(DrawLayer *layer)
{
}

// The name entry for a course record, on the name kept in g_GameState.
void TimeTrialName_Enter(int back)
{
    QuickMenuPage *page = g_QuickMenu->activePage;
    TextEntry *entry;

    page->timer = 0x18;
    TextEntry_Init(&g_OVL0_EndFontInfo->entry, &D_8005AF68);
    entry = &g_OVL0_EndFontInfo->entry;
    entry->text = GameState_Base() + 0x7c8f;
    entry->maxLen = 0xb;
    entry->maxWidth = 0x100;
}

#ifdef NON_MATCHING
// Runs the entry; when done, stores the name and car in the course record.
int TimeTrialName_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    PadState *pad = menu->padState;
    QuickMenuPage *page = menu->activePage;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            TextEntry *entry;

            TextEntry_Open(&g_OVL0_EndFontInfo->entry);
            entry = &g_OVL0_EndFontInfo->entry;
            entry->cursorPos = strlen(entry->text);
        }
    }
    switch (TextEntry_Update(&g_OVL0_EndFontInfo->entry, pad)) {
    case -1:
        Sfx_PlayMenu(0);
        break;
    case 0:
    {
        char *base;
        CourseRecord *record;

        Sfx_PlayMenu(3);
        record = CourseRecord_GetCurrent();
        base = (char *)&g_GameState;
        strcpy(record->name, base + 0x7c8f);
        {
            CourseRecord *record = CourseRecord_GetCurrent();
            RaceSetup *setup;

            result = 1;
            setup = (RaceSetup *)(base + 0xbf7c);
            record->carId = setup->cars[0].carId;
            strcpy((char *)setup->unk53c, setup->cars[0].name);
        }
        TextEntry_Close(&g_OVL0_EndFontInfo->entry);
        QuickMenu_PushPage(g_QuickMenu, &g_Page_SessionResults);
        break;
    }
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialName_Update);
#endif

// TextEntry_Draw.
void TimeTrialName_Draw(DrawLayer *layer)
{
    TextEntry_Draw(&g_OVL0_EndFontInfo->entry, (uint *)layer->ot);
}

#ifdef NON_MATCHING
// A lap's row: drawn while the list shows, its fade scaled by the row's brightness.
int SessionResults_ItemCallback(MenuListEventId event, MenuListEvent *e, int i)
{
    TextCtx ctx;
    int result = 0;
    void *ot = e->ot;
    int y = e->y + 8;
    MenuList *list = e->list;
    TextCtx *c;

    Text_Init(&ctx, 6);
    c = &ctx;
    c->unk1c = 1;
    switch (event) {
    case MLE_DRAW:
    {
        int state = list->state;
        int shown = state >= 0;
        int closing = state < -1;

        if (shown | closing) {
            int fade = (list->fade << 7) / list->fadeMax;

            RecordRow_Draw(g_OVL0_EndFontInfo->rows[i], (uint *)ot, c, 0x22, y, e->brightness * fade >> 7);
        }
        break;
    }
    case MLE_IS_ENABLED:
        result = 1;
        break;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", SessionResults_ItemCallback);
#endif

// The best time of each of the four sectors, over the kept laps and the session's best.
void Records_FindBestSectors(uint *best)
{
    RaceRecord *record = Session_Record();
    uint none = (uint)-1;
    int i;
    int j;

    for (i = 3; i >= 0; i--) {
        best[i] = none;
    }
    for (i = 0; i < record->lapsKept; i++) {
        for (j = 0; j < 4; j++) {
            uint time = Records_GetSectorTime(&record->laps[i], j);

            if (time < best[j]) {
                best[j] = time;
            }
        }
    }
    for (j = 0; j < 4; j++) {
        uint time = Records_GetSectorTime(&record->best, j);

        if (time < best[j]) {
            best[j] = time;
        }
    }
}

// A row's colours: the label, the four sectors (the best ones stand out) and the total.
void RecordRow_SetColors(uint *colors, int current, TimeRecord *times, uint *best)
{
    int i;

    colors[0] = 0x214465a;
    colors[5] = 0x25a5a5a;
    if (current != 0) {
        colors[5] = 0x2013060;
    }
    for (i = 0; i < 4; i++) {
        colors[i + 1] = 0x25a5a5a;
        if (best[i] >= (uint)Records_GetSectorTime(times, i)) {
            colors[i + 1] = 0x2013060;
        }
    }
}

// The kept laps, the session's best, the course record, and the bars.
void SessionResults_Enter(int back)
{
    uint best[4];
    uint colors[6];
    TextCtx ctx;
    RaceRecord *record = Session_Record();
    int bestLap = record->bestLap;
    int firstLap;
    int i;
    int width;
    int other;

    TitleBanner_Init(&g_OVL0_EndFontInfo->sessionBanner, RaceSetup_CourseName(), &g_QuickMenu->text);
    g_OVL0_EndFontInfo->sessionBanner.bar.t = 0;
    TextItem_Show(&g_OVL0_EndFontInfo->sessionBanner.text, -1);
    Records_FindBestSectors(best);
    {
        TimeTrialWork *work = g_OVL0_EndFontInfo;
        int kept = record->lapsKept;

        work->rowCount = kept;
    }
    firstLap = record->lapCount - record->lapsKept;
    for (i = 0; i < g_OVL0_EndFontInfo->rowCount; i++) {
        int lap = firstLap + i;

        RecordRow_SetColors(colors, lap == bestLap, &record->laps[i], best);
        RecordRow_Init(g_OVL0_EndFontInfo->rows[i], lap + 1, &record->laps[i], colors);
    }
    RecordRow_SetColors(colors, 1, &record->best, best);
    RecordRow_Init(g_OVL0_EndFontInfo->rows[10], bestLap + 1, &record->best, colors);
    g_OVL0_EndFontInfo->totalBar = D_8005AECC;
    g_OVL0_EndFontInfo->totalBar.t = -1;
    InfoBar_Init(&g_OVL0_EndFontInfo->totalInfo, &D_8005AEE8);
    for (i = 4; i >= 0; i--) {
        colors[i] = 0x25a5a5a;
    }
    colors[5] = 0x2011660;
    {
        void *row = g_OVL0_EndFontInfo->bestRow;

        RecordRow_Init(row, -1, CourseRecord_GetCurrent(), colors);
    }
    g_OVL0_EndFontInfo->bestBar = D_8005AECC;
    g_OVL0_EndFontInfo->bestBar.t = -1;
    InfoBar_Init(&g_OVL0_EndFontInfo->bestInfo, &D_8005AF08);
    Text_SetFont(&ctx, &g_OVL0_FontB);
    width = Text_MeasureString(&ctx, (signed char *)sR_CAR, 1);
    other = Text_MeasureString(&ctx, (signed char *)sR_CAR + 0xb, 1);
    if (width < other) {
        width = other;
    }
    D_8005AF28.width = width + 4;
    D_8005AF48.width = width + 4;
    g_OVL0_EndFontInfo->nameX = width + 0x20;
    InfoBar_Init(&g_OVL0_EndFontInfo->carInfo, &D_8005AF28);
    InfoBar_Init(&g_OVL0_EndFontInfo->driverInfo, &D_8005AF48);
    {
        int count = g_OVL0_EndFontInfo->rowCount;

        g_SessionResults_List.count = count;
    }
    MenuList_Init(&g_SessionResults_List, SessionResults_ItemCallback, 0);
    g_OVL0_EndFontInfo->grow = D_8005AEB8;
    g_OVL0_EndFontInfo->grow.t = -1;
    g_QuickMenu->activePage->timer = 0x18;
}

// Opens the page in steps, runs the bars, and leaves on a button.
int SessionResults_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    PadState *pad = menu->padState;
    QuickMenuPage *page = menu->activePage;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        switch (page->timer) {
        case 0:
            MenuList_Open(&g_SessionResults_List);
            break;
        case 8:
            g_OVL0_EndFontInfo->totalBar.t = 0;
            g_OVL0_EndFontInfo->bestBar.t = 0;
            break;
        case 12:
            InfoBar_Show(&g_OVL0_EndFontInfo->totalInfo);
            InfoBar_Show(&g_OVL0_EndFontInfo->bestInfo);
            InfoBar_Show(&g_OVL0_EndFontInfo->carInfo);
            InfoBar_Show(&g_OVL0_EndFontInfo->driverInfo);
            g_OVL0_EndFontInfo->grow.t = 0;
            break;
        }
    }
    MenuList_Update(&g_SessionResults_List, pad);
    MenuList_ClampSelection(&g_SessionResults_List, 3, g_OVL0_EndFontInfo->rowCount - 3);
    GrowBar_Update((unsigned char *)&g_OVL0_EndFontInfo->grow);
    WipeBar_Update(&g_OVL0_EndFontInfo->totalBar);
    InfoBar_Update(&g_OVL0_EndFontInfo->totalInfo);
    WipeBar_Update(&g_OVL0_EndFontInfo->bestBar);
    InfoBar_Update(&g_OVL0_EndFontInfo->bestInfo);
    InfoBar_Update(&g_OVL0_EndFontInfo->carInfo);
    InfoBar_Update(&g_OVL0_EndFontInfo->driverInfo);
    {
        TimeTrialWork *work = g_OVL0_EndFontInfo;

        WipeBar_Update(&work->sessionBanner.bar);
        TextItem_Update(&work->sessionBanner.text);
    }
    if (page->timer == 0 && (active & 1) != 0) {
        unsigned int pressed = pad->pressed;
        int ok = (pressed & 0xa00) != 0;
        int cancel = (pressed & 0x500) != 0;

        if (ok | cancel) {
            if (cancel != 0) {
                if (g_OVL0_EndFontInfo->fromResults != 0) {
                    Sfx_PlayMenu(0);
                    return result;
                }
                Sfx_PlayMenu(4);
            } else if (ok != 0) {
                Sfx_PlayMenu(3);
            }
            {
                TitleBanner *banner = &g_OVL0_EndFontInfo->sessionBanner;

                banner->bar.t = ~banner->bar.duration;
                TextItem_Hide(&banner->text);
            }
            MenuList_Close(&g_SessionResults_List);
            {
                TimeTrialWork *work = g_OVL0_EndFontInfo;

                WipeBar_Hide(&work->totalBar);
                InfoBar_Hide(&work->totalInfo, -0x11);
                work = g_OVL0_EndFontInfo;
                WipeBar_Hide(&work->bestBar);
                InfoBar_Hide(&work->bestInfo, -0x11);
                InfoBar_Hide(&g_OVL0_EndFontInfo->carInfo, -0x11);
                InfoBar_Hide(&g_OVL0_EndFontInfo->driverInfo, -0x11);
                work = g_OVL0_EndFontInfo;
                {
                    GrowBar *grow = &work->grow;

                    grow->t = ~grow->duration;
                }
            }
            if (g_OVL0_EndFontInfo->fromResults != 0) {
                QuickMenu_PushPage(g_QuickMenu, &g_Page_TimeTrialMenu);
                result = 1;
            } else {
                result = 2;
            }
        }
    }
    return result;
}

#ifdef NON_MATCHING
// The list with its headings, the session's best with its top speed, and the course
// record with its car and driver.
void SessionResults_Draw(DrawLayer *layer)
{
    TextCtx ctx;
    char buf[0x40];
    uint *ot = (uint *)layer->ot;
    TextCtx *c;
    RaceRecord *record;
    MenuList *list;
    int i;
    int x;

    Text_Init(&ctx, 6);
    c = &ctx;
    c->unk1c = 1;
    TitleBanner_Draw(&g_OVL0_EndFontInfo->sessionBanner, ot, 0xb0, 0x5e, c);
    list = &g_SessionResults_List;
    MenuList_Draw(list, ot + 2);
    if (list->state >= 0) {
        char label[0x40];
        int t = (list->fade << 7) / list->fadeMax;

        c->ot = ot;
        *(unsigned int *)&c->clut = (*(unsigned int *)&c->clut & ~0x600000) | 0x200000;
        c->rgb = Rgb_Lerp(D_8005AE78, D_8005AE7C, t, 0x80);
        Text_SetFont(c, &g_OVL0_FontB);
        x = 0x4a;
        Text_DrawStringRight(c, (signed char *)sR_Lap, 0x2a, 0x88, 1);
        for (i = 0; i < 4; ) {
            sprintf(label, sR_SectorN, ++i);
            Text_DrawCentered(&ctx, (signed char *)label, x, 0x88, 1);
            x += 0x3c;
        }
        Text_DrawCentered(&ctx, (signed char *)sR_Total, x, 0x88, 1);
    }
    WipeBar_Draw(&g_OVL0_EndFontInfo->totalBar, ot, 0, 0x130);
    Prim_AddDrawMode(ot, 0x220);
    {
        TimeTrialWork *work = g_OVL0_EndFontInfo;
        WipeBar *bar = &work->totalBar;
        int t = bar->t;
        int shown = t >= 0;
        int closing = t < -1;

        if (shown | closing) {
            void *row = work->rows[10];

            RecordRow_Draw(row, ot, &ctx, 0x22, 0x140, WipeBar_GetLevel(bar));
            record = Session_Record();
            Speed_Format(buf, (unsigned short)record->best.unk10);
            strcat(buf, sR_Mph);
            if (~record->best.time != 0) {
                Text_DrawStringMonoDigitsRight(&ctx, (signed char *)buf, 0x150, 0x12c, 1, -3, 0);
                Text_DrawStringRight(&ctx, (signed char *)sR_Mph + 0x2fa, 0x108, 0x12c, 1);
            }
        }
    }
    InfoBar_Draw(&g_OVL0_EndFontInfo->totalInfo, ot, 0x14, 0x122, &ctx);
    GrowBar_Draw(&g_OVL0_EndFontInfo->grow, ot);
    WipeBar_Draw(&g_OVL0_EndFontInfo->bestBar, ot, 0, 0x16c);
    Prim_AddDrawMode(ot, 0x220);
    {
        TimeTrialWork *work = g_OVL0_EndFontInfo;
        WipeBar *bar = &work->bestBar;
        int t = bar->t;
        int shown = t >= 0;
        int closing = t < -1;

        if (shown | closing) {
            void *row = work->bestRow;

            RecordRow_Draw(row, ot, &ctx, 0x22, 0x17c, WipeBar_GetLevel(bar));
            Speed_Format(buf, (unsigned short)CourseRecord_GetCurrent()->best.unk10);
            strcat(buf, sR_Mph);
            if (~CourseRecord_GetCurrent()->best.time != 0) {
                char *base;
                unsigned int rgb;
                RECT *tile;
                unsigned short left;

                Text_DrawStringMonoDigitsRight(&ctx, (signed char *)buf, 0x150, 0x168, 1, -3, 0);
                Text_DrawStringRight(&ctx, (signed char *)sR_Mph + 0x2fa, 0x108, 0x168, 1);
                base = (char *)&g_GameState;
                {
                    int offset = 0xc4b8;

                    base += offset;
                }
                rgb = Rgb_Lerp(D_8005AE78, D_8005AE80, WipeBar_GetLevel(&g_OVL0_EndFontInfo->bestBar), 0x80);
                ctx.rgb = rgb;
                Text_DrawString(&ctx, (signed char *)base, g_OVL0_EndFontInfo->nameX + 6, 0x194, 1);
                tile = Prim_AddTile(ot, rgb);
                left = g_OVL0_EndFontInfo->nameX;
                tile->y = 0x188;
                tile->w = 4;
                tile->h = 6;
                tile->x = left;
                Text_DrawString(&ctx, (signed char *)CourseRecord_GetCurrent()->name, g_OVL0_EndFontInfo->nameX + 6, 0x1aa, 1);
                tile = Prim_AddTile(ot, rgb);
                left = g_OVL0_EndFontInfo->nameX;
                tile->y = 0x19e;
                tile->w = 4;
                tile->h = 6;
                tile->x = left;
                Prim_AddDrawMode(ot, 0x220);
                InfoBar_Draw(&g_OVL0_EndFontInfo->carInfo, ot, 0x14, 0x18c, &ctx);
                InfoBar_Draw(&g_OVL0_EndFontInfo->driverInfo, ot, 0x14, 0x1a2, &ctx);
            }
        }
    }
    InfoBar_Draw(&g_OVL0_EndFontInfo->bestInfo, ot, 0x14, 0x15e, &ctx);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", SessionResults_Draw);
#endif

// A menu row: its bar is as wide as the widest label plus 16.
int TimeTrialMenu_ItemCallback(MenuListEventId event, MenuListEvent *e, int i)
{
    TextCtx ctx;
    int result = 0;
    void *ot = e->ot;
    int x = e->x;
    int y = e->y;
    TextItem *item = &g_OVL0_EndFontInfo->items[i];
    WipeBar *bar = &g_OVL0_EndFontInfo->bars[i];
    int width = 0;
    TextCtx *c;

    Text_Init(&ctx, 6);
    c = &ctx;
    c->unk1c = 1;
    Text_SetFont(c, &g_OVL0_FontD);
    switch (event) {
    case MLE_INIT:
    {
        int n;
        int w;

        for (n = 0; n < 9; n++) {
            w = Text_MeasureString(&ctx, (signed char *)g_TimeTrialMenu_Options[n].label, 2);
            if (width < w) {
                width = w;
            }
        }
        TextItem_Init(item, &D_8005AFE4, g_TimeTrialMenu_Options[i].label);
        *bar = D_8005B000;
        bar->width = width + 0x10;
        bar->t = -1;
        break;
    }
    case MLE_REVEAL:
        TextItem_Show(item, -1);
        bar->t = 0;
        break;
    case MLE_CLOSE:
        TextItem_Hide(item);
        bar->t = ~bar->duration;
        break;
    case MLE_UPDATE:
        TextItem_Update(item);
        WipeBar_Update(bar);
        break;
    case MLE_DRAW:
    {
        int brightness = e->brightness;
        uint *ot1 = (uint *)ot;

        item->brightness = brightness;
        TextItem_Draw(item, ot1, x, y + 0x18, &ctx);
        ot1++;
        WipeBar_Draw(bar, ot1, x - 8, y - 2);
        Prim_AddDrawMode(ot1, 0x220);
        break;
    }
    case MLE_LEAVE:
        *(uint *)item->rgbFrom = 0x707070;
        item->flags &= ~8;
        break;
    case MLE_ENTER:
        *(uint *)item->rgbFrom = 0x907040;
        item->flags |= 8;
        TextItem_ShowNow(item);
        break;
    case MLE_OPEN:
        *(uint *)item->rgbFrom = 0x907040;
        item->flags |= 8;
        break;
    case MLE_IS_ENABLED:
        result = g_TimeTrialMenu_Options[i].enabled;
        break;
    }
    return result;
}

#ifdef NON_MATCHING
// A ghost setting: a framed box with its name, the selected one pulsing.
int GhostTypeList_ItemCallback(MenuListEventId event, MenuListEvent *e, int i)
{
    TextCtx ctx;
    unsigned int colors[4];
    int top;
    int y;
    int result = 0;
    int x = e->x;
    MenuList *list = e->list;
    uint *ot;
    TextCtx *c;

    y = e->y;
    ot = (uint *)e->ot;

    Text_Init(&ctx, 6);
    c = &ctx;
    c->ot = ot;
    *(unsigned int *)&c->clut = (*(unsigned int *)&c->clut & ~0x600000) | 0x200000;
    Text_SetFont(c, &g_OVL0_FontB);
    top = y + 8;
    if (event != MLE_DRAW) {
        if (event >= 5) {
            result = event == MLE_IS_ENABLED;
        }
    } else {
        int state = list->state;
        int shown = state >= 0;
        int closing = state < -1;

        if (shown | closing) {
            int t = (list->fade << 7) / list->fadeMax;

            if (t > 0) {
                unsigned int fill;
                int blink;
                PrimLineF4 *line;
                RECT *tile;

                colors[0] = 0x323c50;
                colors[1] = 0x2a2a2a2;
                colors[3] = 0x646464;
                if (list->state >= 0) {
                    x += (0x80 - t) >> 1;
                }
                colors[2] = Rgb_Lerp(D_8005AE78, (unsigned char *)&colors[0], t, 0x200);
                fill = Rgb_Lerp(D_8005AE78, (unsigned char *)&colors[0], t, 0x100);
                blink = g_GhostTypeList_Blink;
                if (blink >= 0xd) {
                    blink = 0xc;
                }
                if (i == list->selected) {
                    colors[2] = Rgb_Lerp(D_8005AE78, (unsigned char *)&colors[0], t, 0x80);
                    fill = Rgb_Lerp((unsigned char *)&colors[1], (unsigned char *)&colors[2], blink, 0xc);
                }
                c->rgb = Rgb_Lerp(D_8005AE78, (unsigned char *)&colors[3], t, 0x80);
                Text_DrawCentered(c, (signed char *)g_GhostTypeNames[i], x + 0x28, y + 0x1c, 1);
                tile = Prim_AddTile(ot, fill);
                tile->x = x + 2;
                tile->y = y + 10;
                tile->w = 0x4d;
                tile->h = 0x15;
                line = Prim_AddLineF4(ot, colors[2]);
                line->x4 = x;
                line->x3 = x;
                line->x0 = x;
                line->x2 = x + 0x50;
                line->x1 = x + 0x50;
                line->y4 = top;
                line->y1 = top;
                line->y0 = top;
                line->y3 = y + 0x20;
                line->y2 = y + 0x20;
                Prim_AddDrawMode(ot, 0x220);
                tile = Prim_AddTile(ot, 0x2000000);
                tile->x = x;
                tile->w = 0x50;
                tile->h = 0x18;
                tile->y = top;
                Prim_AddDrawMode(ot, 0x200);
            }
        }
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", GhostTypeList_ItemCallback);
#endif

#ifdef NON_MATCHING
// Sets up the lists, the rows that depend on the car, the ghost and the settings, the
// car and the banner.
void TimeTrialMenu_Enter(int back)
{
    QuickMenu *menu = g_QuickMenu;
    TimeTrialWork *work = g_OVL0_EndFontInfo;
    QuickMenuPage *page = menu->activePage;
    TimeTrialOption *options;
    char *base;
    RaceSetup *setup;
    int enabled;
    int slot;
    int kind;
    short *count;
    int used;

    work->fromResults = 0;
    page->timer = 0x10;
    page->sel = -1;
    page->mode = 0;
    if (back == 0) {
        g_TimeTrial_SettingsUsed = 0;
        MenuList_Init(&g_TimeTrialMenu_List, TimeTrialMenu_ItemCallback, 0);
    }
    MenuList_Init(&g_GhostTypeList, GhostTypeList_ItemCallback, 0);
    enabled = 1;
    base = (char *)&g_GameState;
    setup = RaceSetup_Get();
    kind = setup->unk582;
    slot = setup->garageSlot;
    count = (short *)(base + 0x7c9c);
    switch (kind) {
    case 0:
        count = (short *)(base + 0x3c74);
        break;
    case 1:
        break;
    default:
        enabled = 0;
        goto done;
    }
    {
        int none = slot < 0;
        int over = slot >= *count;

        if (none | over) {
            enabled = 0;
        }
    }
done:
    options = g_TimeTrialMenu_Options;
    options[2].enabled = enabled;
    used = g_TimeTrial_SettingsUsed == 0;
    options[0].enabled = used;
    options[6].enabled = used;
    options[4].enabled = g_TT_GhostReady;
    if (GhostState_Get()[6] != 0) {
        options[4].enabled = 0;
    }
    {
        ResultCar *car = &g_OVL0_EndFontInfo->car;

        car->brightness = 0x40;
        car->enabled = 1;
        car->unk00 = 0x7fd7;
        car->tpage = 9;
        car->model = D_800A9F00;
        GroundDisc_Init(&car->disc, 0x80000);
    }
    ResultCarView_Init(&g_OVL0_EndFontInfo->view, 200, 200);
    TitleBanner_Init(&g_OVL0_EndFontInfo->banner, RaceSetup_CourseName(), &menu->text);
    g_OVL0_EndFontInfo->banner.bar.t = 0;
    TextItem_Show(&g_OVL0_EndFontInfo->banner.text, -1);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialMenu_Enter);
#endif

#ifdef NON_MATCHING
// The menu, or the ghost setting list over it.
int TimeTrialMenu_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    int active2 = active;
    int result = 0;
    QuickMenuPage *page = menu->activePage;
    PadState *pad = menu->padState;
    int choice;
    int leaving;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            MenuList_Open(&g_TimeTrialMenu_List);
            g_TimeTrialMenu_List.revealDelay = -1;
            page->sel = 0;
        }
    }
    {
        TimeTrialWork *work = g_OVL0_EndFontInfo;

        WipeBar_Update(&work->banner.bar);
        TextItem_Update(&work->banner.text);
    }
    ResultCarView_Spin(&g_OVL0_EndFontInfo->view, menu->drawStamp);
    if (active2 == 0) {
        pad = 0;
    }
    if (page->mode == 1) {
        if (++g_GhostTypeList_Blink >= 0x2e) {
            g_GhostTypeList_Blink = 0;
        }
        MenuList_Update(&g_TimeTrialMenu_List, 0);
        choice = MenuList_Update(&g_GhostTypeList, pad);
        switch (choice) {
        case -3:
            Sfx_PlayMenu(5);
            break;
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            page->mode = 0;
            MenuList_Close(&g_GhostTypeList);
            break;
        default:
            D_801C9995 = choice;
            Sfx_PlayMenu(1);
            page->mode = 0;
            MenuList_Close(&g_GhostTypeList);
            break;
        }
        return result;
    }
    MenuList_Update(&g_GhostTypeList, 0);
    choice = MenuList_Update(&g_TimeTrialMenu_List, pad);
    switch (choice) {
    case -3:
        Sfx_PlayMenu(6);
        return result;
    case -2:
        return result;
    case -4:
    case -1:
        Sfx_PlayMenu(0);
        return result;
    }
    {
        int next = (unsigned char)g_TimeTrialMenu_Options[choice].next;

        leaving = 1;
        switch ((signed char)(next + 7)) {
        case 0:
        {
            char *base = (char *)&g_GameState;

            g_GhostTypeList_Blink = 0;
            Sfx_PlayMenu(1);
            page->mode = 1;
            g_GhostTypeList.selected = (unsigned char)base[0xb5];
            leaving = 0;
            MenuList_Open(&g_GhostTypeList);
            break;
        }
        case 1:
        {
            QuickMenuPage *parts = &g_Page_ChangeParts;

            g_TimeTrial_SettingsUsed = result = 1;
            QuickMenu_PushPage(g_QuickMenu, parts);
            break;
        }
        case 2:
            QuickMenu_PushPage(g_QuickMenu, &g_Page_SessionResults);
            result = 1;
            break;
        case 3:
            QuickMenu_PushPage(menu, &g_Page_SaveReplay);
            QuickMenu_SetupCardUi(g_TimeTrial_CardWork);
            result = 1;
            break;
        case 4:
            QuickMenu_PushPage(menu, &g_Page_SaveGhost);
            QuickMenu_SetupCardUi2(g_TimeTrial_CardWork);
            result = 1;
            break;
        case 5:
            QuickMenu_PushPage(menu, &g_Page_LoadGhost);
            result = 1;
            break;
        default:
            menu->unk7c = next;
            QuickMenu_PushPage(menu, &g_Page_TimeTrialQuit);
            result = 1;
            break;
        }
    }
    if (leaving) {
        Sfx_PlayMenu(3);
        {
            TitleBanner *banner = &g_OVL0_EndFontInfo->banner;

            banner->bar.t = ~banner->bar.duration;
            TextItem_Hide(&banner->text);
        }
        MenuList_Close(&g_TimeTrialMenu_List);
        page->sel = -1;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialMenu_Update);
#endif

#ifdef NON_MATCHING
// The lists, the banner, the car and the title sprite fading in with the list.
void TimeTrialMenu_Draw(DrawLayer *layer)
{
    QuickMenu *menu = g_QuickMenu;
    uint *ot = (uint *)layer->ot;
    QuickMenuPage *page = menu->activePage;
    MenuList *list;
    RECT area;
    int fade;
    int size;
    PrimSprite *sprite;

    MenuList_Draw(&g_GhostTypeList, ot);
    TitleBanner_Draw(&g_OVL0_EndFontInfo->banner, ot, 0xb0, 0x5e, &menu->text);
    list = &g_TimeTrialMenu_List;
    MenuList_Draw(list, ot);
    if (page->sel >= 0) {
        ResultCar_Draw(&g_OVL0_EndFontInfo->car, &menu->layers[4], &menu->layers[5], &g_OVL0_EndFontInfo->view, 0);
    }
    area.x = 0x8a;
    area.y = 0xf0;
    size = 0xc8;
    area.w = size;
    area.h = size;
    DrawLayer_SetArea(&menu->layers[4], &area);
    fade = (list->fade << 7) / list->fadeMax;
    ot += 2;
    sprite = Prim_AddSprite(ot, fade | fade << 8 | fade << 16 | 0x2000000);
    sprite->x = 0x94;
    sprite->y = 0x6c;
    sprite->clut = 0x401c;
    sprite->v = 1;
    sprite->u = 0;
    sprite->w = 0xbc;
    sprite->h = size;
    Prim_AddDrawMode(ot, 0x37);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialMenu_Draw);
#endif
}
