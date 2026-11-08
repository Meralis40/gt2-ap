// OVL4 8001b3ac-8001d090 (menudraw): The menu page's elements.
// MenuPage_DrawElements is still the original assembly, with its C++ under
// NON_MATCHING. Assembled by GNU as (GAS_CXX): a branch in GTMenu::BeginFrame
// lands after the load delay nop.
#include "gptr.h"
#include "common.h"

extern "C" {
extern MenuCarList (&g_GarageList);
extern MenuCarList (&g_UsedCarList);
extern int (&g_MenuElementCount);
extern unsigned int (&g_MenuPageAttr);
extern GameState (&g_GameState);
extern ShopTxn (&g_ShopTxn);
extern CarSpecStrings (&g_CarSpecStrings);
extern SpriteDesc (&g_GTIcon_Drivetrain)[];
extern SpriteDesc (&g_GTIcon_Medal)[];
extern SpriteDesc (&g_GTIcon_LicenseBadge)[];
extern SpriteDesc (&g_GTIcon_RequiredLicense)[];
extern SpriteDesc (&g_GTIcon_LicenseEarned);
extern SpriteDesc (&g_GTIcon_EventWon);
extern game_pointer<char> (&g_DrivetrainNames)[];
extern char (&sB_Nlb)[];
extern char (&sB_Nhp)[];
}

// Runs the hook, then what Update queued for this frame: the wheel on the
// preview car, the page (with the cursor on its garage or used car list when
// pendingPageArg asks and the list isn't empty, then the queued car at once)
// and the car. Anything shown blanks the next 4 frames.
void GTMenu::BeginFrame()
{
    int shown;

    beginFrameTime = Timer_SinceFrameStart();
    psxViewLoop::BeginFrame();
    shown = 0;
    if (hookActive) {
        hookActive = hook(hookArg, 1, shown);
    }
    MenuView_BeginFrame(&view, (unsigned int *)layers[1].ot);
    if (previewDelay == 1) {
        CarPreview_SetWheel(&view.preview, previewWheel, previewWheelColor);
        shown = 1;
    }
    if (pendingPageDelay == 1) {
        MenuElement *garage;
        MenuElement *used;
        int nonEmpty;
        int onList;

        MenuPage_Enter(&view, pendingPage, pendingPageArg);
        garage = MenuView_FindGarageList(&view);
        used = MenuView_FindUsedCarList(&view);
        // The game computes each list's count test apart and ands it with
        // pendingPageArg; in one expression GCC branches on the count.
        nonEmpty = g_GarageList.count > 0;
        onList = garage ? pendingPageArg & nonEmpty : 0;
        if (onList) {
            listMode = 1;
            MapCursor_MoveTo(&cursor, garage);
        } else {
            nonEmpty = g_UsedCarList.count > 0;
            onList = used ? pendingPageArg & nonEmpty : 0;
            if (onList) {
                listMode = 2;
                MapCursor_MoveTo(&cursor, used);
            } else {
                MapCursor_ResetToDefault(&cursor);
            }
        }
        pendingPageDelay = 0;
        shown = 1;
        if (pendingCarDelay > 0) {
            MenuView_ShowCar(&view, pendingCarBaseId, pendingCarId, pendingCarColor, pendingCarSlot,
                             pendingCarHasWheel != 0, pendingCarWheel, pendingCarDirt);
        }
        pendingCarDelay = 0;
    }
    if (pendingPageDelay == 0) {
        // A switch, for the game's registers (as ifs, they swap).
        switch (pendingCarDelay) {
        case 0:
            break;
        case 1:
            MenuView_ShowCar(&view, pendingCarBaseId, pendingCarId, pendingCarColor, pendingCarSlot,
                             pendingCarHasWheel != 0, pendingCarWheel, pendingCarDirt);
            shown = 1;
            pendingCarDelay = 0;
            break;
        default:
            pendingCarDelay--;
            break;
        }
    }
    if (shown) {
        blankFrames = 4;
    }
    if (blankFrames > 0) {
        draw.otHead = 0;
    }
    updateStartTime = Timer_SinceFrameStart();
}

extern "C" {

// Sets the menu's hook, run with arg until it returns 0; returns 0 (and sets
// nothing) while one is running.
int func_8001B610(GTMenu *menu, unsigned char (*hook)(void *, int, int), void *arg)
{
    if (menu->hookActive) {
        return 0;
    }
    menu->hookActive = 1;
    menu->hook = hook;
    menu->hookArg = arg;
    return 1;
}

// Ends the menu's hook, calling it once more with 2.
void func_8001B63C(GTMenu *menu)
{
    if (menu->hookActive) {
        menu->hook(menu->hookArg, 0, 2);
        menu->hookActive = 0;
    }
}

// 0 for a license-gated element (flag bit 18, no kind) whose license isn't
// earned: the IA license (licenseReq 0) or the S license (1).
int MenuElement_IsLicenseOk(MenuElement *e)
{
    int ok = 1;

    if ((e->flags & 0xffff) == 0 && (e->flags >> 18 & 1)) {
        ok = 0;
        switch (e->licenseReq) {
        case 0:
            ok = License_IsEarned(LICENSE_IA);
            break;
        case 1:
            ok = License_IsEarned(LICENSE_S);
            break;
        }
    }
    return ok;
}

// Whether an element can be selected: a race entry (flag bit 27) unless one of
// its hidden or inactive bits (19 to 23) is set; kind 0 when not license-gated
// (bit 18) or its license is earned; kind 157 with at least two tuning slots;
// and the kinds the table lists.
int MenuElement_IsActive(MenuElement *e)
{
    int active = 0;
    unsigned int flags = e->flags;

    if (flags >> 27 & 1) {
        if (flags >> 19 & 1) {
            return active;
        }
        if (flags >> 23 & 1) {
            return 0;
        }
        if (flags >> 22 & 1) {
            return 0;
        }
        if (flags >> 21 & 1) {
            return 0;
        }
        return (flags >> 20 & 1) ^ 1;
    }
    switch (flags & 0xffff) {
    case 157:
        if (CarWork_CountSlots() < 2) {
            break;
        }
        return 1;
    case 0:
        if (!(e->flags >> 18 & 1)) {
            return 1;
        }
        switch (e->licenseReq) {
        case 0:
            active = License_IsEarned(LICENSE_IA);
            break;
        case 1:
            active = License_IsEarned(LICENSE_S);
            break;
        default:
            return active;
        }
        break;
    case 1 ... 6:
    case 8 ... 9:
    case 11 ... 13:
    case 16:
    case 18 ... 70:
    case 80:
    case 149 ... 150:
    case 170 ... 171:
    case 187:
        active = 1;
        break;
    // Listed, as the game does: without them GCC compares instead of using
    // the jump table.
    case 7:
    case 10:
    case 14 ... 15:
    case 17:
    case 71 ... 79:
    case 81 ... 148:
    case 151 ... 156:
    case 158 ... 169:
    case 172 ... 186:
        break;
    }
    return active;
}

// Draws the maker and (with model) the model name of carId at x, y, with the
// garage mark first (with mark); right-aligned at x with right.
// Doesn't match: the game saves $s5 and copies y to it after loading rgb
// from the stack, GCC before.
void Car_DrawName(void *ot, unsigned int carId, int x, int y, int numFont, int right, int mark, int model,
                  unsigned int rgb)
{
    unsigned short *maker = Car_GetMakerName(carId);
    unsigned short *name = Car_GetModelName(carId);

    if (right) {
        if (model) {
            int w = GTFont_DrawWRight(ot, name, x, y, rgb, numFont) + 8;

            x -= w;
        }
        x -= GTFont_DrawWRight(ot, maker, x, y, rgb, numFont);
        if (mark) {
            GTFont_DrawWRight(ot, GarageCar_GetMark(1), x, y, rgb, numFont);
        }
    } else {
        if (mark) {
            x += GTFont_DrawW(ot, GarageCar_GetMark(1), x, y, rgb, numFont);
        }
        x += GTFont_DrawW(ot, maker, x, y, rgb, numFont) + 8;
        if (model) {
            GTFont_DrawW(ot, name, x, y, rgb, numFont);
        }
    }
}

// MenuPage_DrawElements's C++ still differs in 112 of its 1465 instructions
// (docs/decomp.md). The default build uses the game's assembly, whose .s file
// has the jump table and the padding after it.
// A garage's car, with the index's offset added to the base, as the game does.
static inline Car *Garage_Car(Garage *garage, int i)
{
    return (Car *)rt::from_guest((i * (int)sizeof(Car) + (int)rt::to_guest(garage->cars)));
}

// A coordinate stored through an int, which the game loads with lh where a
// short copied straight is narrowed to lhu.
static inline int Coord(int v)
{
    return v;
}

// Draws icon centred in e's rectangle, moved left by dx and up by dy. A macro
// on the function's prim and icon: as variables of the whole function, the
// sprite gets the game's $a2 and the icon its $s1.
#define MenuElement_DrawIcon(ot, e, icon, dx, dy)                  \
    do {                                                           \
        prim = (unsigned int *)Prim_AddSprite(ot, 0x808080);       \
        {                                                          \
            int x = (e->x0 + e->x1) >> 1;                          \
            int y = (e->y0 + e->y1) >> 1;                          \
            int hw;                                                \
            int hh;                                                \
                                                                   \
            hw = (((short *)&icon->wh)[0] >> 1) + dx;              \
            hh = (((short *)&icon->wh)[1] >> 1) + dy;              \
            x -= hw;                                               \
            y -= hh;                                               \
            prim[0] = x + (y << 16);                               \
        }                                                          \
        prim[1] = icon->uvClut;                                    \
        prim[2] = icon->wh;                                        \
        Prim_AddDrawMode(ot, icon->tpage);                         \
    } while (0)

// The medal won in a license test (signed, lb).
static inline int LicenseTest_Medal(LicenseTest *tests, int i)
{
    return (signed char)tests[i].medal;
}

// Draws what each element of the page shows besides the page image: values,
// icons and the car model. Elements with flag bit 27 are race entries (their
// event code at target): bit 19 the best finish (a medal for 1st to 3rd), bit
// 23 the won mark (else the place), bit 22 the prize for prizePlace, bit 21
// the power limit, bit 20 the required license; kind 207 the championship
// prize. Flag bit 31 picks the number font. Then, on a page with attribute
// bit 9, a black tile over the screen. The game reaches the save, each
// license's tests and g_ShopTxn through locals, whose addresses it rebuilds
// at each use.
void MenuPage_DrawElements(GTMenuView *view, void *ot, int frameParity, void *otBack, DrawLayer *modelLayer)
{
    unsigned short wtext[256];
    char text[256];
    RECT area;
    int prizeHi;
    unsigned int prizeLo;
    int valueHi;
    unsigned int valueLo;
    int i = 0;
    CarPreview *preview = &view->preview;
    SpriteDesc *icon;
    unsigned int *prim;
    int medal;
    GameState *state = &g_GameState;
    Garage *garage = &g_GameState.garages[0];
    PlayerStats *stats = &g_GameState.stats;
    ShopTxn *txn = &g_ShopTxn;
    LicenseTest *testsS = g_GameState.licenses[LICENSE_S];
    LicenseTest *testsIA = g_GameState.licenses[LICENSE_IA];
    LicenseTest *testsIB = g_GameState.licenses[LICENSE_IB];
    LicenseTest *testsIC = g_GameState.licenses[LICENSE_IC];
    LicenseTest *testsA = g_GameState.licenses[LICENSE_A];
    LicenseTest *testsB = g_GameState.licenses[LICENSE_B];

    for (; i < g_MenuElementCount; i++) {
        MenuElement *e = MenuPage_GetElement(i);
        unsigned int flags = e->flags;
        unsigned int kind = flags & 0xffff;
        int numFont = (flags & -0x80000000LL) != 0;

        if (flags >> 27 & 1) {
            int event = Event_IndexFromCode((char *)&e->target);
            int result;

            if (kind == 207) {
                FormatPrice(wtext, EventRules_GetChampionshipPrize(event));
                GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            }
            if ((e->flags >> 19 & 1) && (result = PlayerStats_GetEventResult(&state->stats, event)) != 0) {
                if (result >= 0 ? result < 4 : 0) {
                    icon = &g_GTIcon_Medal[4 - result];
                    MenuElement_DrawIcon(ot, e, icon, 0, 0);
                } else {
                    sprintf(text, "%d", result);
                    GTFont_Draw(ot, text, ((e->x0 + e->x1) >> 1) - 4, ((e->y0 + e->y1) >> 1) + 4, 0x6e6e6e,
                                numFont);
                }
            }
            if ((e->flags >> 23 & 1) && (result = PlayerStats_GetEventResult(&state->stats, event)) != 0) {
                if (result == 1) {
                    icon = &g_GTIcon_EventWon;
                    MenuElement_DrawIcon(ot, e, icon, 2, -6);
                } else {
                    sprintf(text, "%d", result);
                    GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
                }
            }
            if (e->flags >> 22 & 1) {
                int place = e->prizePlace - 1;

                if (place >= 0) {
                    FormatPrice(wtext, EventRules_GetPrize(event, place));
                    GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
                }
            }
            if (e->flags >> 21 & 1) {
                int limit = EventRules_GetPowerLimit(event);

                strcpy(text, g_CarSpecStrings.noLimit);
                if (limit > 0) {
                    sprintf(text, g_CarSpecStrings.powerApprox, limit * 1000 / 1014);
                }
                GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            }
            if (e->flags >> 20 & 1) {
                int license = (int)EventRules_GetRequiredLicense(event) + 1;

                if (license >= 0) {
                    icon = &g_GTIcon_RequiredLicense[license];
                    MenuElement_DrawIcon(ot, e, icon, 0, 0);
                }
            }
        }

        switch (kind) {
        case 0:
            if (e->flags >> 18 & 1) {
                int earned = 0;

                switch (e->licenseReq) {
                case 0:
                    earned = License_IsEarned(LICENSE_IA);
                    break;
                case 1:
                    earned = License_IsEarned(LICENSE_S);
                    break;
                }
                if (earned) {
                    icon = &g_GTIcon_LicenseEarned;
                    MenuElement_DrawIcon(ot, e, icon, 0, 0);
                }
            }
            break;
        case 188: {
            int v = Events_GetWinPercent();

            sprintf(text, "%d.%02d", v / 100, v % 100);
            GTFont_DrawRight(ot, text, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        }
        case 189:
            PlayerStats_GetPrizeMoney(&prizeHi, &prizeLo);
            FormatPriceHiLo(wtext, prizeHi, prizeLo);
            GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 190:
            sprintf(text, "%d", PlayerStats_GetRacesEntered());
            GTFont_DrawRight(ot, text, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 191:
            sprintf(text, "%d", PlayerStats_GetRacesWon());
            GTFont_DrawRight(ot, text, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 192: {
            int v = PlayerStats_GetWinRate();

            sprintf(text, "%d.%02d", v / 100, v % 100);
            GTFont_DrawRight(ot, text, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        }
        case 193: {
            int v = PlayerStats_GetRatio50();

            sprintf(text, "%d.%02d", v / 100, v % 100);
            GTFont_DrawRight(ot, text, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        }
        case 194:
            sprintf(text, "%d", Garage_GetCarCount());
            GTFont_DrawRight(ot, text, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 195:
            Garage_GetTotalValue(&valueHi, &valueLo);
            FormatPriceHiLo(wtext, valueHi, valueLo);
            GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 196:
            if (state->machineTests[2].count > 0) {
                unsigned int carId = state->machineTests[2].entries[0].carId;
                int x = e->x1;

                CarSpec_FormatSpeed(wtext, state->machineTests[2].entries[0].result);
                x -= GTFont_DrawWRight(ot, wtext, x, e->y1, 0x6e6e6e, numFont) + 12;
                Car_DrawName(ot, carId, x, e->y1, numFont, 1, 0, 1, 0x6e6e6e);
            }
            break;
        case 197: {
            int x = e->x1;
            int slot = Garage_FindMostPowerfulCar();
            int w;

            if (slot >= 0) {
                Car *car = Garage_Car(garage, slot);
                unsigned int carId = car->carId;

                FormatHpW(wtext, (car->flags98 & 0x3fff) * 1000 / 1014);
                w = GTFont_DrawWRight(ot, wtext, x, e->y1, 0x6e6e6e, numFont) + 12;
                Car_DrawName(ot, carId, x - w, e->y1, numFont, 1, car->flags98 >> 15, 1, 0x6e6e6e);
            }
            break;
        }
        case 198:
            if (state->machineTests[0].count > 0) {
                unsigned int carId = state->machineTests[0].entries[0].carId;
                int x = e->x1;

                Time_FormatMs(text, state->machineTests[0].entries[0].result);
                x -= GTFont_DrawRight(ot, text, x, e->y1, 0x6e6e6e, numFont) + 12;
                Car_DrawName(ot, carId, x, e->y1, numFont, 1, 0, 1, 0x6e6e6e);
            }
            break;
        case 200:
        case 201:
        case 202:
        case 203:
            switch (kind) {
            case 200:
                kind = MEDAL_GOLD;
                break;
            case 201:
                kind = MEDAL_SILVER;
                break;
            case 202:
                kind = MEDAL_BRONZE;
                break;
            case 203:
                kind = MEDAL_KIDS;
                break;
            }
            sprintf(text, "%d", License_CountMedals(kind));
            GTFont_DrawRight(ot, text, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 204: {
            LicenseId license = License_GetHighest();

            if ((int)license < 6) {
                icon = &g_GTIcon_LicenseBadge[license];
                MenuElement_DrawIcon(ot, e, icon, 0, 0);
            }
            break;
        }
        case 157:
            if (CarWork_CountSlots() < 2) {
                RECT *tile = Prim_AddTile(ot, 0);
                tile->x = Coord(e->x0);
                tile->y = Coord(e->y0);
                tile->w = Coord(e->x1 - e->x0);
                tile->h = Coord(e->y1 - e->y0);
            }
            break;
        case 9:
            CarList_SetPosition(&g_GarageList, 0x100, 0xb0);
            CarList_DrawOrEmpty(&g_GarageList.count, ot);
            break;
        case 80:
            UsedCarList_SetPosition(&g_UsedCarList, 0x100, 0x9c);
            UsedCarList_DrawOrEmpty(&g_UsedCarList.count, ot);
            break;
        case 79:
            FormatDayW(wtext, stats->day);
            GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 77:
            FormatPrice(wtext, garage->credits);
            GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 208:
            FormatPrice(wtext, (unsigned int)preview->spec.price >> 2);
            GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 205:
            Tune_DrawInstalledParts(ot, 0x20, 0xa0);
            break;
        case 178:
            sprintf(text, g_CarSpecStrings.dimension, preview->spec.length);
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        case 179:
            sprintf(text, g_CarSpecStrings.dimension, preview->spec.width);
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        case 180:
            sprintf(text, g_CarSpecStrings.dimension, preview->spec.height);
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        case 181:
            sprintf(text, sB_Nlb, preview->spec.weight * 22046 / 10000);
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        case 182: {
            int cc = preview->spec.displacement;

            if (cc <= 0) {
                sprintf(text, "----");
            } else if ((cc & 0xe000) >> 13) {
                sprintf(text, g_CarSpecStrings.displacementRotary, cc & ~0xe000, (cc & 0xe000) >> 13);
            } else {
                sprintf(text, g_CarSpecStrings.displacement, cc);
            }
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        }
        case 183:
            GTFont_Draw(ot, g_DrivetrainNames[preview->spec.drivetrain], e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        case 184: {
            int w = GTFont_DrawW(ot, CarPreview_GetEngineTextA(preview), e->x0, e->y1, 0x6e6e6e, numFont);

            GTFont_DrawW(ot, CarPreview_GetEngineTextC(preview), e->x0 + w + 12, e->y1, 0x6e6e6e, numFont);
            break;
        }
        case 185: {
            int power = preview->spec.power;
            int rpm = preview->spec.powerRpm;

            if (power <= 0) {
                sprintf(text, "----");
            } else if (rpm <= 0) {
                sprintf(text, g_CarSpecStrings.power, power * 1000 / 1014, rpm);
            } else {
                sprintf(text, g_CarSpecStrings.powerAtRpm, power * 1000 / 1014, rpm);
            }
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        }
        case 186: {
            CarPreview *car = &view->preview;
            int torque = car->spec.torque;

            if (torque <= 0) {
                sprintf(text, "----");
                GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            } else {
                int w;

                torque = torque * 72329 / 10000;
                sprintf(text, g_CarSpecStrings.torque, torque / 10, torque % 10);
                w = GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
                GTFont_DrawW(ot, CarPreview_GetTorqueRpmText(car), e->x0 + w, e->y1, 0x6e6e6e, numFont);
            }
            break;
        }
        case 154: {
            int year = (short)preview->spec.modelYear;

            if (year <= 0) {
                sprintf(text, "----");
            } else {
                sprintf(text, "'%d", year);
            }
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        }
        case 151:
            icon = &g_GTIcon_Drivetrain[preview->spec.drivetrain];
            MenuElement_DrawIcon(ot, e, icon, 0, 0);
            break;
        case 76:
            sprintf(text, sB_Nlb, preview->spec.weight * 22046 / 10000);
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        case 74:
            sprintf(text, sB_Nhp, preview->spec.power * 1000 / 1014);
            GTFont_Draw(ot, text, e->x0, e->y1, 0x6e6e6e, numFont);
            break;
        case 71: {
            int slot = garage->currentCar;
            int x = e->x0;

            if (slot >= 0) {
                Car *car = Garage_Car(garage, slot);
                unsigned int carId = car->carId;

                Car_DrawName(ot, carId, x, e->y1, numFont, 0, car->flags98 >> 15, 0, 0x6e6e6e);
            }
            break;
        }
        case 73:
            if (txn->powerWithPart < 0) {
                GTFont_Draw(ot, "----", e->x0, e->y1, 0x145a78, numFont);
            } else {
                FormatHpW(wtext, txn->powerWithPart * 1000 / 1014);
                GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x145a78, numFont);
            }
            break;
        case 72:
            FormatHpW(wtext, txn->powerNow * 1000 / 1014);
            GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x145a78, numFont);
            break;
        case 155:
            if (txn->type == 3 || txn->type == 4) {
                unsigned short *name = PartName_GetW(txn->part);
                int x = (e->x0 + e->x1) >> 1;

                GTFont_DrawW(ot, name, x - (GTText_WidthW(name) >> 1), e->y1, 0x646464, numFont);
            }
            break;
        case 6:
            if ((e->flags >> 29 ^ 1) & 1) {
                FormatPrice(wtext, Car_GetNewPrice(e->arg10));
                GTFont_DrawWRight(ot, wtext, e->x1 - 0x50, e->y1 + 0x14, 0x2808080, numFont);
            }
            break;
        case 78:
            if (g_ShopTxn.type == 1) {
                FormatPrice(wtext, txn->price);
                GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            } else if (g_ShopTxn.type == 5) {
                if (txn->price > 0) {
                    FormatPrice(wtext, txn->price);
                    GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
                }
            } else if (txn->partOwned) {
                GTFont_Draw(ot, g_CarSpecStrings.owned, e->x0, e->y1, 0xf05028, numFont);
            } else if (txn->price >= 0) {
                FormatPrice(wtext, txn->price);
                GTFont_DrawWRight(ot, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            } else {
                GTFont_Draw(ot, g_CarSpecStrings.emptyList, e->x0, e->y1, 0x2850f0, numFont);
            }
            break;
        case 17:
            CarPreview_DrawLogo(&view->preview, otBack, (e->x0 + e->x1) >> 1, (e->y0 + e->y1) >> 1);
            break;
        case 148:
            CarPreview_DrawColorChips(&view->preview, otBack, e->x1, e->y0);
            break;
        case 7:
            CarPreview_GetColorNameW(&view->preview, wtext);
            GTFont_DrawWRight(otBack, wtext, e->x1, e->y1, 0x6e6e6e, numFont);
            break;
        case 10: {
            CarCamera *cam = &view->camera;
            int w = e->x1 - e->x0;
            int h = e->y1 - e->y0;
            int half;

            cam->screen[0] = 0;
            cam->screen[1] = 0;
            cam->screen[2] = w;
            cam->screen[3] = h;
            half = ((e->x1 - e->x0) << 8) / (e->y1 - e->y0) >> 1;
            cam->frustum[2] = 0xb3;
            cam->frustum[3] = -50;
            cam->fieldCC = 800;
            cam->fieldCE = 0x7fff;
            cam->frustum[0] = -half;
            cam->frustum[1] = half;
            CarPreview_DrawModel(&view->preview, modelLayer, cam);
            area.x = Coord(e->x0);
            area.w = Coord(e->x1 - e->x0);
            area.y = Coord(e->y0);
            area.h = Coord(e->y1 - e->y0);
            DrawLayer_SetArea(modelLayer, &area);
            break;
        }
        case 94 ... 103:
            medal = LicenseTest_Medal(testsB, kind - 94) - 1;
            if (medal >= 0) {
                icon = &g_GTIcon_Medal[medal];
                MenuElement_DrawIcon(ot, e, icon, 0, 0);
            }
            break;
        case 105 ... 114:
            medal = LicenseTest_Medal(testsA, kind - 105) - 1;
            if (medal >= 0) {
                icon = &g_GTIcon_Medal[medal];
                MenuElement_DrawIcon(ot, e, icon, 0, 0);
            }
            break;
        case 83 ... 92:
            medal = LicenseTest_Medal(testsIC, kind - 83) - 1;
            if (medal >= 0) {
                icon = &g_GTIcon_Medal[medal];
                MenuElement_DrawIcon(ot, e, icon, 0, 0);
            }
            break;
        case 116 ... 125:
            medal = LicenseTest_Medal(testsIB, kind - 116) - 1;
            if (medal >= 0) {
                icon = &g_GTIcon_Medal[medal];
                MenuElement_DrawIcon(ot, e, icon, 0, 0);
            }
            break;
        case 127 ... 136:
            medal = LicenseTest_Medal(testsIA, kind - 127) - 1;
            if (medal >= 0) {
                icon = &g_GTIcon_Medal[medal];
                MenuElement_DrawIcon(ot, e, icon, 0, 0);
            }
            break;
        case 138 ... 147:
            medal = LicenseTest_Medal(testsS, kind - 138) - 1;
            if (medal >= 0) {
                icon = &g_GTIcon_Medal[medal];
                MenuElement_DrawIcon(ot, e, icon, 0, 0);
            }
            break;
        }
    }
    if (view->loaded && (g_MenuPageAttr >> 9 & 1)) {
        RECT *tile = Prim_AddTile((char *)ot + 0xc, 0);

        tile->w = 0x200;
        tile->y = 0;
        tile->x = 0;
        tile->h = 0x1f8;
    }
}

// The game pads the file's .rodata to 8 bytes after the jump table.
extern const int D_80024058 = 0;
}
