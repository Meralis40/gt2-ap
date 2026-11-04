// OVL2 8001e53c-8001f238 (carselect): Car selection page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "syslib.h"

// The page's state after its definition.
struct ArcCarSelectPage {
    MenuPage def;
    int delay; // opens the carousel at 0
    int state; // 0 carousel, 1 transmission, 2 setting
    int car;
    int specPending; // show the car's specs once computed
    int viewDelay; // draws the car view at 0
    int pickerDelay; // opens the setting picker at 0
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
extern signed char g_ArcSel_Class;
extern signed char g_ArcSel_Trans;
extern signed char g_ArcSel_Setting;
extern ArcadeRaceSel g_ArcSel_Level;
extern ArcCarSpecPanel g_ArcCarSpecPanel;
extern ArcSprite D_8004F7C4[]; // the makers
extern char D_80027224[];      // the two cars without a maker sprite
extern char D_8002723C[];
extern ArcGridPickerTmpl D_8004FD50;
extern ArcDialogDesc D_8004FD74;
extern ArcDialogDesc D_8004FD8C;
extern signed char *g_ArcCarList_Avail[];
extern short g_ArcCarList_Count[];
extern unsigned char *g_ArcCarList_Spec3[];
extern ArcCarRating *g_ArcCarList_Ratings[];
extern unsigned char g_ArcPickerValues[];
extern MenuPage g_ArcPage_Course;
extern ArcGridPicker g_Arc_CarCarousel;
extern GrowBar g_Arc_CarGrowBar;
extern ArcDialog g_Arc_TransPicker;
extern ArcDialog g_Arc_SettingPicker;
extern ResultCarView g_Arc_CarView;

// Same as ResultCarView_Init (OVL0) with the 1P car selection camera: distance
// 0x94ccc, pitch 0x5e, yaw 0x1500, ground disc on (colour 0xa2a2a2).
void ArcCarView_InitLarge(ResultCarView *view)
{
    view->drawDisc = '\x01';
    view->discSemi = '\x01';
    view->rotX = 0x5e;
    view->rotY = 0x1500;
    view->viewport[2] = 0x100;
    view->viewport[3] = 0xf0;
    view->frustum[0] = -0x80;
    view->frustum[1] = 0x80;
    view->frustum[2] = 0x5a;
    view->frustum[3] = -0x32;
    view->frustum[4] = 400;
    view->discRgb = 0xa2a2a2;
    view->rotZ = 0;
    view->unk_b4 = 0;
    view->unk_b8 = 0;
    view->unk_bc = 0;
    view->pos[1] = 0;
    view->pos[0] = 0;
    view->pos[2] = 0x94ccc;
    view->viewport[0] = 0;
    view->viewport[1] = 0;
    view->frustum[5] = 0x7fff;
    view->unk00 = 0;
}

// Copy of ResultCarView_Spin: turns the car 0x10 per frame; after frame 0xfb90 also
// pulls the camera back.
void ResultCarView_Spin_OVL2(ResultCarView *view, PadState *pad, int frame)
{
    view->rotY = view->rotY + 0x10U & 0x3fff;
    if (0xfb90 < frame) {
        view->pos[2] = view->pos[2] + 0x8000;
    }
}

// The spec panel's parts around the car, then its logo. Two cars have their logo at the
// bottom and no maker sprite.
void CarSelect_DrawItem(ArcGridPickerDraw *ctx, int item)
{
    int cls = g_ArcSel_Class;
    int car = ctx->pos.item;
    ArcadeMenu *menu = (ArcadeMenu *)ctx->arg;
    ArcLogo *logo = &menu->logos[0];
    TextCtx *text = menu->text;
    void *ot = ctx->ot;
    int x = ctx->x;
    int y = ctx->y;
    int brightness = ctx->brightness;
    int selected;
    ArcSprite *sprite = &D_8004F7C4[ArcCarList_GetLogo(cls, car)];
    int bottom;

    selected = car == item;
    if (item < 0) {
        selected = 1;
    }
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x, 0x1a4, text, selected, 0);
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x + 0x58, 0xf0, text, selected, 1);
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x - 0x60, 0x194, text, selected, 2);
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x - 0x84, 0x174, text, selected, 3);
    CarSpecPanel_Draw(&g_ArcCarSpecPanel, ot, x + 0xa0, 0x194, text, selected, 4);
    if (item >= 0) {
        bottom = strcmp(D_80027224, ArcCarList_GetCode(cls, car)) == 0;
        if (strcmp(D_8002723C, ArcCarList_GetCode(cls, car)) == 0) {
            bottom = 1;
        }
        if (!bottom) {
            PrimSprite *p = Prim_AddSprite(ot, brightness | brightness << 8 | brightness << 16);
            int xy = x - Half(sprite->w);
            int halfH = Half(sprite->h);
            int top = y - halfH;

            halfH += 2;
            top -= halfH;
            xy += top << 16;
            *(int *)p = xy;
            *(int *)&p->u = *(int *)sprite;
            *(int *)&p->w = *(int *)&sprite->w;
            Prim_AddDrawMode(ot, sprite->tpage);
        }
        ArcLogo_Draw(logo, ot, CarCode_Encode((signed char *)ArcCarList_GetCode(cls, car)), x, y,
                     brightness, bottom, -1);
    }
}

// Coming back keeps the carousel and reloads the logo, the pickers opening after a delay.
void CarSelect_Enter(ArcadeMenu *menu, int back)
{
    ArcLogo &logo = menu->logos[0];
    ArcCarSelectPage &page = *(ArcCarSelectPage *)menu->activePage;
    int cls = g_ArcSel_Class;

    if (back != 0) {
        ArcGridPicker *carousel;
        int color;

        page.viewDelay = 0x18;
        page.pickerDelay = 0x20;
        g_Arc_CarGrowBar.t = 0;
        page.state = 2;
        ArcLogo_Reset(&logo);
        carousel = &g_Arc_CarCarousel;
        color = ArcCarList_GetColor(cls, carousel->cur.item);
        ArcLogo_Request(&logo, color,
                        CarCode_Encode((signed char *)ArcCarList_GetCode(cls, carousel->cur.item)));
    } else {
        int car;
        int color;
        int count;

        page.delay = 0x18;
        page.viewDelay = 0x18;
        page.pickerDelay = -1;
        page.state = 0;
        car = ArcCarList_FirstAvailable(cls);
        page.car = car;
        ArcCarView_SetFastDecode(menu->carViews[0]);
        D_8004FD50.draw = CarSelect_DrawItem;
        D_8004FD50.drawArg = menu;
        GridPicker_Init(&g_Arc_CarCarousel, &D_8004FD50, g_ArcCarList_Avail[cls]);
        count = g_ArcCarList_Count[cls];
        g_Arc_CarCarousel.rowCount = 1;
        g_Arc_CarCarousel.cur.item = car;
        g_Arc_CarGrowBar.t = 0;
        g_Arc_CarCarousel.rowItems[0] = count;
        Dialog_Init_OVL2(&g_Arc_TransPicker, &D_8004FD74);
        Dialog_Init_OVL2(&g_Arc_SettingPicker, &D_8004FD8C);
        ArcCarView_LoadCarCode(menu->carViews[0], menu, ArcCarList_GetCode(cls, car), 0);
        ArcCarView_InitLarge(&g_Arc_CarView);
        ArcLogo_Reset(&logo);
        color = ArcCarList_GetColor(cls, car);
        ArcLogo_Request(&logo, color, CarCode_Encode((signed char *)ArcCarList_GetCode(cls, car)));
        CarSpecPanel_Init(&g_ArcCarSpecPanel);
        CarSpecPanel_Next(&g_ArcCarSpecPanel);
        page.specPending = 1;
    }
}

#ifdef NON_MATCHING
// The carousel, with buttons 1 and 2 cycling the colour, then the transmission and setting
// pickers. Returns 2 to go back and 1 once the course page is pushed.
int CarSelect_Update(ArcadeMenu *menu, int active)
{
    ArcLogo *logo = &menu->logos[0];
    int result = 0;
    ArcCarSelectPage *page = (ArcCarSelectPage *)menu->activePage;
    TextCtx *text = menu->text;
    int cls = g_ArcSel_Class;
    unsigned char *spec3 = g_ArcCarList_Spec3[cls];
    ArcCarRating *ratings = g_ArcCarList_Ratings[cls];
    PadState *pad;
    ArcCarView *view;
    int done;
    ArcCarSpecPanel *panel;
    int choice;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            GridPicker_Open(&g_Arc_CarCarousel);
        }
    }
    if (page->pickerDelay > 0) {
        if (--page->pickerDelay == 0) {
            Dialog_Open_OVL2(&g_Arc_SettingPicker);
            GridPicker_Open(&g_Arc_CarCarousel);
        }
    }
    if (page->viewDelay > 0) {
        page->viewDelay--;
    }
    GrowBar_Update((unsigned char *)&g_Arc_CarGrowBar);
    view = menu->carViews[0];
    done = view->perfDone != 0;
    if (page->specPending & done) {
        unsigned short *rgb;
        char *codes;
        int count = CarInfoA_GetColors(view->carId, &rgb, &codes);
        int icon = ArcCarView_GetDrivetrainIcon(menu->carViews[0]);
        int index = g_Arc_CarCarousel.cur.item;

        CarSpecPanel_Show(&g_ArcCarSpecPanel, icon, &menu->carViews[0]->perf, spec3 + index * 3,
                          ratings + index, count, rgb, text);
        page->specPending = 0;
    }
    if (menu->carViews[0]->loaded != 0) {
        ResultCarView_Spin_OVL2(&g_Arc_CarView, &menu->padState[1], menu->drawStamp);
    }
    panel = &g_ArcCarSpecPanel;
    CarSpecPanel_Update(panel);
    switch (page->state) {
    case 0: {
        unsigned int buttons;

        pad = &menu->padState[0];
        buttons = pad->pressed | pad->repeat;
        if (active == 0) {
            buttons = 0;
        }
        if (buttons & 1) {
            ArcCarView_CycleColor(menu->carViews[0], -1);
        }
        if (buttons & 2) {
            ArcCarView_CycleColor(menu->carViews[0], 1);
        }
        if (buttons & 3) {
            Sfx_PlayMenu(5);
        }
        CarSpecPanel_SelectColor(panel, menu->carViews[0]->colorIdx);
        Dialog_Update_OVL2(&g_Arc_TransPicker, 0);
        Dialog_Update_OVL2(&g_Arc_SettingPicker, 0);
        if (active == 0) {
            pad = 0;
        }
        choice = GridPicker_Update(&g_Arc_CarCarousel, pad);
        {
            int car = g_Arc_CarCarousel.cur.item;

            switch (choice) {
            case -2:
                break;
            case -3: {
                int color;

                Sfx_PlayMenu(7);
                page->car = car;
                color = ArcCarList_GetColor(cls, car);
                ArcLogo_Request(logo, color,
                                CarCode_Encode((signed char *)ArcCarList_GetCode(cls, car)));
                ArcCarView_LoadCarCode(menu->carViews[0], menu, ArcCarList_GetCode(cls, car), car);
                ArcCarView_InitLarge(&g_Arc_CarView);
                CarSpecPanel_Next(panel);
                page->specPending = 1;
                break;
            }
            case -1:
                Sfx_PlayMenu(4);
                GridPicker_Close(&g_Arc_CarCarousel);
                GrowBar_Shrink(&g_Arc_CarGrowBar);
                result = 2;
                CarSpecPanel_Close(panel);
                ArcLogo_Reset(logo);
                ArcCarView_Unload(menu->carViews[0], menu);
                break;
            default: {
                unsigned int code;
                int color;

                Sfx_PlayMenu(1);
                Dialog_Init_OVL2(&g_Arc_TransPicker, &D_8004FD74);
                g_Arc_TransPicker.x = 0xb0;
                g_Arc_TransPicker.y = 0x1a4;
                Dialog_Open_OVL2(&g_Arc_TransPicker);
                page->specPending = 0;
                CarSpecPanel_Close(&g_ArcCarSpecPanel);
                page->state = 1;
                code = CarCode_Encode((signed char *)ArcCarList_GetCode(cls, car));
                g_ArcSel_Level.carCodes[1] = code;
                g_ArcSel_Level.carCodes[0] = code;
                color = menu->carViews[0]->colorIdx;
                g_ArcSel_Level.color = color;
                break;
            }
            }
        }
        break;
    }
    case 1:
        pad = &menu->padState[0];
        if (active == 0) {
            pad = 0;
        }
        GridPicker_Update(&g_Arc_CarCarousel, 0);
        Dialog_Update_OVL2(&g_Arc_SettingPicker, 0);
        choice = Dialog_Update_OVL2(&g_Arc_TransPicker, pad);
        switch (choice) {
        case -2:
            break;
        case -3:
            Sfx_PlayMenu(5);
            break;
        case -1:
            Sfx_PlayMenu(2);
            CarSpecPanel_Next(panel);
            page->specPending = 1;
            Dialog_Close_OVL2(&g_Arc_TransPicker);
            page->state = 0;
            break;
        default:
            Sfx_PlayMenu(1);
            Dialog_Init_OVL2(&g_Arc_SettingPicker, &D_8004FD8C);
            g_Arc_SettingPicker.x = 0xb0;
            g_Arc_SettingPicker.y = 0x1a4;
            Dialog_Open_OVL2(&g_Arc_SettingPicker);
            Dialog_Close_OVL2(&g_Arc_TransPicker);
            page->state = 2;
            g_ArcSel_Trans = g_ArcPickerValues[choice];
            break;
        }
        break;
    case 2:
        pad = &menu->padState[0];
        if (active == 0) {
            pad = 0;
        }
        GridPicker_Update(&g_Arc_CarCarousel, 0);
        Dialog_Update_OVL2(&g_Arc_TransPicker, 0);
        choice = Dialog_Update_OVL2(&g_Arc_SettingPicker, pad);
        switch (choice) {
        case -2:
            break;
        case -3:
            Sfx_PlayMenu(5);
            break;
        case -1:
            Sfx_PlayMenu(2);
            Dialog_Open_OVL2(&g_Arc_TransPicker);
            Dialog_Close_OVL2(&g_Arc_SettingPicker);
            page->state = 1;
            break;
        default:
            if (menu->carViews[0]->loaded != 0) {
                Sfx_PlayMenu(3);
                GridPicker_Close(&g_Arc_CarCarousel);
                GrowBar_Shrink(&g_Arc_CarGrowBar);
                result = 1;
                Dialog_Close_OVL2(&g_Arc_SettingPicker);
                ArcLogo_Reset(logo);
                page->viewDelay = -1;
                ArcadeMenu_PushPage(menu, &g_ArcPage_Course);
                g_ArcSel_Setting = choice;
            } else {
                Sfx_PlayMenu(0);
            }
            break;
        }
        break;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carselect", CarSelect_Update);
#endif

// Carousel, grow bar, pickers and the 3D car view.
void CarSelect_Draw(void *menu, DrawLayer *layer)
{
    void *ot;
    TextCtx *ctx;
    int iVar1;
    RECT local_20;

    ot = layer->ot;
    iVar1 = *(int *)((int)menu + 0x1c8);
    ctx = *(TextCtx **)((int)menu + 0x1c4);
    GridPicker_Draw(&g_Arc_CarCarousel, (unsigned int *)((int)ot + 8));
    GrowBar_Draw((GrowBar *)&g_Arc_CarGrowBar, (unsigned int *)((int)ot + 8));
    Dialog_Draw_OVL2(&g_Arc_TransPicker, ot, ctx);
    Dialog_Draw_OVL2(&g_Arc_SettingPicker, ot, ctx);
    if (*(int *)(iVar1 + 0x24) == 0) {
        ArcCarView_Draw(*(ArcCarView **)((int)menu + 0x228), (DrawLayer *)((int)menu + 0xbc),
                                        (DrawLayer *)((int)menu + 0xcc), (unsigned char *)&g_Arc_CarView);
    }
    local_20.x = 0x30;
    local_20.y = 0xb4;
    local_20.w = 0x100;
    local_20.h = 200;
    DrawLayer_SetArea((DrawLayer *)((int)menu + 0xbc), &local_20);
    DrawLayer_SetArea((DrawLayer *)((int)menu + 0xcc), &local_20);
}
}
