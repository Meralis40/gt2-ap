// OVL1 80017d74-80019218 (options): Options pages.
// The functions that don't match yet are the original assembly, with their C++
// under NON_MATCHING.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

// An option row (0x14 bytes). type: 0 choices, 1 a bar of 16 steps, 2 a balance,
// 3 a lap count.
struct OptionItem {
    signed char id;
    signed char type;
    short min;
    short max;
    short labelX;
    short valueX;
    short pad0a;
    signed char *label;
    signed char **choices;
};

// What the option lists keep beside them: the brightness and the rows' y offsets.
struct OptionRows {
    int brightness;
    short y[1];
};

struct OptionList {
    MenuList list;
    OptionRows *rows;
};

// A root page: a BaseMenuPage, then a delay and the back flag.
extern "C" {
extern BaseMenu *g_CurrentMenu;
extern Font g_OVL1_FontMedium;
extern Font g_OVL1_FontLarge;
extern Font g_OVL1_FontTitle;
extern unsigned char D_8004BCCC[];
extern unsigned int D_8004BCD0;
extern unsigned int D_8004BCD4;
extern unsigned int D_8004BCD8;
extern unsigned int D_8004BCDC;
extern unsigned char D_8004BCE0[];
extern unsigned int D_8004BCE4;
extern unsigned int D_8004BCE8;
extern unsigned char D_8004BCEC[];
extern signed char s_1p[];
extern signed char s_1Lap[];
extern signed char s_ViewSettings[];
extern signed char s_ArcadeRace[];
extern OptionItem g_Options_GlobalItems[];
extern OptionItem g_Options_RaceItems[];
extern OptionList g_Options_GlobalList;
extern OptionList g_Options_RaceList;
extern OptionRows g_Options_GlobalRows;
extern OptionRows g_Options_RaceRows;
extern WipeBar g_Options_IconView;
extern WipeBar g_Options_IconSound;
extern WipeBar g_Options_IconDualShock;
extern WipeBar g_Options_IconArcade;
extern WipeBar g_Options_Icon2P;
extern signed char *g_Options_TabTitles[];
extern TabSelector g_Options_Tabs;
extern KeyConfigPage g_KeyConfigPage;
extern AnalogSettings g_AnalogSettings1P;
extern AnalogSettings g_AnalogSettings2P;
extern BaseMenuPage g_Page_OptionsMenu;
void PushPage(BaseMenu *menu, BaseMenuPage *page);
void TabSelector_Open(TabSelector *tabs);
int TabSelector_Update(TabSelector *tabs, PadState *pad, PadState *pad2, int solo);
void TabSelector_Draw(TabSelector *tabs, uint *ot);
void KeyConfigPage_Init(KeyConfigPage *page);
int KeyConfigPage_Update(KeyConfigPage *page, PadState *pad);
void AnalogSettings_Init(AnalogSettings *s, int port);
int AnalogSettings_Open(AnalogSettings *s);
int AnalogSettings_Update(AnalogSettings *s, PadState *pad);
void AnalogSettings_HideIcons(AnalogSettings *s);
void AnalogSettings_Reset(AnalogSettings *s);
void AnalogSettings_Draw(AnalogSettings *s, unsigned int *ot, int x, int y, int brightness);
int Option_Get(OptionItem *item);
void Option_Set(OptionItem *item, int value);
int OptionsMenu_TabCallback(int event, TabEvent *ev, int tab);
int OptionList_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index);

// An option's value in g_GameState. Id 14 is stored inverted.
int Option_Get(OptionItem *item)
{
    GameState *gs = &g_GameState;
    int value = 0;

    switch (item->id) {
    case 0:
        value = gs->options[0];
        break;
    case 1:
        value = gs->options[1];
        break;
    case 2:
        value = gs->options[2];
        break;
    case 3:
        value = gs->options[3];
        break;
    case 4:
        value = gs->options[4];
        break;
    case 5:
        value = (signed char)gs->options[5];
        break;
    case 6:
        value = (signed char)gs->options[6];
        break;
    case 7:
        value = gs->options2[0];
        break;
    case 8:
        value = gs->options2[1];
        break;
    case 9:
        value = gs->options2[2];
        break;
    case 10:
        value = gs->options2[3];
        break;
    case 11:
        value = gs->options2[4];
        break;
    case 12:
        value = gs->options2[5];
        break;
    case 13:
        value = gs->options2[6];
        break;
    case 14:
        value = ((unsigned char *)gs)[0x36] == 0;
        break;
    }
    return value;
}

// Stores an option. Ids 0 and 3 are flags; id 14 goes to both ports.
void Option_Set(OptionItem *item, int value)
{
    GameState *gs = &g_GameState;
    int flag;

    switch (item->id) {
    case 0:
        gs->options[0] = value != 0;
        break;
    case 1:
        gs->options[1] = value;
        break;
    case 2:
        gs->options[2] = value;
        break;
    case 3:
        gs->options[3] = value != 0;
        break;
    case 4:
        gs->options[4] = value;
        break;
    case 5:
        gs->options[5] = value;
        break;
    case 6:
        gs->options[6] = value;
        break;
    case 7:
        gs->options2[0] = value;
        break;
    case 8:
        gs->options2[1] = value;
        break;
    case 9:
        gs->options2[2] = value;
        break;
    case 10:
        gs->options2[3] = value;
        break;
    case 11:
        gs->options2[4] = value;
        break;
    case 12:
        gs->options2[5] = value;
        break;
    case 13:
        gs->options2[6] = value;
        break;
    case 14:
        flag = value != 1;
        ((unsigned char *)gs)[0x36] = flag;
        ((unsigned char *)gs)[0x88] = flag;
        break;
    }
}

#ifdef NON_MATCHING
// Moves an option by delta (in steps of 16 for a bar) and clamps it: non-zero when
// it changed.
int Option_Step(OptionItem *item, int delta, int big)
{
    int old;
    int value;
    int step;
    int min;

    switch (item->type) {
    case 2:
        value = Option_Get(item);
        old = value;
        step = delta + big;
        min = item->min;
        value += step;
        goto clamp;
    case 1:
        value = Option_Get(item);
        delta <<= 4;
        old = value;
        value = ((Option_Get(item) + 15) / 16) * 16;
        break;
    case 3:
        delta += big;
    default:
        value = Option_Get(item);
        old = value;
        break;
    }
    min = item->min;
    value += delta;
clamp:
    if (value < min) {
        value = min;
    }
    if (value >= item->max) {
        value = item->max - 1;
    }
    Option_Set(item, value);
    return old != value;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", Option_Step);
#endif

#ifdef NON_MATCHING
// Draws a row: the label, then the choices, the bar, the balance or the laps.
void Option_Draw(OptionItem *item, void *ot, int x, int y, int brightness, int active)
{
    TextCtx ctx;
    char num[64];
    char laps[64];
    unsigned int rgb = D_8004BCD0;
    unsigned int rgb2;
    int right;
    int left;
    TextCtx *c;
    int value = Option_Get(item);
    int i;
    int pos;
    int ty;
    signed char *s;
    RECT *r;

    if (active) {
        rgb = D_8004BCD4;
    }
    Text_Init(&ctx, 0x1e);
    c = &ctx;
    *(uint *)&c->clut = (*(uint *)&c->clut & 0xff9fffff) | 0x200000;
    c->ot = (uint *)ot;
    Text_SetFont(c, &g_OVL1_FontMedium);
    c->rgb = Rgb_Lerp(D_8004BCCC, (unsigned char *)&rgb, brightness, 0x80);
    Text_DrawStringMonoDigits(c, item->label, x + item->labelX, y, 1, -2, 0);
    switch (item->type) {
    case 0:
        for (i = 0; i < item->max; i++) {
            rgb2 = D_8004BCD8;
            s = item->choices[i];
            if (i == value) {
                rgb2 = D_8004BCDC;
            }
            c->rgb = Rgb_Lerp(D_8004BCCC, (unsigned char *)&rgb2, brightness, 0x80);
            Text_DrawStringMonoDigits(c, s, x + item->valueX, y, 1, -2, 0);
            x += 0x3c;
        }
        break;
    case 1:
        for (i = 0; i < 16; i++) {
            rgb = D_8004BCE8;
            if (i * 16 >= value) {
                rgb = D_8004BCE4;
            }
            r = Prim_AddTile(ot, Rgb_Lerp(D_8004BCCC, (unsigned char *)&rgb, brightness, 0x80));
            r->y = y - 0x14;
            r->w = 5;
            r->h = 0x14;
            r->x = item->valueX + x;
            x += 6;
        }
        break;
    case 2:
        rgb = 0x2785030;
        r = Prim_AddTile(ot, Rgb_Lerp(D_8004BCCC, (unsigned char *)&rgb, brightness, 0x80));
        r->w = 0x8c;
        r->y = y - 10;
        r->h = 4;
        r->x = item->valueX + x;
        pos = ((value - item->min) * 0x8c) / (item->max - item->min);
        right = 0;
        pos += x + item->valueX;
        rgb = 0x2909090;
        r = Prim_AddTile(ot, Rgb_Lerp(D_8004BCCC, (unsigned char *)&rgb, brightness, 0x80));
        r->x = pos - 1;
        r->w = 2;
        r->y = y - 13;
        r->h = 10;
        left = 0;
        if (value < 0) {
            right = -value * 10;
        } else {
            left = value * 10;
        }
        ty = y + 0x12;
        Text_DrawStringRight(c, s_1p, x + item->valueX + 0x10, ty, 1);
        sprintf(num, (char *)(s_1p + 10), left);
        Text_DrawStringMonoDigitsRight(c, (signed char *)num, x + item->valueX + 0x38, ty, 1, -2, 0);
        Text_DrawStringRight(c, s_1p + 5, x + item->valueX + 0x60, ty, 1);
        sprintf(num, (char *)(s_1p + 10), right);
        Text_DrawStringMonoDigitsRight(c, (signed char *)num, x + item->valueX + 0x88, ty, 1, -2, 0);
        break;
    case 3:
        strcpy(laps, (char *)s_1Lap);
        if (value >= 2) {
            sprintf(laps, (char *)(s_1Lap + 9), value);
        }
        rgb = D_8004BCE8;
        c->rgb = Rgb_Lerp(D_8004BCCC, (unsigned char *)&rgb, brightness, 0x80);
        Text_DrawStringMonoDigits(c, (signed char *)laps, x + item->valueX, y, 1, -2, 0);
        break;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", Option_Draw);
#endif

#ifdef NON_MATCHING
// Left and Right step the selected row; the buttons of bits 4 and 12 add to the step.
int OptionList_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index)
{
    OptionList *list = (OptionList *)ev->list;
    OptionRows *rows = list->rows;
    int b = ev->brightness * rows->brightness;
    int y = ev->y;
    void *ot = ev->ot;
    int x = ev->x;
    PadState *pad = ev->pad;
    OptionItem *item = &((OptionItem *)list->list.userData)[index];
    int selected = list->list.selected;
    int brightness = b >> 7;
    int delta;
    int big;
    unsigned int buttons;

    switch ((int)event) {
    case 3:
        if (index == selected && pad != 0) {
            delta = 0;
            buttons = pad->pressed | pad->repeat;
            big = delta;
            if ((buttons & 4) != 0) {
                delta = -1;
            }
            if ((buttons & 8) != 0) {
                delta = 1;
            }
            buttons = pad->held;
            if ((buttons & 0x10) != 0) {
                big = -1;
            }
            if ((buttons & 0x1000) != 0) {
                big = 1;
            }
            if ((delta != 0) | (big != 0)) {
                if (Option_Step(item, delta, big) != 0 && delta != 0) {
                    Sfx_PlayMenu(5);
                }
            }
        }
        break;
    case 4:
        y += rows->y[index];
        if ((selected >= 0) & (index != selected)) {
            brightness = b >> 8;
        }
        Option_Draw(item, ot, x, y, brightness, index == selected);
        break;
    }
    return 1;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionList_ItemCallback);
#endif

#ifdef NON_MATCHING
// A tab of the options: 0 update, 1 draw, 3 confirm (enters it), 4 changed.
int OptionsMenu_TabCallback(int event, TabEvent *ev, int tab)
{
    TextCtx ctx;
    PadState *pad = ev->pad;
    uint *ot = ev->ot;
    int x = ev->x;
    int y = ev->y;
    int brightness = ev->brightness;
    TabSelector *tabs = ev->tabs;
    TextCtx *c;
    AnalogSettings *settings;
    signed char *s;
    int r;

    Text_Init(&ctx, 0x1e);
    c = &ctx;
    *(uint *)&c->clut = (*(uint *)&c->clut & 0xff9fffff) | 0x200000;
    c->ot = ot;
    Text_SetFont(c, &g_OVL1_FontTitle);
    if (tabs->selected != tab) {
        pad = 0;
    }
    switch (event) {
    case 3:
        tabs->active = 1;
        tabs->skipInput = 1;
        switch (tab) {
        case 0:
            g_Options_GlobalList.list.selected = 0;
            break;
        case 1:
            g_Options_RaceList.list.selected = 0;
            break;
        case 2:
            KeyConfigPage_Open(&g_KeyConfigPage);
            break;
        case 3:
            if ((AnalogSettings_Open(&g_AnalogSettings1P) ^ 1) != 0) {
                Sfx_PlayMenu(0);
                tabs->active = 0;
                tabs->skipInput = 1;
                return 0;
            }
            break;
        case 4:
            if ((AnalogSettings_Open(&g_AnalogSettings2P) ^ 1) != 0) {
                Sfx_PlayMenu(0);
                tabs->active = 0;
                tabs->skipInput = 1;
                return 0;
            }
            break;
        default:
            return 0;
        }
        Sfx_PlayMenu(1);
        break;
    case 4:
        switch (tabs->previous) {
        case 0:
            g_Options_IconView.t = ~g_Options_IconView.duration;
            g_Options_IconSound.t = ~g_Options_IconSound.duration;
            g_Options_IconDualShock.t = ~g_Options_IconDualShock.duration;
            break;
        case 1:
            g_Options_IconArcade.t = ~g_Options_IconArcade.duration;
            g_Options_Icon2P.t = ~g_Options_Icon2P.duration;
            break;
        case 2:
            break;
        case 3:
            AnalogSettings_HideIcons(&g_AnalogSettings1P);
            break;
        case 4:
            AnalogSettings_HideIcons(&g_AnalogSettings2P);
            break;
        }
        switch (tabs->selected) {
        case 0:
            g_Options_IconView.t = 0;
            g_Options_IconSound.t = 0;
            g_Options_IconDualShock.t = 0;
            break;
        case 1:
            g_Options_IconArcade.t = 0;
            g_Options_Icon2P.t = 0;
            break;
        case 3:
            AnalogSettings_Reset(&g_AnalogSettings1P);
            break;
        case 4:
            AnalogSettings_Reset(&g_AnalogSettings2P);
            break;
        }
        break;
    case 0:
        if (tabs->active == 0) {
            pad = 0;
        }
        switch (tab) {
        case 0:
            r = MenuList_Update(&g_Options_GlobalList.list, pad);
            switch (r) {
            case -3:
                Sfx_PlayMenu(6);
                break;
            case -2:
                break;
            default:
                Sfx_PlayMenu(1);
                g_Options_GlobalList.list.selected = -1;
                tabs->active = 0;
                tabs->skipInput = 1;
                break;
            }
            break;
        case 1:
            r = MenuList_Update(&g_Options_RaceList.list, pad);
            switch (r) {
            case -3:
                Sfx_PlayMenu(6);
                break;
            case -2:
                break;
            default:
                Sfx_PlayMenu(1);
                g_Options_RaceList.list.selected = -1;
                tabs->active = 0;
                tabs->skipInput = 1;
                break;
            }
            break;
        case 2:
            settings = (AnalogSettings *)&g_KeyConfigPage;
            r = KeyConfigPage_Update((KeyConfigPage *)settings, pad);
            if (r == 0) {
                Sfx_PlayMenu(1);
                KeyConfigPage_Close((KeyConfigPage *)settings);
                tabs->active = 0;
                tabs->skipInput = 1;
            }
            break;
        case 3:
            settings = &g_AnalogSettings1P;
            goto analog;
        case 4:
            settings = &g_AnalogSettings2P;
        analog:
            r = AnalogSettings_Update(settings, pad);
            switch (r) {
            case 0:
                Sfx_PlayMenu(1);
            case -2:
                AnalogSettings_Close(settings);
                tabs->active = 0;
                tabs->skipInput = 1;
                break;
            }
            break;
        }
        WipeBar_Update(&g_Options_IconView);
        WipeBar_Update(&g_Options_IconSound);
        WipeBar_Update(&g_Options_IconDualShock);
        WipeBar_Update(&g_Options_IconArcade);
        WipeBar_Update(&g_Options_Icon2P);
        break;
    case 1:
        c->rgb = Rgb_Lerp(D_8004BCCC, D_8004BCE0, brightness, tabs->active != 0 ? 0x200 : 0x80);
        Text_DrawCentered(c, g_Options_TabTitles[tab], x, y + 0x14, 0);
        switch (tab) {
        case 0:
            g_Options_GlobalList.list.y = y + 0x2e;
            g_Options_GlobalList.list.x = x;
            g_Options_GlobalRows.brightness = brightness;
            MenuList_Draw(&g_Options_GlobalList.list, ot);
            Text_SetFont(&ctx, &g_OVL1_FontLarge);
            s = s_ViewSettings;
            c->rgb = Rgb_Lerp(D_8004BCCC, D_8004BCEC, brightness, 0x80);
            Text_DrawString(c, s, x - 0x8e, y + 0x3a, 1);
            WipeBar_Draw(&g_Options_IconView, ot, x - 0x94, y + 0x21);
            Prim_AddDrawMode(ot, 0x220);
            c->rgb = Rgb_Lerp(D_8004BCCC, D_8004BCEC, brightness, 0x80);
            Text_DrawString(c, s + 0x12, x - 0x8e, y + 0xd2, 1);
            WipeBar_Draw(&g_Options_IconSound, ot, x - 0x94, y + 0xb9);
            Prim_AddDrawMode(ot, 0x220);
            c->rgb = Rgb_Lerp(D_8004BCCC, D_8004BCEC, brightness, 0x80);
            Text_DrawString(c, s + 0x19, x - 0x8e, y + 0x122, 1);
            WipeBar_Draw(&g_Options_IconDualShock, ot, x - 0x94, y + 0x109);
            Prim_AddDrawMode(ot, 0x220);
            break;
        case 1:
            g_Options_RaceList.list.y = y + 0x2e;
            g_Options_RaceList.list.x = x;
            g_Options_RaceRows.brightness = brightness;
            MenuList_Draw(&g_Options_RaceList.list, ot);
            Text_SetFont(&ctx, &g_OVL1_FontLarge);
            s = s_ArcadeRace;
            c->rgb = Rgb_Lerp(D_8004BCCC, D_8004BCEC, brightness, 0x80);
            Text_DrawString(c, s, x - 0x8e, y + 0x3a, 1);
            WipeBar_Draw(&g_Options_IconArcade, ot, x - 0x94, y + 0x21);
            Prim_AddDrawMode(ot, 0x220);
            c->rgb = Rgb_Lerp(D_8004BCCC, D_8004BCEC, brightness, 0x80);
            Text_DrawString(c, s + 0xf, x - 0x8e, y + 0x8a, 1);
            WipeBar_Draw(&g_Options_Icon2P, ot, x - 0x94, y + 0x71);
            Prim_AddDrawMode(ot, 0x220);
            break;
        case 2:
            KeyConfigPage_Draw(&g_KeyConfigPage, ot, x, y + 0x40, brightness);
            break;
        case 3:
            AnalogSettings_Draw(&g_AnalogSettings1P, ot, x, y + 0x2e, brightness);
            break;
        case 4:
            AnalogSettings_Draw(&g_AnalogSettings2P, ot, x, y + 0x2e, brightness);
            break;
        }
        break;
    }
    return 0;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionsMenu_TabCallback);
#endif

#ifdef NON_MATCHING
// Builds the tabs, the two option lists, the key configuration and both analog pages.
void OptionsMenu_Enter(int back)
{
    TabSelector_Init(&g_Options_Tabs, OptionsMenu_TabCallback, 0);
    TabSelector_Open(&g_Options_Tabs);
    MenuList_Init(&g_Options_GlobalList.list, OptionList_ItemCallback, g_Options_GlobalItems);
    g_Options_GlobalList.rows = &g_Options_GlobalRows;
    MenuList_Open(&g_Options_GlobalList.list);
    g_Options_GlobalList.list.selected = -1;
    g_Options_IconView.t = -1;
    g_Options_IconSound.t = -1;
    g_Options_IconDualShock.t = -1;
    g_Options_IconView.t = 0;
    g_Options_IconSound.t = 0;
    g_Options_IconDualShock.t = 0;
    MenuList_Init(&g_Options_RaceList.list, OptionList_ItemCallback, g_Options_RaceItems);
    g_Options_RaceList.rows = &g_Options_RaceRows;
    MenuList_Open(&g_Options_RaceList.list);
    g_Options_RaceList.list.selected = -1;
    g_Options_IconArcade.t = -1;
    g_Options_Icon2P.t = -1;
    KeyConfigPage_Init(&g_KeyConfigPage);
    AnalogSettings_Init(&g_AnalogSettings1P, 0);
    AnalogSettings_Init(&g_AnalogSettings2P, 1);
    MainMenu_PlayMusic(6);
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/options", OptionsMenu_Enter);
#endif

// Runs the tabs: back (2) when they're left. The fifth tab reads the second pad.
int OptionsMenu_Update(int arg)
{
    int result = 0;
    TabSelector *tabs = &g_Options_Tabs;
    char *menu = (char *)g_CurrentMenu;

    switch (TabSelector_Update(tabs, (PadState *)(menu + 0x1a4), (PadState *)(menu + 0x1b4), tabs->selected != 4)) {
    case -3:
        Sfx_PlayMenu(7);
        break;
    case -2:
        break;
    case -1:
        TabSelector_Close(tabs);
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

// Draws the tabs.
void OptionsMenu_Draw(DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;

    TabSelector_Draw(&g_Options_Tabs, ot);
}

// Root page of Options: stores back, waits 16 frames (no music change).
void OptionsRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].enter = (MenuPageEnterFn *)0x10;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
}

// After the delay: pushes the options page, or leaves the menu (4) coming back.
int OptionsRoot_Update(int arg)
{
    BaseMenu *menu = g_CurrentMenu;
    RootMenuPage *page = (RootMenuPage *)menu->activePage;
    int result = 0;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            result = 4;
            if (page->back != 1) {
                PushPage(menu, &g_Page_OptionsMenu);
                result = 1;
            }
        }
    }
    return result;
}

// Empty.
void OptionsRoot_Draw(DrawLayer *layer)
{
}

// Unreferenced page: 16-frame delay, MainMenu_StopMusic.
void OptionsUnusedWait_Enter(int back)
{
    g_CurrentMenu->activePage[1].enter = (MenuPageEnterFn *)0x10;
    MainMenu_StopMusic();
}

// After the delay: leaves the menu (4).
int OptionsUnusedWait_Update(int arg)
{
    RootMenuPage *page = (RootMenuPage *)g_CurrentMenu->activePage;
    int result = 0;

    if (page->delay > 0) {
        page->delay--;
        result = (page->delay == 0) * 4;
    }
    return result;
}

// Empty.
void OptionsUnusedWait_Draw(DrawLayer *layer)
{
}
}
