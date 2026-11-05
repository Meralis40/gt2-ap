// OVL2 8002344c-80023690 (racelaunch): Race launch and guest garage pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern ArcGarageInfo g_Arc_GuestGarageInfo;
extern ArcGuestLoadConfig D_800521A4;
extern ArcBonusCourse g_ArcBonusCourses[];
extern short g_ArcCardSlotStatus[];
extern void *g_ArcKanjiFont;
extern unsigned char D_801C9AF5;



// 24-frame delay.
void RaceLaunch_Enter(void *menu, int back)
{
    *(unsigned int *)(*(int *)((int)menu + 0x1c8) + 0x14) = 0x18;
}

// 3 at the end of the delay: leaves the menu for the race.
int RaceLaunch_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    int result = 0;

    if (page->delay > 0 && --page->delay == 0) {
        result = 3;
    }
    return result;
}

// Empty.
void RaceLaunch_Draw(void *menu, DrawLayer *layer)
{
}

// Starts the load of a guest garage.
void LoadGuest_Enter(ArcadeMenu *menu, int back)
{
    D_800521A4.buffer = ArcadeMenu_GetCarGzBuf();
    D_800521A4.font = g_ArcKanjiFont;
    D_800521A4.slots = g_ArcCardSlotStatus;
    GuestLoad_Setup(&D_800521A4);
    GuestLoad_Start();
}

// 2 when cancelled; rates the guest garage's cars once it's loaded.
int LoadGuest_Update(ArcadeMenu *menu, int active)
{
    int result = 0;
    PadState *pad = &menu->padState[0];

    if (active == 0) {
        pad = 0;
    }
    switch (GuestLoad_Update(pad)) {
    case 3: {
        GameState *state = &g_GameState;

        CarRatingTable_Build(&g_Arc_GuestGarageInfo, &state->garages[1]);
        break;
    }
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    case 0:
        break;
    }
    return result;
}

// GuestLoad_Draw.
void LoadGuest_Draw(void *menu, DrawLayer *layer)
{
    GuestLoad_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// Always 0: LEVEL SELECTION shows 3 levels (the 4-level layout is unused).
int ArcLevel_HasFourthLevel(void)
{
    return 0;
}

// Copy of License_IsEarned: all 10 tests have a medal. A negative license counts as
// earned.
int License_IsEarned_OVL2(int license)
{
    int earned = 1;
    int i;
    GameState *state;
    LicenseTest *test;

    if (license < 0) {
        return earned;
    }
    i = 0;
    state = &g_GameState;
    test = state->licenses[license];
    for (; i < 10; test++, i++) {
        signed char medal = *(signed char *)&test->medal;

        if (medal == 0) {
            earned = 0;
        }
    }
    return earned;
}

// All the bonus courses that have a stat byte have bit 1 or 2 set in it.
int ArcBonus_AllCoursesWon(void)
{
    ArcBonusCourse *course = g_ArcBonusCourses;
    GameState *state = &g_GameState;
    unsigned char *stats = (unsigned char *)&state->stats;

    for (; course->name != 0; course++) {
        if (course->stat >= 0) {
            ArcBonusFlags *flags = (ArcBonusFlags *)&stats[course->stat];

            if ((flags->bit2 | flags->bit1) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

// Stat byte 0x349 is set.
int ArcBonus_IsStat349Set(void)
{
    return D_801C9AF5 != 0;
}
}
