// OVL0 80017098-80017e18 (gtrace): GT mode race loop and practice.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"
#include "gamestate.h"

extern "C" unsigned char (&g_Race_IsDemo);

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// The race's memory, as the game reaches it, offset at run time.
static inline void *RaceMem_Aligned(int offset)
{
    return (void *)rt::from_guest((((int)rt::to_guest(&g_Race_IsDemo) + offset) & -4));
}

// The event buffers the menus keep over the race.
struct EventBlock {
    int data[0x17e8 / 4];
};

struct RecordBlock {
    int data[0x364 / 4];
};

// The championship's list.
static inline ChampRaceList *ChampList_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xc514;

    return (ChampRaceList *)(base + offset);
}

// The menu and the two flags after it.
struct RaceMenu {
    QuickMenu menu;
    unsigned char doneA;
    unsigned char doneB;
};

static inline void RaceMenu_SetDone(RaceMenu *menu, int done)
{
    menu->doneA = done;
    menu->doneB = done;
}

// What the player chose in the menus.
static inline int RaceLoop_Choice(RaceLoop *loop)
{
    return (signed char)loop->menu.unk7c;
}

// Keeps the event's data over the race.
static inline void EventBlock_Keep(void)
{
    int raceOffset = 0xc5397;
    int stateOffset = 0x10bd8;
    char *race;
    char *state;
    int mask;
    EventBlock *dst;

    race = (char *)&g_Race_IsDemo + raceOffset;
    mask = -4;
    dst = (EventBlock *)rt::from_guest(((int)rt::to_guest(race) & mask));
    state = (char *)&g_GameState;
    *dst = *(EventBlock *)rt::from_guest(((int)rt::to_guest((state + stateOffset)) & mask));
}

// Runs a screen loop until it returns 0 or 1.
#define RUN_LOOP(screen)                                         \
    do {                                                         \
        do {                                                     \
            ret = ((ScreenViewLoop *)&(screen))->Execute();      \
        } while (ret > 1);                                       \
    } while (ret < 0)

// How many races were entered (PlayerStats).
#define STATS_ENTERED(stats) (*(int *)((char *)(stats) + 0x48))

extern "C" {
extern game_pointer<void> (&g_EventDataBuf);
extern signed char (&g_Flow_HomeOverlay);
extern signed char (&g_Flow_GameMode);
extern unsigned char (&g_Race_EndRequest);
extern short (&D_8002F518)[];
extern QuickMenuPage (&g_Page_GtRaceRoot);
extern QuickMenuPage (&g_Page_ResultRoot);
extern QuickMenuPage (&g_Page_LicenseRoot);
extern QuickMenuPage (&g_Page_RaceResultsRoot);
extern QuickMenuPage (&g_Page_EventPrizeRoot);
extern QuickMenuPage (&g_Page_ChampRacePrizeRoot);
extern QuickMenuPage (&g_Page_ChampWonPrizeRoot);
extern QuickMenuPage (&g_Page_EndingRoot);
void GTRace_BeginPractice(RaceLoop *loop);
void GTRace_EndPractice(RaceLoop *loop, int raceType, int nextType);
void GTRace_RunMenus(RaceLoop *loop, int fresh);
// Practice: 100 laps, alone and without the intro.
void GTRace_BeginPractice(RaceLoop *loop)
{
    RaceSetup *setup = RaceSetup_Get();
    int i;

    loop->savedLapCount = setup->lapCount;
    loop->savedCarCount = setup->carCount;
    loop->savedIntro = setup->showIntro;
    setup->lapCount = 100;
    setup->showIntro = 0;
    setup->carCount = 1;
    for (i = 1; i < loop->savedCarCount; i++) {
        RaceCar *car = &setup->cars[i];

        car->active = 0;
    }
}

// Puts the setup back, for the next race type.
void GTRace_EndPractice(RaceLoop *loop, int raceType, int nextType)
{
    RaceSetup *setup = RaceSetup_Get();
    int saved;
    int i;

    loop->savedRaceType = raceType;
    setup->raceType = nextType;
    saved = loop->savedIntro;
    setup->showIntro = saved;
    saved = loop->savedCarCount;
    setup->carCount = saved;
    saved = loop->savedLapCount;
    setup->lapCount = saved;
    for (i = 0; i < loop->savedCarCount; i++) {
        RaceCar *car = &setup->cars[i];

        car->active = 1;
    }
}

// Runs the PreQuickMenuIO screen in mode 0 (from the race to the menus).
void RaceLoop_EnterMenus(RaceLoop *loop)
{
    int iVar1;

    (loop->menuIO).mode = '\0';
    do {
        do {
            iVar1 = ((ScreenViewLoop *)&loop->menuIO)->Execute();
        } while (1 < iVar1);
    } while (iVar1 < 0);
}

// Runs the PreQuickMenuIO screen in mode 1 (from the menus back to the race).
void RaceLoop_LeaveMenus(RaceLoop *loop)
{
    int iVar1;

    (loop->menuIO).mode = '\x01';
    do {
        do {
            iVar1 = ((ScreenViewLoop *)&loop->menuIO)->Execute();
        } while (1 < iVar1);
    } while (iVar1 < 0);
}

// The pages of the race type; fresh shows the race results first.
void GTRace_RunMenus(RaceLoop *loop, int fresh)
{
    int ret;

    switch (RaceSetup_Get()->raceType) {
    case 1:
    case 10:
        QuickMenu_SetRootPage(&loop->menu, &g_Page_GtRaceRoot.page);
        RUN_LOOP(loop->menu);
        break;
    case 2:
    case 11:
        if (loop->resultsPending) {
            if (fresh) {
                QuickMenu_SetRootPage(&loop->menu, &g_Page_RaceResultsRoot.page);
                RUN_LOOP(loop->menu);
                if (ChampList_Get()->raceCount < 2) {
                    QuickMenu_SetRootPage(&loop->menu, &g_Page_EventPrizeRoot.page);
                } else {
                    QuickMenu_SetRootPage(&loop->menu, &g_Page_ChampRacePrizeRoot.page);
                }
                RUN_LOOP(loop->menu);
            }
            loop->resultsPending = 0;
        }
        QuickMenu_SetRootPage(&loop->menu, &g_Page_ResultRoot.page);
        RUN_LOOP(loop->menu);
        break;
    case 3:
        LicenseResult_SetFresh(loop->resultsPending != 0);
        QuickMenu_SetRootPage(&loop->menu, &g_Page_LicenseRoot.page);
        RUN_LOOP(loop->menu);
        loop->resultsPending = 0;
        break;
    case 7:
    case 8:
    case 9:
        if (loop->resultsPending) {
            if (fresh) {
                QuickMenu_SetRootPage(&loop->menu, &g_Page_RaceResultsRoot.page);
                RUN_LOOP(loop->menu);
            }
            loop->resultsPending = 0;
        }
        QuickMenu_SetRootPage(&loop->menu, &g_Page_GtRaceRoot.page);
        RUN_LOOP(loop->menu);
        break;
    }
}

// Loading: what the race type keeps over the race.
int GranTurismoRaceLoop__State4(RaceLoop *loop)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup;
    PlayerStats *stats;

    RaceMenu_SetDone((RaceMenu *)&loop->menu, 0);
    loop->firstEntry = 1;
    setup = (RaceSetup *)(base + offset);
    loop->resultsPending = 0;
    stats = (PlayerStats *)(base + 0xb8);
    switch (setup->raceType) {
    case 3:
        VolId_ReadFile((VolBootId)D_8002F518[g_GameState.language], RaceMem_Aligned(0xca397));
        g_EventDataBuf = GameData_LoadLicense(RaceMem_Aligned(0xc0397), 0x9ff0);
        break;
    case 10:
        STATS_ENTERED(stats)++;
        GTRace_BeginPractice(loop);
        break;
    case 1:
        STATS_ENTERED(stats)++;
        GTRace_BeginPractice(loop);
        EventBlock_Keep();
        break;
    case 2:
    case 11:
        RaceFont_Load(6);
        {
            char *race = (char *)&g_Race_IsDemo;
            int raceOffset = 0x118000;

            (race + raceOffset)[0x5701] = 0;
        }
        break;
    case 7:
    case 8:
    case 9:
        {
            int raceOffset = 0xc0397;
            char *race;
            char *state;
            int stateOffset;
            int mask;
            RecordBlock *dst;

            race = (char *)&g_Race_IsDemo + raceOffset;
            mask = -4;
            dst = (RecordBlock *)rt::from_guest(((int)rt::to_guest(race) & mask));
            state = (char *)&g_GameState;
            stateOffset = 0xc6c0;
            *dst = *(RecordBlock *)rt::from_guest(((int)rt::to_guest((state + stateOffset)) & mask));
        }
        EventBlock_Keep();
        break;
    }
    return 5;
}

// Before the race: the menus, unless the race has just been entered.
int GranTurismoRaceLoop__State8(RaceLoop *loop)
{
    RaceSetup *setup = RaceSetup_Get();
    int raceType = setup->raceType;
    int nextType = 2;

    if (loop->firstEntry != 0) {
        switch (raceType) {
        case 3:
            RaceLoop_EnterMenus(loop);
            GTRace_RunMenus(loop, 0);
            RaceLoop_LeaveMenus(loop);
            switch ((signed char)loop->menu.unk7c) {
            case 0:
                return 11;
            case 2:
                return 7;
            }
            break;
        case 7:
        case 8:
        case 9:
            RaceLoop_EnterMenus(loop);
            GTRace_RunMenus(loop, 0);
            switch ((signed char)loop->menu.unk7c) {
            case 0:
            case 2:
                return 7;
            }
            RaceLoop_LeaveMenus(loop);
            break;
        case 10:
            nextType = 11;
        case 1:
            RaceLoop_EnterMenus(loop);
            GTRace_RunMenus(loop, 0);
            switch (RaceLoop_Choice(loop)) {
            case 0:
            case 4:
                GTRace_EndPractice(loop, raceType, nextType);
                return 4;
            case 2:
                return 7;
            }
            RaceLoop_LeaveMenus(loop);
            break;
        }
    }
    {
        GameState *state = &g_GameState;
        int offset = 0xbf7c;
        RaceSetup *car = (RaceSetup *)((char *)state + offset);
        int garage = car->unk582;
        int slot = car->garageSlot;

        if ((garage >= 0) & (slot >= 0)) {
            Garage *g = &state->garages[garage];
            Car *from = (Car *)rt::from_guest((slot * sizeof(Car) + (int)rt::to_guest(g->cars)));

            car->dirt = from->dirt;
        }
    }
    loop->resultsPending = 1;
    return 9;
}

// After the race: the car's dirt goes back to the garage.
int GranTurismoRaceLoop__State10(RaceLoop *loop)
{
    GameState *state = &g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)((char *)state + offset);
    int dirt = *(short *)((char *)state + 0x14fd8);
    int garage = setup->unk582;
    int slot = setup->garageSlot;

    if ((garage >= 0) & (slot >= 0)) {
        Garage *g = &state->garages[garage];
        Car *car = (Car *)rt::from_guest((slot * sizeof(Car) + (int)rt::to_guest(g->cars)));

        car->dirt = dirt;
    }
    if (g_Race_EndRequest >= 2 && (setup->raceType == 2 || setup->raceType == 11)) {
        return 7;
    }
    RaceMenu_SetDone((RaceMenu *)&loop->menu, 1);
    return 11;
}

// The menus, then the next race of a championship, its prize and the ending.
int GranTurismoRaceLoop__State6(RaceLoop *loop)
{
    RaceSetup *setup;
    int nextType;
    int ret;
    int home;

    loop->firstEntry = 0;
    *((char *)&loop->view + 0x2e7) = 0;
    RaceLoop_EnterMenus(loop);
    home = g_Flow_HomeOverlay;
    nextType = 2;
    if (home == 0 || home == 2) {
        return 7;
    }
    {
        char *base = (char *)&g_GameState;
        int setupOffset = 0xbf7c;
        int countOffset = 0xc5a8;
        int fresh;

        setup = (RaceSetup *)(base + setupOffset);
        fresh = *(short *)(base + countOffset) > 0;
        if (setup->raceType == 12) {
            return 7;
        }
        GTRace_RunMenus(loop, fresh);
    }
    switch (setup->raceType) {
    case 10:
        nextType = 11;
    case 1:
        if ((signed char)loop->menu.unk7c == 4) {
            GTRace_EndPractice(loop, setup->raceType, nextType);
            return 4;
        }
        break;
    case 2:
    case 11:
        if ((signed char)loop->menu.unk7c == 4) {
            ChampRaceList *champ = ChampList_Get();
            signed char order[6];
            int more;
            int place;
            int i;

            champ->unk00++;
            if (champ->unk00 < champ->raceCount) {
                int saved = loop->savedRaceType;

                setup->raceType = saved;
                RaceSetup_SetCourse(setup, champ->courseHash[champ->unk00]);
                {
                    int laps = (short)champ->param45[champ->unk00];

                    setup->lapCount = laps;
                }
                RaceSetup_SetEventCode(setup, champ->raceCode[champ->unk00]);
                {
                    char *race = (char *)&g_Race_IsDemo;
                    int raceOffset = 0x118000;

                    (race + raceOffset)[0x5700] = 0;
                }
                RaceFont_Load(6);
                return 4;
            }
            more = champ->raceCount >= 2;
            if (more == 0) {
                return 7;
            }
            place = 6;
            ChampRaceList_SortStandings(champ, order);
            for (i = 0; i < 6; i++) {
                if (order[i] == 0) {
                    place = i;
                }
            }
            PlayerStats_SetEventResult(&g_GameState.stats, RaceSetup_Get()->eventIndex, place + 1);
            if (place == 0) {
                QuickMenu_SetRootPage(&loop->menu, &g_Page_ChampWonPrizeRoot.page);
                RUN_LOOP(loop->menu);
            }
            if (place == 0 && *(signed char *)&RaceSetup_Get()->worldLeague != 0) {
                QuickMenu_SetRootPage(&loop->menu, &g_Page_EndingRoot.page);
                RUN_LOOP(loop->menu);
            }
            return 7;
        }
        break;
    }
    RaceLoop_LeaveMenus(loop);
    switch ((signed char)loop->menu.unk7c) {
    case 0:
        return 11;
    case 1:
        return 5;
    }
    return 7;
}

int GranTurismoRaceLoop__State13(RaceLoop *loop)
{
    return 6;
}

// The GT mode race of the race type, then the next step from g_Flow_GameMode.
int GTRace_Run(int arg)
{
    int ret;

    switch (RaceSetup_Get()->raceType) {
    case 12:
        arg = 1;
    case 1:
    case 2:
    case 3:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11: {
        RaceLoop loop(arg);

        loop.Execute();
        break;
    }
    }
    ret = 1;
    switch (g_Flow_GameMode) {
    case 0:
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        ret = 4;
        break;
    }
    return ret;
}

}

RaceLoop::RaceLoop(int arg) : RaceMenuLoop(&view, arg)
{
    vptr.address = 0x8002f110;  // RaceLoop's vtable
}

int RaceLoop::State4()
{
    return GranTurismoRaceLoop__State4(this);
}

int RaceLoop::State6()
{
    return GranTurismoRaceLoop__State6(this);
}

int RaceLoop::State8()
{
    return GranTurismoRaceLoop__State8(this);
}

int RaceLoop::State10()
{
    return GranTurismoRaceLoop__State10(this);
}

int RaceLoop::State13()
{
    return GranTurismoRaceLoop__State13(this);
}
