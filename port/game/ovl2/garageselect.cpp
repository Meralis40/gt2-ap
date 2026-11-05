// OVL2 8001f238-80020224 (garageselect): Garage car selection page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"
#include "syslib.h"

// The page's state after its definition.
struct ArcGarageSelectPage {
    MenuPage def;
    int viewDelay; // draws the car view at 0
    int pickerDelay; // opens the transmission picker at 0
    int state; // 0 list, 1 carousel, 2 transmission
    int specPending; // show the car's specs once computed
    int listDelay; // opens the next step at 0
};

// Counts t back down.
static inline void GrowBar_Shrink(GrowBar *bar)
{
    bar->t = ~bar->duration;
}

static inline int Half(unsigned short v)
{
    return (short)v >> 1;
}

extern "C" {
extern GameState g_GameState;
extern ArcadeRaceSel g_ArcSel_Level;
extern signed char g_ArcSel_Class;
extern signed char g_ArcSel_Trans;
extern short g_ArcSel_Garage;
extern unsigned int g_ArcSel_CarCode;
extern int g_Arc_RallyMode;
extern short g_Arc_GarageOrder[100];
extern ArcGarageInfo g_Arc_HomeGarageInfo;
extern ArcGarageInfo g_Arc_GuestGarageInfo;
extern ArcCarSpecPanel g_ArcCarSpecPanel;
extern ArcGridPickerTmpl D_8004FD50;
extern ArcDialogDesc D_8004FD74;
extern unsigned char g_ArcPickerValues[];
extern unsigned char D_80052190[];
extern MenuPage g_ArcPage_Course;
extern Font D_80123810;
extern ArcSprite D_800455D0; // the row's arrow
extern char *D_8004FCB0[];   // the classes' names
extern char D_800F859E[];    // the power's format
extern MenuList g_ArcList_Garage;
extern ArcGridPicker g_Arc_CarCarousel;
extern GrowBar g_Arc_CarGrowBar;
extern ArcDialog g_Arc_TransPicker;
extern ResultCarView g_Arc_CarView;

// A garage car's name, colour swatch, power, class and arrow.
void GarageList_DrawRow(void *ot, Garage *garage, int slot, ArcCarRating *rating, int x, int y,
                        int brightness, int fade, TextCtx *text, int active)
{
    GouraudQuad quad;
    char power[0x40];
    int textY = y + 8;
    int level;
    Car *car;
    int spacing;

    Text_SetFont(text, &D_80123810);
    text->ot = (uint *)ot;
    *(uint *)&text->clut = (*(uint *)&text->clut & 0xff9fffff) | 0x200000;
    level = brightness;
    if (fade < 0) {
        int raised = level + 0x80;

        level = raised + fade;
    } else {
        level = level * fade >> 7;
    }
    if (level > 0xff) {
        level = 0xff;
    }
    if (active == 0) {
        level = level / 2;
    }
    text->rgb = level / 2 | (level / 2) << 8 | (level / 3) << 16 | 0x2000000;
    car = &garage->cars[slot];
    {
        char *name = CarInfoA_GetName(car->baseCarId);

        spacing = 1;
        Text_DrawString(text, (signed char *)name, x - 0x64, textY, spacing);
    }
    quad.rect.x = x - 0x74;
    quad.rect.y = y - 8;
    quad.rect.w = 9;
    quad.rect.h = 0x10;
    quad.top = CarInfoA_GetColorRgb(car->baseCarId, car->color);
    Menu_DrawColorSwatch((uint *)ot, &quad, level);
    sprintf(power, D_800F859E, rating->power * 1000 / 1014);
    {
        int width = Text_MeasureStringMonoDigits(text, (signed char *)power, 1, 0);

        width -= 0x64;
        Text_DrawStringMonoDigits(text, (signed char *)power, x - width, textY, 1, -3, 0);
    }
    Text_DrawCentered(text, (signed char *)D_8004FCB0[rating->pwrClass], x + 0x6e, textY, 1);
    {
        PrimSprite *p = Prim_AddSprite(ot, level | level << 8 | level << 16);

        ArcSprite *sprite;

        *(int *)p = x - Half(D_800455D0.w) + ((y - Half(D_800455D0.h)) << 16);
        sprite = &D_800455D0;
        *(int *)&p->u = *(int *)sprite;
        *(int *)&p->w = *(int *)&sprite->w;
        Prim_AddDrawMode(ot, sprite->tpage);
    }
}

// Draws a row, and on a rally only cars with dirt tyres are enabled.
int GarageList_Callback(int event, MenuListEvent *ctx, int index, Garage *garage,
                        ArcCarRating *rating, int slot)
{
    int result = 0;
    MenuList *list = ctx->list;
    TextCtx *text = ((ArcadeMenu *)list->userData)->text;
    void *ot = (char *)ctx->ot + 4;

    text->ot = (uint *)ot;
    Text_SetFont(text, &D_80123810);
    switch (event) {
    case MLE_INIT:
    case MLE_REVEAL:
    case MLE_CLOSE:
    case MLE_UPDATE:
        break;
    case MLE_DRAW: {
        int state;
        int shown;
        int closing;
        int fade = (list->fade << 7) / list->fadeMax;

        if (index == list->selected) {
            fade = -MenuList_GetFade(list);
        }
        if (fade == 0) {
            return result;
        }
        state = list->state;
        shown = state >= 0;
        closing = state < -1;
        if (shown | closing) {
            GarageList_DrawRow(ot, garage, slot, rating, ctx->x, ctx->y, ctx->brightness, fade,
                               text, *(signed char *)&ctx->enabled != 0);
            return result;
        }
        break;
    }
    case MLE_LEAVE:
    case MLE_ENTER:
    case MLE_OPEN:
        break;
    case MLE_IS_ENABLED:
        result = 1;
        if (g_Arc_RallyMode != 0) {
            result = rating->dirtTires != 0;
        }
        break;
    }
    return result;
}

// GarageList_Callback for the home garage.
int GarageList_HomeCallback(int event, MenuListEvent *ctx, int index)
{
    int slot = g_Arc_GarageOrder[index];
    ArcGarageInfo *info = &g_Arc_HomeGarageInfo;
    GameState *gs = &g_GameState;

    return GarageList_Callback(event, ctx, index, &gs->garages[0], &info->ratings[slot], slot);
}

// GarageList_Callback for the guest garage.
int GarageList_GuestCallback(int event, MenuListEvent *ctx, int index)
{
    int slot = g_Arc_GarageOrder[index];
    ArcGarageInfo *info = &g_Arc_GuestGarageInfo;
    GameState *gs = &g_GameState;

    return GarageList_Callback(event, ctx, index, &gs->garages[1], &info->ratings[slot], slot);
}

// As CarSelect_DrawItem, with the chosen car's logo.
void GarageSelect_DrawItem(ArcGridPickerDraw *ctx, int item)
{
    int selected = ctx->pos.item;
    ArcadeMenu *menu = (ArcadeMenu *)ctx->arg;
    TextCtx &text = *menu->text;
    void *ot = ctx->ot;
    int x = ctx->x;
    int y = ctx->y;
    int brightness = ctx->brightness;
    ArcLogo *logo;

    selected = selected == item;
    logo = &menu->logos[0];
    if (item < 0) {
        selected = 1;
    }
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x, 0x1a4, &text, selected, 0);
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x + 0x58, 0xf0, &text, selected, 1);
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x - 0x60, 0x194, &text, selected, 2);
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x - 0x84, 0x174, &text, selected, 3);
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x + 0xa0, 0x194, &text, selected, 4);
    if (item >= 0) {
        ArcLogo_Draw(logo, ot, g_ArcSel_CarCode, x, y, brightness, 0, -1);
    }
}

// Coming back opens the transmission picker. Otherwise sets up the list of the chosen
// garage, in slot order.
void GarageSelect_Enter(ArcadeMenu *menu, int back)
{
    ArcLogo *logo = &menu->logos[0];
    ArcadeRaceSel *sel = &g_ArcSel_Level;
    GameState *state = &g_GameState;
    ArcGarageSelectPage &page = *(ArcGarageSelectPage *)menu->activePage;
    int index = sel->garage;
    Garage *garage = &state->garages[index];

    if (back != 0) {
        page.viewDelay = 0x18;
        page.pickerDelay = 0x20;
        page.listDelay = 0xc;
        g_Arc_CarGrowBar.t = 0;
        page.state = 2;
        ArcLogo_Reset(logo);
        ArcLogo_Request(logo, -1, sel->carCodes[1]);
    } else {
        int i;
        int count;

        page.viewDelay = 0x18;
        page.pickerDelay = -1;
        page.state = 0;
        page.specPending = 0;
        page.listDelay = 0xc;
        ArcCarView_SetFastDecode(menu->carViews[0]);
        count = garage->carCount;
        g_ArcList_Garage.count = count;
        if (index == 0) {
            MenuList_Init(&g_ArcList_Garage, (MenuListFn *)GarageList_HomeCallback, menu);
        } else {
            MenuList_Init(&g_ArcList_Garage, (MenuListFn *)GarageList_GuestCallback, menu);
        }
        D_8004FD50.draw = GarageSelect_DrawItem;
        D_8004FD50.drawArg = menu;
        GridPicker_Init(&g_Arc_CarCarousel, &D_8004FD50, 0);
        g_Arc_CarCarousel.rowItems[0] = 1;
        g_Arc_CarCarousel.rowCount = 1;
        g_Arc_CarCarousel.cur.item = 0;
        g_Arc_CarGrowBar.t = -1;
        Dialog_Init_OVL2(&g_Arc_TransPicker, &D_8004FD74);
        ArcCarView_InitLarge(&g_Arc_CarView);
        ArcLogo_Reset(logo);
        CarSpecPanel_Init(&g_ArcCarSpecPanel);
        for (i = 99; i >= 0; i--) {
            g_Arc_GarageOrder[i] = i;
        }
    }
}

// The car list, the carousel of its car, then the transmission picker. Returns 2 to go
// back and 1 once the course page is pushed.
int GarageSelect_Update(ArcadeMenu *menu, int active)
{
    int result = 0;
    ArcLogo *logo = &menu->logos[0];
    int garageIndex = g_ArcSel_Garage;
    GameState *state = &g_GameState;
    Garage *garage = &state->garages[garageIndex];
    ArcGarageSelectPage *page = (ArcGarageSelectPage *)menu->activePage;
    PadState *pad = &menu->padState[0];
    ArcCarSpecPanel *panel;
    ArcCarView *view;
    int done;
    int choice;

    if (active == 0) {
        pad = 0;
    }
    view = menu->carViews[0];
    done = view->perfDone != 0;
    if (page->specPending & done) {
        unsigned short stats[8];

        ArcCarView_GetStats(view, stats);
        CarSpecPanel_Show(&g_ArcCarSpecPanel, ArcCarView_GetDrivetrainIcon(menu->carViews[0]), 0,
                          D_80052190, (ArcCarRating *)stats, -1, 0, menu->text);
        page->specPending = 0;
    }
    if (page->pickerDelay > 0) {
        if (--page->pickerDelay == 0) {
            Dialog_Open_OVL2(&g_Arc_TransPicker);
            GridPicker_Open(&g_Arc_CarCarousel);
        }
    }
    GrowBar_Update((unsigned char *)&g_Arc_CarGrowBar);
    if (page->viewDelay > 0) {
        page->viewDelay--;
    }
    if (menu->carViews[0]->loaded != 0) {
        ResultCarView_Spin_OVL2(&g_Arc_CarView, &menu->padState[1], menu->drawStamp);
    }
    panel = &g_ArcCarSpecPanel;
    CarSpecPanel_Update(panel);
    switch (page->state) {
    case 0:
        if (page->listDelay > 0) {
            if (--page->listDelay == 0) {
                MenuList_Open(&g_ArcList_Garage);
            }
        }
        choice = MenuList_Update(&g_ArcList_Garage, pad);
        GridPicker_Update(&g_Arc_CarCarousel, 0);
        Dialog_Update_OVL2(&g_Arc_TransPicker, 0);
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
            g_ArcSel_Class = garageIndex + 4;
            MenuList_Close(&g_ArcList_Garage);
            ArcLogo_Reset(&menu->logos[0]);
            result = 2;
            break;
        default: {
            int slot = g_Arc_GarageOrder[choice];
            Car *car;

            Sfx_PlayMenu(1);
            MenuList_Close(&g_ArcList_Garage);
            car = &garage->cars[slot];
            ArcLogo_Request(logo, -1, car->baseCarId);
            ArcCarView_LoadGarageCar(menu->carViews[0], menu, garageIndex, slot);
            ArcCarView_InitLarge(&g_Arc_CarView);
            g_Arc_CarGrowBar.t = 0;
            page->state = 1;
            page->listDelay = 0x18;
            g_ArcSel_Level.slot = slot;
            g_ArcSel_Level.carCodes[1] = car->baseCarId;
            g_ArcSel_Level.carCodes[0] = car->carId;
            g_ArcSel_Level.color = menu->carViews[0]->colorIdx;
            g_ArcSel_Level.cls = (&g_Arc_HomeGarageInfo)[garageIndex].ratings[slot].pwrClass;
            break;
        }
        }
        break;
    case 1:
        if (page->listDelay > 0) {
            if (--page->listDelay == 0) {
                GridPicker_Open(&g_Arc_CarCarousel);
                CarSpecPanel_Next(panel);
                page->specPending = 1;
            }
        }
        MenuList_Update(&g_ArcList_Garage, 0);
        choice = GridPicker_Update(&g_Arc_CarCarousel, pad);
        Dialog_Update_OVL2(&g_Arc_TransPicker, 0);
        switch (choice) {
        case -2:
            break;
        case -3:
        case -1:
            Sfx_PlayMenu(2);
            GridPicker_Close(&g_Arc_CarCarousel);
            ArcCarView_Unload(menu->carViews[0], menu);
            page->specPending = 0;
            CarSpecPanel_Close(&g_ArcCarSpecPanel);
            GrowBar_Shrink(&g_Arc_CarGrowBar);
            page->state = 0;
            page->listDelay = 0xc;
            break;
        default:
            Sfx_PlayMenu(1);
            page->specPending = 0;
            CarSpecPanel_Close(&g_ArcCarSpecPanel);
            Dialog_Init_OVL2(&g_Arc_TransPicker, &D_8004FD74);
            g_Arc_TransPicker.x = 0xb0;
            g_Arc_TransPicker.y = 0x1a4;
            Dialog_Open_OVL2(&g_Arc_TransPicker);
            page->state = 2;
            break;
        }
        break;
    case 2:
        MenuList_Update(&g_ArcList_Garage, 0);
        GridPicker_Update(&g_Arc_CarCarousel, 0);
        choice = Dialog_Update_OVL2(&g_Arc_TransPicker, pad);
        switch (choice) {
        case -2:
            break;
        case -3:
            Sfx_PlayMenu(5);
            break;
        case -1:
            Sfx_PlayMenu(2);
            Dialog_Close_OVL2(&g_Arc_TransPicker);
            CarSpecPanel_Next(panel);
            page->specPending = 1;
            page->state = 1;
            page->listDelay = 0;
            break;
        default:
            if (menu->carViews[0]->loaded != 0) {
                Sfx_PlayMenu(3);
                GridPicker_Close(&g_Arc_CarCarousel);
                GrowBar_Shrink(&g_Arc_CarGrowBar);
                result = 1;
                Dialog_Close_OVL2(&g_Arc_TransPicker);
                ArcadeMenu_PushPage(menu, &g_ArcPage_Course);
                ArcLogo_Reset(&menu->logos[0]);
                page->viewDelay = -1;
                g_ArcSel_Trans = g_ArcPickerValues[choice];
            } else {
                Sfx_PlayMenu(0);
            }
            break;
        }
        break;
    }
    return result;
}

// Car list, carousel, grow bar, picker and the 3D car view.
void GarageSelect_Draw(void *menu, DrawLayer *layer)
{
    unsigned int *ot;
    void *ot_00;
    TextCtx *ctx;
    int iVar1;
    RECT local_20;

    ot_00 = layer->ot;
    iVar1 = *(int *)((int)menu + 0x1c8);
    ctx = *(TextCtx **)((int)menu + 0x1c4);
    ot = (unsigned int *)((int)ot_00 + 8);
    MenuList_Draw(&g_ArcList_Garage, ot);
    GridPicker_Draw(&g_Arc_CarCarousel, ot);
    GrowBar_Draw((GrowBar *)&g_Arc_CarGrowBar, ot);
    Dialog_Draw_OVL2(&g_Arc_TransPicker, ot_00, ctx);
    if (*(int *)(iVar1 + 0x14) == 0) {
        ArcCarView_Draw(*(ArcCarView **)((int)menu + 0x228), (DrawLayer *)((int)menu + 0xbc),
                                        (DrawLayer *)((int)menu + 0xcc), &g_Arc_CarView);
    }
    local_20.x = 0x30;
    local_20.y = 0xb4;
    local_20.w = 0x100;
    local_20.h = 200;
    DrawLayer_SetArea((DrawLayer *)((int)menu + 0xbc), &local_20);
    DrawLayer_SetArea((DrawLayer *)((int)menu + 0xcc), &local_20);
}

// Camera of the 2P (and garage) car views: distance 0xb0000, smaller viewport, ground
// disc off (colour 0x404040).
void ArcCarView_InitSmall(ResultCarView *view)
{
    view->drawDisc = '\x01';
    view->rotX = 0x5e;
    view->rotY = 0x1500;
    view->pos[2] = 0xb0000;
    view->viewport[2] = 0xd2;
    view->viewport[3] = 0x90;
    view->frustum[0] = -0x69;
    view->frustum[1] = 0x69;
    view->frustum[2] = 0x43;
    view->frustum[3] = -0x11;
    view->frustum[4] = 400;
    view->discSemi = '\0';
    view->discRgb = 0x404040;
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
}
}
