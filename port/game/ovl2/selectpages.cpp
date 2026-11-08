// OVL2 8001d568-8001e53c (selectpages): Root, mode, level and class pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
void ArcCourseList_InitAll(void);
void ArcCarList_UpdateUnlocks(void);
extern GameState (&g_GameState);
extern ArcGarageInfo (&g_Arc_HomeGarageInfo);
extern ArcadeRaceSel (&g_ArcSel_Level);
extern short (&g_Arc_CourseSet);
extern int (&g_Arc_RallyMode);
extern ArcMenuList (&g_ArcMenu_Mode);
extern ArcMenuList (&g_ArcMenu_Game1P);
extern ArcMenuList (&g_ArcMenu_Game2P);
extern ArcMenuList (&g_ArcMenu_Level);
extern ArcMenuList (&g_ArcMenu_Class);
extern ArcMenuList (&g_ArcMenu_RallyClass);
extern game_pointer<MenuPage> (&g_ArcModePages)[];
extern unsigned char (&g_Arc1PRaceTypes)[];
extern unsigned int (&g_ArcClassColors)[];
extern MenuPage (&g_ArcPage_Mode);
extern MenuPage (&g_ArcPage_Level);
extern MenuPage (&g_ArcPage_Class);
extern MenuPage (&g_ArcPage_RallyClass);
extern MenuPage (&g_ArcPage_CarSelect);
extern MenuPage (&g_ArcPage_Garage);
extern MenuPage (&g_ArcPage_Battle2P);
extern ArcMenuItem (&D_8004FBFC)[]; // the class rows with class S
extern ArcMenuItem (&D_8004FC10)[]; // without
extern ArcMenuItem (&D_8004FC4C)[2]; // the garage rows
extern ArcMenuItem (&D_8004FD08)[2]; // the rally page's
extern game_pointer<char> (&D_8004FDDC)[]; // the garages' titles

static inline ArcSavedSel *ArcSavedOf(GameState *state)
{
    return (ArcSavedSel *)(state + 1);
}

static inline ArcSavedSel *ArcSaved(void)
{
    return ArcSavedOf(&g_GameState);
}



// Entering the menu: the music, and the garages' ratings and unlocks; coming back,
// stops the music and waits to leave.
void ArcadeRoot_Enter(ArcadeMenu *menu, int back)
{
    ArcPage *page = (ArcPage *)menu->activePage;

    page->choice = back;
    page->delay = 1;
    if (back != 0) {
        page->delay = 0x18;
        ArcadeBgm_Stop();
    } else {
        ArcadeBgm_Play();
        {
            ArcGarageInfo *info = &g_Arc_HomeGarageInfo;
            GameState *state = &g_GameState;

            CarRatingTable_Build(info, &state->garages[0]);
            CarRatingTable_Build(info + 1, &state->garages[1]);
        }
        ArcCourseList_InitAll();
        g_Arc_CourseSet = 5;
        ArcCarList_UpdateUnlocks();
    }
}

// After the delay, the mode page, or 4 to leave the menu.
int ArcadeRoot_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    int result = 0;

    if (page->delay > 0 && --page->delay == 0) {
        result = 4;
        if (page->choice != 1) {
            ArcadeMenu_PushPage(menu, &g_ArcPage_Mode);
            result = 1;
        }
    }
    return result;
}

// Empty.
void ArcadeRoot_Draw(void *menu, DrawLayer *layer)
{
}

// The list on the saved mode, or on the last choice when coming back.
void ModeSelect_Enter(ArcadeMenu *menu, int back)
{
    int selected = ArcSaved()->mode;
    ArcPage *page = (ArcPage *)menu->activePage;
    ArcMenuList *list;

    if (selected >= 4) {
        selected = 0;
    }
    page->delay = 0x18;
    list = &g_ArcMenu_Mode;
    MenuList_Init_OVL2(list);
    list->selected = selected;
    if (back != 0) {
        int choice = page->choice;

        list->selected = choice;
    }
}

// Pushes the chosen mode's page.
int ModeSelect_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    int result = 0;
    int choice;
    PadState *pad;
    GameState *state;
    ArcMenuList *list;

    if (page->delay > 0 && --page->delay == 0) {
        MenuList_Open_OVL2(&g_ArcMenu_Mode);
    }
    pad = &menu->padState[0];
    if (active == 0) {
        pad = 0;
    }
    list = &g_ArcMenu_Mode;
    choice = MenuList_Update_OVL2(list, pad);
    switch (choice) {
    case -4:
        Sfx_PlayMenu(0);
        break;
    case -3:
        Sfx_PlayMenu(6);
        break;
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close_OVL2(list);
        result = 2;
        break;
    case -2:
        break;
    default:
        state = &g_GameState;
        ArcSavedOf(state)->mode = choice;
        if (choice == 1) {
            g_ArcSel_Level.lapOption = state->options[4];
        } else {
            g_ArcSel_Level.lapOption = state->options[1];
        }
        Sfx_PlayMenu(3);
        MenuList_Close_OVL2(&g_ArcMenu_Mode);
        page->choice = choice;
        ArcadeMenu_PushPage(menu, g_ArcModePages[choice]);
        result = 1;
        break;
    }
    return result;
}

// The list.
void ModeSelect_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    TextCtx *text = menu->text;

    MenuList_Draw_OVL2(&g_ArcMenu_Mode, layer->ot, text);
}

// Same as ModeSelect_Enter.
void Game1PSelect_Enter(ArcadeMenu *menu, int back)
{
    int selected = ArcSaved()->game1P;
    ArcPage *page = (ArcPage *)menu->activePage;
    ArcMenuList *list;

    if (selected >= 3) {
        selected = 0;
    }
    page->delay = 0x18;
    list = &g_ArcMenu_Game1P;
    MenuList_Init_OVL2(list);
    list->selected = selected;
    if (back != 0) {
        int choice = page->choice;

        list->selected = choice;
    }
}

// Sets the race type and pushes the level, rally class or class page.
int Game1PSelect_Update(ArcadeMenu *menu, int active)
{
    int result = 0;
    ArcPage *page = (ArcPage *)menu->activePage;
    int choice;
    PadState *pad;
    ArcMenuList *list;

    if (page->delay > 0 && --page->delay == 0) {
        MenuList_Open_OVL2(&g_ArcMenu_Game1P);
    }
    pad = &menu->padState[0];
    if (active == 0) {
        pad = 0;
    }
    list = &g_ArcMenu_Game1P;
    choice = MenuList_Update_OVL2(list, pad);
    switch (choice) {
    case -4:
        Sfx_PlayMenu(0);
        break;
    case -3:
        Sfx_PlayMenu(6);
        break;
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close_OVL2(list);
        result = 2;
        break;
    case -2:
        break;
    default:
        Sfx_PlayMenu(3);
        MenuList_Close_OVL2(&g_ArcMenu_Game1P);
        page->choice = choice;
        ArcSaved()->game1P = choice;
        g_ArcSel_Level.raceType = g_Arc1PRaceTypes[choice];
        switch (choice) {
        case 0:
            g_Arc_CourseSet = 0;
            g_Arc_RallyMode = 0;
            ArcadeMenu_PushPage(menu, &g_ArcPage_Level);
            result = 1;
            break;
        case 1:
            g_Arc_CourseSet = 2;
            g_Arc_RallyMode = 1;
            ArcadeMenu_PushPage(menu, &g_ArcPage_RallyClass);
            result = 1;
            break;
        case 2:
            g_Arc_CourseSet = 1;
            g_Arc_RallyMode = 0;
            ArcadeMenu_PushPage(menu, &g_ArcPage_Class);
            result = 1;
            break;
        default:
            result = 1;
            break;
        }
        g_ArcSel_Level.dirt = g_Arc_RallyMode;
        break;
    }
    return result;
}

// The list.
void Game1PSelect_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    TextCtx *text = menu->text;

    MenuList_Draw_OVL2(&g_ArcMenu_Game1P, layer->ot, text);
}

// The list on the saved choice.
void Game2PSelect_Enter(ArcadeMenu *menu, int back)
{
    int selected = ArcSaved()->game2P;
    ArcPage *page = (ArcPage *)menu->activePage;
    ArcMenuList *list;

    if (selected >= 2) {
        selected = 0;
    }
    list = &g_ArcMenu_Game2P;
    page->delay = 0x18;
    MenuList_Init_OVL2(list);
    list->selected = selected;
}

// Pushes the 2-player battle page, with the rally courses or not.
int Game2PSelect_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    int result = 0;
    int choice;
    PadState *pad;
    ArcMenuList *list;

    if (page->delay > 0 && --page->delay == 0) {
        MenuList_Open_OVL2(&g_ArcMenu_Game2P);
    }
    pad = &menu->padState[0];
    if (active == 0) {
        pad = 0;
    }
    list = &g_ArcMenu_Game2P;
    choice = MenuList_Update_OVL2(list, pad);
    switch (choice) {
    case -4:
        Sfx_PlayMenu(0);
        break;
    case -3:
        Sfx_PlayMenu(6);
        break;
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close_OVL2(list);
        result = 2;
        break;
    case -2:
        break;
    default:
        Sfx_PlayMenu(3);
        MenuList_Close_OVL2(&g_ArcMenu_Game2P);
        ArcSaved()->game2P = choice;
        if (choice == 1) {
            g_Arc_CourseSet = 4;
            g_Arc_RallyMode = choice;
        } else {
            g_Arc_CourseSet = 3;
            g_Arc_RallyMode = 0;
        }
        {
            ArcadeRaceSel *sel = &g_ArcSel_Level;

            sel->raceType = 0;
            sel->dirt = g_Arc_RallyMode;
        }
        ArcadeMenu_PushPage(menu, &g_ArcPage_Battle2P);
        result = 1;
        break;
    }
    return result;
}

// The list.
void Game2PSelect_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    TextCtx *text = menu->text;

    MenuList_Draw_OVL2(&g_ArcMenu_Game2P, layer->ot, text);
}

// Three levels (ArcLevel_HasFourthLevel returns 0), the list placed for their number.
void LevelSelect_Enter(ArcadeMenu *menu, int back)
{
    int selected = ArcSaved()->level;
    ArcPage *page = (ArcPage *)menu->activePage;
    ArcMenuList *list;

    if (selected >= 4) {
        selected = 0;
    }
    page->delay = 0x18;
    if (ArcLevel_HasFourthLevel() != 0) {
        if (selected >= 4) {
            selected = 0;
        }
        g_ArcMenu_Level.count = 4;
        g_ArcMenu_Level.visibleRows = 4;
        g_ArcMenu_Level.itemGap = 10;
        g_ArcMenu_Level.y = 0x7b;
    } else {
        if (selected >= 3) {
            selected = 0;
        }
        g_ArcMenu_Level.count = 3;
        g_ArcMenu_Level.visibleRows = 3;
        g_ArcMenu_Level.itemGap = 0x14;
        g_ArcMenu_Level.y = 0x96;
    }
    list = &g_ArcMenu_Level;
    MenuList_Init_OVL2(list);
    list->selected = selected;
    if (back != 0) {
        int level = g_ArcSel_Level.level;

        list->selected = level;
    }
}

// Pushes the class page.
int LevelSelect_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    int result = 0;
    int choice;
    PadState *pad;
    ArcMenuList *list;

    if (page->delay > 0 && --page->delay == 0) {
        MenuList_Open_OVL2(&g_ArcMenu_Level);
    }
    pad = &menu->padState[0];
    if (active == 0) {
        pad = 0;
    }
    list = &g_ArcMenu_Level;
    choice = MenuList_Update_OVL2(list, pad);
    switch (choice) {
    case -4:
        Sfx_PlayMenu(0);
        break;
    case -3:
        Sfx_PlayMenu(6);
        break;
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close_OVL2(list);
        result = 2;
        break;
    case -2:
        break;
    default:
        Sfx_PlayMenu(3);
        g_ArcSel_Level.level = choice;
        ArcSaved()->level = choice;
        MenuList_Close_OVL2(&g_ArcMenu_Level);
        ArcadeMenu_PushPage(menu, &g_ArcPage_Class);
        result = 1;
        break;
    }
    return result;
}

// The list.
void LevelSelect_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    TextCtx *text = menu->text;

    MenuList_Draw_OVL2(&g_ArcMenu_Level, layer->ot, text);
}

// Class S shows when it's unlocked; the two garage rows are disabled when empty.
void ClassSelect_Enter(ArcadeMenu *menu, int back)
{
    int selected = ArcSaved()->cls;
    ArcPage *page = (ArcPage *)menu->activePage;
    GameState *state;
    ArcMenuList *list;
    ArcMenuItem *items;
    int count;

    if (selected >= 5) {
        selected = 0;
    }
    page->delay = 0x18;
    if (ArcCarList_IsClassSUnlocked() != 0) {
        count = 6;
        g_ArcMenu_Class.count = count;
        items = D_8004FBFC;
    } else {
        selected--;
        if (selected < 0) {
            selected = 0;
        }
        count = 5;
        g_ArcMenu_Class.count = count;
        items = D_8004FC10;
    }
    g_ArcMenu_Class.items = items;
    if (selected >= count) {
        selected = count - 1;
    }
    state = &g_GameState;
    D_8004FC4C[0].kind &= 0x7f;
    if (state->garages[0].carCount == 0) {
        D_8004FC4C[0].kind |= 0x80;
    }
    D_8004FC4C[1].kind &= 0x7f;
    if (state->garages[1].carCount == 0) {
        D_8004FC4C[1].kind |= 0x80;
    }
    list = &g_ArcMenu_Class;
    MenuList_Init_OVL2(list);
    list->selected = selected;
}

// Rows 4 and 5 are the garages, the others the classes of the car selection page.
int ClassSelect_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    int result = 0;
    int choice;
    PadState *pad;

    if (page->delay > 0 && --page->delay == 0) {
        MenuList_Open_OVL2(&g_ArcMenu_Class);
    }
    pad = &menu->padState[0];
    if (active == 0) {
        pad = 0;
    }
    choice = MenuList_Update_OVL2(&g_ArcMenu_Class, pad);
    switch (choice) {
    case -4:
        Sfx_PlayMenu(0);
        break;
    case -3:
        Sfx_PlayMenu(6);
        break;
    case -2:
        break;
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close_OVL2(&g_ArcMenu_Class);
        result = 2;
        break;
    case 4:
    case 5: {
        ArcadeRaceSel *sel;
        MenuPage *next;

        ArcSaved()->cls = choice;
        Sfx_PlayMenu(3);
        sel = &g_ArcSel_Level;
        sel->cls = choice;
        sel->garage = choice - 4;
        MenuList_Close_OVL2(&g_ArcMenu_Class);
        next = &g_ArcPage_Garage;
        ArcadeMenu_PushPage(menu, next);
        next->title = D_8004FDDC[sel->garage];
        result = 1;
        break;
    }
    default: {
        ArcadeRaceSel *sel;

        ArcSaved()->cls = choice;
        Sfx_PlayMenu(3);
        sel = &g_ArcSel_Level;
        sel->cls = choice;
        sel->garage = -1;
        MenuList_Close_OVL2(&g_ArcMenu_Class);
        g_ArcPage_CarSelect.titleColor = g_ArcClassColors[choice];
        ArcadeMenu_PushPage(menu, &g_ArcPage_CarSelect);
        result = 1;
        break;
    }
    }
    return result;
}

// The list.
void ClassSelect_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    TextCtx *text = menu->text;

    MenuList_Draw_OVL2(&g_ArcMenu_Class, layer->ot, text);
}

// The two garage rows are disabled when empty.
void RallyClassSelect_Enter(ArcadeMenu *menu, int back)
{
    GameState *state = &g_GameState;
    int selected = ArcSavedOf(state)->rallyCls;
    ArcPage *page = (ArcPage *)menu->activePage;

    if (selected >= 3) {
        selected = 0;
    }
    page->delay = 0x18;
    D_8004FD08[0].kind &= 0x7f;
    if (state->garages[0].carCount == 0) {
        D_8004FD08[0].kind |= 0x80;
    }
    D_8004FD08[1].kind &= 0x7f;
    if (state->garages[1].carCount == 0) {
        D_8004FD08[1].kind |= 0x80;
    }
    MenuList_Init_OVL2(&g_ArcMenu_RallyClass);
    g_ArcMenu_RallyClass.selected = selected;
}

// Rows 4 and 5 are the garages, the others the rally cars of the car selection page.
int RallyClassSelect_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    int result = 0;
    int choice;
    PadState *pad;

    if (page->delay > 0 && --page->delay == 0) {
        MenuList_Open_OVL2(&g_ArcMenu_RallyClass);
    }
    pad = &menu->padState[0];
    if (active == 0) {
        pad = 0;
    }
    choice = MenuList_Update_OVL2(&g_ArcMenu_RallyClass, pad);
    switch (choice) {
    case -4:
        Sfx_PlayMenu(0);
        break;
    case -3:
        Sfx_PlayMenu(6);
        break;
    case -2:
        break;
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close_OVL2(&g_ArcMenu_RallyClass);
        result = 2;
        break;
    case 4:
    case 5: {
        ArcadeRaceSel *sel;
        MenuPage *next;

        ArcSaved()->rallyCls = choice - 3;
        Sfx_PlayMenu(3);
        sel = &g_ArcSel_Level;
        sel->cls = choice;
        sel->garage = choice - 4;
        MenuList_Close_OVL2(&g_ArcMenu_RallyClass);
        next = &g_ArcPage_Garage;
        ArcadeMenu_PushPage(menu, next);
        next->title = D_8004FDDC[sel->garage];
        result = 1;
        break;
    }
    default: {
        ArcadeRaceSel *sel;

        ArcSaved()->rallyCls = choice - 6;
        Sfx_PlayMenu(3);
        sel = &g_ArcSel_Level;
        sel->cls = 6;
        sel->garage = -1;
        MenuList_Close_OVL2(&g_ArcMenu_RallyClass);
        g_ArcPage_CarSelect.titleColor = 0x2084b6;
        ArcadeMenu_PushPage(menu, &g_ArcPage_CarSelect);
        result = 1;
        break;
    }
    }
    return result;
}

// The list.
void RallyClassSelect_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    TextCtx *text = menu->text;

    MenuList_Draw_OVL2(&g_ArcMenu_RallyClass, layer->ot, text);
}
}
