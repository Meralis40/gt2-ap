// OVL0 80049a14-8004a718 (resultmenu): Result menu pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"
#include "gamestate.h"

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

// A row of the result menu.
struct ResultOption {
    char *label;
    signed char enabled;
    signed char next;
};

// The pages' working memory.
struct ResultWork {
    TextItem items[8];
    WipeBar bars[8];
    ResultCar car;
    ResultCarView view;
    TitleBanner banner;
    InfoBar info[5];
    char wins[2][0x20];
    CardDialog dialog;
};

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

// The championship's progress in g_GameState (0xc514).
struct ChampProgress {
    short race;
    short count;
};

extern "C" {
extern QuickMenu *g_QuickMenu;
extern ResultWork *g_OVL0_EndFontInfo;
extern ResultOption *g_ResultMenu_Options;
extern ResultOption g_ResultMenu_OptionsSingle[];
extern ResultOption g_ResultMenu_OptionsChamp[];
extern ResultOption g_ResultMenu_OptionsChampLast[];
extern ResultOption g_ResultMenu_OptionsRally[];
extern MenuList g_ResultMenu_List;
extern char *g_ResultMenu_PlaceTexts[];
extern QuickMenuPage g_Page_ResultMenu;
extern QuickMenuPage g_Page_Quit;
extern QuickMenuPage g_Page_SaveReplay;
extern TextStyle D_8005AD20;
extern WipeBar D_8005AD3C;
extern InfoBarDesc D_8005AC50;
extern InfoBarDesc D_8005AC70;
extern InfoBarDesc D_8005AC90;
extern InfoBarDesc D_8005ACB0;
extern InfoBarDesc D_8005ACD0;
extern CardDialogDesc D_8005ACF0;
extern void *D_800A9F00;
extern char sR_CHAMPIONSHIP[];
extern char sR_SINGLERACE[];
extern char sR_WinN[];
extern char sR_Retire[];
void QuickMenu_PushPage(QuickMenu *menu, QuickMenuPage *page);
void InfoBar_Init(InfoBar *bar, InfoBarDesc *desc);
void InfoBar_Draw(InfoBar *bar, uint *ot, int x, int y, TextCtx *ctx);
void TitleBanner_Init(TitleBanner *banner, char *text, TextCtx *ctx);
void ResultCarView_Init(ResultCarView *view, int w, int h);

// Leaves for the quit page, with the choice it should act on.
static inline int QuickMenu_Quit(QuickMenu *menu, int action)
{
    int pushed = 1;

    menu->unk7c = action;
    QuickMenu_PushPage(menu, &g_Page_Quit);
    return pushed;
}

// A list with no reveal timer yet.
static inline void MenuList_NoReveal(MenuList *list, int delay)
{
    list->revealDelay = delay;
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


// A result menu row: its text item and wipe bar.
int ResultMenu_ItemCallback(MenuListEventId event, MenuListEvent *e, int i)
{
    TextCtx ctx;
    int result = 0;
    TextItem *item = &g_OVL0_EndFontInfo->items[i];
    WipeBar *bar = &g_OVL0_EndFontInfo->bars[i];

    Text_Init(&ctx, 6);
    ctx.unk1c = 1;
    switch (event) {
    case MLE_INIT:
        TextItem_Init(item, &D_8005AD20, g_ResultMenu_Options[i].label);
        *bar = D_8005AD3C;
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
        TextItem_Draw(item, (uint *)e->ot, e->x, e->y + 0x18, &ctx);
        WipeBar_Draw(bar, (uint *)e->ot + 1, e->x - 8, e->y - 2);
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
        result = g_ResultMenu_Options[i].enabled;
        break;
    }
    return result;
}

// Root page after an arcade race, a 2P battle or a championship race: stores back,
// waits 20 frames, sets up the card UI (QuickMenu_SetupCardUi on work+0x6e8) and plays
// music 8.
void ResultRoot_Enter(int back)
{
    void *pvVar1;
    QuickMenuPage *pQVar2;

    pvVar1 = g_OVL0_EndFontInfo;
    pQVar2 = g_QuickMenu->activePage;
    pQVar2->sel = (short)back;
    pQVar2->timer = 0x14;
    QuickMenu_SetupCardUi((void *)((int)pvVar1 + 0x6e8));
    QuickMenu_PlayMusic(8);
}

// After the wait, goes to the result menu, or leaves on sel 1.
int ResultRoot_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            if (page->sel == 1) {
                result = 4;
            } else {
                QuickMenu_PushPage(menu, &g_Page_ResultMenu);
                result = 1;
            }
        }
    }
    return result;
}

// Empty.
void ResultRoot_Draw(DrawLayer *layer)
{
}

// g_Page_Quit, pushed by most result pages once a choice is made (QuickMenu.unk7c
// holds it): 20-frame timer and music stop.
void QuitPage_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x14;
    QuickMenu_StopMusic();
}

// Returns 4 (leave the QuickMenu) when the timer runs out.
int QuitPage_Update(int active)
{
    unsigned int uVar1;

    uVar1 = (unsigned short)g_QuickMenu->activePage->timer - 1;
    g_QuickMenu->activePage->timer = (short)uVar1;
    return uVar1 >> 0xd & 4;
}

// Empty.
void QuitPage_Draw(DrawLayer *layer)
{
}

// Picks the options and title for the race type and builds the page.
void ResultMenu_Enter(int back)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    char *title = sR_CHAMPIONSHIP;
    int count;

    page->timer = 0x10;
    page->sel = -1;
    page->mode = 0;
    {
        char *base = (char *)&g_GameState;
        int offset = 0xbf7c;
        ChampProgress *champ = (ChampProgress *)(base + 0xc514);

        switch (RaceSetup_Type()) {
        case 11:
            title += 0xb74;
            count = 2;
            g_ResultMenu_Options = g_ResultMenu_OptionsRally;
            g_Page_ResultMenu.page.title = title;
            break;
        case 2:
        {
            int last = champ->race + 1 >= champ->count;

            if (last) {
                count = 3;
                g_ResultMenu_Options = g_ResultMenu_OptionsChampLast;
            } else {
                count = 4;
                g_ResultMenu_Options = g_ResultMenu_OptionsChamp;
            }
            g_Page_ResultMenu.page.title = title;
            break;
        }
        case 0:
            count = 4;
            g_ResultMenu_Options = g_ResultMenu_OptionsSingle;
            g_Page_ResultMenu.page.title = title - 0xabf;
            break;
        default:
            count = 4;
            g_ResultMenu_Options = g_ResultMenu_OptionsSingle;
            g_Page_ResultMenu.page.title = sR_SINGLERACE;
            break;
        }
    }
    if (back == 0) {
        g_ResultMenu_List.count = count;
        g_ResultMenu_List.visibleRows = count;
        MenuList_Init(&g_ResultMenu_List, ResultMenu_ItemCallback, 0);
    }
    CardDialog_Init(&g_OVL0_EndFontInfo->dialog, &D_8005ACF0, 0);
    {
        ResultCar *car = &g_OVL0_EndFontInfo->car;

        car->brightness = 0x40;
        car->enabled = 1;
        car->unk00 = 0x7fd7;
        car->tpage = 9;
        car->model = D_800A9F00;
        GroundDisc_Init(&car->disc, 0xa0000);
    }
    ResultCarView_Init(&g_OVL0_EndFontInfo->view, 200, 200);
    {
        char *base = (char *)&g_GameState;
        int offset = 0xbf7c;
        RaceSetup *setup = (RaceSetup *)(base + offset);

        TitleBanner_Init(&g_OVL0_EndFontInfo->banner, setup->courseName, &menu->text);
        g_OVL0_EndFontInfo->banner.bar.t = 0;
        TextItem_Show(&g_OVL0_EndFontInfo->banner.text, -1);
        if (setup->raceType == 0) {
            char *format = sR_WinN;
            unsigned short *wins = (unsigned short *)(base + 0xb8);

            sprintf(g_OVL0_EndFontInfo->wins[0], format, wins[0x22]);
            D_8005ACB0.value = (uint)g_OVL0_EndFontInfo->wins[0];
            InfoBar_Init(&g_OVL0_EndFontInfo->info[3], &D_8005ACB0);
            sprintf(g_OVL0_EndFontInfo->wins[1], format, wins[0x23]);
            D_8005ACD0.value = (uint)g_OVL0_EndFontInfo->wins[1];
            InfoBar_Init(&g_OVL0_EndFontInfo->info[4], &D_8005ACD0);
        } else {
            RaceRecord *record = (RaceRecord *)(base + 0xc5a8);
            InfoBarDesc *place = &D_8005AC50;
            InfoBarDesc *total = &D_8005AC70;
            InfoBarDesc *best = &D_8005AC90;

            place->value = (uint)sR_Retire;
            total->value = record->unkf8;
            best->value = *(uint *)&record->best;
            if (record->unk00 > 0) {
                place->value = (uint)g_ResultMenu_PlaceTexts[record->unk00 - 1];
            }
            InfoBar_Init(&g_OVL0_EndFontInfo->info[0], place);
            InfoBar_Init(&g_OVL0_EndFontInfo->info[1], total);
            InfoBar_Init(&g_OVL0_EndFontInfo->info[2], best);
        }
    }
}

// Opens the page after its wait, then runs the menu, or the quit dialog of a
// championship.
int ResultMenu_Update(int active)
{
    int result = 0;
    int leaving = result;
    QuickMenu *menu = g_QuickMenu;
    int single = RaceSetup_Get()->raceType == 0;
    QuickMenuPage *page = menu->activePage;
    PadState *pad = menu->padState;
    int choice;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            MenuList_Open(&g_ResultMenu_List);
            MenuList_NoReveal(&g_ResultMenu_List, -1);
            page->sel = 0;
            if (single) {
                InfoBar_Show(&g_OVL0_EndFontInfo->info[3]);
                InfoBar_Show(&g_OVL0_EndFontInfo->info[4]);
            } else {
                InfoBar_Show(&g_OVL0_EndFontInfo->info[0]);
                InfoBar_Show(&g_OVL0_EndFontInfo->info[1]);
                InfoBar_Show(&g_OVL0_EndFontInfo->info[2]);
            }
        }
    }
    {
        ResultWork *work = g_OVL0_EndFontInfo;

        WipeBar_Update(&work->banner.bar);
        TextItem_Update(&work->banner.text);
    }
    if (single) {
        InfoBar_Update(&g_OVL0_EndFontInfo->info[3]);
        InfoBar_Update(&g_OVL0_EndFontInfo->info[4]);
    } else {
        InfoBar_Update(&g_OVL0_EndFontInfo->info[0]);
        InfoBar_Update(&g_OVL0_EndFontInfo->info[1]);
        InfoBar_Update(&g_OVL0_EndFontInfo->info[2]);
    }
    ResultCarView_Spin(&g_OVL0_EndFontInfo->view, menu->drawStamp);
    if (active == 0) {
        pad = 0;
    }
    if (page->mode == 1) {
        MenuList_Update(&g_ResultMenu_List, 0);
        choice = CardDialog_Update(&g_OVL0_EndFontInfo->dialog, pad);
        switch (choice) {
        case -3:
        case -2:
            break;
        case 0:
            Sfx_PlayMenu(3);
            leaving = 1;
            menu->unk7c = 2;
            QuickMenu_PushPage(menu, &g_Page_Quit);
            result = leaving;
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
    } else {
        CardDialog_Update(&g_OVL0_EndFontInfo->dialog, 0);
        choice = MenuList_Update(&g_ResultMenu_List, pad);
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
            int next = g_ResultMenu_Options[choice].next;

            if (RaceSetup_Get()->raceType == 2 && next == 2) {
                Sfx_PlayMenu(1);
                CardDialog_Open(&g_OVL0_EndFontInfo->dialog);
                {
                    int one = 1;

                    g_OVL0_EndFontInfo->dialog.choice = one;
                    page->mode = one;
                }
            } else {
                leaving = 1;
                Sfx_PlayMenu(3);
                if (next == -2) {
                    QuickMenu_PushPage(menu, &g_Page_SaveReplay);
                    result = leaving;
                } else {
                    result = QuickMenu_Quit(menu, g_ResultMenu_Options[choice].next);
                }
            }
            break;
        }
        }
    }
    if (leaving) {
        MenuList_Close(&g_ResultMenu_List);
        {
            TitleBanner *banner = &g_OVL0_EndFontInfo->banner;

            banner->bar.t = ~banner->bar.duration;
            TextItem_Hide(&banner->text);
        }
        if (single) {
            InfoBar_Hide(&g_OVL0_EndFontInfo->info[3], -0x11);
            InfoBar_Hide(&g_OVL0_EndFontInfo->info[4], -0x11);
        } else {
            InfoBar_Hide(&g_OVL0_EndFontInfo->info[0], -0x11);
            InfoBar_Hide(&g_OVL0_EndFontInfo->info[1], -0x11);
            InfoBar_Hide(&g_OVL0_EndFontInfo->info[2], -0x11);
        }
        page->sel = -1;
    }
    return result;
}

void ResultMenu_Draw(DrawLayer *layer)
{
    void *ot = layer->ot;
    QuickMenu *menu = g_QuickMenu;
    TextCtx *text = &menu->text;
    QuickMenuPage *page = menu->activePage;
    RECT area;

    CardDialog_Draw(&g_OVL0_EndFontInfo->dialog, (uint *)ot, text);
    TitleBanner_Draw(&g_OVL0_EndFontInfo->banner, (uint *)ot, 0xb0, 0x68, text);
    if (RaceSetup_Get()->raceType == 0) {
        InfoBar_Draw(&g_OVL0_EndFontInfo->info[3], (uint *)ot + 1, 0x20, 0x186, text);
        InfoBar_Draw(&g_OVL0_EndFontInfo->info[4], (uint *)ot + 1, 0xd0, 0x186, text);
    } else {
        TextCtx *ctx = text;

        InfoBar_Draw(&g_OVL0_EndFontInfo->info[0], (uint *)ot + 1, 0x30, 0x186, ctx);
        InfoBar_Draw(&g_OVL0_EndFontInfo->info[1], (uint *)ot + 1, 0xa8, 0x186, ctx);
        InfoBar_Draw(&g_OVL0_EndFontInfo->info[2], (uint *)ot + 1, 0xa8, 0x1a2, ctx);
    }
    MenuList_Draw(&g_ResultMenu_List, ot);
    if (page->sel >= 0) {
        ResultCar_Draw(&g_OVL0_EndFontInfo->car, &menu->layers[0], &menu->layers[1], &g_OVL0_EndFontInfo->view, 0);
    }
    area.x = 0x7c;
    area.y = 0xa0;
    area.w = 0xc8;
    area.h = 0xc8;
    DrawLayer_SetArea(&menu->layers[0], &area);
    DrawLayer_SetArea(&menu->layers[1], &area);
}
}
