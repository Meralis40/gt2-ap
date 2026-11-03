// OVL0 80016b78-80017098 (arcaderace): the ArcadeRaceLoop screen loop and
// ArcadeRace_Run. Its vtable and type name (.rodata), its __tf function and
// typeinfo (.bss, which build.py's CXX_NAMES places) are emitted here.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

extern "C" {
extern unsigned char g_Race_IsDemo;     // the race's memory (raceloops.cpp's RaceMem)
extern void *g_EventDataBuf;
extern signed char g_Flow_HomeOverlay;
extern signed char g_Flow_GameMode;
extern QuickMenuPage g_Page_ResultRoot;         // the post-race menu
extern QuickMenuPage g_Page_TimeTrialRoot;
extern QuickMenuPage g_Page_LicenseRoot;        // the license test results
extern QuickMenuPage g_Page_RaceResultsRoot;

}

// Offsets in the race's memory from g_Race_IsDemo.
#define RACE_EVENT_BUF 0xc0394          // the license data (GameData_LoadLicense)
#define RACE_LICENSE_INFO 0xca394       // the license info file
#define RACE_ASSETS_LOADED 0x11d701     // g_Race_AssetsLoaded

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// The counts of race records (g_GameState + 0xc5a8) and ghosts (+ 0x10ac0).
static inline short *RaceRecords_Count(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xc5a8;

    return (short *)(base + offset);
}

static inline short *GhostData_Count(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0x10ac0;

    return (short *)(base + offset);
}

// Runs a screen loop until it returns 0 or 1.
#define RUN_LOOP(loop)                  \
    do {                                \
        do {                            \
            ret = (loop).Execute();     \
        } while (ret > 1);              \
    } while (ret < 0)

// Enters a car in the race: its grid slot and driver type.
static inline void RaceCar_SetDriver(RaceCar *car, int active, int gridSlot, int driverType)
{
    car->active = active;
    car->gridSlot = gridSlot;
    car->driverType = driverType;
}

ArcadeRaceLoop::ArcadeRaceLoop(int arg) : RaceMenuLoop(&raceView, arg)
{
}

ArcadeRaceLoop::~ArcadeRaceLoop()
{
}

// Loading: a license test (race type 3) loads its info and data.
int ArcadeRaceLoop::State4()
{
    char *race;

    ghostKept = 0;
    if (RaceSetup_Get()->raceType == 3) {
        race = (char *)&g_Race_IsDemo;
        VolId_ReadFile(VOLID_ARCADE_LICENSE_INFO_JP, (void *)((int)(race + RACE_LICENSE_INFO + 3) & ~3));
        g_EventDataBuf = GameData_LoadLicense((void *)((int)(race + RACE_EVENT_BUF + 3) & ~3), 0x9ff0);
    }
    return 5;
}

// Before the race: once a time trial kept a ghost, car 1 races it and the
// assets reload.
int ArcadeRaceLoop::State8()
{
    RaceCar *car;
    char *race;

    firstEntry = 1;
    if (ghostKept) {
        car = &RaceSetup_Get()->cars[1];
        RaceCar_SetDriver(car, 1, 0, 2);
        race = (char *)&g_Race_IsDemo;
        race[RACE_ASSETS_LOADED] = 0;
        ghostKept = 0;
    }
    return 9;
}

// After the race: the replay (11).
int ArcadeRaceLoop::State10()
{
    return 11;
}

// The menus: the loading screen, then the pages of the race type (the license
// results, the race results and the post-race menu, the time trial), then the
// loading screen back. Leaves (7) when the game goes home; else the menu's
// choice: retry (5), the replay (11) or quit (7).
#ifdef NON_MATCHING
// The game's code but for one register copy of the menu choice before its switch.
int ArcadeRaceLoop::State6()
{
    int records;
    int count;
    int ret;

    raceView.unk2e7 = 0;
    menuIO.mode = 0;
    RUN_LOOP(menuIO.base);
    if (g_Flow_HomeOverlay == 0 || g_Flow_HomeOverlay == 2) {
        return 7;
    }
    count = *RaceRecords_Count();
    records = count > 0;
    switch (RaceSetup_Get()->raceType) {
    case 0:
        records = ((count > 0) | (*GhostData_Count() > 0)) != 0;
    case 4:
        if (firstEntry) {
            if (records) {
                QuickMenu_SetRootPage(&menu, &g_Page_RaceResultsRoot.page);
                RUN_LOOP(menu.base);
            }
            firstEntry = 0;
        }
        QuickMenu_SetRootPage(&menu, &g_Page_ResultRoot.page);
        RUN_LOOP(menu.base);
        break;
    case 6:
        TimeTrial_SetFreshResult(firstEntry != 0);
        QuickMenu_SetRootPage(&menu, &g_Page_TimeTrialRoot.page);
        RUN_LOOP(menu.base);
        firstEntry = 0;
        ghostKept = TimeTrial_IsGhostLoaded();
        break;
    case 3:
        LicenseResult_SetFresh(firstEntry != 0);
        QuickMenu_SetRootPage(&menu, &g_Page_LicenseRoot.page);
        RUN_LOOP(menu.base);
        firstEntry = 0;
        break;
    }
    menuIO.mode = 1;
    RUN_LOOP(menuIO.base);
    switch ((signed char)menu.unk7c) {
    case 0:
        return 11;
    case 1:
        return 5;
    case 2:
    default:
        return 7;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/arcaderace", ArcadeRaceLoop__State6);
#endif

// After the replay: the menus (6).
int ArcadeRaceLoop::State13()
{
    return 6;
}

// The arcade race of ArcadeMode_Run (race types 0 to 4 and 6), then the next
// step from g_Flow_GameMode: 2 after mode 1, 4 after mode 3, else 1.
#ifdef NON_MATCHING
// The game's code but for one register copy of the race type before its switch.
extern "C" int ArcadeRace_Run(int arg)
{
    int ret;

    switch (RaceSetup_Get()->raceType) {
    case 0:
    case 3:
    case 4:
    case 6: {
        ArcadeRaceLoop loop(arg);

        loop.Execute();
        break;
    }
    }
    ret = 1;
    switch (g_Flow_GameMode) {
    case 0:
        break;
    case 1:
        ret = 2;
        break;
    case 2:
        break;
    case 3:
        ret = 4;
        break;
    }
    return ret;
}
#else
extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/arcaderace", ArcadeRace_Run);
}
#endif
