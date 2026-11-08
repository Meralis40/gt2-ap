// OVL0 80057a70-800593f4 (gtracemenu): GT race menu and machine test.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
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

static inline RaceCar *RaceSetup_Car0(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbfd8;

    return (RaceCar *)(base + offset);
}

static inline int RaceSetup_Type(void)
{
    return RaceSetup_Get()->raceType;
}

static inline char *RaceSetup_CourseName(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf9c;

    return base + offset;
}

// The championship's progress in g_GameState (0xc514).
struct ChampProgress {
    short race;
    short count;
};

static inline ChampProgress *Champ_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xc514;

    return (ChampProgress *)(base + offset);
}

// The menu inside its race loop: the two bytes after it are RaceLoop's raceDoneA and B.
struct LoopMenu {
    QuickMenu menu;
    unsigned char raceDone;  // the race has been run, so its settings are used
    unsigned char showCar;
};

// A row of these menus.
struct GtMenuOption {
    game_pointer<char> label;
    signed char enabled;
    signed char next;
};

// The pages' working memory.
struct GtMenuWork {
    TextItem items[8];
    WipeBar bars[8];
    ResultCar car;
    ResultCarView view;
    TitleBanner banner;
    // Two dialogs 0x94 bytes apart, four short of sizeof(CardDialog).
    unsigned char startDialog[0x94];    // the confirmation before a race, on D_8005AC20
    unsigned char quitDialog[0x94];
    char title[0x40];   // "Session n"
};

#define START_DIALOG ((CardDialog *)g_OVL0_EndFontInfo->startDialog)
#define QUIT_DIALOG ((CardDialog *)g_OVL0_EndFontInfo->quitDialog)

// A rounded box for Prim_AddRoundedBox: its colour and code, then its rectangle.
struct RoundedBox {
    unsigned int rgb;
    RECT rect;
};

extern "C" {
extern game_pointer<LoopMenu> (&g_QuickMenu);
extern game_pointer<GtMenuWork> (&g_OVL0_EndFontInfo);
extern GtMenuOption (&g_GtRaceMenu_Options)[];
extern GtMenuOption (&g_MachineTestMenu_Options)[];
extern signed char (&D_8005D27C)[];    // the GT race rows' y offsets
extern signed char (&D_8005D2E8)[];    // the machine test rows' y offsets
extern MenuList (&g_GtRaceMenu_List);
extern MenuList (&g_MachineTestMenu_List);
extern signed char (&g_GtRaceMenu_SettingsUsed);
extern game_pointer<char> (&g_GhostTypeNames2)[];
extern QuickMenuPage (&g_Page_GtRaceMenu);
extern QuickMenuPage (&g_Page_MachineTestMenu);
extern QuickMenuPage (&g_Page_MachineTestRecords);
extern QuickMenuPage (&g_Page_Quit);
extern QuickMenuPage (&g_Page_SaveReplay);
extern QuickMenuPage (&g_Page_ChangeParts);
extern game_pointer<char> (&D_8005D37C);            // g_Page_GtRaceMenu.page.title
extern TextStyle (&D_8005D20C);
extern WipeBar (&D_8005D228);
extern CardDialogDesc (&D_8005AC20);
extern CardDialogDesc (&D_8005ACF0);
extern game_pointer<void> (&D_800A9F00);
extern unsigned char (&D_8005D208)[];
extern unsigned char (&D_8005D334)[];
extern unsigned char (&D_8005D338)[];
extern unsigned char (&D_8005D33C)[];
extern unsigned char (&D_8005D340)[];
extern unsigned char (&D_8005D344)[];
extern char (&D_8005AB48)[];           // "%d."
extern char (&D_80169897);
extern Font (&g_OVL0_FontB);
extern Font (&g_OVL0_FontD);
extern char (&sR_SINGLERACE)[];
extern char (&sR_RALLY)[];
extern char (&sR_SESSIONN)[];
extern char (&sR_Start)[];
extern char (&sR_NoRecords)[];
extern char (&sR_Mph)[];
extern char (&sB_0400m)[];
extern char (&sB_01000m)[];
extern char (&sB_MaxSpeed)[];
void QuickMenu_PushPage(LoopMenu *menu, QuickMenuPage *page);
void TitleBanner_Init(TitleBanner *banner, char *text, TextCtx *ctx);
void ResultCarView_Init(ResultCarView *view, int w, int h);

// Goes to the parts change; the rows that need the settings unused go off.
static inline int GtRaceMenu_OpenParts(QuickMenuPage *page)
{
    int pushed = 1;

    g_GtRaceMenu_SettingsUsed = pushed;
    QuickMenu_PushPage(g_QuickMenu, page);
    return pushed;
}

// The car on the menu's stage.
static inline void GtMenu_InitCar(char *work)
{
    ResultCar *car = (ResultCar *)(work + 0x220);

    car->brightness = 0x40;
    car->enabled = 1;
    *(short *)(work + 0x220) = 0x7fd7;
    car->tpage = 9;
    car->model = D_800A9F00;
    work += 0x22c;
    GroundDisc_Init((GroundDisc *)work, 0xa0000);
}

// Centered text in a colour.
static inline void Text_DrawCenteredRgb(TextCtx *ctx, signed char *s, int x, int y, int spacing, unsigned int rgb)
{
    ctx->rgb = rgb;
    Text_DrawCentered(ctx, s, x, y, spacing);
}

// Leaves for the quit page, with the choice it should act on.
static inline int QuickMenu_Quit(LoopMenu *menu, int action)
{
    int pushed = 1;

    menu->menu.unk7c = action;
    QuickMenu_PushPage(menu, &g_Page_Quit);
    return pushed;
}

// A row of the GT race or machine test menu: its text item and wipe bar. Both menus
// use it; the race type picks the table.
int GtRaceMenu_ItemCallback(MenuListEventId event, MenuListEvent *e, int i)
{
    TextCtx ctx;
    int result = 0;
    GtMenuOption *options = g_GtRaceMenu_Options;
    int y = e->y + D_8005D27C[i];
    TextItem *item;
    WipeBar *bar;

    switch (RaceSetup_Type()) {
    case 7:
    case 8:
    case 9:
        options = g_MachineTestMenu_Options;
        y = e->y + D_8005D2E8[i];
        break;
    }
    item = &g_OVL0_EndFontInfo->items[i];
    bar = &g_OVL0_EndFontInfo->bars[i];
    Text_Init(&ctx, 6);
    ctx.unk1c = 1;
    switch (event) {
    case MLE_INIT:
        TextItem_Init(item, &D_8005D20C, options[i].label);
        *bar = D_8005D228;
        bar->t = -1;
        break;
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

        item->brightness = brightness;
        TextItem_Draw(item, (uint *)e->ot, e->x, y + 0x18, &ctx);
        WipeBar_Draw(bar, (uint *)e->ot + 1, e->x - 8, y - 2);
        Prim_AddDrawMode((uint *)e->ot + 1, 0x220);
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
        result = options[i].enabled;
        break;
    }
    return result;
}

// Root page of the GT race menu: stores back, 20-frame delay, card UI on the work area
// after the settings work, music 8.
void GtRaceRoot_Enter(int back)
{
    void *pvVar1;
    QuickMenuPage *pQVar2;

    pvVar1 = g_OVL0_EndFontInfo;
    pQVar2 = g_QuickMenu->menu.activePage;
    pQVar2->sel = (short)back;
    pQVar2->timer = 0x14;
    pvVar1 = Settings_SetWork((void *)rt::from_guest(((int)rt::to_guest(pvVar1) + 0x5ec)));
    QuickMenu_SetupCardUi(pvVar1);
    QuickMenu_PlayMusic(8);
}

// After the wait, the machine test or GT race menu, titled for the race.
int GtRaceRoot_Update(int active)
{
    QuickMenuPage *page = g_QuickMenu->menu.activePage;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            QuickMenuPage *next = &g_Page_GtRaceMenu;

            switch (RaceSetup_Type()) {
            case 7:
            case 8:
            case 9:
                next = &g_Page_MachineTestMenu;
                break;
            case 10:
                next->page.title = sR_RALLY;
                break;
            case 0:
            default:
            {
                char *base = (char *)&g_GameState;
                int offset = 0xc514;
                ChampProgress *champ = (ChampProgress *)(base + offset);

                if (champ->count < 2) {
                    D_8005D37C = sR_SINGLERACE;
                } else {
                    sprintf(g_OVL0_EndFontInfo->title, sR_SESSIONN, champ->race + 1);
                    D_8005D37C = g_OVL0_EndFontInfo->title;
                }
                break;
            }
            }
            QuickMenu_PushPage(g_QuickMenu, next);
            result = 1;
        }
    }
    return result;
}

// Empty.
void GtRaceRoot_Draw(DrawLayer *layer)
{
}

// Sets up both dialogs, the rows that depend on the race having been run, the list,
// the car and the banner.
void GtRaceMenu_Enter(int back)
{
    LoopMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->menu.activePage;
    GtMenuOption *options;
    int on;

    page->timer = 0x10;
    page->sel = -1;
    page->mode = 0;
    CardDialog_Init(QUIT_DIALOG, &D_8005ACF0, 0);
    {
        int x = 0xb0;
        int y = 0x19a;

        QUIT_DIALOG->x = x;
        QUIT_DIALOG->y = y;
        CardDialog_Init(START_DIALOG, &D_8005AC20, 0);
        START_DIALOG->x = x;
        START_DIALOG->y = y;
    }
    if (back == 0) {
        g_GtRaceMenu_SettingsUsed = 0;
    }
    options = g_GtRaceMenu_Options;
    options[0].enabled = 0;
    options[3].enabled = 0;
    {
        bool done = menu->raceDone != 0;

        on = 0;
        if (g_GtRaceMenu_SettingsUsed == 0) {
            on = done & 1;
        }
    }
    if (on) {
        options[0].enabled = 1;
        options[3].enabled = 1;
    }
    if (RaceSetup_Get()->raceType != 10) {
        g_GtRaceMenu_List.count = 6;
        g_GtRaceMenu_List.visibleRows = 6;
        options[5] = options[6];
        D_8005D27C[5] = D_8005D27C[6];
    }
    if (back != 0) {
        int selected = g_GtRaceMenu_List.selected;

        MenuList_Init(&g_GtRaceMenu_List, GtRaceMenu_ItemCallback, 0);
        g_GtRaceMenu_List.selected = selected;
    } else {
        MenuList_Init(&g_GtRaceMenu_List, GtRaceMenu_ItemCallback, 0);
        g_GtRaceMenu_SettingsUsed = 0;
    }
    if (menu->showCar != 0) {
        GtMenu_InitCar((char *)g_OVL0_EndFontInfo);
        ResultCarView_Init(&g_OVL0_EndFontInfo->view, 200, 200);
    }
    TitleBanner_Init(&g_OVL0_EndFontInfo->banner, RaceSetup_CourseName(), &menu->menu.text);
    g_OVL0_EndFontInfo->banner.bar.t = 0;
    TextItem_Show(&g_OVL0_EndFontInfo->banner.text, -1);
}

// The next of the four ghost settings.
void GhostType_Next(void)
{
    char *base = (char *)&g_GameState;
    int type = (unsigned char)base[0xb5];

    type++;
    if (type >= 4) {
        type = 0;
    }
    base[0xb5] = type;
}

// Opens the list after the wait, then runs the list or whichever dialog is up. The
// rows' next value picks where to go; a race with a save needs the dialog first
// unless its event says otherwise.
int GtRaceMenu_Update(int active)
{
    LoopMenu *menu = g_QuickMenu;
    int result = 0;
    int leaving = result;
    QuickMenuPage *page = menu->menu.activePage;
    PadState *pad = menu->menu.padState;
    int choice;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            MenuList_Open(&g_GtRaceMenu_List);
            g_GtRaceMenu_List.revealDelay = -1;
            page->sel = 0;
        }
    }
    {
        GtMenuWork *work = g_OVL0_EndFontInfo;

        WipeBar_Update(&work->banner.bar);
        TextItem_Update(&work->banner.text);
    }
    if (menu->showCar != 0) {
        ResultCarView_Spin(&g_OVL0_EndFontInfo->view, menu->menu.drawStamp);
    }
    if (active == 0) {
        pad = 0;
    }
    switch (page->mode) {
    case 2:
        MenuList_Update(&g_GtRaceMenu_List, 0);
        CardDialog_Update(QUIT_DIALOG, 0);
        choice = CardDialog_Update(START_DIALOG, pad);
        switch (choice) {
        case -3:
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            page->mode = 0;
            break;
        default:
        {
            char *base = (char *)&g_GameState;

            RaceSetup_Car0()->unk8f = choice;
            base[0x7c8e] = choice;
            Sfx_PlayMenu(3);
            leaving = 1;
            result = QuickMenu_Quit(menu, page->arg);
            break;
        }
        }
        break;
    case 1:
        MenuList_Update(&g_GtRaceMenu_List, 0);
        CardDialog_Update(START_DIALOG, 0);
        choice = CardDialog_Update(QUIT_DIALOG, pad);
        switch (choice) {
        case -3:
        case -2:
            break;
        case 0:
            Sfx_PlayMenu(3);
            leaving = 1;
            result = QuickMenu_Quit(menu, 2);
            break;
        case -1:
            Sfx_PlayMenu(2);
            page->mode = 0;
            break;
        default:
            Sfx_PlayMenu(1);
            page->mode = 0;
            break;
        }
        break;
    default:
        CardDialog_Update(START_DIALOG, 0);
        CardDialog_Update(QUIT_DIALOG, 0);
        choice = MenuList_Update(&g_GtRaceMenu_List, pad);
        switch (choice) {
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -2:
            break;
        case -4:
        case -1:
            Sfx_PlayMenu(0);
            break;
        default:
        {
            GtMenuOption *options = g_GtRaceMenu_Options;
            GtMenuOption *option = &options[choice];
            int type = RaceSetup_Get()->raceType;
            int one = 1;
            int next = option->next;

            if (type == one || type == 10) {
                switch (next) {
                case -5:
                    Sfx_PlayMenu(1);
                    GhostType_Next();
                    goto done;
                case 1:
                case 4:
                {
                    char *base = (char *)&g_GameState;

                    if ((RaceSetup_Get()->flags >> 3) & 1) {
                        Sfx_PlayMenu(3);
                        leaving = 1;
                        result = QuickMenu_Quit(menu, next);
                    } else {
                        Sfx_PlayMenu(1);
                        START_DIALOG->choice = base[0x7c8e] != 0;
                        CardDialog_Open(START_DIALOG);
                        page->mode = 2;
                        page->arg = next;
                    }
                    goto done;
                }
                case 2:
                    Sfx_PlayMenu(1);
                    CardDialog_Open(QUIT_DIALOG);
                    QUIT_DIALOG->choice = one;
                    page->mode = one;
                    goto done;
                }
            }
            Sfx_PlayMenu(3);
            leaving = 1;
            switch (next) {
            case -3:
                result = GtRaceMenu_OpenParts(&g_Page_ChangeParts);
                break;
            case -2:
                QuickMenu_PushPage(menu, &g_Page_SaveReplay);
                result = 1;
                break;
            default:
                result = QuickMenu_Quit(menu, g_GtRaceMenu_Options[choice].next);
                break;
            }
            break;
        }
        }
        break;
    }
done:
    if (leaving) {
        MenuList_Close(&g_GtRaceMenu_List);
        {
            TitleBanner *banner = &g_OVL0_EndFontInfo->banner;

            banner->bar.t = ~banner->bar.duration;
            TextItem_Hide(&banner->text);
        }
        page->sel = -1;
    }
    return result;
}

// The dialogs, the rally ghost setting under the list, the banner, the list and the car.
void GtRaceMenu_Draw(DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;
    LoopMenu *menu = g_QuickMenu;
    TextCtx *text = &menu->menu.text;
    QuickMenuPage *page = menu->menu.activePage;
    MenuList *list;
    RECT area;
    int on;

    CardDialog_Draw(START_DIALOG, ot, text);
    CardDialog_Draw(QUIT_DIALOG, ot, text);
    list = &g_GtRaceMenu_List;
    if (list->state >= 0) {
        char *base = (char *)&g_GameState;
        int t = (list->fade << 7) / list->fadeMax;

        if (RaceSetup_Get()->raceType == 10) {
            Text_Init(text, 6);
            text->ot = ot;
            Text_SetFont(text, &g_OVL0_FontB);
            *(unsigned int *)&text->clut = (*(unsigned int *)&text->clut & ~0x600000) | 0x200000;
            text->rgb = Rgb_Lerp(D_8005D208, D_8005D334, t, 0x80);
            Text_DrawString(text, (signed char *)g_GhostTypeNames2[(unsigned char)base[0xb5]], 0xa0, 0x184, 1);
        }
    }
    TitleBanner_Draw(&g_OVL0_EndFontInfo->banner, ot, 0xb0, 0x68, text);
    MenuList_Draw(&g_GtRaceMenu_List, ot);
    {
        bool show = menu->showCar != 0;

        on = 0;
        if (page->sel >= 0) {
            on = show & 1;
        }
    }
    if (on) {
        ResultCar_Draw(&g_OVL0_EndFontInfo->car, &menu->menu.layers[4], &menu->menu.layers[5], &g_OVL0_EndFontInfo->view, 0);
    }
    area.x = 0x7c;
    area.y = 0xa0;
    area.w = 0xc8;
    area.h = 0xc8;
    DrawLayer_SetArea(&menu->menu.layers[4], &area);
    DrawLayer_SetArea(&menu->menu.layers[5], &area);
}

// As GtRaceMenu_Enter, with one dialog. The start row's label changes once the race
// has been run.
void MachineTestMenu_Enter(int back)
{
    LoopMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->menu.activePage;
    GtMenuOption *options;
    char *label;
    char *title;
    int on;

    page->timer = 0x10;
    page->sel = -1;
    page->mode = 0;
    CardDialog_Init(START_DIALOG, &D_8005AC20, 0);
    START_DIALOG->x = 0xb0;
    START_DIALOG->y = 0x19a;
    if (back == 0) {
        g_GtRaceMenu_SettingsUsed = 0;
    }
    options = g_MachineTestMenu_Options;
    options[0].enabled = 0;
    options[4].enabled = 0;
    {
        bool done = menu->raceDone != 0;

        on = 0;
        if (g_GtRaceMenu_SettingsUsed == 0) {
            on = done & 1;
        }
    }
    if (on) {
        options[0].enabled = 1;
        options[4].enabled = 1;
    }
    label = sR_Start;
    options[1].label = label;
    if (menu->raceDone != 0) {
        options[1].label = label - 0x239;
    }
    if (back != 0) {
        int selected = g_MachineTestMenu_List.selected;

        MenuList_Init(&g_MachineTestMenu_List, GtRaceMenu_ItemCallback, 0);
        g_MachineTestMenu_List.selected = selected;
    } else {
        MenuList_Init(&g_MachineTestMenu_List, GtRaceMenu_ItemCallback, 0);
    }
    if (menu->showCar != 0) {
        GtMenu_InitCar((char *)g_OVL0_EndFontInfo);
        ResultCarView_Init(&g_OVL0_EndFontInfo->view, 200, 200);
    }
    switch (RaceSetup_Get()->raceType) {
    case 7:
        title = sB_0400m;
        break;
    case 8:
        title = sB_01000m;
        break;
    default:
        title = sB_MaxSpeed;
        break;
    }
    TitleBanner_Init(&g_OVL0_EndFontInfo->banner, title, &menu->menu.text);
    g_OVL0_EndFontInfo->banner.bar.t = 0;
    TextItem_Show(&g_OVL0_EndFontInfo->banner.text, -1);
}

// The list, or the dialog before a test. A row's next value picks where to go.
int MachineTestMenu_Update(int active)
{
    LoopMenu *menu = g_QuickMenu;
    int result = 0;
    int leaving = result;
    QuickMenuPage *page = menu->menu.activePage;
    PadState *pad = menu->menu.padState;
    int choice;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            MenuList_Open(&g_MachineTestMenu_List);
            g_MachineTestMenu_List.revealDelay = -1;
            page->sel = 0;
        }
    }
    {
        GtMenuWork *work = g_OVL0_EndFontInfo;

        WipeBar_Update(&work->banner.bar);
        TextItem_Update(&work->banner.text);
    }
    if (menu->showCar != 0) {
        ResultCarView_Spin(&g_OVL0_EndFontInfo->view, menu->menu.drawStamp);
    }
    if (active == 0) {
        pad = 0;
    }
    switch (page->mode) {
    case 1:
        MenuList_Update(&g_MachineTestMenu_List, 0);
        choice = CardDialog_Update(START_DIALOG, pad);
        switch (choice) {
        case -3:
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            page->mode = 0;
            break;
        default:
        {
            char *base = (char *)&g_GameState;

            RaceSetup_Car0()->unk8f = choice;
            base[0x7c8e] = choice;
            Sfx_PlayMenu(3);
            leaving = 1;
            QuickMenu_PushPage(menu, &g_Page_Quit);
            result = leaving;
            break;
        }
        }
        break;
    case 0:
        CardDialog_Update(START_DIALOG, 0);
        choice = MenuList_Update(&g_MachineTestMenu_List, pad);
        switch (choice) {
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -2:
            break;
        case -4:
        case -1:
            Sfx_PlayMenu(0);
            break;
        default:
        {
            GtMenuOption *options = g_MachineTestMenu_Options;
            int next = options[choice].next;

            switch (next) {
            case 1:
                if (((RaceSetup_Get()->flags >> 3) & 1) == 0) {
                    char *base = (char *)&g_GameState;

                    Sfx_PlayMenu(1);
                    START_DIALOG->choice = base[0x7c8e] != 0;
                    CardDialog_Open(START_DIALOG);
                    page->mode = next;
                    menu->menu.unk7c = next;
                    goto done;
                }
                goto quit;
            case -3:
            {
                QuickMenuPage *parts = &g_Page_ChangeParts;

                g_GtRaceMenu_SettingsUsed = result = 1;
                QuickMenu_PushPage(g_QuickMenu, parts);
                break;
            }
            case -2:
                QuickMenu_PushPage(menu, &g_Page_SaveReplay);
                result = 1;
                break;
            case -4:
                QuickMenu_PushPage(menu, &g_Page_MachineTestRecords);
                result = 1;
                break;
            default:
            quit:
                result = QuickMenu_Quit(menu, next);
                break;
            }
            Sfx_PlayMenu(3);
            leaving = 1;
            break;
        }
        }
        break;
    }
done:
    if (leaving) {
        MenuList_Close(&g_MachineTestMenu_List);
        {
            TitleBanner *banner = &g_OVL0_EndFontInfo->banner;

            banner->bar.t = ~banner->bar.duration;
            TextItem_Hide(&banner->text);
        }
        page->sel = -1;
    }
    return result;
}

void MachineTestMenu_Draw(DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;
    LoopMenu *menu = g_QuickMenu;
    TextCtx *text = &menu->menu.text;
    QuickMenuPage *page = menu->menu.activePage;
    RECT area;
    LoopMenu *loop;
    int on;

    CardDialog_Draw(START_DIALOG, ot, text);
    TitleBanner_Draw(&g_OVL0_EndFontInfo->banner, ot, 0xb0, 0x68, text);
    MenuList_Draw(&g_MachineTestMenu_List, ot);
    loop = menu;
    {
        bool show = menu->showCar != 0;

        on = 0;
        if (page->sel >= 0) {
            on = show & 1;
        }
    }
    if (on) {
        ResultCar_Draw(&g_OVL0_EndFontInfo->car, &menu->menu.layers[4], &menu->menu.layers[5], &g_OVL0_EndFontInfo->view, 0);
    }
    area.x = 0x7c;
    area.y = 0xa0;
    area.w = 0xc8;
    area.h = 0xc8;
    DrawLayer_SetArea(&loop->menu.layers[4], &area);
    DrawLayer_SetArea(&loop->menu.layers[5], &area);
}

// One row of the ranking: rank, the result (a speed for the top speed test), the name
// and car, or "No Records", on a rounded box.
void MachineTestRecord_DrawRow(uint *ot, int rank, MachineTestEntry *entry, char *carName, int x, int y, int t, int isSpeed)
{
    TextCtx ctx;
    char buf[0x40];
    RoundedBox box;
    int value = entry->result;
    int ms;
    TextCtx *c;
    unsigned char *from;
    char *s;
    int textY;
    int spacing;

    x += (0x80 - t) * 2;
    Text_Init(&ctx, 6);
    c = &ctx;
    c->unk1c = 1;
    c->ot = ot;
    Text_SetFont(c, &g_OVL0_FontD);
    from = D_8005D208;
    *(unsigned int *)&c->clut = (*(unsigned int *)&c->clut & ~0x600000) | 0x200000;
    c->rgb = Rgb_Lerp(from, D_8005D338, t, 0x80);
    s = buf;
    sprintf(s, D_8005AB48, rank + 1);
    spacing = 1;
    textY = y + 0xc;
    Text_DrawStringMonoDigits(c, (signed char *)s, x - 2, textY, spacing, -2, 0);
    ms = value;
    if (~entry->result == 0) {
        Text_SetFont(c, &g_OVL0_FontB);
        Text_DrawCenteredRgb(c, (signed char *)sR_NoRecords, x + 0x70, y + 8, spacing, Rgb_Lerp(from, D_8005D340, t, 0x80));
    } else {
        c->rgb = Rgb_Lerp(from, D_8005D33C, t, 0x80);
        if (isSpeed != 0) {
            Speed_Format(s, value);
            strcat(s, sR_Mph);
            Text_DrawStringMonoDigitsRight(c, (signed char *)s, x + 0x60, textY, spacing, -3, 0);
        } else {
            Time_FormatMs(s, ms);
            Text_DrawDigitsRight(c, (signed char *)s, x + 0x60, textY, 8, 7, 0, 0);
        }
        Text_SetFont(&ctx, &g_OVL0_FontB);
        {
            unsigned char *from2 = D_8005D208;
            unsigned char *to = D_8005D340;
            TextCtx *c2 = &ctx;
            int nameX = x + 0x70;
            int spacing2 = 1;

            c2->rgb = Rgb_Lerp(from2, to, t, 0x80);
            Text_DrawString(c2, (signed char *)entry->name, nameX, y, spacing2);
            c2->rgb = Rgb_Lerp(from2, to, t, 0x80);
            Text_DrawString(c2, (signed char *)carName, nameX, y + 0x12, spacing2);
        }
    }
    box.rect.w = 0x190;
    box.rect.h = 0x28;
    box.rect.x = x + 0xc8;
    box.rect.y = y;
    box.rgb = Rgb_Lerp(D_8005D208, D_8005D344, t, 0x80);
    Prim_AddRoundedBox(ot, &box, 0x18);
    Prim_AddDrawMode(ot, 0);
}

// Machine test ranking page: resets the slide-in counter.
void MachineTestRecords_Enter(int back)
{
    QuickMenuPage *pQVar1;

    pQVar1 = g_QuickMenu->menu.activePage;
    pQVar1->timer = 0;
    pQVar1->sel = 0;
}

// The slide-in counter, then any of the four buttons leaves.
int MachineTestRecords_Update(int active)
{
    LoopMenu *menu = g_QuickMenu;
    int result = 0;
    QuickMenuPage *page = menu->menu.activePage;
    PadState *pad = menu->menu.padState;
    int t = page->timer;

    if (page->sel != 0) {
        if (--t < 0) {
            t = 0;
        }
    } else {
        if (++t >= 0x39) {
            t = 0x38;
        }
    }
    page->timer = t;
    if (t >= 0x11 && (active & 1) != 0 && (pad->pressed & 0xf00) != 0) {
        Sfx_PlayMenu(3);
        result = 2;
        page->timer = 0xc;
        page->sel = 1;
    }
    return result;
}

// The eight rows of this test's ranking, each sliding in four frames after the last.
void MachineTestRecords_Draw(DrawLayer *layer)
{
    QuickMenuPage *page = g_QuickMenu->menu.activePage;
    int isSpeed = 0;
    int counter = page->timer;
    char *base = (char *)&g_GameState;
    int type = RaceSetup_Get()->raceType;
    uint *ot = (uint *)layer->ot;
    MachineTestRecords *records;
    int x;
    int y;
    int i;

    switch (type) {
    case 7:
        records = (MachineTestRecords *)(base + 0x3a88);
        break;
    case 8:
        records = (MachineTestRecords *)(base + 0x3b2c);
        break;
    case 9:
    default:
        isSpeed = 1;
        records = (MachineTestRecords *)(GameState_Base() + 0x3bd0);
        break;
    }
    x = 0x20;
    y = 0x78;
    for (i = 0; i < 8; i++) {
        int t;

        if (page->sel != 0) {
            t = (counter << 7) / 12;
        } else {
            t = counter - 0x10;
            if (t > 0) {
                if (t > 8) {
                    t = 8;
                }
                t = (t << 7) / 8;
            }
            counter -= 4;
        }
        if (t > 0) {
            MachineTestEntry *entry = &records->entries[i];
            char *name = CarNameTable_Find((CarNameTable *)rt::from_guest(((int)rt::to_guest(&D_80169897) & -4)), entry->carId);

            MachineTestRecord_DrawRow(ot, i, entry, name, x, y, t, isSpeed);
        }
        x += 4;
        y += 0x2c;
    }
}
}
