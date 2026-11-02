// OVL4 8001b3ac-8001d090 (menudraw): The menu page's elements.
// MenuPage_DrawElements is still the original assembly. Assembled by GNU as
// (GAS_CXX): a branch in GTMenu::BeginFrame lands after the load delay nop.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern MenuCarList g_GarageList;
extern MenuCarList g_UsedCarList;
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
    MenuView_BeginFrame(&view, (void **)layers[1].ot);
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
#ifdef NON_MATCHING
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
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudraw", Car_DrawName);
#endif

// MenuPage_DrawElements's formats.
extern const char s_FmtD_OVL4_80023cfc[] = "%d";
extern const char D_80023D00[] = "%d.%02d";
extern const char s_Dashes_OVL4_80023d08[] = "----";

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudraw", MenuPage_DrawElements);
}
