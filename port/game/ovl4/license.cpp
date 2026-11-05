// OVL4 80010000-80010550: License tests.
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern void *g_EventDataBuf;
extern int g_GameDataMode;
extern char *g_GtModeRaceData;
extern unsigned char D_80091620[];
extern RaceEventParams g_RaceEventParams;
extern char *D_80024418[6];
extern char *g_LicenseTestFmt[6];
extern CarSetup g_License_TmpSetup;
extern CarParams g_License_TmpParams;
extern EnginePerf g_License_TmpPerf;

// The license of a license test code ("LIS..." to "LIB..."), from its first 3
// letters (this file's copy of License_IdFromRaceCode2).
LicenseId License_IdFromRaceCode(char *raceCode)
{
    char prefix[16];
    LicenseId id = LICENSE_S;
    LicenseId i;

    strcpy(prefix, raceCode);
    prefix[3] = 0;
    for (i = id; (int)i < 6; i++) {
        if (strcmp(D_80024418[i], prefix) == 0)
            id = i;
    }
    return id;
}

// Fills g_RaceSetup for the license test raceCode: one car, the event's
// (the car code and colour index in the event's word 1).
RaceSetup *RaceSetup_InitLicenseTest(char *raceCode)
{
    void *data = g_EventDataBuf;
    GameState *gs = &g_GameState;
    char *base = (char *)gs;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    RaceEvent *event = (RaceEvent *)GameData_FindEvent((GameData *)data, raceCode);
    GameDataTable *table;
    unsigned int colorIdx;
    unsigned int carCode;
    unsigned int *entry;
    RaceCar *car;
    int two;
    int one;

    memset(setup, 0, sizeof(RaceSetup));
    // The constants in registers, for the game's sllv and sltu.
    two = 2;
    one = 1;
    // Ghidra's multi-field copies: the options (8 bytes at any alignment) and,
    // below, g_RaceEventParams.
    *(RaceOptions *)setup->options = *(RaceOptions *)(base + 1);
    setup->frameStep = two;
    setup->game = one;
    setup->raceType = 3;
    setup->showIntro = event->noIntro == 0;
    setup->unk0e = 0;
    RaceSetup_SetEventCode(setup, GameData_GetString((GameData *)data, event->codeString));
    RaceSetup_SetCourseByName(setup, GameData_GetString((GameData *)data, event->courseString));
    strcpy(setup->eventName, GameData_GetString((GameData *)data, event->nameString));
    setup->licenseId = License_IdFromRaceCode(raceCode);
    {
        int ones = ((signed char *)raceCode)[4]; // keeps its lb

        setup->licenseTest = (((signed char *)raceCode)[3] - '0') * 10 - '0' + ones;
    }
    setup->lapCount = event->lapCount;
    setup->eventIndex = -1;
    setup->unk582 = -1;
    setup->garageSlot = -1;
    setup->carCount = one;
    setup->maxPower = 0;
    setup->flags = (setup->flags | one) & ~6;
    setup->worldLeague = 0;
    if (g_GameDataMode == two) {
        table = (GameDataTable *)(g_GtModeRaceData + 0x10);
    } else {
        table = (GameDataTable *)((char *)data + 0x100);
    }
    carCode = *(unsigned int *)((char *)event + 4);
    colorIdx = carCode >> 26;
    carCode &= 0x3ffffff;
    entry = (unsigned int *)GameDataTable_At(table, carCode - 1);
    car = &setup->cars[0];
    memset(car, 0, sizeof(RaceCar));
    car->carId = *entry;
    car->colorCode = D_80091620[colorIdx];
    car->active = 1;
    car->gridSlot = 0;
    CarSetup_FromCarEntry(GameData_GetCarEntry(carCode), &setup->cars[0].setup);
    car->driverType = 3;
    car->unk8f = 0;
    {
        int idx = car->setup.idx_t05; // blez, not bnez

        if (idx > 0) {
            car->carId = ((Table5Entry *)GameData_GetEntry(5, idx))->carId;
        }
    }
    g_RaceEventParams = *(RaceEventParams *)((char *)event + 0x44);
    {
        // g_RaceCarParams[0], past the end of g_GameState.
        char *base2 = (char *)&g_GameState;
        int offset2 = 0x14fda;
        CarParams_BuildFromSetup(&setup->cars[0].setup, (CarParams *)(base2 + offset2));
    }
    return setup;
}

// For the 6 licenses x 10 tests (g_LicenseTestFmt, with the test number): the
// name, max power and drivetrain of the test car, computed in
// g_License_TmpSetup, g_License_TmpParams and g_License_TmpPerf.
// Doesn't match: the game keeps the license, its row offset and table on the
// stack and &code, g_License_TmpParams and g_License_TmpPerf in registers, and
// addresses the name and the other fields from two different offsets.
void License_BuildTestCarTable(LicenseTestCar *table)
{
    char code[32];
    int license;
    int test;

    for (license = 0; license < 6; license++) {
        for (test = 0; test < 10; test++) {
            void *data;
            RaceEvent *event;
            GameDataTable *t;
            unsigned int carCode;
            unsigned int *entry;
            LicenseTestCar *car = &table[license * 10 + test];

            sprintf(code, g_LicenseTestFmt[license], test);
            data = g_EventDataBuf;
            event = (RaceEvent *)GameData_FindEvent((GameData *)data, code);
            if (g_GameDataMode == 2) {
                t = (GameDataTable *)(g_GtModeRaceData + 0x10);
            } else {
                t = (GameDataTable *)((char *)data + 0x100);
            }
            carCode = *(unsigned int *)((char *)event + 4) & 0x3ffffff;
            entry = (unsigned int *)GameDataTable_At(t, carCode - 1);
            CarSetup_FromCarEntry(GameData_GetCarEntry(carCode), &g_License_TmpSetup);
            CarParams_BuildFromSetup(&g_License_TmpSetup, &g_License_TmpParams);
            CarParams_GetPerformance(&g_License_TmpParams, &g_License_TmpPerf);
            strcpy(car->name, CarInfoA_GetName(*entry));
            car->drivetrain = g_License_TmpParams.drivetrain;
            car->maxPower = g_License_TmpPerf.maxPower;
        }
    }
}

// Sets up the license test raceCode and builds the test car table in
// g_RaceCarWork (g_GameState + 0x10bd8, rounded down to a word).
void GTMenu_StartLicenseTest(char *raceCode)
{
    GameState *gs = &g_GameState;
    char *base = (char *)gs;
    int offset = 0x10bd8;
    LicenseTestCar *table = (LicenseTestCar *)((int)(base + offset) & ~3);

    RaceSetup_InitLicenseTest(raceCode);
    License_BuildTestCarTable(table);
}
}
