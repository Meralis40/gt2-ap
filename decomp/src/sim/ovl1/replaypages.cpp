// OVL1 800124a4-80013230 (replaypages): Replay theater, demo, save and load pages.
// The functions that don't match yet are the original assembly, with their C++ under
// NON_MATCHING.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern BaseMenu *g_CurrentMenu;
extern pointer g_OVL1_EndFontInfo;
extern KanjiFont *g_OVL1_KanjiFont;
extern short g_CardSlotStatus[];
extern CardMenuConfig g_OVL1_CardMenuConfig;
extern ReplayCopyConfig g_OVL1_ReplayCopyConfig;
extern CardMenu *g_LoadReplay_CardMenu;
extern CardMenu *g_RenameDeleteReplay_CardMenu;
extern void *g_CopyReplay_End;
extern MenuSprite g_ReplayTheater_Items[];
extern MenuSpriteStyle g_ReplayTheater_ItemStyle;
extern SpriteDesc g_ReplayTheater_Sprites[];
extern MenuList g_ReplayTheater_List;
extern BaseMenuPage *g_ReplayTheater_SubPages[];
extern MenuList g_DemoSelect_List;
extern short g_DemoSelect_OpenDelay;
extern int g_DemoSelect_Index;
extern ReplayEntryStyle D_8004B20C;
extern BaseMenuPage g_Page_ReplayTheaterMenu;
extern BaseMenuPage g_Page_StartReplayWait;
extern BaseMenuPage g_Page_DemoStart;
extern BaseMenuPage g_Page_SaveGameCard;
extern BaseMenuPage g_Page_LoadGameCard;
void PushPage(BaseMenu *menu, BaseMenuPage *page);
int ReplayTheaterMenu_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index);
int DemoSelect_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index);

// Root page of the Replay Theater: stores back, waits 20 frames, starts music track 0.
// On first entry also calls DemoReplay_Count on the replay buffer.
void ReplayTheaterRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].enter = (MenuPageEnterFn *)0x14;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
    if (back == 0) {
        DemoReplay_Count(g_OVL1_EndFontInfo);
    }
    MainMenu_PlayMusic(0);
}

// After the delay: pushes the Replay Theater menu, or leaves the menu (4) on the way back.
int ReplayTheaterRoot_Update(int arg)
{
    BaseMenu *menu = g_CurrentMenu;
    RootMenuPage *page = (RootMenuPage *)menu->activePage;
    int result = 0;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            result = 4;
            if (page->back != 1) {
                PushPage(menu, &g_Page_ReplayTheaterMenu);
                result = 1;
            }
        }
    }
    return result;
}

// Empty.
void ReplayTheaterRoot_Draw(DrawLayer *layer)
{
}

// Page pushed after a replay was chosen (BaseMenu.result already set): sets a 20-frame
// delay and stops the music (MainMenu_StopMusic).
void StartReplayWait_Enter(int back)
{
    g_CurrentMenu->activePage[1].enter = (MenuPageEnterFn *)0x14;
    MainMenu_StopMusic();
}

// After the delay: leaves the menu (4).
int StartReplayWait_Update(int arg)
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
void StartReplayWait_Draw(DrawLayer *layer)
{
}

// A row of the Replay Theater menu: one sprite, from g_ReplayTheater_Sprites.
int ReplayTheaterMenu_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index)
{
    MenuSprite *item = &g_ReplayTheater_Items[index];
    int x;
    int y;

    switch (event) {
    case MLE_INIT:
        MenuSprite_Init(item, &g_ReplayTheater_ItemStyle);
        item->sprite = &g_ReplayTheater_Sprites[index];
        break;
    case MLE_REVEAL:
        item->anim = 0;
        break;
    case MLE_CLOSE:
        item->anim = -13;
        break;
    case MLE_UPDATE:
        MenuSprite_Update(item);
        break;
    case MLE_DRAW:
        x = ev->x;
        y = ev->y;
        item->x = x;
        item->y = y;
        MenuSprite_Draw(item, ev->ot, 0);
        break;
    case MLE_LEAVE:
        break;
    case MLE_ENTER:
        item->anim = 12;
        break;
    case MLE_OPEN:
    case MLE_IS_ENABLED:
        break;
    }
    return 1;
}

// Delays the list's opening 20 frames and, on the way back, keeps the chosen row.
void ReplayTheaterMenu_Enter(int back)
{
    RootMenuPage *page = (RootMenuPage *)g_CurrentMenu->activePage;
    MenuList *list;
    int row;

    page->delay = 0x14;
    list = &g_ReplayTheater_List;
    MenuList_Init(list, ReplayTheaterMenu_ItemCallback, 0);
    list->selected = 0;
    if (back) {
        row = page->back;
        list->selected = row;
    }
}

// Opens the list after the delay. A chosen row is kept in back and pushes its page.
int ReplayTheaterMenu_Update(int arg)
{
    BaseMenu *menu = g_CurrentMenu;
    RootMenuPage *page = (RootMenuPage *)menu->activePage;
    int result = 0;
    PadState *pad = menu->padState;
    int choice;
    MenuList *list;

    if (arg == 0) {
        pad = 0;
    }
    choice = page->delay;
    if (choice > 0) {
        choice--;
        if (choice == 0) {
            MenuList_Open(&g_ReplayTheater_List);
        }
    }
    page->delay = choice;
    list = &g_ReplayTheater_List;
    choice = MenuList_Update(list, pad);
    switch (choice) {
    case -4:
        break;
    case -3:
        Sfx_PlayMenu(5);
        break;
    case -2:
        break;
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close(list);
        result = 2;
        break;
    default:
        Sfx_PlayMenu(3);
        page->back = choice;
        MenuList_Close(&g_ReplayTheater_List);
        result = 1;
        PushPage(g_CurrentMenu, g_ReplayTheater_SubPages[choice]);
        break;
    }
    return result;
}

// Draws the list.
void ReplayTheaterMenu_Draw(DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;

    MenuList_Draw(&g_ReplayTheater_List, ot + 4);
}

// Starts the card menu on the replays to play.
void LoadReplay_Enter(int back)
{
    g_OVL1_CardMenuConfig.slotStatus = g_CardSlotStatus;
    g_OVL1_CardMenuConfig.work = (CardMenu *)((char *)g_OVL1_EndFontInfo + 0x40000);
    g_OVL1_CardMenuConfig.kanjiFont = g_OVL1_KanjiFont;
    g_LoadReplay_CardMenu = CardMenu_Setup(&g_OVL1_CardMenuConfig);
    CardMenu_StartPlayReplay();
}

#ifdef NON_MATCHING
// When a replay was loaded, sets BaseMenu.result and pushes the page that waits.
int LoadReplay_Update(int arg)
{
    int result = 0;
    PadState *pad = g_CurrentMenu->padState;

    if (arg == 0) {
        pad = 0;
    }
    switch (CardMenu_Update(pad)) {
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    case 2:
        g_CurrentMenu->result = 1;
        result = 1;
        Sfx_PlayMenu(3);
        PushPage(g_CurrentMenu, &g_Page_StartReplayWait);
        break;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", LoadReplay_Update);
#endif

// CardMenu_Draw.
void LoadReplay_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// Starts the card menu on the replays to rename or delete.
void RenameDeleteReplay_Enter(int back)
{
    g_OVL1_CardMenuConfig.slotStatus = g_CardSlotStatus;
    g_OVL1_CardMenuConfig.work = (CardMenu *)((char *)g_OVL1_EndFontInfo + 0x40000);
    g_OVL1_CardMenuConfig.kanjiFont = g_OVL1_KanjiFont;
    g_RenameDeleteReplay_CardMenu = CardMenu_Setup(&g_OVL1_CardMenuConfig);
    CardMenu_StartEditReplay();
}

// Back when the card menu is left.
int RenameDeleteReplay_Update(int arg)
{
    int result = 0;
    PadState *pad = g_CurrentMenu->padState;

    if (arg == 0) {
        pad = 0;
    }
    switch (CardMenu_Update(pad)) {
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

// CardMenu_Draw.
void RenameDeleteReplay_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// Starts the replay copy.
void CopyReplay_Enter(int back)
{
    g_OVL1_ReplayCopyConfig.cardSlots = g_CardSlotStatus;
    g_OVL1_ReplayCopyConfig.heap = (unsigned int)g_OVL1_EndFontInfo + 0x40000;
    g_OVL1_ReplayCopyConfig.font = g_OVL1_KanjiFont;
    g_CopyReplay_End = ReplayCopy_Setup(&g_OVL1_ReplayCopyConfig);
    ReplayCopy_Start();
}

// Back when the copy is left.
int CopyReplay_Update(int arg)
{
    int result = 0;
    PadState *pad = g_CurrentMenu->padState;

    if (arg == 0) {
        pad = 0;
    }
    switch (ReplayCopy_Update(pad)) {
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

// ReplayCopy_Draw.
void CopyReplay_Draw(DrawLayer *layer)
{
    ReplayCopy_Draw((unsigned int *)((int)layer->ot + 0x10));
}

#ifdef NON_MATCHING
// A row of the demo list: the replay's entry from the demo file, sliding in with the
// list's fade.
int DemoSelect_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index)
{
    uint *ot = (uint *)ev->ot + 1;
    MenuList *list = ev->list;
    int x = ev->x;
    int y = ev->y;
    ReplayDirEntry *entry = &((ReplayDir *)g_OVL1_EndFontInfo)->entries[index];
    TextCtx ctx;
    TextCtx *t;
    int result = 1;

    Text_Init(&ctx, 0x1e);
    t = &ctx;
    t->unk1c = 1;
    if (event == MLE_DRAW) {
        int state = list->state;
        int open = state >= 0;
        int closing = state < -1;
        int fade;
        int alpha;
        int slide;

        open |= closing;
        if (!open) {
            return result;
        }
        fade = (list->fade << 7) / list->fadeMax;
        alpha = (fade * ev->brightness) >> 7;
        if (alpha != 0) {
            slide = 128 - fade;
            if (slide < 0) {
                slide = 0;
            }
            slide = slide * 176 >> 7;
            if (closing) {
                slide = 0;
            }
            ReplayEntry_Draw(&D_8004B20C, ot, t, entry, x + slide, y, alpha, 0);
        }
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", DemoSelect_ItemCallback);
#endif

// On first entry: the list has a row for each replay of the demo file.
void DemoSelect_Enter(int back)
{
    if (back == 0) {
        int count;

        g_DemoSelect_OpenDelay = 0x18;
        count = ((ReplayDir *)g_OVL1_EndFontInfo)->count;
        g_DemoSelect_List.count = count;
        MenuList_Init(&g_DemoSelect_List, DemoSelect_ItemCallback, 0);
    }
}

// Opens the list after the delay; a chosen row is kept and pushes g_Page_DemoStart.
int DemoSelect_Update(int arg)
{
    PadState *pad = g_CurrentMenu->padState;
    int result = 0;
    int choice;
    MenuList *list;

    if (g_DemoSelect_OpenDelay > 0) {
        if (--g_DemoSelect_OpenDelay == 0) {
            Sfx_PlayMenu(7);
            MenuList_Open(&g_DemoSelect_List);
        }
    }
    if (arg == 0) {
        pad = 0;
    }
    list = &g_DemoSelect_List;
    choice = MenuList_Update(list, pad);
    switch (choice) {
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close(list);
        result = 2;
        break;
    case -3:
        Sfx_PlayMenu(6);
        break;
    case -4:
        Sfx_PlayMenu(0);
        break;
    case -2:
        break;
    default:
        Sfx_PlayMenu(3);
        MenuList_Close(&g_DemoSelect_List);
        g_DemoSelect_Index = choice;
        PushPage(g_CurrentMenu, &g_Page_DemoStart);
        result = 1;
        break;
    }
    return result;
}

// Draws the list.
void DemoSelect_Draw(DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;

    MenuList_Draw(&g_DemoSelect_List, ot + 4);
}

// Sets BaseMenu.result (start a replay) and a 20-frame delay.
void DemoStart_Enter(int back)
{
    BaseMenu *pBVar1;

    pBVar1 = g_CurrentMenu;
    g_CurrentMenu->activePage[1].enter = (MenuPageEnterFn *)0x14;
    pBVar1->result = '\x01';
}

// After the delay: loads the chosen demo and leaves the menu (4).
int DemoStart_Update(int arg)
{
    RootMenuPage *page = (RootMenuPage *)g_CurrentMenu->activePage;
    int result = 0;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            DemoReplay_Load(g_OVL1_EndFontInfo, (char *)g_OVL1_EndFontInfo + 0x20000, g_DemoSelect_Index);
            result = 4;
        }
    }
    return result;
}

// Empty.
void DemoStart_Draw(DrawLayer *layer)
{
}

// Root page of the Save Game menu: stores back, waits 20 frames, starts music track 7.
void SaveGameRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
    pMVar1[1].enter = (MenuPageEnterFn *)0x14;
    MainMenu_PlayMusic(7);
}

// After the delay: pushes the save page, or leaves the menu (4) on the way back.
int SaveGameRoot_Update(int arg)
{
    BaseMenu *menu = g_CurrentMenu;
    RootMenuPage *page = (RootMenuPage *)menu->activePage;
    int result = 0;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            result = 4;
            if (page->back != 1) {
                PushPage(menu, &g_Page_SaveGameCard);
                result = 1;
            }
        }
    }
    return result;
}

// Empty.
void SaveGameRoot_Draw(DrawLayer *layer)
{
}

// Starts the card menu saving the game.
void SaveGameCard_Enter(int back)
{
    g_OVL1_CardMenuConfig.slotStatus = g_CardSlotStatus;
    g_OVL1_CardMenuConfig.work = (CardMenu *)((char *)g_OVL1_EndFontInfo + 0x40000);
    g_OVL1_CardMenuConfig.kanjiFont = g_OVL1_KanjiFont;
    CardMenu_Setup(&g_OVL1_CardMenuConfig);
    CardMenu_StartSave(0);
}

// Back when the card menu is left.
int SaveGameCard_Update(int arg)
{
    int result = 0;
    PadState *pad = g_CurrentMenu->padState;

    if (arg == 0) {
        pad = 0;
    }
    switch (CardMenu_Update(pad)) {
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

// CardMenu_Draw.
void SaveGameCard_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// Root page of the Load Game menu: stores back, waits 20 frames, starts music track 7.
void LoadGameRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
    pMVar1[1].enter = (MenuPageEnterFn *)0x14;
    MainMenu_PlayMusic(7);
}

// After the delay: pushes the load page, or leaves the menu (4) on the way back.
int LoadGameRoot_Update(int arg)
{
    BaseMenu *menu = g_CurrentMenu;
    RootMenuPage *page = (RootMenuPage *)menu->activePage;
    int result = 0;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            result = 4;
            if (page->back != 1) {
                PushPage(menu, &g_Page_LoadGameCard);
                result = 1;
            }
        }
    }
    return result;
}

// Empty.
void LoadGameRoot_Draw(DrawLayer *layer)
{
}

// Starts the card menu loading a game.
void LoadGameCard_Enter(int back)
{
    g_OVL1_CardMenuConfig.slotStatus = g_CardSlotStatus;
    g_OVL1_CardMenuConfig.work = (CardMenu *)((char *)g_OVL1_EndFontInfo + 0x40000);
    g_OVL1_CardMenuConfig.kanjiFont = g_OVL1_KanjiFont;
    CardMenu_Setup(&g_OVL1_CardMenuConfig);
    CardMenu_StartLoad(0);
}

// Back when the card menu is left.
int LoadGameCard_Update(int arg)
{
    int result = 0;
    PadState *pad = g_CurrentMenu->padState;

    if (arg == 0) {
        pad = 0;
    }
    switch (CardMenu_Update(pad)) {
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

// CardMenu_Draw.
void LoadGameCard_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}
}
