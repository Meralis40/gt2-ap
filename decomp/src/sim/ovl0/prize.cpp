// OVL0 800593f4-8005a474 (prize): Prize pages and money.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"
#include "ovl4.h"

// How InfoBar_Init sets up a bar.
struct InfoBarDesc;

// The prize pages' working memory.
struct PrizeWork {
    short step;
    signed char done;
    signed char closing;
    short place;
    short pad6;
    uint total;
    uint money;
    int gotCar;
    int isChampionship;
    uint speed;
    short creditsFade;
    short totalFade;
    InfoBar placeBar;
    TextItem placeText;
    InfoBar moneyBar;
    InfoBar totalBar;
    // The prompt starts over the dialog's last word.
    union {
        CardDialog dialog;
        struct {
            char pad[0x94];
            ButtonPrompt prompt;
        } after;
    } ui;
    int showCar;
    short carStep;
    short pad20e;
    ResultCar car;
    char pad240[0x354 - 0x210 - sizeof(ResultCar)];
    ResultCarView view;
    char unk430[1];
};

// What the race left in g_GameState, at run time offsets.
static inline EventPrizeInfo *Prize_Info(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbccc;

    return (EventPrizeInfo *)(base + offset);
}

static inline signed char *Race_Place(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xc508;

    return (signed char *)(base + offset);
}

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

extern "C" {
extern QuickMenu *g_QuickMenu;
extern PrizeWork *g_OVL0_EndFontInfo;
extern RaceMem g_Race_IsDemo;
extern QuickMenuPage g_Page_Quit;
extern QuickMenuPage g_Page_SaveGame;
extern QuickMenuPage g_Page_ChampRacePrize;
extern QuickMenuPage g_Page_ChampWonPrize;
extern QuickMenuPage g_Page_EventPrize;
extern uint D_801D1568;
extern InfoBarDesc D_8005D3D8;
extern TextStyle D_8005D3F8;
extern InfoBarDesc D_8005D414;
extern InfoBarDesc D_8005D434;
extern WipeBar D_8005D454;
extern CardDialogDesc D_8005D470;
extern ButtonPromptDesc D_8005D4A0;
extern char *g_PlaceTexts[];
extern uint g_PlaceColors[];
extern signed char D_8005AB50[];
extern Font g_OVL0_FontC;
extern Font g_OVL0_FontD;
extern char sR_NewCarAcquired[];
void QuickMenu_PushPage(QuickMenu *menu, QuickMenuPage *page);
void QuickMenu_SetupCardUiThen(QuickMenuPage *page, void *arg);
void InfoBar_Init(InfoBar *bar, InfoBarDesc *desc);
void InfoBar_Draw(InfoBar *bar, uint *ot, int x, int y, TextCtx *ctx);
void ResultCarView_Init(ResultCarView *view, int w, int h);
void Race_ShowPrizeDialog(int place, int money, int gotCar);
void Prize_DrawCredits(uint *ot, int x, int y, uint value, int fade, uint rgb);

static inline void InfoBar_Show(InfoBar *bar)
{
    TextItem_Show(&bar->label, -1);
    bar->bar.t = 0;
    bar->anim = 0;
}

static inline void InfoBar_Hide(InfoBar *bar, int anim)
{
    bar->bar.t = ~bar->bar.duration;
    TextItem_Hide(&bar->label);
    bar->anim = anim;
}

// The car turns in the view and comes closer for its first 90 frames.
void PrizeCarView_Approach(ResultCarView *view, int t)
{
    view->pos[2] = 0xc0000;
    if (t < 90) {
        view->pos[2] = (t << 19) / 90 + 0x40000;
    }
    view->rotZ = -0x40;
    if (t < 90) {
        view->rotZ = -((90 - t) * 6 + 0x40);
    }
    view->pos[1] = 0x4ccc;
    view->rotY = (view->rotY + 8) & 0x3fff;
}

void PrizeRoot_Enter(void)
{
    g_QuickMenu->activePage->timer = 0x10;
    g_OVL0_EndFontInfo->showCar = 0;
    QuickMenu_SetupCardUiThen(&g_Page_Quit, g_OVL0_EndFontInfo->unk430);
}

int ChampRacePrizeRoot_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            QuickMenu_PushPage(menu, &g_Page_ChampRacePrize);
            result = 1;
        }
    }
    return result;
}

int ChampWonPrizeRoot_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            QuickMenu_PushPage(menu, &g_Page_ChampWonPrize);
            result = 1;
        }
    }
    return result;
}

int EventPrizeRoot_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            QuickMenu_PushPage(menu, &g_Page_EventPrize);
            result = 1;
        }
    }
    return result;
}

// Empty.
void PrizeRoot_Draw(DrawLayer *layer)
{
}

#ifdef NON_MATCHING
void Race_ShowPrizeDialog(int place, int money, int gotCar)
{
    PrizeWork *w = g_OVL0_EndFontInfo;

    w->place = place;
    w->money = money;
    w->gotCar = gotCar;
    w->step = 0;
    w->total = D_801D1568 - money;
    w->done = 0;
    g_OVL0_EndFontInfo->closing = 0;
    w = g_OVL0_EndFontInfo;
    w->speed = 3;
    w->creditsFade = 0;
    w->totalFade = 0;
    w->showCar = 0;
    w->carStep = 0;
    InfoBar_Init(&g_OVL0_EndFontInfo->placeBar, &D_8005D3D8);
    TextItem_Init(&g_OVL0_EndFontInfo->placeText, &D_8005D3F8, g_PlaceTexts[place]);
    *(uint *)g_OVL0_EndFontInfo->placeText.rgbFrom = g_PlaceColors[place];
    InfoBar_Init(&g_OVL0_EndFontInfo->moneyBar, &D_8005D414);
    InfoBar_Init(&g_OVL0_EndFontInfo->totalBar, &D_8005D434);
    D_8005D454.t = -1;
    CardDialog_Init(&g_OVL0_EndFontInfo->ui.dialog, &D_8005D470, 0);
    ButtonPrompt_Init(&g_OVL0_EndFontInfo->ui.after.prompt, &D_8005D4A0);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Race_ShowPrizeDialog);
#endif

// A race of a championship: the place's money.
void Race_AwardPrizeMoney(void)
{
    signed char *placed = Race_Place();
    EventPrizeInfo *info = Prize_Info();
    int place = *placed;
    int index = place - 1;
    int money = info->prizes[index];
    Garage *garage = &g_GameState.garages[0];

    PlayerStats_AddRaceResult(&g_GameState.stats, place);
    Stats_AddWinnings(&g_GameState.stats, money);
    Garage_AddCredits(garage, money);
    Race_ShowPrizeDialog(index, money, 0);
    g_OVL0_EndFontInfo->isChampionship = 0;
    if (index == 0) {
        QuickMenu_PlayMusic(0x11);
    } else {
        QuickMenu_PlayMusic(0x12);
    }
}

// Random number below n, seeded from the vsync counter.
unsigned int Rand_Below(unsigned int n)
{
    unsigned int uVar1;
    unsigned int local_10 [2];

    local_10[0] = Irq_WaitVSyncs(0);
    uVar1 = Rand_Next(local_10);
    return uVar1 % n;
}

#ifdef NON_MATCHING
// The championship: its money, one of its cars, and that car's model for the page.
void Race_AwardChampionshipPrize(void)
{
    EventPrizeInfo *info = Prize_Info();
    Garage *garage = &g_GameState.garages[0];
    int money;
    int gotCar = 0;
    int count;
    char *base;
    int mask;
    int offset;
    uint *buf;
    uint *image;
    CarAsset *asset;
    char *model;
    RECT rect;
    unsigned short clut[0x100];

    money = info->championshipPrize;
    Garage_AddCredits(garage, money);
    count = (signed char)info->prizeCarCount;
    if (count != 0) {
        gotCar = Garage_AddCar(garage, &info->prizeCars[Rand_Below(count)]);
    }
    if (*(signed char *)&RaceSetup_Get()->worldLeague != 0) {
        ((char *)&g_GameState)[0x215] = 1;
    }
    offset = 0xbb397;
    base = (char *)&g_Race_IsDemo;
    mask = -4;
    buf = (uint *)(((int)base + offset) & mask);
    VolId_ReadFile((VolBootId)0x20, buf);
    image = buf + 2;
    if ((buf[1] >> 3) & 1) {
        image = (uint *)((char *)image + buf[2]);
    }
    rect.x = 0x280;
    rect.y = 0x100;
    rect.w = ((unsigned short *)image)[4];
    rect.h = ((unsigned short *)image)[5];
    Gpu_LoadImageBlock((int *)image, &rect);
    Gpu_DrawSync();
    asset = CarAsset_Find(CarCode_Encode(D_8005AB50));
    {
        int modelOffset = 0xb6397;

        model = (char *)(((int)base + modelOffset) & mask);
    }
    Vol_ReadFileAsync(asset->file, model);
    Vol_ReadFileAsync(asset->file + 1, buf);
    CarModel_Init((CarModelHeader *)model, 0x19);
    rect.x = 0x240;
    rect.y = 0x100;
    rect.w = 0x40;
    rect.h = 0xe0;
    Gpu_TransferImage(&rect, (unsigned int *)((char *)buf + 0x43a0), 0);
    Gpu_DrawSync();
    Clut_Blend(clut, (unsigned short *)(buf + 8), 0, 0);
    Clut_UploadToSlot(clut, 0x19, 0);
    {
        ResultCar *car = &g_OVL0_EndFontInfo->car;

        car->brightness = 0x20;
        car->enabled = 0;
        car->unk00 = 0x6028;
        car->tpage = 0x1a;
        car->model = model;
        GroundDisc_Init(&car->disc, 0xa0000);
    }
    ResultCarView_Init(&g_OVL0_EndFontInfo->view, 0x160, 0x1e0);
    g_OVL0_EndFontInfo->view.drawDisc = 0;
    Race_ShowPrizeDialog(0, money, gotCar);
    g_OVL0_EndFontInfo->isChampionship = 1;
    g_OVL0_EndFontInfo->showCar = 1;
    g_OVL0_EndFontInfo->step = -0x2d;
    QuickMenu_PlayMusic(9);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Race_AwardChampionshipPrize);
#endif

// A single event: the place's money, and a car for winning.
void Race_AwardEventPrize(void)
{
    int place = *Race_Place();
    EventPrizeInfo *info = Prize_Info();
    int index = place - 1;
    int money = info->prizes[index];
    int gotCar = 0;
    int count;

    PlayerStats_AddRaceResult(&g_GameState.stats, place);
    Stats_AddWinnings(&g_GameState.stats, money);
    PlayerStats_SetEventResult(&g_GameState.stats, RaceSetup_Get()->eventIndex, place);
    Garage_AddCredits(&g_GameState.garages[0], money);
    count = (signed char)info->prizeCarCount;
    if (count != 0 && index == 0) {
        gotCar = Garage_AddCar(&g_GameState.garages[0], &info->prizeCars[Rand_Below(count)]);
    }
    Race_ShowPrizeDialog(index, money, gotCar);
    g_OVL0_EndFontInfo->isChampionship = 1;
    if (index == 0) {
        QuickMenu_PlayMusic(0x11);
    } else {
        QuickMenu_PlayMusic(0x12);
    }
}

#ifdef NON_MATCHING
// Plays the page's steps, counts the money over and waits for the player.
int Prize_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    PrizeWork *w = g_OVL0_EndFontInfo;
    PadState *pad = menu->padState;
    int step = w->step;
    int result = 0;
    QuickMenuPage *next;
    int choice;

    if (w->done == 0) {
        step++;
    }
    switch (step) {
    case 0x30:
        InfoBar_Show(&w->placeBar);
        Sfx_PlayMenu(1);
        break;
    case 0x3c:
        TextItem_Show(&w->placeText, -1);
        Sfx_PlayMenu(1);
        break;
    case 0x48:
        InfoBar_Show(&w->moneyBar);
        Sfx_PlayMenu(1);
        break;
    case 0x54:
    case 0x6c:
        Sfx_PlayMenu(1);
        break;
    case 0x60:
        InfoBar_Show(&w->totalBar);
        Sfx_PlayMenu(1);
        break;
    case 0x90:
        if (w->money != 0) {
            uint buttons = *(uint *)pad;
            uint amount;

            w->speed++;
            if (w->speed > 3000) {
                w->speed = 3000;
            }
            amount = g_OVL0_EndFontInfo->speed / 3;
            amount = amount * amount / 50 + 1;
            if (buttons & 0xa00) {
                amount = amount * 10 + 1;
            }
            step = 0x8f;
            if (g_OVL0_EndFontInfo->money < amount) {
                amount = g_OVL0_EndFontInfo->money;
            }
            g_OVL0_EndFontInfo->money -= amount;
            g_OVL0_EndFontInfo->total += amount;
            if (g_OVL0_EndFontInfo->money != 0) {
                Sfx_PlayMenu(8);
            } else {
                Sfx_PlayMenu(3);
            }
        }
        break;
    case 0x92:
        if (w->gotCar != 0) {
            Sfx_PlayMenu(3);
            D_8005D454.t = 0;
        }
        break;
    case 0xce:
        w->done = 1;
        if (g_OVL0_EndFontInfo->isChampionship != 0) {
            CardDialog_Open(&g_OVL0_EndFontInfo->ui.dialog);
            *(unsigned char *)&g_OVL0_EndFontInfo->ui.dialog.choice = 1;
        } else {
            ButtonPrompt_Open(&g_OVL0_EndFontInfo->ui.after.prompt);
        }
        step++;
        break;
    }
    g_OVL0_EndFontInfo->step = step;
    InfoBar_Update(&g_OVL0_EndFontInfo->placeBar);
    TextItem_Update(&g_OVL0_EndFontInfo->placeText);
    InfoBar_Update(&g_OVL0_EndFontInfo->moneyBar);
    InfoBar_Update(&g_OVL0_EndFontInfo->totalBar);
    WipeBar_Update(&D_8005D454);
    g_OVL0_EndFontInfo->creditsFade = step - 0x54;
    g_OVL0_EndFontInfo->totalFade = step - 0x6c;
    if (g_OVL0_EndFontInfo->closing != 0) {
        g_OVL0_EndFontInfo->totalFade = -1;
        g_OVL0_EndFontInfo->creditsFade = -1;
    }
    if (g_OVL0_EndFontInfo->showCar != 0 && g_OVL0_EndFontInfo->carStep >= 0) {
        g_OVL0_EndFontInfo->carStep++;
        if (g_OVL0_EndFontInfo->carStep >= 0x19) {
            if (g_OVL0_EndFontInfo->carStep > 0x90) {
                g_OVL0_EndFontInfo->carStep = 0x90;
            }
            PrizeCarView_Approach(&g_OVL0_EndFontInfo->view, g_OVL0_EndFontInfo->carStep - 0x18);
        }
    }
    next = &g_Page_Quit;
    if (step < 0xce || (active ^ 1)) {
        pad = 0;
    }
    if (g_OVL0_EndFontInfo->isChampionship != 0) {
        choice = CardDialog_Update(&g_OVL0_EndFontInfo->ui.dialog, pad);
    } else {
        choice = ButtonPrompt_Update(&g_OVL0_EndFontInfo->ui.after.prompt, pad);
        switch (choice) {
        case -1:
            break;
        case 0:
            choice = 1;
            break;
        }
    }
    switch (choice) {
    case 0:
        next = &g_Page_SaveGame;
    case 1:
        g_OVL0_EndFontInfo->closing = 1;
        Sfx_PlayMenu(3);
        InfoBar_Hide(&g_OVL0_EndFontInfo->placeBar, -0x11);
        TextItem_Hide(&g_OVL0_EndFontInfo->placeText);
        InfoBar_Hide(&g_OVL0_EndFontInfo->moneyBar, -0x11);
        InfoBar_Hide(&g_OVL0_EndFontInfo->totalBar, -0x11);
        if (g_OVL0_EndFontInfo->gotCar != 0) {
            D_8005D454.t = ~D_8005D454.duration;
        }
        if (g_OVL0_EndFontInfo->isChampionship != 0) {
            CardDialog_Close(&g_OVL0_EndFontInfo->ui.dialog);
        } else {
            ButtonPrompt_Close(&g_OVL0_EndFontInfo->ui.after.prompt);
        }
        result = 1;
        g_OVL0_EndFontInfo->carStep = -1;
        menu->unk7c = 2;
        QuickMenu_PushPage(menu, next);
        break;
    case -1:
        Sfx_PlayMenu(0);
        break;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Prize_Update);
#endif

// A money figure, fading in from white over 12 frames.
void Prize_DrawCredits(uint *ot, int x, int y, uint value, int fade, uint rgb)
{
    char buf[0x40];
    TextCtx ctx;
    uint white = 0x2000000;

    if (fade >= 0) {
        if (fade > 12) {
            fade = 12;
        }
        Text_Init(&ctx, 6);
        ctx.ot = ot;
        Text_SetFont(&ctx, &g_OVL0_FontC);
        *(unsigned int *)&ctx.clut = (*(unsigned int *)&ctx.clut & ~0x600000) | 0x200000;
        ctx.rgb = Rgb_Lerp((unsigned char *)&white, (unsigned char *)&rgb, fade, 12);
        Credits_FormatCommas(buf, value);
        Text_DrawStringMonoDigitsRight(&ctx, (signed char *)buf, x, y, 2, 0, 0);
    }
}

#ifdef NON_MATCHING
void Prize_Draw(DrawLayer *layer)
{
    uint from = 0x2000000;
    uint to;
    uint *ot = (uint *)layer->ot;
    QuickMenu *menu = g_QuickMenu;
    TextCtx *text = &menu->text;
    TextCtx *ctx;
    uint *ot2;

    CardDialog_Draw(&g_OVL0_EndFontInfo->ui.dialog, ot, text);
    ButtonPrompt_Draw(&g_OVL0_EndFontInfo->ui.after.prompt, ot, text);
    ctx = text;
    if (D_8005D454.t >= 0) {
        to = 0x2505050;
        Text_Init(text, 6);
        text->ot = ot;
        Text_SetFont(text, &g_OVL0_FontD);
        *(unsigned int *)&text->clut = (*(unsigned int *)&text->clut & ~0x600000) | 0x200000;
        text->rgb = Rgb_Lerp((unsigned char *)&from, (unsigned char *)&to, WipeBar_GetLevel(&D_8005D454), 0x80);
        Text_DrawCentered(text, (signed char *)sR_NewCarAcquired, 0xb0, 0x128, 1);
    }
    WipeBar_Draw(&D_8005D454, ot, 0, 0x10c);
    Prim_AddDrawMode(ot, 0x220);
    ot2 = ot + 2;
    InfoBar_Draw(&g_OVL0_EndFontInfo->placeBar, ot2, 0x28, 0x88, ctx);
    TextItem_Draw(&g_OVL0_EndFontInfo->placeText, ot, 0xb0, 0xb8, ctx);
    InfoBar_Draw(&g_OVL0_EndFontInfo->moneyBar, ot2, 0x28, 0xd2, ctx);
    Prize_DrawCredits(ot, 0x100, 0x102, g_OVL0_EndFontInfo->money, g_OVL0_EndFontInfo->creditsFade, 0x2503c28);
    InfoBar_Draw(&g_OVL0_EndFontInfo->totalBar, ot2, 0x28, 0x140, ctx);
    Prize_DrawCredits(ot, 0x100, 0x170, g_OVL0_EndFontInfo->total, g_OVL0_EndFontInfo->totalFade, 0x2505050);
    if (g_OVL0_EndFontInfo->showCar != 0 && g_OVL0_EndFontInfo->carStep >= 0x18) {
        ResultCar_Draw(&g_OVL0_EndFontInfo->car, &menu->layers[4], &menu->layers[5], &g_OVL0_EndFontInfo->view, 1);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/prize", Prize_Draw);
#endif
}
