// OVL2 80020224-800228c0 (battle2p): 2-player battle page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

// A player's step through the page.
struct Arc2PPlayer {
    signed char step; // 0 class, 1 garage list, 2 garage car, 3 transmission, 4 ready (garage car), 5 class car, 6 transmission, 7 settings, 8 ready, 9 left
    signed char cls; // class index (4/5: home/guest garage)
    signed char car;
    unsigned char refresh; // rebuild the spec panels
    short timer;
};

// The page and its timers.
struct Arc2PPage {
    MenuPage def;
    int titleDelay; // shows the titles at 0
    int carViewDelay; // draws the car views at 0
    int pickerDelay; // opens the pickers at 0
};

// A garage car list (MenuList, then a word).
struct ArcGarageList {
    MenuList list;
    int unk30;
};

extern GameState g_GameState;
extern Arc2PPlayer g_Arc_2PPlayers[2];
extern short g_Arc_2PGarageOrder[2][100];
extern ArcGarageInfo g_Arc_HomeGarageInfo;
extern ArcGarageInfo g_Arc_GuestGarageInfo;
extern short D_8005002C[]; // the item layouts of the carousels
extern short D_80050040[];
extern GrowBar D_8004FDE4; // the grow bar behind the class lists
extern ArcMenuList g_ArcMenu_2PClass[2];
extern ArcGarageList g_ArcList_2PGarage[2][2]; // per player, home and guest
extern ArcGridPicker g_Arc_2PCarousel[2];
extern unsigned char g_Arc_2PCarView[2][0xdc];
extern ArcTextItem D_800F1778[2]; // the players' titles
extern short D_800F1740[2][0xe]; // the gradient bars behind them
extern ArcTextItem D_800F1980[2]; // what each player picks
extern ArcCarSpecPanel D_800F1BE8[2];
extern ArcDialog g_Arc_2PTransPicker[2];
extern ArcDialog g_Arc_2PSettingPicker[2];
extern ArcadeRaceSel g_ArcSel_Level;
extern MenuPage g_ArcPage_Course;

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/battle2p", Battle2P_DrawPlayerItem);

// Player 1's carousel callback.
void Battle2P_DrawItemP1(ArcGridPickerDraw *ctx, int item)
{
    Battle2P_DrawPlayerItem(ctx, item, 0, g_Arc_2PPlayers[0].cls, D_8005002C);
}

// Player 2's carousel callback.
void Battle2P_DrawItemP2(ArcGridPickerDraw *ctx, int item)
{
    Battle2P_DrawPlayerItem(ctx, item, 1, g_Arc_2PPlayers[1].cls, D_80050040);
}

// GarageList_Callback for player 1, home garage.
int Battle2P_ListP1Home(int event, void *ctx, int index)
{
    int slot = g_Arc_2PGarageOrder[0][index];
    ArcGarageInfo *info = &g_Arc_HomeGarageInfo;
    GameState *gs = &g_GameState;

    return GarageList_Callback(event, (MenuListEvent *)ctx, index, &gs->garages[0], &info->ratings[slot], slot);
}

// GarageList_Callback for player 1, guest garage.
int Battle2P_ListP1Guest(int event, void *ctx, int index)
{
    int slot = g_Arc_2PGarageOrder[0][index];
    ArcGarageInfo *info = &g_Arc_GuestGarageInfo;
    GameState *gs = &g_GameState;

    return GarageList_Callback(event, (MenuListEvent *)ctx, index, &gs->garages[1], &info->ratings[slot], slot);
}

// GarageList_Callback for player 2, home garage.
int Battle2P_ListP2Home(int event, void *ctx, int index)
{
    int slot = g_Arc_2PGarageOrder[1][index];
    ArcGarageInfo *info = &g_Arc_HomeGarageInfo;
    GameState *gs = &g_GameState;

    return GarageList_Callback(event, (MenuListEvent *)ctx, index, &gs->garages[0], &info->ratings[slot], slot);
}

// GarageList_Callback for player 2, guest garage.
int Battle2P_ListP2Guest(int event, void *ctx, int index)
{
    int slot = g_Arc_2PGarageOrder[1][index];
    ArcGarageInfo *info = &g_Arc_GuestGarageInfo;
    GameState *gs = &g_GameState;

    return GarageList_Callback(event, (MenuListEvent *)ctx, index, &gs->garages[1], &info->ratings[slot], slot);
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/battle2p", Battle2P_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/battle2p", Battle2P_UpdatePlayer);

// Closes the widgets of the player's step (the other player backed out).
void Battle2P_ClosePlayer(int player, Arc2PPlayer *st)
{
    switch (st->step) {
    case 0:
        if (st->timer == 0) {
            MenuList_Close_OVL2(&g_ArcMenu_2PClass[player]);
        }
        break;
    case 1:
        if (st->timer == 0) {
            MenuList_Close(&g_ArcList_2PGarage[player][g_ArcSel_Level.garages[player]].list);
        }
        break;
    case 2:
    case 5:
        TextItem_Hide_OVL2(&D_800F1980[player]);
        if (st->timer == 0) {
            GridPicker_Close(&g_Arc_2PCarousel[player]);
            CarSpecPanel_Close(&D_800F1BE8[player]);
        }
        break;
    case 3:
    case 6:
        TextItem_Hide_OVL2(&D_800F1980[player]);
        GridPicker_Close(&g_Arc_2PCarousel[player]);
        Dialog_Close_OVL2(&g_Arc_2PTransPicker[player]);
        break;
    case 7:
        TextItem_Hide_OVL2(&D_800F1980[player]);
        GridPicker_Close(&g_Arc_2PCarousel[player]);
        Dialog_Close_OVL2(&g_Arc_2PSettingPicker[player]);
        break;
    case 4:
    case 8:
        TextItem_Hide_OVL2(&D_800F1980[player]);
        GridPicker_Close(&g_Arc_2PCarousel[player]);
        break;
    }
}

// Runs both players: one backing out goes back (2); both ready (step 4 or 8) pushes the
// course selection (1).
// The game keeps the result on the stack and more addresses in saved registers.
#ifdef NON_MATCHING
int Battle2P_Update(ArcadeMenu *menu, int active)
{
    Arc2PPage *page = (Arc2PPage *)menu->activePage;
    int result;
    unsigned int done1;
    unsigned int done2;

    GrowBar_Update((unsigned char *)&D_8004FDE4);
    result = 0;
    if (page->carViewDelay > 0) {
        page->carViewDelay--;
    }
    if (page->titleDelay > 0 && --page->titleDelay == 0) {
        TextItem_Show_OVL2(&D_800F1778[0], -1);
        TextItem_Show_OVL2(&D_800F1778[1], -1);
        GradientBar_Start(D_800F1740[0]);
        GradientBar_Start(D_800F1740[1]);
    }
    if (page->pickerDelay > 0 && --page->pickerDelay == 0) {
        Dialog_Open_OVL2(g_Arc_2PPlayers[0].step == 7 ? &g_Arc_2PSettingPicker[0] : &g_Arc_2PTransPicker[0]);
        Dialog_Open_OVL2(g_Arc_2PPlayers[1].step == 7 ? &g_Arc_2PSettingPicker[1] : &g_Arc_2PTransPicker[1]);
        GridPicker_Open(&g_Arc_2PCarousel[0]);
        GridPicker_Open(&g_Arc_2PCarousel[1]);
    }
    TextItem_Update_OVL2(&D_800F1778[0]);
    TextItem_Update_OVL2(&D_800F1778[1]);
    GradientBar_Update(D_800F1740[0]);
    GradientBar_Update(D_800F1740[1]);
    TextItem_Update_OVL2(&D_800F1980[0]);
    TextItem_Update_OVL2(&D_800F1980[1]);
    done1 = Battle2P_UpdatePlayer(menu, active, 0, &g_Arc_2PPlayers[0]);
    done2 = Battle2P_UpdatePlayer(menu, active, 1, &g_Arc_2PPlayers[1]);
    if (done1 & (done2 ^ 1)) {
        Battle2P_ClosePlayer(0, &g_Arc_2PPlayers[0]);
    }
    if (done2 & (done1 ^ 1)) {
        Battle2P_ClosePlayer(1, &g_Arc_2PPlayers[1]);
    }
    if ((done1 ^ 1) || (done2 ^ 1)) {
        Sfx_PlayMenu(4);
        result = 2;
        TextItem_Hide_OVL2(&D_800F1778[0]);
        TextItem_Hide_OVL2(&D_800F1778[1]);
        GradientBar_Close(D_800F1740[0]);
        GradientBar_Close(D_800F1740[1]);
        D_8004FDE4.t = ~D_8004FDE4.duration;
        ArcLogo_Reset(&menu->logos[0]);
        ArcLogo_Reset(&menu->logos[1]);
        ArcCarView_Unload(menu->carViews[0], menu);
        ArcCarView_Unload(menu->carViews[1], menu);
        g_Arc_2PPlayers[0].step = 9;
        g_Arc_2PPlayers[1].step = 9;
    }
    if ((g_Arc_2PPlayers[0].step == 8 || g_Arc_2PPlayers[0].step == 4) &&
        (g_Arc_2PPlayers[1].step == 8 || g_Arc_2PPlayers[1].step == 4)) {
        g_Arc_2PPlayers[0].step = 9;
        g_Arc_2PPlayers[1].step = 9;
        TextItem_Hide_OVL2(&D_800F1778[0]);
        TextItem_Hide_OVL2(&D_800F1778[1]);
        GradientBar_Close(D_800F1740[0]);
        GradientBar_Close(D_800F1740[1]);
        TextItem_Hide_OVL2(&D_800F1980[0]);
        TextItem_Hide_OVL2(&D_800F1980[1]);
        result = 1;
        D_8004FDE4.t = ~D_8004FDE4.duration;
        GridPicker_Close(&g_Arc_2PCarousel[0]);
        GridPicker_Close(&g_Arc_2PCarousel[1]);
        ArcLogo_Reset(&menu->logos[0]);
        ArcLogo_Reset(&menu->logos[1]);
        page->carViewDelay = -1;
        Sfx_PlayMenu(3);
        ArcadeMenu_PushPage(menu, &g_ArcPage_Course);
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/battle2p", Battle2P_Update);
#endif

static inline void TextItem_DrawAt(ArcTextItem *item, int x, int y, void *ot, TextCtx *ctx)
{
    item->x = x;
    item->y = y;
    TextItem_Draw_OVL2(item, ot, ctx);
}

// Both halves of the screen.
// The game copies the text context into another register before the car views.
#ifdef NON_MATCHING
void Battle2P_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    void *ot = layer->ot;
    TextCtx *ctx = menu->text;
    Arc2PPage *page = (Arc2PPage *)menu->activePage;
    uint *ot2 = (uint *)ot + 2;
    RECT area;

    GrowBar_Draw(&D_8004FDE4, ot2);
    MenuList_Draw_OVL2(&g_ArcMenu_2PClass[0], ot2, ctx);
    MenuList_Draw_OVL2(&g_ArcMenu_2PClass[1], ot2, ctx);
    MenuList_Draw(&g_ArcList_2PGarage[0][0].list, ot2);
    MenuList_Draw(&g_ArcList_2PGarage[0][1].list, ot2);
    MenuList_Draw(&g_ArcList_2PGarage[1][0].list, ot2);
    MenuList_Draw(&g_ArcList_2PGarage[1][1].list, ot2);
    GridPicker_Draw(&g_Arc_2PCarousel[0], ot2);
    GridPicker_Draw(&g_Arc_2PCarousel[1], ot2);
    if (page->carViewDelay == 0) {
        ArcCarView_Draw(menu->carViews[0], &menu->layers[4], 0, g_Arc_2PCarView[0]);
        ArcCarView_Draw(menu->carViews[1], &menu->layers[5], 0, g_Arc_2PCarView[1]);
    }
    TextItem_DrawAt(&D_800F1778[0], 0x14, 0x6c, ot, ctx);
    TextItem_DrawAt(&D_800F1778[1], 0x14c, 0x1c8, ot, ctx);
    GradientBar_Draw((ArcGradientBar *)D_800F1740[0], ot2, 0, 0x58);
    GradientBar_Draw((ArcGradientBar *)D_800F1740[1], ot2, 0x160, 0x1b4);
    Prim_AddDrawMode(ot2, 0x220);
    TextItem_DrawAt(&D_800F1980[0], 0x9c, 0x6c, ot, ctx);
    TextItem_DrawAt(&D_800F1980[1], 0xc4, 0x1c8, ot, ctx);
    area.x = 0x8a;
    area.y = 0x5e;
    area.w = 0xd2;
    area.h = 0x90;
    DrawLayer_SetArea(&menu->layers[4], &area);
    area.x = 4;
    area.y = 0x12e;
    area.w = 0xd2;
    area.h = 0x90;
    DrawLayer_SetArea(&menu->layers[5], &area);
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/battle2p", Battle2P_Draw);
#endif
}
