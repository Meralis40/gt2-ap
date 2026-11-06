// OVL1 8001e808-8001f508 (buycar): The data transfer's car purchase page: buying the
// cars of a GT1 save with the GT2 credits.
#include "common.h"
#include "ap.h"
#include "gamestate.h"

// "%d/%3d cars", after "Cr." in the same strings.
#define s_N3dCars (s_Cr + 4)

// The state and the delay a page keeps after its BaseMenuPage.
#define PAGE_STATE(page) (((int *)((page) + 1))[0])
#define PAGE_DELAY(page) (((int *)((page) + 1))[1])

extern "C" {
extern KanjiFont *g_OVL1_KanjiFont;
extern char s_Available[];
extern char s_Cr[];
extern char s_NotEnoughMoney[];
extern char s_GarageIsFull[];
extern BaseMenu *g_CurrentMenu;
extern WipeBar g_BuyCar_CreditsBar;
extern WipeBar g_BuyCar_CarsBar;
extern MenuList g_BuyCar_List;
extern CardDialog g_BuyCar_Dialog;
extern unsigned char g_BuyCar_DialogDesc[];
extern Font g_OVL1_FontMedium;
extern SpriteDesc g_BuyCar_Sprite;

// The value with a comma every 3 digits (up to 8 digits), like the resident
// Credits_FormatCommas.
void Str_FormatThousands(char *buf, unsigned int v)
{
    char digits[64];
    int div = 10000000;
    int started = 0;
    int n;
    int group;
    int c;

    if (v == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    n = 0;
    for (;;) {
        int d = v / div;
        int nonzero = d != 0;

        if (nonzero | started) {
            digits[n] = d + '0';
            started = 1;
            n++;
            v -= d * div;
        }
        if (div == 1) {
            break;
        }
        div /= 10;
    }
    digits[n] = 0;
    group = n % 3;
    if (group == 0) {
        group = 3;
    }
    *buf = digits[0];
    c = digits[1];
    n = 0;
    goto test;
loop:
    if (--group == 0) {
        *buf++ = ',';
        group = 3;
    }
    n++;
    *buf = digits[n];
    c = (signed char)digits[n + 1];
test:
    buf++;
    if (c != 0) {
        goto loop;
    }
    *buf = 0;
}

// Japanese only: caches the glyphs of the page's status strings.
void BuyCar_CacheKanji(void)
{
    KanjiFont *font = g_OVL1_KanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_Reset(font);
        KanjiFont_CacheString(font, s_Available);
        KanjiFont_CacheString(font, s_Available + 0x14);
        KanjiFont_CacheString(font, s_Available + 0x30);
        KanjiFont_CacheString(font, s_Available + 0x4f);
    }
}

// Japanese only: draws SJIS text from the kanji cache, 12 pixels a character
// (centred on x if center is set).
void BuyCar_DrawKanji(void *ot, char *sjis, int x, int y, int center, unsigned int rgb)
{
    KanjiFont *font = g_OVL1_KanjiFont;
    unsigned char *p;
    unsigned int semi;
    int w = 0;

    if (g_GameState.language != 0) {
        return;
    }
    if (center) {
        unsigned char *s;

        for (s = (unsigned char *)sjis; *s != 0; s += 2) {
            w += 12;
        }
        x -= w >> 1;
    }
    p = (unsigned char *)sjis;
    semi = 0x2000000;
    while (*p != 0) {
        KanjiGlyph *glyph = KanjiFont_FindGlyph(font, p[0] << 8 | p[1]);

        if (glyph != 0) {
            PrimFT4 *q = (PrimFT4 *)KanjiFont_AddGlyphQuad(glyph, (unsigned int *)ot, rgb | semi);

            q->x0 = x;
            q->y0 = y;
            q->x1 = x + 15;
            q->y1 = y;
            q->x2 = x;
            q->y2 = y + 31;
            q->x3 = x + 15;
            q->y3 = y + 31;
            q->tpage = (q->tpage & ~0x60) | 0x20;
        }
        x += 12;
        p += 2;
    }
}

// The cars of the GT1 save (DataXfer_GetCarList).
struct Gt1CarList {
    short count;
    unsigned char pad02[2];
    Car cars[1];
};

// 0 when car index of the GT1 save can be bought, 1 when the garage is full (100
// cars), 2 when the credits don't cover its value.
// The game keeps the garage pointer in a saved register (s2) of a larger frame.
int UsedCar_CanBuy(int index)
{
    GameState *gs = &g_GameState;
    Gt1CarList *list = (Gt1CarList *)DataXfer_GetCarList();
    Garage *garage = gs->garages;
    int r = 1;

    if (Ap_Active() && !Ap_HasManufacturer(list->cars[index].setup.manufacturerId)) {
        return 2;
    }
    if (gs->garages[0].carCount < 100) {
        r = (garage->credits < list->cars[index].value) << r;
    }
    return r;
}

// Buys car index of the GT1 save into garage 0, for its value.
void UsedCar_Buy(int index)
{
    GameState *gs = &g_GameState;
    Garage *garage = gs->garages;
    Car *car = &((Gt1CarList *)DataXfer_GetCarList())->cars[index];

    Garage_AddCar(garage, car);
    garage->credits -= car->value;
}

// Draws row index of the car list: the car's name, its colour, its value and the row's
// frame, at brightness (faded in by fade when positive, out when negative).
// The game swaps the registers of the tpage mask and word, and adds 0x80 to the
// brightness before the fade.
void BuyCar_DrawRow(unsigned int *ot, int index, int x, int y, int brightness, int fade)
{
    Car *car;
    int ty = y + 8;
    int scale;
    GouraudQuad swatch;
    TextCtx ctx;
    char buf[64];
    PrimSprite *p;

    car = &((Gt1CarList *)DataXfer_GetCarList())->cars[index];
    Text_Init(&ctx, 0x1e);
    Text_SetFont(&ctx, &g_OVL1_FontMedium);
    ctx.ot = ot;
    *(uint *)&ctx.clut = (*(uint *)&ctx.clut & 0xff9fffff) | 0x200000;
    scale = brightness;
    if (fade < 0) {
        scale = scale + 0x80 + fade;
    } else {
        scale = scale * fade >> 7;
    }
    if (scale > 0xff) {
        scale = 0xff;
    }
    ctx.rgb = scale / 2 | (scale / 2) << 8 | (scale / 3) << 16 | 0x2000000;
    Text_DrawString(&ctx, (signed char *)CarInfoA_GetName(car->baseCarId), x - 0x6e, ty, 1);
    swatch.rect.x = x - 0x7c;
    swatch.rect.y = y - 8;
    swatch.rect.w = 9;
    swatch.rect.h = 0x10;
    swatch.top = CarInfoA_GetColorRgb(car->baseCarId, car->color);
    Menu_DrawColorSwatch(ot, &swatch, scale);
    Str_FormatThousands(buf, car->value);
    Text_DrawStringMonoDigitsRight(&ctx, (signed char *)buf, x + 0x78, ty, 1, -2, 0);
    p = Prim_AddSprite(ot, scale | scale << 8 | scale << 16);
    *(int *)p = (x - (((short *)&g_BuyCar_Sprite.wh)[0] >> 1)) + ((y - (((short *)&g_BuyCar_Sprite.wh)[1] >> 1)) << 16);
    ((unsigned int *)p)[1] = g_BuyCar_Sprite.uvClut;
    ((unsigned int *)p)[2] = g_BuyCar_Sprite.wh;
    Prim_AddDrawMode(ot, g_BuyCar_Sprite.tpage);
}

// The car list's callback: draws a car's row (BuyCar_DrawRow) and, for the selected
// one, whether it can be bought; a car is enabled when UsedCar_CanBuy gives 0.
// The game allocates the saved registers in another order and reaches the text
// context through a pointer; its jump table is still in the rodata subsegment.
int BuyCar_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index)
{
    int result = 0;
    MenuList *list = ev->list;
    uint *ot = (uint *)ev->ot;
    BaseMenuPage *page = (BaseMenuPage *)list->userData;
    char *text = 0;

    switch (event) {
    case MLE_INIT:
    case MLE_REVEAL:
    case MLE_CLOSE:
    case MLE_UPDATE:
        break;
    case MLE_DRAW: {
        int fade;
        unsigned int rgb;

        if (index < 0) {
            return result;
        }
        fade = (list->fade << 7) / list->fadeMax;
        if (index == list->selected) {
            fade = -MenuList_GetFade(list);
        }
        if (fade == 0) {
            return result;
        }
        {
            int open = list->state >= 0;
            int closing = list->state < -1;

            if (open | closing) {
                BuyCar_DrawRow(ot + 1, index, ev->x, ev->y, ev->brightness, fade);
            }
        }
        if (index != list->selected) {
            return result;
        }
        rgb = 0x606060;
        switch (UsedCar_CanBuy(index)) {
        case 0:
            text = s_Available;
            break;
        case 1:
            text = s_GarageIsFull;
            rgb = 0x142864;
            break;
        case 2:
            text = s_NotEnoughMoney;
            rgb = 0x142864;
            break;
        }
        if (PAGE_STATE(page) == 0) {
            int show = 0;

            if (text != 0) {
                show = list->state >= 0;
            }
            if (show) {
                if (g_GameState.language == 0) {
                    BuyCar_DrawKanji(ot, text, 0xb0, 0x182, 1, rgb);
                } else {
                    TextCtx ctx;

                    Text_Init(&ctx, 0x1e);
                    Text_SetFont(&ctx, &g_OVL1_FontMedium);
                    ctx.ot = ot;
                    ctx.rgb = rgb;
                    *(uint *)&ctx.clut = (*(uint *)&ctx.clut & 0xff9fffff) | 0x200000;
                    Text_DrawCentered(&ctx, (signed char *)text, 0xb0, 0x19a, 1);
                }
                return result;
            }
        }
        break;
    }
    case MLE_LEAVE:
    case MLE_ENTER:
    case MLE_OPEN:
        break;
    case MLE_IS_ENABLED:
        result = 0;
        if (index >= 0) {
            result = UsedCar_CanBuy(index) == 0;
        }
        break;
    }
    return result;
}

// Sets up the list of the GT1 save's cars and the "Buy?" dialog; the list opens after
// 12 frames.
void DataTransferBuyCar_Enter(int back)
{
    Gt1CarList *list = (Gt1CarList *)DataXfer_GetCarList();
    BaseMenuPage *page = g_CurrentMenu->activePage;

    BuyCar_CacheKanji();
    g_BuyCar_CreditsBar.t = -1;
    g_BuyCar_CarsBar.t = -1;
    {
        int count = list->count;

        g_BuyCar_List.count = count;
    }
    MenuList_Init(&g_BuyCar_List, BuyCar_ItemCallback, page);
    CardDialog_Init(&g_BuyCar_Dialog, (CardDialogDesc *)g_BuyCar_DialogDesc, 0);
    PAGE_STATE(page) = 0;
    PAGE_DELAY(page) = 12;
}

// The car list: confirming a car that can be bought opens the "Buy?" dialog, whose
// Yes buys it (UsedCar_Buy). Cancel leaves the page (2).
// The game stores the dialog's choice and the state from one register holding 1.
int DataTransferBuyCar_Update(int arg)
{
    int next = 0;
    BaseMenuPage *page = g_CurrentMenu->activePage;
    PadState *pad = g_CurrentMenu->padState;
    WipeBar *credits;
    WipeBar *cars;
    int r;
    int sfx;

    if (arg == 0) {
        pad = (PadState *)next;
    }
    credits = &g_BuyCar_CreditsBar;
    WipeBar_Update(credits);
    cars = &g_BuyCar_CarsBar;
    WipeBar_Update(cars);
    if (PAGE_DELAY(page) > 0 && --PAGE_DELAY(page) == 0) {
        MenuList_Open(&g_BuyCar_List);
        credits->t = 0;
        cars->t = 0;
    }
    switch (PAGE_STATE(page)) {
    case 0:
        CardDialog_Update(&g_BuyCar_Dialog, 0);
        r = MenuList_Update(&g_BuyCar_List, pad);
        switch (r) {
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -4:
            Sfx_PlayMenu(0);
            break;
        case -2:
            break;
        case -1:
            MenuList_Close(&g_BuyCar_List);
            next = 2;
            g_BuyCar_CreditsBar.t = ~g_BuyCar_CreditsBar.duration;
            g_BuyCar_CarsBar.t = ~g_BuyCar_CarsBar.duration;
            Sfx_PlayMenu(4);
            break;
        default:
            Sfx_PlayMenu(1);
            CardDialog_Open(&g_BuyCar_Dialog);
            g_BuyCar_Dialog.choice = 1;
            PAGE_STATE(page) = 1;
            break;
        }
        break;
    case 1:
        MenuList_Update(&g_BuyCar_List, 0);
        switch (CardDialog_Update(&g_BuyCar_Dialog, pad)) {
        case -1:
            sfx = 2;
            break;
        case 0:
            UsedCar_Buy(g_BuyCar_List.selected);
        case 1:
            sfx = 1;
            break;
        case -3:
        case -2:
        default:
            return next;
        }
        Sfx_PlayMenu(sfx);
        CardDialog_Close(&g_BuyCar_Dialog);
        PAGE_STATE(page) = 0;
        break;
    }
    return next;
}

// Draws the dialog and the list, then, unless the dialog is open, the player's credits
// and "N/100 cars", each over its wipe bar.
// The game keeps 1 in s5 for the text flag, the state test and the spacing.
void DataTransferBuyCar_Draw(DrawLayer *layer)
{
    GameState *gs = &g_GameState;
    char *base = (char *)layer->ot;
    BaseMenuPage *page = g_CurrentMenu->activePage;
    TextCtx ctx;
    TextCtx *t;
    char buf[256];
    uint *ot;
    Garage *garage;

    Text_Init(&ctx, 0x1e);
    Text_SetFont(&ctx, &g_OVL1_FontMedium);
    t = &ctx;
    t->ot = (uint *)(base + 0x10);
    t->unk1c = 1;
    *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
    CardDialog_Draw(&g_BuyCar_Dialog, t->ot, t);
    ot = (uint *)(base + 0x18);
    MenuList_Draw(&g_BuyCar_List, ot);
    garage = gs->garages;
    if (PAGE_STATE(page) != 1) {
        t->ot = ot;
        Str_FormatThousands(buf, garage->credits);
        t->rgb = 0x2604214;
        Text_DrawStringRight(t, (signed char *)s_Cr, 0x40, 0x1b8, 1);
        Text_DrawStringMonoDigitsRight(t, (signed char *)buf, 0x90, 0x1b8, 1, -2, 0);
        WipeBar_Draw(&g_BuyCar_CreditsBar, ot, 0x20, 0x1a4);
        sprintf(buf, s_N3dCars, gs->garages[0].carCount, 100);
        Text_DrawStringMonoDigitsRight(t, (signed char *)buf, 0x136, 0x1b8, 1, -2, 0);
        WipeBar_Draw(&g_BuyCar_CarsBar, ot, 0x146, 0x1a4);
    }
}
}
