// OVL2 80020224-800228c0 (battle2p): 2-player battle page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

static inline void Item_Enable(ArcMenuItem *item)
{
    *(unsigned char *)&item->kind &= 0x7f;
}

static inline void Item_Disable(ArcMenuItem *item)
{
    *(unsigned char *)&item->kind |= 0x80;
}

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

extern GameState (&g_GameState);
extern Arc2PPlayer (&g_Arc_2PPlayers)[2];
extern short (&g_Arc_2PGarageOrder)[2][100];
extern ArcGarageInfo (&g_Arc_HomeGarageInfo);
extern ArcGarageInfo (&g_Arc_GuestGarageInfo);
extern short (&D_8005002C)[]; // the item layouts of the carousels
extern short (&D_80050040)[];
extern GrowBar (&D_8004FDE4); // the grow bar behind the class lists
extern ArcMenuList (&g_ArcMenu_2PClass)[2];
extern ArcGarageList (&g_ArcList_2PGarage)[2][2]; // per player, home and guest
extern ArcGridPicker (&g_Arc_2PCarousel)[2];
extern unsigned char (&g_Arc_2PCarView)[2][0xdc];
extern ArcTextItem (&D_800F1778)[2]; // the players' titles
extern short (&D_800F1740)[2][0xe]; // the gradient bars behind them
extern ArcTextItem (&D_800F1980)[2]; // what each player picks
extern ArcCarSpecPanel (&D_800F1BE8)[2];
extern ArcDialog (&g_Arc_2PTransPicker)[2];
extern ArcDialog (&g_Arc_2PSettingPicker)[2];
extern ArcadeRaceSel (&g_ArcSel_Level);
extern MenuPage (&g_ArcPage_Course);
extern char (&D_80027224)[];
extern char (&D_8002723C)[];
extern ArcSprite (&D_8004F7C4)[]; // the makers
extern unsigned char (&D_8004FE20)[];
extern char (&D_800F86CD)[];
extern short (&D_8004FDF8)[];
extern short (&D_8004FE0C)[];
extern unsigned char (&D_8004FFF0)[];
extern short (&g_Arc_CourseSet);
extern ArcMenuItem (&D_8004FE48)[];
extern ArcMenuItem (&D_8004FE5C)[];
extern ArcMenuItem (&D_8004FE98)[];
extern ArcMenuItem (&D_8004FEC0)[];
extern ArcMenuItem (&D_8004FEFC)[];
extern ArcMenuItem (&D_8004FF10)[];
extern ArcMenuItem (&D_8004FF4C)[];
extern ArcMenuItem (&D_8004FF74)[];
extern ArcGridPickerTmpl (&D_8005000C);
extern ArcGridPickerTmpl (&D_8005001C);
extern ArcDialogDesc (&D_8004FD74);
extern ArcDialogDesc (&D_8004FD8C);
extern ArcDialogDesc (&D_80050054);
extern ArcDialogDesc (&D_80050074);
extern game_pointer<ArcDialogDesc> (&D_8005006C)[2];
extern game_pointer<ArcDialogDesc> (&D_8005008C)[2];
extern unsigned char (&D_80052190)[];
extern game_pointer<unsigned char> (&g_ArcCarList_Spec3)[];
extern game_pointer<ArcCarRating> (&g_ArcCarList_Ratings)[];
extern game_pointer<signed char> (&g_ArcCarList_Avail)[];
extern short (&g_ArcCarList_Count)[];
extern unsigned char (&g_ArcPickerValues)[];
extern unsigned int (&g_ArcClassColors)[];
extern game_pointer<char> (&D_8004FCCC)[]; // the class names
int ArcCarList_FirstAvailable(int cls);
int strcmp(const char *a, const char *b);
void Battle2P_DrawPlayerItem(ArcGridPickerDraw *ctx, int item, int player, int cls, short *layout);

// Requests the logo of the player's class car and loads the car in colour color.
static inline void Battle2P_ShowCar(ArcadeMenu *menu, int player, Arc2PPlayer *st, ArcLogo *logo, int car,
                                    int color)
{
    int logoColor = ArcCarList_GetColor(st->cls, car);

    ArcLogo_Request(logo, logoColor, CarCode_Encode((signed char *)ArcCarList_GetCode(st->cls, car)));
    ArcCarView_LoadCarCode(menu->carViews[player], menu, ArcCarList_GetCode(st->cls, car), color);
    ArcCarView_InitSmall((ResultCarView *)g_Arc_2PCarView[player]);
}

// Draws a player's carousel item: the pickers, the spec panel, then the car's maker and
// logo. Two debug cars (D_80027224, D_8002723C) show no maker.
void Battle2P_DrawPlayerItem(ArcGridPickerDraw *ctx, int item, int player, int cls, short *layout)
{
    int selected = ctx->pos.item;
    ArcadeMenu *menu = (ArcadeMenu *)ctx->arg;
    int isCurrent = selected == item;
    ArcLogo *logo = &menu->logos[player];
    int y = ctx->y;
    TextCtx &text = *menu->text;
    void *ot = ctx->ot;
    int x = ctx->x;
    int brightness = ctx->brightness;
    ArcCarSpecPanel *panel;

    if (item < 0) {
        isCurrent = 1;
    }
    g_Arc_2PTransPicker[player].x = layout[8];
    g_Arc_2PTransPicker[player].y = layout[9];
    Dialog_Draw_OVL2(&g_Arc_2PTransPicker[player], ot, &text);
    g_Arc_2PSettingPicker[player].x = layout[8];
    g_Arc_2PSettingPicker[player].y = layout[9];
    Dialog_Draw_OVL2(&g_Arc_2PSettingPicker[player], ot, &text);
    panel = &D_800F1BE8[player];
    CarSpecPanel_Draw(panel, ot, x + layout[0], layout[1], &text, isCurrent, 0);
    CarSpecPanel_Draw(panel, ot, x + layout[2], layout[3], &text, isCurrent, 2);
    CarSpecPanel_Draw(panel, ot, x + layout[6], layout[7], &text, isCurrent, 3);
    if (item >= 0) {
        if (cls < 6 && cls >= 4) {
            ArcLogo_Draw(logo, ot, g_ArcSel_Level.carIds[player], x, y, brightness, 0, 0x40);
        } else {
            int noMaker = strcmp(D_80027224, ArcCarList_GetCode(cls, selected)) == 0;

            if (strcmp(D_8002723C, ArcCarList_GetCode(cls, selected)) == 0) {
                noMaker = 1;
            }
            if (!noMaker) {
                ArcSprite *sprite = &D_8004F7C4[ArcCarList_GetLogo(cls, selected)];

                Sprite_AddScaled(ot, sprite, x, y - ((short)sprite->h >> 2) - 2, brightness, 0x40);
            }
            ArcLogo_Draw(logo, ot, CarCode_Encode((signed char *)ArcCarList_GetCode(cls, selected)), x, y,
                         brightness, noMaker, 0x40);
        }
    }
}

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

// Coming back opens each player's last picker. Otherwise sets up both players' widgets;
// the class lists leave out an empty garage.
void Battle2P_Enter(ArcadeMenu *menu, int back)
{
    Arc2PPage &page = *(Arc2PPage *)menu->activePage;
    int i;

    if (back != 0) {
        ArcLogo *logo = &menu->logos[0];

        page.carViewDelay = 0x18;
        page.pickerDelay = 0x20;
        ArcLogo_Reset(logo);
        if (g_ArcSel_Level.garages[0] < 0) {
            int cls = g_Arc_2PPlayers[0].cls;
            int car = g_Arc_2PCarousel[0].cur.item;
            int color;

            g_Arc_2PPlayers[0].step = 7;
            color = ArcCarList_GetColor(cls, car);
            ArcLogo_Request(logo, color, CarCode_Encode((signed char *)ArcCarList_GetCode(cls, car)));
        } else {
            int slot = g_ArcSel_Level.slots[0];

            g_Arc_2PPlayers[0].step = 3;
            ArcLogo_Request(logo, -1, g_GameState.garages[g_ArcSel_Level.garages[0]].cars[slot].baseCarId);
        }
        logo = &menu->logos[1];
        ArcLogo_Reset(logo);
        if (g_ArcSel_Level.garages[1] < 0) {
            int cls = g_Arc_2PPlayers[1].cls;
            int car = g_Arc_2PCarousel[1].cur.item;
            int color;

            g_Arc_2PPlayers[1].step = 7;
            color = ArcCarList_GetColor(cls, car);
            ArcLogo_Request(logo, color, CarCode_Encode((signed char *)ArcCarList_GetCode(cls, car)));
        } else {
            int slot = g_ArcSel_Level.slots[1];

            g_Arc_2PPlayers[1].step = 3;
            ArcLogo_Request(logo, -1, g_GameState.garages[g_ArcSel_Level.garages[1]].cars[slot].baseCarId);
        }
        TextItem_Show_OVL2(&D_800F1778[0], -1);
        TextItem_Show_OVL2(&D_800F1778[1], -1);
        GradientBar_Start(D_800F1740[0]);
        GradientBar_Start(D_800F1740[1]);
        TextItem_Show_OVL2(&D_800F1980[0], -1);
        TextItem_Show_OVL2(&D_800F1980[1], -1);
        D_8004FDE4.t = 0;
        return;
    }
    page.pickerDelay = -1;
    page.titleDelay = 0x18;
    page.carViewDelay = 0x18;
    g_Arc_2PPlayers[0].step = 0;
    g_Arc_2PPlayers[1].step = 0;
    g_Arc_2PPlayers[0].cls = 0;
    g_Arc_2PPlayers[1].cls = 0;
    g_Arc_2PPlayers[0].refresh = 0;
    g_Arc_2PPlayers[1].refresh = 0;
    g_Arc_2PPlayers[0].timer = 0x18;
    g_Arc_2PPlayers[1].timer = 0x18;
    ArcCarView_SetSlowDecode(menu->carViews[0]);
    D_8004FDE4.t = 0;
    TextItem_Init_OVL2(&D_800F1778[0], D_8004FE20);
    D_800F1778[0].text = D_800F86CD;
    TextItem_Init_OVL2(&D_800F1778[1], D_8004FE20);
    D_800F1778[1].text = D_800F86CD + 0xc;
    D_800F1778[1].flags |= 0x100;
    GradientBar_Init(D_800F1740[0], D_8004FDF8);
    GradientBar_Init(D_800F1740[1], D_8004FE0C);
    TextItem_Init_OVL2(&D_800F1980[0], D_8004FFF0);
    D_800F1980[0].text = D_800F86CD - 0xc7;
    TextItem_Init_OVL2(&D_800F1980[1], D_8004FFF0);
    D_800F1980[1].text = D_800F86CD - 0xc7;
    if (g_Arc_CourseSet == 4) {
        g_ArcMenu_2PClass[0].items = D_8004FEC0;
        g_ArcMenu_2PClass[1].items = D_8004FF74;
        g_ArcMenu_2PClass[0].count = 3;
        g_ArcMenu_2PClass[1].count = 3;
        Item_Enable(&D_8004FEC0[1]);
        Item_Enable(&D_8004FF74[1]);
        if (g_GameState.garages[0].carCount == 0) {
            Item_Disable(&D_8004FEC0[1]);
            Item_Disable(&D_8004FF74[1]);
        }
        Item_Enable(&D_8004FEC0[2]);
        Item_Enable(&D_8004FF74[2]);
        if (g_GameState.garages[1].carCount == 0) {
            Item_Disable(&D_8004FEC0[2]);
            Item_Disable(&D_8004FF74[2]);
        }
    } else {
        if (ArcCarList_IsClassSUnlocked() != 0) {
            g_ArcMenu_2PClass[0].count = 6;
            g_ArcMenu_2PClass[0].items = D_8004FE48;
            g_ArcMenu_2PClass[1].count = 6;
            g_ArcMenu_2PClass[1].items = D_8004FEFC;
        } else {
            g_ArcMenu_2PClass[0].count = 5;
            g_ArcMenu_2PClass[0].items = D_8004FE5C;
            g_ArcMenu_2PClass[1].count = 5;
            g_ArcMenu_2PClass[1].items = D_8004FF10;
        }
        Item_Enable(&D_8004FE98[0]);
        Item_Enable(&D_8004FF4C[0]);
        if (g_GameState.garages[0].carCount == 0) {
            Item_Disable(&D_8004FE98[0]);
            Item_Disable(&D_8004FF4C[0]);
        }
        Item_Enable(&D_8004FE98[1]);
        Item_Enable(&D_8004FF4C[1]);
        if (g_GameState.garages[1].carCount == 0) {
            Item_Disable(&D_8004FE98[1]);
            Item_Disable(&D_8004FF4C[1]);
        }
    }
    MenuList_Init_OVL2(&g_ArcMenu_2PClass[0]);
    MenuList_Init_OVL2(&g_ArcMenu_2PClass[1]);
    D_8005000C.draw = Battle2P_DrawItemP1;
    D_8005000C.drawArg = menu;
    GridPicker_Init(&g_Arc_2PCarousel[0], &D_8005000C, 0);
    D_8005001C.draw = Battle2P_DrawItemP2;
    D_8005001C.drawArg = menu;
    GridPicker_Init(&g_Arc_2PCarousel[1], &D_8005001C, 0);
    ArcCarView_Unload(menu->carViews[0], menu);
    ArcCarView_Unload(menu->carViews[1], menu);
    ArcCarView_InitSmall((ResultCarView *)g_Arc_2PCarView[0]);
    ArcCarView_InitSmall((ResultCarView *)g_Arc_2PCarView[1]);
    ArcLogo_Reset(&menu->logos[0]);
    ArcLogo_Reset(&menu->logos[1]);
    CarSpecPanel_Init(&D_800F1BE8[0]);
    CarSpecPanel_Init(&D_800F1BE8[1]);
    Dialog_Init_OVL2(&g_Arc_2PTransPicker[0], &D_8004FD74);
    Dialog_Init_OVL2(&g_Arc_2PTransPicker[1], &D_80050054);
    Dialog_Init_OVL2(&g_Arc_2PSettingPicker[0], &D_8004FD8C);
    Dialog_Init_OVL2(&g_Arc_2PSettingPicker[1], &D_80050074);
    g_ArcList_2PGarage[0][0].list.count = g_GameState.garages[0].carCount;
    MenuList_Init(&g_ArcList_2PGarage[0][0].list, (MenuListFn *)Battle2P_ListP1Home, menu);
    g_ArcList_2PGarage[0][1].list.count = g_GameState.garages[1].carCount;
    MenuList_Init(&g_ArcList_2PGarage[0][1].list, (MenuListFn *)Battle2P_ListP1Guest, menu);
    g_ArcList_2PGarage[1][0].list.count = g_GameState.garages[0].carCount;
    MenuList_Init(&g_ArcList_2PGarage[1][0].list, (MenuListFn *)Battle2P_ListP2Home, menu);
    g_ArcList_2PGarage[1][1].list.count = g_GameState.garages[1].carCount;
    MenuList_Init(&g_ArcList_2PGarage[1][1].list, (MenuListFn *)Battle2P_ListP2Guest, menu);
    for (i = 0; i < 100; i++) {
        g_Arc_2PGarageOrder[0][i] = i;
        g_Arc_2PGarageOrder[1][i] = i;
    }
}

// Runs one player's step. Returns 0 when the player backs out of the first one.
int Battle2P_UpdatePlayer(ArcadeMenu *menu, int active, int player, Arc2PPlayer *st)
{
    PadState *pad = &menu->padState[player];
    int result = 1;
    ArcLogo *logo = &menu->logos[player];
    PadState *classPad = 0;
    PadState *carPad = 0;
    PadState *transPad = 0;
    PadState *settingPad = 0;
    PadState *homePad = 0;
    PadState *guestPad = 0;
    unsigned int pressed;
    int classPick;
    int carPick;
    int transPick;
    int settingPick;
    int homePick;
    int guestPick;

    if (st->refresh & (menu->carViews[player]->perfDone != 0)) {
        int cls = st->cls;

        if (cls < 6 && cls >= 4) {
            short stats[5];

            ArcCarView_GetStats(menu->carViews[player], (unsigned short *)stats);
            CarSpecPanel_Show(&D_800F1BE8[player], ArcCarView_GetDrivetrainIcon(menu->carViews[player]), 0,
                              D_80052190, (ArcCarRating *)stats, -1, 0, menu->text);
        } else {
            unsigned short *rgb;
            char *codes;
            unsigned char *spec = g_ArcCarList_Spec3[cls];
            ArcCarRating *ratings = g_ArcCarList_Ratings[cls];
            int count = CarInfoA_GetColors(menu->carViews[player]->carId, &rgb, &codes);
            int car = g_Arc_2PCarousel[player].cur.item;

            CarSpecPanel_Show(&D_800F1BE8[player], ArcCarView_GetDrivetrainIcon(menu->carViews[player]),
                              &menu->carViews[player]->perf, spec + car * 3, ratings + car, count, rgb,
                              menu->text);
        }
        st->refresh = 0;
    }
    if (menu->carViews[player]->loaded != 0) {
        ResultCarView_Spin_OVL2((ResultCarView *)g_Arc_2PCarView[player], &menu->padState[player], menu->frameCount);
    }
    CarSpecPanel_Update(&D_800F1BE8[player]);
    pressed = pad->pressed;
    if (active == 0) {
        pad = 0;
        pressed = 0;
    }
    switch (st->step) {
    case 0:
        classPad = pad;
        break;
    case 1:
        switch (g_ArcSel_Level.garages[player]) {
        case 0:
            homePad = pad;
            break;
        case 1:
            guestPad = pad;
            break;
        }
        break;
    case 2:
    case 5:
        carPad = pad;
        break;
    case 3:
    case 6:
        transPad = pad;
        break;
    case 7:
        settingPad = pad;
        break;
    }
    classPick = MenuList_Update_OVL2(&g_ArcMenu_2PClass[player], classPad);
    carPick = GridPicker_Update(&g_Arc_2PCarousel[player], carPad);
    transPick = Dialog_Update_OVL2(&g_Arc_2PTransPicker[player], transPad);
    settingPick = Dialog_Update_OVL2(&g_Arc_2PSettingPicker[player], settingPad);
    homePick = MenuList_Update(&g_ArcList_2PGarage[player][0].list, homePad);
    guestPick = MenuList_Update(&g_ArcList_2PGarage[player][1].list, guestPad);
    switch (st->step) {
    case 0:
        if (st->timer > 0 && --st->timer == 0) {
            MenuList_Open_OVL2(&g_ArcMenu_2PClass[player]);
        }
        switch (classPick) {
        case -4:
            Sfx_PlayMenu(0);
            break;
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -2:
            break;
        case -1:
            MenuList_Close_OVL2(&g_ArcMenu_2PClass[player]);
            result = 0;
            break;
        case 4:
        case 5:
            Sfx_PlayMenu(1);
            st->cls = classPick;
            MenuList_Close_OVL2(&g_ArcMenu_2PClass[player]);
            st->timer = 0x18;
            st->step = 1;
            g_ArcSel_Level.garages[player] = classPick - 4;
            break;
        default: {
            int car;

            Sfx_PlayMenu(1);
            st->cls = classPick;
            car = ArcCarList_FirstAvailable(classPick);
            MenuList_Close_OVL2(&g_ArcMenu_2PClass[player]);
            *(unsigned int *)D_800F1980[player].rgbTo = g_ArcClassColors[classPick];
            D_800F1980[player].text = D_8004FCCC[classPick];
            TextItem_Show_OVL2(&D_800F1980[player], -1);
            g_Arc_2PCarousel[player].rowCount = 1;
            g_Arc_2PCarousel[player].cur.item = car;
            g_Arc_2PCarousel[player].rowItems[0] = g_ArcCarList_Count[classPick];
            g_Arc_2PCarousel[player].enabled = g_ArcCarList_Avail[classPick];
            Battle2P_ShowCar(menu, player, st, logo, car, 0);
            st->step = 5;
            st->timer = 0xc;
            g_ArcSel_Level.garages[player] = -1;
            break;
        }
        }
        break;
    case 1: {
        int garage = g_ArcSel_Level.garages[player];
        MenuList *list = &g_ArcList_2PGarage[player][garage].list;
        int pick;

        if (st->timer > 0 && --st->timer == 0) {
            MenuList_Open(list);
        }
        pick = homePick;
        if (garage != 0) {
            pick = guestPick;
        }
        switch (pick) {
        case -4:
            Sfx_PlayMenu(0);
            break;
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            MenuList_Close(list);
            st->step = 0;
            st->timer = 0xc;
            break;
        default: {
            int slot = g_Arc_2PGarageOrder[player][pick];
            int cls;

            Sfx_PlayMenu(1);
            MenuList_Close(list);
            cls = (&g_Arc_HomeGarageInfo)[garage].ratings[slot].pwrClass;
            *(unsigned int *)D_800F1980[player].rgbTo = g_ArcClassColors[cls];
            D_800F1980[player].text = D_8004FCCC[cls];
            TextItem_Show_OVL2(&D_800F1980[player], -1);
            g_Arc_2PCarousel[player].rowItems[0] = 1;
            g_Arc_2PCarousel[player].rowCount = 1;
            g_Arc_2PCarousel[player].cur.item = 0;
            g_Arc_2PCarousel[player].enabled = 0;
            ArcLogo_Request(logo, -1, g_GameState.garages[garage].cars[slot].baseCarId);
            ArcCarView_LoadGarageCar(menu->carViews[player], menu, garage, slot);
            ArcCarView_InitSmall((ResultCarView *)g_Arc_2PCarView[player]);
            st->step = 2;
            st->timer = 0x14;
            g_ArcSel_Level.slots[player] = slot;
            g_ArcSel_Level.carIds[player] = g_GameState.garages[garage].cars[slot].baseCarId;
            g_ArcSel_Level.colors[player] = menu->carViews[player]->colorIdx;
            break;
        }
        }
        break;
    }
    case 2:
        if (st->timer > 0 && --st->timer == 0) {
            GridPicker_Open(&g_Arc_2PCarousel[player]);
            CarSpecPanel_Next(&D_800F1BE8[player]);
            st->refresh = 1;
        }
        switch (carPick) {
        case -3:
            Sfx_PlayMenu(7);
            break;
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            TextItem_Hide_OVL2(&D_800F1980[player]);
            GridPicker_Close(&g_Arc_2PCarousel[player]);
            CarSpecPanel_Close(&D_800F1BE8[player]);
            ArcCarView_Unload(menu->carViews[player], menu);
            st->step = 1;
            st->timer = 0xc;
            break;
        default:
            Sfx_PlayMenu(1);
            Dialog_Init_OVL2(&g_Arc_2PTransPicker[player], D_8005006C[player]);
            Dialog_Open_OVL2(&g_Arc_2PTransPicker[player]);
            st->refresh = 0;
            CarSpecPanel_Close(&D_800F1BE8[player]);
            st->step = 3;
            break;
        }
        break;
    case 3:
        switch (transPick) {
        case -3:
            Sfx_PlayMenu(5);
            break;
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            CarSpecPanel_Next(&D_800F1BE8[player]);
            st->refresh = 1;
            Dialog_Close_OVL2(&g_Arc_2PTransPicker[player]);
            st->step = 2;
            st->timer = 0;
            break;
        default:
            if (menu->carViews[player]->loaded == 0) {
                Sfx_PlayMenu(0);
            } else {
                Sfx_PlayMenu(1);
                Dialog_Close_OVL2(&g_Arc_2PTransPicker[player]);
                g_ArcSel_Level.unkB6[player] = g_ArcPickerValues[transPick];
                st->step = 4;
            }
            break;
        }
        break;
    case 4:
        if (pressed & 0x500) {
            Sfx_PlayMenu(2);
            Dialog_Open_OVL2(&g_Arc_2PTransPicker[player]);
            st->step = 3;
        }
        break;
    case 5: {
        int car;

        if (st->timer > 0 && --st->timer == 0) {
            GridPicker_Open(&g_Arc_2PCarousel[player]);
            CarSpecPanel_Next(&D_800F1BE8[player]);
            st->refresh = 1;
        }
        if (st->timer == 0) {
            unsigned int buttons = menu->padState[player].pressed | menu->padState[player].repeat;

            if (active == 0) {
                buttons = 0;
            }
            if (buttons & 1) {
                ArcCarView_CycleColor(menu->carViews[player], -1);
            }
            if (buttons & 2) {
                ArcCarView_CycleColor(menu->carViews[player], 1);
            }
            if (buttons & 3) {
                Sfx_PlayMenu(5);
            }
            CarSpecPanel_SelectColor(&D_800F1BE8[player], menu->carViews[player]->colorIdx);
        }
        car = g_Arc_2PCarousel[player].cur.item;
        switch (carPick) {
        case -3:
            Sfx_PlayMenu(7);
            st->car = car;
            Battle2P_ShowCar(menu, player, st, logo, car, car);
            CarSpecPanel_Next(&D_800F1BE8[player]);
            st->refresh = 1;
            break;
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            TextItem_Hide_OVL2(&D_800F1980[player]);
            GridPicker_Close(&g_Arc_2PCarousel[player]);
            CarSpecPanel_Close(&D_800F1BE8[player]);
            ArcCarView_Unload(menu->carViews[player], menu);
            st->step = 0;
            st->timer = 0xc;
            break;
        default:
            Sfx_PlayMenu(1);
            Dialog_Init_OVL2(&g_Arc_2PTransPicker[player], D_8005006C[player]);
            Dialog_Open_OVL2(&g_Arc_2PTransPicker[player]);
            st->refresh = 0;
            CarSpecPanel_Close(&D_800F1BE8[player]);
            g_ArcSel_Level.carIds[player] = CarCode_Encode((signed char *)ArcCarList_GetCode(st->cls, car));
            g_ArcSel_Level.colors[player] = menu->carViews[player]->colorIdx;
            st->step = 6;
            break;
        }
        break;
    }
    case 6:
        switch (transPick) {
        case -3:
            Sfx_PlayMenu(5);
            break;
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            CarSpecPanel_Next(&D_800F1BE8[player]);
            st->refresh = 1;
            Dialog_Close_OVL2(&g_Arc_2PTransPicker[player]);
            st->step = 5;
            st->timer = 0;
            break;
        default:
            Sfx_PlayMenu(1);
            Dialog_Close_OVL2(&g_Arc_2PTransPicker[player]);
            Dialog_Init_OVL2(&g_Arc_2PSettingPicker[player], D_8005008C[player]);
            Dialog_Open_OVL2(&g_Arc_2PSettingPicker[player]);
            g_ArcSel_Level.unkB6[player] = g_ArcPickerValues[transPick];
            st->step = 7;
            break;
        }
        break;
    case 7:
        switch (settingPick) {
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            Dialog_Open_OVL2(&g_Arc_2PTransPicker[player]);
            Dialog_Close_OVL2(&g_Arc_2PSettingPicker[player]);
            st->step = 6;
            break;
        default:
            if (menu->carViews[player]->loaded == 0) {
                Sfx_PlayMenu(0);
            } else {
                Sfx_PlayMenu(1);
                Dialog_Close_OVL2(&g_Arc_2PSettingPicker[player]);
                g_ArcSel_Level.settings[player] = g_ArcPickerValues[settingPick];
                st->step = 8;
            }
            break;
        }
        break;
    case 8:
        if (pressed & 0x500) {
            Sfx_PlayMenu(2);
            Dialog_Open_OVL2(&g_Arc_2PSettingPicker[player]);
            st->step = 7;
        }
        break;
    }
    return result;
}

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

static inline void TextItem_DrawAt(ArcTextItem *item, int x, int y, void *ot, TextCtx *ctx)
{
    item->x = x;
    item->y = y;
    TextItem_Draw_OVL2(item, ot, ctx);
}

// Both halves of the screen.
// The game copies the text context into another register before the car views.
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
        ArcCarView_Draw(menu->carViews[0], &menu->layers[4], 0, (ResultCarView *)g_Arc_2PCarView[0]);
        ArcCarView_Draw(menu->carViews[1], &menu->layers[5], 0, (ResultCarView *)g_Arc_2PCarView[1]);
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
}
