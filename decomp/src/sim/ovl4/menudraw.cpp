// OVL4 8001b3ac-8001d090 (menudraw): The menu page's elements.
// GTMenu::BeginFrame and MenuPage_DrawElements are still the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudraw", GTMenu__BeginFrame);

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
