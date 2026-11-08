// OVL0 800505ac-80052838 (raceresults): Race and machine test result pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"

// How InfoBar_Init sets up a bar.
struct InfoBarDesc {
    uint unk00;
    uint unk04;
    game_pointer<char> label;
    uint labelRgb;
    uint value;
    uint valueRgb;
    ushort kind;
    ushort pad1a;
    ushort width;
    ushort height;
};

// One lap of a result row.
struct LapTime {
    uint time;
    short lap;
};

// The look of a time: colours, digit widths and font.
struct TimeStyle {
    uint rgb;
    uint lapRgb;
    short width;
    short narrow;
    short lapGap;
    short digitShift;
    game_pointer<Font> font;
};

// A fade counter and the bar it goes with.
struct FadedBar {
    short fade[2];
    WipeBar bar;
};

// The pages' working memory. The machine test pages use rank, and totals for the time
// and the speed.
struct RaceResultsWork {
    short twoPlayer;
    short count;                // laps listed; the machine test pages' rank
    uchar pad04[4];
    uint totals[2];
    uint bests[2];
    uint lapTimes[2][10];
    short lapNo[2][10];
    short revealed;
    short rows;
    ResultCar car;
    ResultCarView view;
    TitleBanner banner;
    InfoBar info;
    TextItem place;
    WipeBar placeBar;
    InfoBar total;
    WipeBar totalBar;
    InfoBar best;
    WipeBar bestBar;
    InfoBar laps;
    WipeBar lapBar;
    uchar pad4d0[0x28];
    uchar dialog[0x94];         // a CardDialog, its last 4 bytes under prompt
    ButtonPrompt prompt;
    TextEntry entry;
};

#define rank count
#define RESULT_DIALOG(work) ((CardDialog *)(work)->dialog)

extern "C" {
extern game_pointer<RaceResultsWork> (&g_OVL0_EndFontInfo);
extern GameState (&g_GameState);
extern RaceMem (&g_Race_IsDemo);
extern game_pointer<QuickMenu> (&g_QuickMenu);
extern QuickMenuPage (&g_Page_Quit);
extern QuickMenuPage (&g_Page_SaveGame);
extern QuickMenuPage (&g_Page_RaceResults);
extern QuickMenuPage (&g_Page_MachineTestName);
extern QuickMenuPage (&g_Page_MachineTestResult);
extern MenuList (&g_LapList_List);
extern RaceRecord (&g_GhostData);
extern game_pointer<char> (&g_PlaceTexts)[];
extern uint (&g_PlaceColors)[];
extern char (&sR_PLAYER1WINS)[];
extern char (&sR_TOTALTIME)[];
extern char (&sR_RECORD)[];
extern char (&sR_OUTOFRANKING)[];
extern char (&D_8005A980)[];
extern CardDialogDesc (&D_8005B5AC);
extern ButtonPromptDesc (&D_8005B5DC);
extern TextEntry (&D_8005B5F8);
extern InfoBarDesc (&D_8005B60C);
extern TextStyle (&D_8005B62C);
extern WipeBar (&D_8005B648);
extern TimeStyle (&D_8005B664);
extern TimeStyle (&D_8005B678);
extern TimeStyle (&D_8005B68C);
extern InfoBarDesc (&D_8005B6A0);
#define g_TotalBarFade D_8005B6C0
extern FadedBar (&D_8005B6C0);
extern InfoBarDesc (&D_8005B6E0);
#define g_BestBarFade D_8005B700
extern FadedBar (&D_8005B700);
extern InfoBarDesc (&D_8005B720);
#define g_LapBarFade D_8005B740
extern FadedBar (&D_8005B740);
extern short (&D_8005B760)[];
void QuickMenu_PushPage(QuickMenu *menu, QuickMenuPage *page);
void QuickMenu_SetupCardUiThen(QuickMenuPage *next, void *work);
void InfoBar_Init(InfoBar *bar, InfoBarDesc *desc);
void InfoBar_Draw(InfoBar *bar, uint *ot, int x, int y, TextCtx *ctx);
void TitleBanner_Init(TitleBanner *banner, char *text, TextCtx *ctx);
void LapTime_Draw(LapTime *t, uint *ot, TextCtx *ctx, int x, int y, TimeStyle *style, int mode);
void AnimatedTime_Draw(short *fade, uint *ot, TextCtx *ctx, int value, int x, int y, int mode, TimeStyle *style,
                       int isSpeed, int centre);
void FadeCounter_Update_8006BDD0(short *c);
void MachineTest_StoreResult(void);
int RaceResults_CanSave(void);
void RaceResults_SetupCar(int index);
void RaceResults_InitView(ResultCarView *view);
int LapList_ItemCallback(MenuListEventId event, MenuListEvent *e, int i);

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline int RaceSetup_Type(void)
{
    return RaceSetup_Get()->raceType;
}

static inline int RaceSetup_TypeOf(RaceSetup *setup)
{
    return setup->raceType;
}

static inline char *GameState_Base(void)
{
    return (char *)&g_GameState;
}

static inline void WipeBar_Hide(WipeBar *bar)
{
    bar->t = ~bar->duration;
}

static inline void FadeCounter_Hide(short *fade)
{
    fade[1] = ~fade[0];
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

// A row of the lap list: the lap times, the newest flashing while it comes in.
int LapList_ItemCallback(MenuListEventId event, MenuListEvent *e, int i)
{
    TextCtx ctx;
    LapTime first;
    LapTime second;
    int result = 0;
    int x = 0xfa;
    int x2;
    int bright = e->brightness;
    uint *ot = (uint *)e->ot;
    MenuList *list = e->list;
    RaceResultsWork *work;
    int y;

    Text_Init(&ctx, 6);
    ctx.unk1c = 1;
    work = g_OVL0_EndFontInfo;
    if (work->twoPlayer != 0) {
        x = 0xd2;
    }
    x2 = x + 0x5a;
    switch ((int)event) {
    case MLE_REVEAL: {
        short step;

        work->revealed++;
        step = work->revealed / 5;
        if (step >= 5) {
            step = 4;
        }
        work->rows = D_8005B760[step];
        list->revealDelay = work->rows;
        g_LapBarFade.fade[1] = 0;
        work->lapBar = g_LapBarFade.bar;
        work->lapBar.t = 0;
        MenuList_SetSelection(list, i - 2);
        break;
    }
    case MLE_DRAW:
        y = e->y + 8;
        if (list->state >= 0) {
            if (i < work->revealed) {
                first.lap = work->lapNo[0][i];
                first.time = work->lapTimes[0][i];
                if (work->twoPlayer != 0) {
                    second.lap = -1;
                    second.time = work->lapTimes[1][i];
                }
                bright = bright * 100 >> 7;
                if (i == work->revealed - 1) {
                    short *fade = g_LapBarFade.fade;

                    bright = bright * FadeCounter_GetLevel(fade) >> 7;
                    if (fade[1] != fade[0]) {
                        int glow = 0x80 - bright;
                        int spread;

                        if (glow < 0) {
                            glow = 0;
                        }
                        if (glow >= 0x81) {
                            glow = 0x80;
                        }
                        spread = glow >> 2;
                        D_8005B678.rgb = D_8005B678.lapRgb = glow | glow << 8 | glow << 16 | 0x2000000;
                        LapTime_Draw(&first, ot + 1, &ctx, x - spread, y, &D_8005B678, 1);
                        LapTime_Draw(&first, ot + 1, &ctx, x + spread, y, &D_8005B678, 1);
                        if (g_OVL0_EndFontInfo->twoPlayer != 0) {
                            LapTime_Draw(&first, ot + 1, &ctx, x2 - spread, y, &D_8005B678, 1);
                            LapTime_Draw(&first, ot + 1, &ctx, x2 + spread, y, &D_8005B678, 1);
                        }
                    }
                }
                D_8005B678.rgb = bright | bright << 8 | bright << 16;
                D_8005B678.lapRgb = bright >> 1 | (bright >> 1) << 8 | bright << 16;
                LapTime_Draw(&first, ot + 1, &ctx, x, y, &D_8005B678, 0);
                if (g_OVL0_EndFontInfo->twoPlayer != 0) {
                    LapTime_Draw(&second, ot + 1, &ctx, x2, y, &D_8005B678, 0);
                }
            }
            if (i == g_OVL0_EndFontInfo->revealed - 1) {
                WipeBar_Draw(&g_OVL0_EndFontInfo->lapBar, ot, e->x - 0x46, y - 0x10);
                Prim_AddDrawMode(ot, 0x220);
            }
        }
        if (list->state < -1) {
            int fade = MenuList_GetFade(list);

            work = g_OVL0_EndFontInfo;
            first.lap = work->lapNo[0][i];
            first.time = work->lapTimes[0][i];
            if (work->twoPlayer != 0) {
                second.lap = -1;
                second.time = work->lapTimes[1][i];
            }
            bright = (bright * 100 >> 7) * fade;
            D_8005B678.rgb = (bright >> 7) | (bright >> 7) << 8 | (bright >> 7) << 16;
            D_8005B678.lapRgb = (bright >> 8) | (bright >> 8) << 8 | (bright >> 7) << 16;
            LapTime_Draw(&first, ot + 1, &ctx, x, y, &D_8005B678, 0);
            if (g_OVL0_EndFontInfo->twoPlayer != 0) {
                LapTime_Draw(&second, ot + 1, &ctx, x2, y, &D_8005B678, 0);
            }
            WipeBar_Draw(&g_OVL0_EndFontInfo->lapBar, ot, e->x - 0x46, y - 0x10);
            Prim_AddDrawMode(ot, 0x220);
        }
        break;
    case MLE_IS_ENABLED:
        result = 1;
        break;
    }
    return result;
}

// The car of index on the page's stage.
void RaceResults_SetupCar(int index)
{
    RaceResultsWork *work = g_OVL0_EndFontInfo;
    RaceMem *race = &g_Race_IsDemo;
    ResultCar *car = &work->car;

    car->brightness = 0x40;
    car->enabled = 1;
    work->car.unk00 = 0x7fd7;
    car->tpage = 9;
    car->model = race->world.vehicles[index].model;
    GroundDisc_Init(&work->car.disc, 0x60000);
}

// The view of the car, from a random angle and distance.
void RaceResults_InitView(ResultCarView *view)
{
    unsigned int seed;

    view->drawDisc = 1;
    view->rotX = 0x5e;
    view->rotY = 0x1500;
    view->pos[2] = 0x80000;
    view->viewport[2] = 0x160;
    view->viewport[3] = 0x12c;
    view->frustum[0] = -0xb0;
    view->frustum[1] = 0xb0;
    view->frustum[2] = 0x85;
    view->frustum[3] = -0x34;
    view->frustum[4] = 0x190;
    view->discSemi = 0;
    view->discRgb = 0x3e3e3e;
    view->rotZ = 0;
    view->unk_b4 = 0;
    view->unk_b8 = 0;
    view->unk_bc = 0;
    view->pos[1] = 0;
    view->pos[0] = 0;
    view->viewport[0] = 0;
    view->viewport[1] = 0;
    view->frustum[5] = 0x7fff;
    view->unk00 = 0;
    seed = Irq_WaitVSyncs(0);
    view->rotY = Rand_Next(&seed) & 0xfff;
    Rand_Next(&seed);
    Rand_Next(&seed);
    view->rotX = (Rand_Next(&seed) & 0x7f) + 0x50;
}

// Turns the car 0xc per frame; after frame 0xfb90 also pulls the camera back.
void RaceResults_SpinView(ResultCarView *view, int frame)
{
    view->rotY = view->rotY + 0xcU & 0x3fff;
    if (0xfb90 < frame) {
        view->pos[2] = view->pos[2] + 0x8000;
    }
}

int RaceResults_CanSave(void)
{
    int can = 1;

    switch (RaceSetup_Type()) {
    case 0:
    case 2:
    case 11:
        can = 0;
        break;
    }
    if (RaceSetup_Get()->game == 0) {
        can = 0;
    }
    return can;
}

// Enters the test's result in its ranking: time for the first two, speed for the last.
void MachineTest_StoreResult(void)
{
    MachineTestEntry entry;
    int rank = -1;
    char *base = (char *)&g_GameState;
    RaceSetup *setup = (RaceSetup *)(base + 0xbf7c);
    RaceRecord *record;
    unsigned int time;
    unsigned int speed;
    MachineTestEntry *e;
    RaceResultsWork *work;

    entry.carId = setup->cars[0].carId;
    record = (RaceRecord *)(base + 0xc5a8);
    time = record->best.time;
    speed = (unsigned short)record->best.unk10;
    strcpy(entry.name, D_8005A980);
    e = &entry;
    switch (RaceSetup_TypeOf(setup)) {
    case 7:
        e->result = time;
        rank = MachineTest_InsertRecord((MachineTestRecords *)(base + 0x3a88), e, 0);
        break;
    case 8:
        e->result = time;
        rank = MachineTest_InsertRecord((MachineTestRecords *)(base + 0x3b2c), e, 0);
        break;
    case 9:
        e->result = speed;
        rank = MachineTest_InsertRecord((MachineTestRecords *)(base + 0x3bd0), e, 1);
        break;
    }
    work = g_OVL0_EndFontInfo;
    work->rank = rank;
    work->totals[0] = time;
    work->totals[1] = speed;
}

void RaceResultsRoot_Enter(int back)
{
    QuickMenu *menu = g_QuickMenu;
    int end = (int)g_OVL0_EndFontInfo + 0x643;
    QuickMenuPage *page = menu->activePage;

    page->timer = 0x18;
    page->sel = back;
    QuickMenu_SetupCardUiThen(&g_Page_Quit, (void *)rt::from_guest((end & ~3)));
}

int RaceResultsRoot_Update(void)
{
    QuickMenuPage *page = g_QuickMenu->activePage;
    int result = 0;

    if (page->timer > 0 && --page->timer == 0) {
        if (page->sel == 1) {
            result = 4;
        } else {
            QuickMenuPage *next = &g_Page_RaceResults;

            switch (RaceSetup_Get()->raceType) {
            case 7:
            case 8:
            case 9:
                next = &g_Page_MachineTestResult;
                MachineTest_StoreResult();
                if (g_OVL0_EndFontInfo->rank >= 0) {
                    next = &g_Page_MachineTestName;
                }
                QuickMenu_PlayMusic(8);
                break;
            }
            QuickMenu_PushPage(g_QuickMenu, next);
            result = 1;
        }
    }
    return result;
}

// Empty.
void RaceResultsRoot_Draw(DrawLayer *layer)
{
}

void RaceResults_Enter(int back)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int car = 0;
    int music = 8;
    char *base = (char *)&g_GameState;
    RaceSetup *setup = (RaceSetup *)(base + 0xbf7c);
    RaceRecord *rec = (RaceRecord *)(base + 0xc5a8);
    RaceRecord *rec2 = &g_GhostData;
    int twoPlayer = setup->raceType == 0;
    RaceResultsWork *work;
    int i;

    g_OVL0_EndFontInfo->twoPlayer = twoPlayer;
    if (twoPlayer) {
        char *text = sR_PLAYER1WINS;
        PlayerStats *stats = (PlayerStats *)(base + 0xb8);
        short *wins = (short *)((char *)stats + 0x44);

        if (rec->unk00 > 0 && rec2->unk00 > 0 ? rec->unk00 < rec2->unk00 : rec->unk00 > 0) {
            wins[0]++;
        } else {
            text += 0x17;
            car = 1;
            wins[1]++;
        }
        TextItem_Init(&g_OVL0_EndFontInfo->place, &D_8005B62C, text);
        work = g_OVL0_EndFontInfo;
        *(uint *)work->place.rgbFrom = g_PlaceColors[0];
        work->placeBar = D_8005B648;
        g_OVL0_EndFontInfo->placeBar.t = -1;
    } else {
        int place = rec->unk00 - 1;

        if ((setup->game == 0) & (setup->raceType == 4)) {
            int event = setup->eventIndex;

            if ((place == 0) & (event >= 0)) {
                PlayerStats_SetArcadeCourseFlags((PlayerStats *)(base + 0xb8), event, setup->pad57f);
            }
        }
        if ((unsigned int)place >= 6) {
            place = 0;
        }
        TextItem_Init(&g_OVL0_EndFontInfo->place, &D_8005B62C, g_PlaceTexts[place]);
        work = g_OVL0_EndFontInfo;
        *(uint *)work->place.rgbFrom = g_PlaceColors[place];
        work->placeBar = D_8005B648;
        g_OVL0_EndFontInfo->placeBar.t = -1;
        if (place != 0) {
            music = 0x13;
        }
    }
    work = g_OVL0_EndFontInfo;
    work->totals[0] = rec->unkf8;
    work->totals[1] = rec2->unkf8;
    work->bests[0] = rec->best.time;
    work->bests[1] = rec2->best.time;
    if (twoPlayer) {
        int count = rec->lapsKept;
        int skip = rec->lapCount - count;

        if (count < rec2->lapsKept) {
            count = rec2->lapsKept;
            skip = rec2->lapCount - count;
        }
        work->count = count;
        for (i = 0; i < count; i++) {
            int lap = i + skip;
            unsigned int time = Records_GetLapTime(rec, lap);

            g_OVL0_EndFontInfo->lapTimes[0][i] = time;
            work = g_OVL0_EndFontInfo;
            work->lapNo[0][i] = lap + 1;
            time = Records_GetLapTime(rec2, lap);
            work->lapTimes[1][i] = time;
            g_OVL0_EndFontInfo->lapNo[1][i] = lap + 1;
        }
    } else {
        work->count = rec->lapsKept;
        for (i = 0; i < work->count; ) {
            work->lapTimes[0][i] = rec->laps[i].time;
            work->lapNo[0][i] = rec->lapCount - rec->lapsKept + i + 1;
            work->lapTimes[1][i] = rec2->laps[i].time;
            i++;
            work->lapNo[1][i - 1] = rec2->lapCount - rec2->lapsKept + i;
        }
    }
    page->timer = 0;
    page->sel = -1;
    page->mode = 0;
    RaceResults_SetupCar(car);
    RaceResults_InitView(&g_OVL0_EndFontInfo->view);
    TitleBanner_Init(&g_OVL0_EndFontInfo->banner, RaceSetup_Get()->courseName, &menu->text);
    work = g_OVL0_EndFontInfo;
    work->banner.bar.t = 0;
    TextItem_Show(&work->banner.text, -1);
    InfoBar_Init(&g_OVL0_EndFontInfo->info, &D_8005B60C);
    D_8005B6A0.label = sR_TOTALTIME;
    InfoBar_Init(&g_OVL0_EndFontInfo->total, &D_8005B6A0);
    g_TotalBarFade.fade[1] = -1;
    work = g_OVL0_EndFontInfo;
    work->totalBar = g_TotalBarFade.bar;
    work->totalBar.t = -1;
    InfoBar_Init(&work->best, &D_8005B6E0);
    g_BestBarFade.fade[1] = -1;
    work = g_OVL0_EndFontInfo;
    work->bestBar = g_BestBarFade.bar;
    work->bestBar.t = -1;
    work->revealed = 0;
    work->rows = 0;
    g_LapList_List.count = work->count;
    MenuList_Init(&g_LapList_List, LapList_ItemCallback, 0);
    g_LapList_List.revealDelay = g_OVL0_EndFontInfo->rows;
    InfoBar_Init(&g_OVL0_EndFontInfo->laps, &D_8005B720);
    g_LapBarFade.fade[1] = -1;
    work = g_OVL0_EndFontInfo;
    work->lapBar = g_LapBarFade.bar;
    work->lapBar.t = -1;
    CardDialog_Init(RESULT_DIALOG(work), &D_8005B5AC, 0);
    ButtonPrompt_Init(&g_OVL0_EndFontInfo->prompt, &D_8005B5DC);
    QuickMenu_PlayMusic(music);
}

int RaceResults_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    PadState *pad = 0;
    int result = 0;
    int frame = page->timer;
    QuickMenuPage *next;
    RaceResultsWork *work;
    int r;

    if (page->mode == 0) {
        frame++;
    }
    switch (frame) {
    case 0x18:
        page->sel = 0;
        break;
    case 0x30:
        InfoBar_Show(&g_OVL0_EndFontInfo->info);
        break;
    case 0x3c:
        TextItem_Show(&g_OVL0_EndFontInfo->place, -1);
        g_OVL0_EndFontInfo->placeBar.t = 0;
        break;
    case 0x48:
        InfoBar_Show(&g_OVL0_EndFontInfo->total);
        break;
    case 0x54:
        g_TotalBarFade.fade[1] = 0;
        g_OVL0_EndFontInfo->totalBar.t = 0;
        break;
    case 0x60:
        InfoBar_Show(&g_OVL0_EndFontInfo->best);
        break;
    case 0x6c:
        g_BestBarFade.fade[1] = 0;
        g_OVL0_EndFontInfo->bestBar.t = 0;
        break;
    case 0x78:
        InfoBar_Show(&g_OVL0_EndFontInfo->laps);
        break;
    case 0x84:
        MenuList_Open(&g_LapList_List);
        break;
    case 0x85:
        {
            RaceResultsWork *w = g_OVL0_EndFontInfo;
            int done = w->revealed >= w->count;

            frame = 0x84;
            if (done) {
                MenuList_ClampSelection(&g_LapList_List, 2, w->count - 3);
                frame = 0x85;
            }
        }
        break;
    case 0xa3: {
        int one = 1;

        page->mode = one;
        if (RaceResults_CanSave()) {
            CardDialog_Open(RESULT_DIALOG(g_OVL0_EndFontInfo));
            RESULT_DIALOG(g_OVL0_EndFontInfo)->choice = one;
        } else {
            ButtonPrompt_Open(&g_OVL0_EndFontInfo->prompt);
        }
        frame++;
        break;
    }
    }
    work = g_OVL0_EndFontInfo;
    page->timer = frame;
    WipeBar_Update(&work->banner.bar);
    TextItem_Update(&work->banner.text);
    InfoBar_Update(&g_OVL0_EndFontInfo->info);
    TextItem_Update(&g_OVL0_EndFontInfo->place);
    WipeBar_Update(&g_OVL0_EndFontInfo->placeBar);
    InfoBar_Update(&g_OVL0_EndFontInfo->total);
    FadeCounter_Update(g_TotalBarFade.fade);
    WipeBar_Update(&g_OVL0_EndFontInfo->totalBar);
    InfoBar_Update(&g_OVL0_EndFontInfo->best);
    FadeCounter_Update(g_BestBarFade.fade);
    WipeBar_Update(&g_OVL0_EndFontInfo->bestBar);
    InfoBar_Update(&g_OVL0_EndFontInfo->laps);
    {
        RaceResultsWork *w = g_OVL0_EndFontInfo;
        int done = w->revealed >= w->count;

        if (done) {
            pad = &menu->padState[0];
        }
    }
    MenuList_Update(&g_LapList_List, pad);
    FadeCounter_Update_8006BDD0(g_LapBarFade.fade);
    pad = &menu->padState[0];
    WipeBar_Update(&g_OVL0_EndFontInfo->lapBar);
    {
        int early = frame < 0xa3;
        int inactive = active ^ 1;

        if (early | inactive) {
            pad = 0;
        }
    }
    next = &g_Page_Quit;
    if (RaceResults_CanSave()) {
        r = CardDialog_Update(RESULT_DIALOG(g_OVL0_EndFontInfo), pad);
    } else {
        r = ButtonPrompt_Update(&g_OVL0_EndFontInfo->prompt, pad);
        switch (r) {
        case -2:
        default:
            break;
        case -1:
            break;
        case 0:
            r = 1;
            break;
        }
    }
    switch (r) {
    case -1:
        Sfx_PlayMenu(0);
        break;
    case 0:
        next = &g_Page_SaveGame;
    case 1:
        if (RaceResults_CanSave()) {
            CardDialog_Close(RESULT_DIALOG(g_OVL0_EndFontInfo));
        } else {
            ButtonPrompt_Close(&g_OVL0_EndFontInfo->prompt);
        }
        Sfx_PlayMenu(3);
        WipeBar_Hide(&g_OVL0_EndFontInfo->banner.bar);
        TextItem_Hide(&g_OVL0_EndFontInfo->banner.text);
        InfoBar_Hide(&g_OVL0_EndFontInfo->info, -0x11);
        TextItem_Hide(&g_OVL0_EndFontInfo->place);
        WipeBar_Hide(&g_OVL0_EndFontInfo->placeBar);
        InfoBar_Hide(&g_OVL0_EndFontInfo->total, -0x11);
        FadeCounter_Hide(g_TotalBarFade.fade);
        WipeBar_Hide(&g_OVL0_EndFontInfo->totalBar);
        InfoBar_Hide(&g_OVL0_EndFontInfo->best, -0x11);
        FadeCounter_Hide(g_BestBarFade.fade);
        WipeBar_Hide(&g_OVL0_EndFontInfo->bestBar);
        InfoBar_Hide(&g_OVL0_EndFontInfo->laps, -0x11);
        MenuList_Close(&g_LapList_List);
        FadeCounter_Hide(g_LapBarFade.fade);
        WipeBar_Hide(&g_OVL0_EndFontInfo->lapBar);
        page->sel = -1;
        QuickMenu_PushPage(menu, next);
        result = 1;
        break;
    }
    RaceResults_SpinView(&g_OVL0_EndFontInfo->view, menu->drawStamp);
    return result;
}

void RaceResults_Draw(DrawLayer *layer)
{
    QuickMenu *menu = g_QuickMenu;
    TextCtx *ctx = &menu->text;
    QuickMenuPage *page = menu->activePage;
    uint *ot = (uint *)layer->ot;
    int x = 0xfa;
    uint *ot2;
    uint *ot1;
    int x2;
    RECT area;

    if (g_OVL0_EndFontInfo->twoPlayer != 0) {
        x = 0xd2;
    }
    TitleBanner_Draw(&g_OVL0_EndFontInfo->banner, ot, 0xb0, 100, ctx);
    CardDialog_Draw(RESULT_DIALOG(g_OVL0_EndFontInfo), ot, ctx);
    ButtonPrompt_Draw(&g_OVL0_EndFontInfo->prompt, ot, ctx);
    ot2 = ot + 2;
    InfoBar_Draw(&g_OVL0_EndFontInfo->info, ot2, 0x28, 0x88, ctx);
    TextItem_Draw(&g_OVL0_EndFontInfo->place, ot, 0xb0, 0xb8, ctx);
    ot1 = ot + 1;
    WipeBar_Draw(&g_OVL0_EndFontInfo->placeBar, ot1, 0x60, 0x88);
    Prim_AddDrawMode(ot1, 0x220);
    InfoBar_Draw(&g_OVL0_EndFontInfo->total, ot2, 0x28, 0xd2, ctx);
    WipeBar_Draw(&g_OVL0_EndFontInfo->totalBar, ot1, 0x74, 0xe2);
    Prim_AddDrawMode(ot1, 0x220);
    AnimatedTime_Draw(g_TotalBarFade.fade, ot1, ctx, g_OVL0_EndFontInfo->totals[0], x, 0xf2, 0,
                      &D_8005B664, 0, 0);
    x2 = x + 0x5a;
    if (g_OVL0_EndFontInfo->twoPlayer != 0) {
        AnimatedTime_Draw(g_TotalBarFade.fade, ot1, ctx, g_OVL0_EndFontInfo->totals[1], x2, 0xf2, 0,
                          &D_8005B664, 0, 0);
    }
    InfoBar_Draw(&g_OVL0_EndFontInfo->best, ot2, 0x28, 0x101, ctx);
    WipeBar_Draw(&g_OVL0_EndFontInfo->bestBar, ot1, 0x74, 0x111);
    Prim_AddDrawMode(ot1, 0x220);
    AnimatedTime_Draw(g_BestBarFade.fade, ot1, ctx, g_OVL0_EndFontInfo->bests[0], x, 0x121, 0,
                      &D_8005B664, 0, 0);
    if (g_OVL0_EndFontInfo->twoPlayer != 0) {
        AnimatedTime_Draw(g_BestBarFade.fade, ot1, ctx, g_OVL0_EndFontInfo->bests[1], x2, 0x121, 0,
                          &D_8005B664, 0, 0);
    }
    InfoBar_Draw(&g_OVL0_EndFontInfo->laps, ot1, 0x28, 0x130, ctx);
    MenuList_Draw(&g_LapList_List, ot2);
    if (page->sel >= 0) {
        ResultCar_Draw(&g_OVL0_EndFontInfo->car, &menu->layers[4], &menu->layers[5],
                       &g_OVL0_EndFontInfo->view, 0);
    }
    area.x = 0;
    area.y = 0x96;
    area.w = 0x160;
    area.h = 0x12c;
    DrawLayer_SetArea(&menu->layers[4], &area);
    DrawLayer_SetArea(&menu->layers[5], &area);
}

void MachineTestName_Enter(int back)
{
    QuickMenuPage *page = g_QuickMenu->activePage;
    TextEntry *entry;

    page->timer = 0x18;
    TextEntry_Init(&g_OVL0_EndFontInfo->entry, &D_8005B5F8);
    entry = &g_OVL0_EndFontInfo->entry;
    entry->text = (char *)GameState_Base() + 0x7c8f;
    entry->maxLen = 0xb;
    entry->maxWidth = 0x100;
}

int MachineTestName_Update(void)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int result = 0;
    PadState *pad = &menu->padState[0];

    if (page->timer > 0 && --page->timer == 0) {
        TextEntry *entry;

        TextEntry_Open(&g_OVL0_EndFontInfo->entry);
        entry = &g_OVL0_EndFontInfo->entry;
        entry->cursorPos = strlen(entry->text);
    }
    switch (TextEntry_Update(&g_OVL0_EndFontInfo->entry, pad)) {
    case -2:
    default:
        break;
    case -1:
        Sfx_PlayMenu(0);
        return result;
    case 0: {
        char *base;
        int offset;

        Sfx_PlayMenu(3);
        base = (char *)&g_GameState;
        offset = 0xbf7c;
        switch (((RaceSetup *)(base + offset))->raceType) {
        case 7:
            MachineTestRecords_SetName((MachineTestRecords *)(base + 0x3a88), g_OVL0_EndFontInfo->rank,
                                       base + 0x7c8f);
            break;
        case 8:
            MachineTestRecords_SetName((MachineTestRecords *)(base + 0x3b2c), g_OVL0_EndFontInfo->rank,
                                       base + 0x7c8f);
            break;
        case 9:
            MachineTestRecords_SetName((MachineTestRecords *)(base + 0x3bd0), g_OVL0_EndFontInfo->rank,
                                       base + 0x7c8f);
            break;
        }
        result = 1;
        TextEntry_Close(&g_OVL0_EndFontInfo->entry);
        QuickMenu_PushPage(g_QuickMenu, &g_Page_MachineTestResult);
        break;
    }
    }
    return result;
}

// TextEntry_Draw.
void MachineTestName_Draw(DrawLayer *layer)
{
    TextEntry_Draw((TextEntry *)rt::from_guest(((int)g_OVL0_EndFontInfo + 0x5c8)), (uint *)layer->ot);
}

void MachineTestResult_Enter(int back)
{
    QuickMenuPage *page = g_QuickMenu->activePage;
    int rank = g_OVL0_EndFontInfo->rank;
    uint color = 0x1652c4;
    char *text;
    RaceResultsWork *work;

    page->timer = 0;
    page->sel = -1;
    page->mode = 0;
    InfoBar_Init(&g_OVL0_EndFontInfo->info, &D_8005B60C);
    text = sR_OUTOFRANKING;
    D_8005B62C.flags = 0xe1;
    if ((unsigned int)rank < 10) {
        text = g_PlaceTexts[rank];
        color = g_PlaceColors[rank];
        D_8005B62C.flags = 0xc8;
    }
    TextItem_Init(&g_OVL0_EndFontInfo->place, &D_8005B62C, text);
    work = g_OVL0_EndFontInfo;
    *(uint *)work->place.rgbFrom = color;
    work->placeBar = D_8005B648;
    work->placeBar.t = -1;
    D_8005B6A0.label = sR_RECORD;
    InfoBar_Init(&work->total, &D_8005B6A0);
    g_TotalBarFade.fade[1] = -1;
    work = g_OVL0_EndFontInfo;
    work->totalBar = g_TotalBarFade.bar;
    work->totalBar.t = -1;
    CardDialog_Init(RESULT_DIALOG(work), &D_8005B5AC, 0);
    RaceResults_SetupCar(0);
    RaceResults_InitView(&g_OVL0_EndFontInfo->view);
}

int MachineTestResult_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int result = 0;
    int frame = page->timer;
    PadState *pad;
    QuickMenuPage *next;

    if (page->mode == 0) {
        frame++;
    }
    switch (frame) {
    case 0x18:
        page->sel = 0;
        break;
    case 0x30:
        InfoBar_Show(&g_OVL0_EndFontInfo->info);
        break;
    case 0x3c:
        TextItem_Show(&g_OVL0_EndFontInfo->place, -1);
        g_OVL0_EndFontInfo->placeBar.t = 0;
        break;
    case 0x48:
        InfoBar_Show(&g_OVL0_EndFontInfo->total);
        break;
    case 0x54:
        g_TotalBarFade.fade[1] = 0;
        g_OVL0_EndFontInfo->totalBar.t = 0;
        break;
    case 0x73: {
        int one = 1;

        page->mode = one;
        CardDialog_Open(RESULT_DIALOG(g_OVL0_EndFontInfo));
        RESULT_DIALOG(g_OVL0_EndFontInfo)->choice = one;
        frame = 0x74;
        break;
    }
    }
    page->timer = frame;
    InfoBar_Update(&g_OVL0_EndFontInfo->info);
    TextItem_Update(&g_OVL0_EndFontInfo->place);
    WipeBar_Update(&g_OVL0_EndFontInfo->placeBar);
    InfoBar_Update(&g_OVL0_EndFontInfo->total);
    FadeCounter_Update(g_TotalBarFade.fade);
    WipeBar_Update(&g_OVL0_EndFontInfo->totalBar);
    pad = &menu->padState[0];
    {
        int early = frame < 0x73;
        int inactive = active ^ 1;

        if (early | inactive) {
            pad = 0;
        }
    }
    next = &g_Page_Quit;
    switch (CardDialog_Update(RESULT_DIALOG(g_OVL0_EndFontInfo), pad)) {
    case -1:
        Sfx_PlayMenu(0);
        break;
    case 0:
        next = &g_Page_SaveGame;
    case 1:
        CardDialog_Close(RESULT_DIALOG(g_OVL0_EndFontInfo));
        Sfx_PlayMenu(3);
        InfoBar_Hide(&g_OVL0_EndFontInfo->info, -0x11);
        TextItem_Hide(&g_OVL0_EndFontInfo->place);
        result = 1;
        WipeBar_Hide(&g_OVL0_EndFontInfo->placeBar);
        InfoBar_Hide(&g_OVL0_EndFontInfo->total, -0x11);
        FadeCounter_Hide(g_TotalBarFade.fade);
        WipeBar_Hide(&g_OVL0_EndFontInfo->totalBar);
        page->sel = -1;
        QuickMenu_PushPage(menu, next);
        break;
    }
    RaceResults_SpinView(&g_OVL0_EndFontInfo->view, menu->drawStamp);
    return result;
}

void MachineTestResult_Draw(DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;
    int isSpeed = 1;
    QuickMenu *menu = g_QuickMenu;
    TextCtx *ctx = &menu->text;
    QuickMenuPage *page = menu->activePage;
    uint *ot2;
    uint *ot1;
    RECT area;
    int value;

    CardDialog_Draw(RESULT_DIALOG(g_OVL0_EndFontInfo), ot, ctx);
    ot2 = ot + 2;
    InfoBar_Draw(&g_OVL0_EndFontInfo->info, ot2, 0x28, 0x88, ctx);
    TextItem_Draw(&g_OVL0_EndFontInfo->place, ot, 0xb0, 0xb8, ctx);
    ot1 = ot + 1;
    WipeBar_Draw(&g_OVL0_EndFontInfo->placeBar, ot1, 0x60, 0x88);
    Prim_AddDrawMode(ot1, 0x220);
    InfoBar_Draw(&g_OVL0_EndFontInfo->total, ot2, 0x28, 0xf0, ctx);
    WipeBar_Draw(&g_OVL0_EndFontInfo->totalBar, ot1, 0x74, 0x100);
    Prim_AddDrawMode(ot1, 0x220);
    value = g_OVL0_EndFontInfo->totals[1];
    switch (RaceSetup_Get()->raceType) {
    case 7:
    case 8:
        value = g_OVL0_EndFontInfo->totals[0];
        isSpeed = 0;
        break;
    }
    AnimatedTime_Draw(g_TotalBarFade.fade, ot + 1, ctx, value, 0xb0, 0x110, 0, &D_8005B68C, isSpeed, 1);
    if (page->sel >= 0) {
        ResultCar_Draw(&g_OVL0_EndFontInfo->car, &menu->layers[4], &menu->layers[5],
                       &g_OVL0_EndFontInfo->view, 0);
    }
    area.x = 0;
    area.y = 0x96;
    area.w = 0x160;
    area.h = 0x12c;
    DrawLayer_SetArea(&menu->layers[4], &area);
    DrawLayer_SetArea(&menu->layers[5], &area);
}
}
