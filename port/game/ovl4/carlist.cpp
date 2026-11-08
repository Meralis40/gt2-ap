// OVL4 80020198-80020c50 (carlist): The garage and used car list widgets, two
// MenuLists whose rows show a car's maker and name, colour swatch and the mark
// (garage) or the price (used cars).
#include "common.h"

extern "C" {
extern GameState (&g_GameState);
extern MenuList (&g_GarageList_Menu);
extern short (&g_GarageList_Blink);
extern unsigned char (&g_GarageList_Black)[];
extern unsigned char (&g_GarageList_TextRgb)[];
extern unsigned char (&g_GarageList_OutlineRgb)[];
extern unsigned char (&g_GarageList_CurrentRgb)[];
extern CarSpecStrings (&g_CarSpecStrings);
extern MenuList (&g_UsedCarList_Menu);
extern short (&g_UsedCarList_Blink);
extern unsigned char (&g_UsedCarList_Black)[];
extern unsigned char (&g_UsedCarList_TextRgb)[];
extern unsigned char (&g_UsedCarList_PriceRgb)[];
extern unsigned char (&g_UsedCarList_OutlineRgb)[];
extern game_pointer<UsedCarEntry> (&g_UsedCarListCars);

// MenuList callback. MLE_DRAW draws garage car index: the blinking highlight box
// when selected, the mark, maker and name, the colour swatch, the current car
// marker and the outline. MLE_ENTER restarts the blink.
// Registers: the game keeps the garage address (&g_GameState.garages[0]) in a
// temporary for each use and the text x (x - 0x98) in $s0 across the mark's
// call, where GCC keeps the address in $s3 and computes x + (w - 0x98) after the
// call. Tried: the car as &cars[index], cars + index, from a Garage pointer (in
// the case, in a block, through an inline function) or a GameState pointer; the
// text x as a local set before the call (GCC then computes it before the car,
// which costs a register and spills t), with += or a second local.
int GarageList_DrawItem(MenuListEventId event, MenuListEvent *ev, int index)
{
    int y = ev->y;
    MenuList *list = ev->list;
    int t = ev->brightness;
    void *ot = ev->ot;
    int x = ev->x;

    y += 4;
    switch (event) {
    case MLE_DRAW: {
        GouraudQuad box;
        RECT outline;
        Car *car;
        unsigned int carId;
        unsigned short *mark;
        unsigned short *maker;
        unsigned short *model;
        unsigned int rgb;
        int tx;
        int w;

        if ((index == list->selected) & (g_GarageList_Blink >= 0)) {
            box.rect.x = x;
            box.rect.y = y - 14;
            box.rect.w = 0x180;
            box.rect.h = 0x14;
            box.top = 0x141414;
            box.bottom = 0x505050;
            Menu_DrawCursorBox((uint *)ot, &box, (g_GarageList_Blink << 7) / 60);
            Prim_AddDrawMode(ot, 0x220);
        }
        car = g_GameState.garages[0].cars + index;
        carId = car->carId;
        mark = GarageCar_GetMark(car->flags98 >> 15);
        maker = Car_GetMakerName(carId);
        model = Car_GetModelName(carId);
        rgb = Rgb_Lerp(g_GarageList_Black, g_GarageList_TextRgb, t, 0x80);
        w = GTFont_DrawString(ot, mark, x - 0x98, y, rgb);
        tx = x - 0x98 + w;
        w = GTFont_DrawString(ot, maker, tx, y, rgb);
        GTFont_DrawString(ot, model, tx + w + 5, y, rgb);
        box.rect.x = x - 0xa5;
        box.rect.y = y - 9;
        box.rect.w = 9;
        box.rect.h = 0xc;
        box.top = CarInfoA_GetColorRgb(car->baseCarId, car->color);
        Menu_DrawColorSwatch((uint *)ot, &box, t);
        if (g_GameState.garages[0].currentCar == index) {
            RECT *r = Prim_AddTile(ot, Rgb_Lerp(g_GarageList_Black, g_GarageList_CurrentRgb, t, 0x80));

            r->x = x - 0xb8;
            r->y = y - 8;
            r->w = 0xe;
            r->h = 8;
        }
        outline.x = x - 0xc0;
        outline.w = 0x180;
        outline.y = y - 14;
        outline.h = 0x14;
        Prim_AddRectOutline(ot, Rgb_Lerp(g_GarageList_Black, g_GarageList_OutlineRgb, t, 0x80), &outline);
        break;
    }
    case MLE_ENTER:
        g_GarageList_Blink = 0;
        break;
    }
    return 1;
}

// Inits the garage list widget, empty.
void GarageList_Init(MenuCarList *list, int arg)
{
    MenuList_Init(&g_GarageList_Menu, GarageList_DrawItem, 0);
    list->count = 0;
    g_GarageList_Blink = -1;
}

// Sets the position of the garage car list widget (0x80052958 +0x10 / +0x12); list is
// unused.
void CarList_SetPosition(void *list, short x, short y)
{
    g_GarageList_Menu.x = x;
    g_GarageList_Menu.y = y;
}

// Opens the garage list on element with one row per garage car, clamping the
// selection.
void GarageList_Open(MenuCarList *list, MenuElement *element)
{
    int count = g_GameState.garages[0].carCount;

    list->element = element;
    if (count > 0) {
        g_GarageList_Menu.count = count;
        MenuList_Open(&g_GarageList_Menu);
        if (g_GarageList_Menu.selected >= count) {
            g_GarageList_Menu.selected = count - 1;
        }
    }
    list->count = count;
}

// Runs the list (the highlight blinks while active). Returns the confirmed row,
// -2 on cancel, else -1. START moves the selected car to the top of the garage.
int GarageList_Update(MenuCarList *list, PadState *pad, int active)
{
    int ret = -1;

    if (active) {
        if (++g_GarageList_Blink > 60) {
            g_GarageList_Blink = 0;
        }
    } else {
        g_GarageList_Blink = -1;
    }
    if (list->count > 0) {
        int r = MenuList_Update(&g_GarageList_Menu, pad);

        switch (r) {
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -4:
            break;
        case -2:
            // GTMenu::Update also runs the list without a pad: the PlayStation reads address 6.
            if (pad != 0 && (pad->pressed & PAD_START)) {
                GameState *state = &g_GameState;

                Garage_MoveCar(&state->garages[0], g_GarageList_Menu.selected, 0);
                Sfx_PlayMenu(1);
            }
            break;
        case -1:
            ret = -2;
            break;
        default:
            list->selected = r;
            ret = r;
            break;
        }
    }
    return ret;
}

// Same as UsedCarList_DrawOrEmpty for the list widget at 0x80052958.
void CarList_DrawOrEmpty(short *list, void *layer)
{
    if (*list < 1) {
        GTText_Draw(layer, g_CarSpecStrings.emptyList, g_GarageList_Menu.x + -0x98, g_GarageList_Menu.y + 0x40, 0xf0780a);
    }
    else {
        MenuList_Draw(&g_GarageList_Menu, layer);
    }
}

// MenuList callback. MLE_DRAW draws used car index: the blinking highlight box
// when selected, the maker and name, the colour swatch, the price and the
// outline. MLE_ENTER restarts the blink.
int UsedCarList_DrawItem(MenuListEventId event, MenuListEvent *ev, int index)
{
    int y = ev->y;
    MenuList *list = ev->list;
    int t = ev->brightness;
    void *ot = ev->ot;
    int x = ev->x;

    y += 4;
    switch (event) {
    case MLE_DRAW: {
        GouraudQuad box;
        UsedCarEntry *e;
        unsigned int carId;
        unsigned int price;
        int color;
        unsigned short *maker;
        unsigned short *model;
        unsigned int rgb;
        int tx;
        int w;

        if ((index == list->selected) & (g_UsedCarList_Blink >= 0)) {
            box.rect.x = x;
            box.rect.y = y - 16;
            box.rect.w = 0x1a0;
            box.rect.h = 0x18;
            box.top = 0x141414;
            box.bottom = 0x505050;
            Menu_DrawCursorBox((uint *)ot, &box, (g_UsedCarList_Blink << 7) / 60);
            Prim_AddDrawMode(ot, 0x220);
        }
        e = g_UsedCarListCars + index;
        carId = e->carId;
        price = e->price;
        color = (signed char)e->colorCode;
        maker = Car_GetMakerName(carId);
        model = Car_GetModelName(carId);
        rgb = Rgb_Lerp(g_UsedCarList_Black, g_UsedCarList_TextRgb, t, 0x80);
        tx = x - 0xbc;
        w = GTFont_DrawString(ot, maker, tx, y, rgb);
        GTFont_DrawString(ot, model, tx + w + 5, y, rgb);
        box.rect.x = x - 0xc9;
        box.rect.y = y - 9;
        box.rect.w = 9;
        box.rect.h = 0xc;
        box.top = CarInfoA_GetColorRgb(carId, color);
        Menu_DrawColorSwatch((uint *)ot, &box, t);
        rgb = Rgb_Lerp(g_UsedCarList_Black, g_UsedCarList_PriceRgb, t, 0x80);
        {
            unsigned short text[64];

            FormatPrice(text, price);
            int pw = GTText_WidthW(text) - 200;

            GTFont_DrawString(ot, text, x - pw, y, rgb);
        }
        {
            RECT outline;

            outline.x = x - 0xd0;
            outline.w = 0x1a0;
            outline.y = y - 16;
            outline.h = 0x18;
            Prim_AddRectOutline(ot, Rgb_Lerp(g_UsedCarList_Black, g_UsedCarList_OutlineRgb, t, 0x80), &outline);
        }
        break;
    }
    case MLE_ENTER:
        g_UsedCarList_Blink = 0;
        break;
    }
    return 1;
}

// Inits the used car list widget, empty.
void UsedCarList_Init(MenuCarList *list, int arg)
{
    MenuList_Init(&g_UsedCarList_Menu, UsedCarList_DrawItem, 0);
    list->count = 0;
    g_UsedCarList_Blink = -1;
}

// Sets the position of the used / new car list widget (0x8005299c +0x10 / +0x12); list
// is unused.
void UsedCarList_SetPosition(void *list, short x, short y)
{
    g_UsedCarList_Menu.x = x;
    g_UsedCarList_Menu.y = y;
}

// Fills the used car list on element with the current period's used cars of
// makerId, clamping the selection.
void UsedCarList_Build(MenuCarList *list, MenuElement *element, int makerId)
{
    int count;

    list->element = element;
    UsedCarList_GetCars(makerId, (unsigned int *)&count, &g_UsedCarListCars);
    if (count > 0) {
        int n = count;

        g_UsedCarList_Menu.count = n;
        MenuList_Open(&g_UsedCarList_Menu);
        if (g_UsedCarList_Menu.selected >= count) {
            n = count;
            g_UsedCarList_Menu.selected = n - 1;
        }
    }
    list->count = count;
}

// Runs the list (the highlight blinks while active). Returns the confirmed row,
// -2 on cancel, else -1.
int UsedCarList_Update(MenuCarList *list, PadState *pad, int active)
{
    int ret = -1;

    if (active) {
        if (++g_UsedCarList_Blink > 60) {
            g_UsedCarList_Blink = 0;
        }
    } else {
        g_UsedCarList_Blink = -1;
    }
    if (list->count > 0) {
        int r = MenuList_Update(&g_UsedCarList_Menu, pad);

        switch (r) {
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -4:
        case -2:
            break;
        case -1:
            ret = -2;
            break;
        default:
            list->selected = r;
            ret = r;
            break;
        }
    }
    return ret;
}

// Draws the used car list, or g_CarSpecStrings.emptyList when it has no cars.
void UsedCarList_DrawOrEmpty(short *list, void *layer)
{
    if (*list < 1) {
        GTText_Draw(layer, g_CarSpecStrings.emptyList, g_UsedCarList_Menu.x + -0xbc, g_UsedCarList_Menu.y + 0x40, 0xf0780a);
    }
    else {
        MenuList_Draw(&g_UsedCarList_Menu, layer);
    }
}

unsigned int UsedCarList_GetSelectedCarId(void)
{
    return g_UsedCarListCars[g_UsedCarList_Menu.selected].carId;
}

int UsedCarList_GetSelectedColor(void)
{
    return (signed char)g_UsedCarListCars[g_UsedCarList_Menu.selected].colorCode;
}

unsigned int UsedCarList_GetSelectedPrice(void)
{
    return g_UsedCarListCars[g_UsedCarList_Menu.selected].price;
}
}
