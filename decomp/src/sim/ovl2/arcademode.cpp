// OVL2 80010c84-80011a50 (arcademode): Arcade race setup, the arcade mode loop and its strings.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

extern "C" {

// The arcade menus' strings of one language.
struct ArcStrings {
    char data[0x314];
};

// RaceSetup.options.
struct ArcOptions {
    unsigned char b[8];
};

extern FlowMenuState g_Flow_IntroShown;
extern void *g_EventDataBuf;
extern GameState g_GameState;
extern RaceEventParams g_RaceEventParams;
extern ArcadeLoadInfo g_ArcadeLoadInfo;
extern char *g_ArcadeEventCodes[][4];
extern signed char g_ArcadeLevelByte[];
extern int D_801C3618;
extern char D_800267CC[];
extern char D_800267D0[];
extern char D_800267D4[];
RaceSetup *RaceSetup_BuildArcade_OVL2(int a, int b, void *event, void *entry, RaceCarPick *pick, int c, int d);
int ArcadeCar_GetColorCode(unsigned int carId, int color);
void *memset(void *p, int c, unsigned int n);
char *strcpy(char *dst, const char *src);
extern MenuPage g_ArcPage_Root;
extern ArcadeRaceSel g_ArcSel_Level;
extern ArcStrings g_OVL2_Strings;
extern unsigned char g_OVL2_StringsGz[];

#ifdef NON_MATCHING
// Builds the race setup from sel (2-player battle, single race or time trial) and what the
// loading screen shows.
void ArcadeRace_Setup(unsigned int seed1, unsigned int seed2, ArcadeRaceSel *sel)
{
    GameData *data = (GameData *)g_EventDataBuf;
    RaceSetup *setup;
    CarSetup cars[2];
    RaceCarPick pick;
    int anyGarage;
    int hasGarageCar[2];
    Car *garageCars[2];
    char *event;
    void *entry;
    GameState *state;
    int i;

    anyGarage = 0;
    hasGarageCar[0] = 0;
    hasGarageCar[1] = 0;
    garageCars[0] = 0;
    garageCars[1] = 0;
    switch (sel->raceType) {
    case 0:
        event = (char *)GameData_FindEvent(data, D_800267CC);
        state = &g_GameState;
        setup = (RaceSetup *)((char *)state + 0xbf7c);
        memset(setup, 0, 0x58c);
        *(ArcOptions *)setup->options = *(ArcOptions *)((char *)state + 1);
        setup->frameStep = 2;
        setup->game = 0;
        setup->raceType = 0;
        setup->showIntro = 1;
        setup->unk0e = 0;
        setup->lapCount = sel->lapOption;
        RaceSetup_SetEventCode(setup, D_800267D0);
        RaceSetup_SetCourse(setup, sel->courseHash);
        strcpy(setup->eventName, GameData_GetString(data, *(unsigned short *)(event + 0x94)));
        setup->carCount = 2;
        setup->unk582 = -1;
        setup->garageSlot = -1;
        setup->maxPower = 0;
        setup->worldLeague = 0;
        setup->flags = ((setup->flags & ~1) | ((sel->dirt ^ 1) != 0)) & ~6;
        for (i = 0; i < 2; i++) {
            if (sel->garages[i] < 0) {
                unsigned int color;

                entry = ArcadeCar_GetEntry(sel->carIds[i], sel->settings[i]);
                color = ArcadeCar_GetColorCode(sel->carIds[i], sel->colors[i]);
                RaceSetup_SetCarFromEntry(setup, i, i, i + 3, sel->unkB6[i], sel->carIds[i], color, entry);
                CarSetup_FromCarEntry(entry, &cars[i]);
            } else {
                Garage *garage = &state->garages[sel->garages[i]];

                cars[i] = garage->cars[sel->slots[i]].setup;
                anyGarage = 1;
                if (i == 0) {
                    hasGarageCar[0] = 1;
                    garageCars[0] = &garage->cars[sel->slots[0]];
                } else {
                    hasGarageCar[1] = 1;
                    garageCars[1] = &garage->cars[sel->slots[1]];
                }
            }
        }
        g_RaceEventParams = *(RaceEventParams *)(event + 0x44);
        break;
    case 6: {
        CourseRecord *record;
        unsigned int recordCar;

        event = (char *)GameData_FindEvent(data, D_800267CC);
        g_RaceEventParams = *(RaceEventParams *)(event + 0x44);
        state = &g_GameState;
        setup = (RaceSetup *)((char *)state + 0xbf7c);
        memset(setup, 0, 0x58c);
        *(ArcOptions *)setup->options = *(ArcOptions *)((char *)state + 1);
        setup->frameStep = 2;
        setup->raceType = 6;
        setup->game = 0;
        setup->showIntro = 0;
        setup->unk0e = 0;
        setup->lapCount = 100;
        RaceSetup_SetEventCode(setup, GameData_GetString(data, *(unsigned short *)event));
        RaceSetup_SetCourse(setup, sel->courseHash);
        strcpy(setup->eventName, GameData_GetString(data, *(unsigned short *)(event + 0x94)));
        setup->carCount = 1;
        setup->unk582 = -1;
        setup->garageSlot = -1;
        setup->maxPower = 0;
        setup->worldLeague = 0;
        setup->flags = ((setup->flags & ~1) | ((sel->dirt ^ 1) != 0)) & ~6;
        record = CourseRecord_GetCurrent();
        recordCar = *(unsigned int *)((char *)record + 0x14);
        strcpy((char *)setup->unk53c, D_800267D4);
        if (recordCar != 0) {
            strcpy((char *)setup->unk53c, CarInfoA_GetName(recordCar));
        }
        if (sel->garage < 0) {
            unsigned int color;

            entry = ArcadeCar_GetEntry(sel->carCodes[1], sel->unk14);
            color = ArcadeCar_GetColorCode(sel->carCodes[1], sel->color);
            RaceSetup_SetCarFromEntry(setup, 0, 0, 3, sel->unk18, sel->carCodes[1], color, entry);
            CarSetup_FromCarEntry(entry, &cars[0]);
        } else {
            Garage *garage = &state->garages[sel->garage];

            cars[0] = garage->cars[sel->slot].setup;
            anyGarage = 1;
            hasGarageCar[0] = 1;
            garageCars[0] = &garage->cars[sel->slot];
        }
        break;
    }
    case 4:
        event = (char *)GameData_FindEvent(data, g_ArcadeEventCodes[sel->level][sel->cls]);
        if (sel->garage < 0) {
            pick.carId = sel->carCodes[1];
            pick.colorCode = ArcadeCar_GetColorCode(sel->carCodes[1], sel->color);
            pick.flags = sel->unk18;
            entry = ArcadeCar_GetEntry(sel->carCodes[1], sel->unk14);
            setup = RaceSetup_BuildArcade_OVL2(0, 0, event, entry, &pick, 0, 0);
            CarSetup_FromCarEntry(entry, &cars[0]);
        } else {
            Garage *garage = &g_GameState.garages[sel->garage];

            setup = RaceSetup_BuildArcade_OVL2(0, 0, event, 0, 0, 0, 0);
            cars[0] = garage->cars[sel->slot].setup;
            anyGarage = 1;
            hasGarageCar[0] = 1;
            garageCars[0] = &garage->cars[sel->slot];
        }
        setup->game = 0;
        setup->raceType = sel->raceType;
        setup->carCount = 6;
        setup->lapCount = sel->lapOption;
        RaceSetup_SetCourse(setup, sel->courseHash);
        setup->unk57e = sel->courseStat;
        setup->pad57f = g_ArcadeLevelByte[sel->level];
        break;
    }
    g_ArcSel_Level.setup = cars[0];
    g_ArcSel_Level.setupCopy[0] = cars[0];
    g_ArcSel_Level.setupCopy[1] = cars[0];
    D_801C3618 = anyGarage;
    strcpy(g_ArcadeLoadInfo.name, sel->courseName);
    g_ArcadeLoadInfo.unk08 = *(short *)&sel->dirt;
    g_ArcadeLoadInfo.seeds[0] = seed1;
    g_ArcadeLoadInfo.seeds[1] = seed2;
    g_ArcadeLoadInfo.raceType = sel->raceType;
    g_ArcadeLoadInfo.carCount = setup->carCount;
    for (i = 0; i < setup->carCount; i++) {
        RaceCar *car = &setup->cars[i];
        int grid = car->gridSlot;

        if (i == 0 && hasGarageCar[0] != 0) {
            g_ArcadeLoadInfo.cars[grid].carId = garageCars[0]->baseCarId;
            g_ArcadeLoadInfo.cars[grid].rgb = CarInfoA_GetColorRgb(garageCars[0]->baseCarId, garageCars[0]->color);
        } else if (i == 1 && hasGarageCar[1] != 0) {
            g_ArcadeLoadInfo.cars[grid].carId = garageCars[1]->baseCarId;
            g_ArcadeLoadInfo.cars[grid].rgb = CarInfoA_GetColorRgb(garageCars[1]->baseCarId, garageCars[1]->color);
        } else {
            g_ArcadeLoadInfo.cars[grid].carId = car->carId;
            g_ArcadeLoadInfo.cars[grid].rgb = CarInfoA_GetColorRgb(car->carId, car->colorCode);
        }
    }
    g_ArcadeLoadInfo.tim = sel->courseMap;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademode", ArcadeRace_Setup);
#endif

// The arcade mode: the menus, then what they chose (g_Flow_IntroShown.arcadeMenuExit):
// 0 back to the title, 1 the race (ArcadeRace_Setup with two random seeds and the
// loading screen, then OVL3 with the selection in g_RaceCarWorkAlt), 2 and 3 the
// movie (OVL5), 4 OVL1's next demo.
void ArcadeMode_Run(void)
{
    int r;

    ArcadeMenu_LoadStrings();
    {
        ArcadeMenu menu;

        ArcadeMenu_SetRootPage(&menu, &g_ArcPage_Root);
        do {
            r = menu.Execute();
        } while (r > 1 ? 1 : r < 0);
    }
    switch (g_Flow_IntroShown.arcadeMenuExit) {
    case 4:
        Overlay_StartAt(1, (void *)MainMenu_StartNextDemo, 0);
        break;
    case 0:
        g_Flow_IntroShown.homeOverlay = 0;
        g_Flow_IntroShown.gameMode = 0;
        Overlay_Start(1);
        break;
    case 2:
        g_Flow_IntroShown.homeOverlay = 0;
        g_Flow_IntroShown.gameMode = 0;
        Overlay_StartAt(5, (void *)Movie_ArcadeEntry, 1);
        break;
    case 3:
        g_Flow_IntroShown.homeOverlay = 0;
        g_Flow_IntroShown.gameMode = 0;
        Overlay_StartAt(5, (void *)Movie_ArcadeEntry, 0);
        break;
    case 1: {
        unsigned int seed = Irq_WaitVSyncs(0);
        unsigned int seed1 = Rand_Next(&seed);
        unsigned int seed2 = Rand_Next(&seed);

        ArcadeRace_Setup(seed1, seed2, &g_ArcSel_Level);
        {
            ArcadeRacingMenu loading;

            do {
                r = loading.Execute();
            } while (r > 1 ? 1 : r < 0);
        }
        {
            char *base = (char *)&g_GameState;
            int offset = 0xc6c0;

            g_Flow_IntroShown.homeOverlay = 1;
            g_Flow_IntroShown.gameMode = 1;
            *(ArcadeRaceSel *)((int)(base + offset) & ~3) = g_ArcSel_Level;
        }
        Overlay_Start(3);
        Overlay_StartAt(0, (void *)Race_Entry, 0);
        break;
    }
    }
}

// Inflates the strings of every language on the stack and keeps the current language's.
void ArcadeMenu_LoadStrings(void)
{
    ArcStrings strings[8];

    Gzip_Inflate(g_OVL2_StringsGz, strings);
    g_OVL2_Strings = strings[g_GameState.language];
}
}
