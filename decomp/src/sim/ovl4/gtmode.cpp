// OVL4 80012c6c-80013a28 (gtmode): Machine tests, event races and the GT mode loop.
// GTMode_SetupEventRace and GTMode_Run are still the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern void *g_EventDataBuf;
extern int g_GameDataMode;
extern char *g_GtModeRaceData;
extern char *g_GtModeRaceDataBuf;
extern RaceEventParams g_RaceEventParams;

// Fills g_RaceSetup for the machine test raceCode (raceType 7 to 9, from
// Event_GetMachineTestType): one lap with the current car of garage 0, against
// the test's records (MachineTest_BuildCarList).
void RaceSetup_InitMachineTest(char *raceCode, int raceType)
{
    void *data = g_EventDataBuf;
    GameState *gs = &g_GameState;
    char *base = (char *)gs;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    RaceEvent *event = (RaceEvent *)GameData_FindEvent((GameData *)data, raceCode);
    Garage *garage = &gs->garages[0];
    int slot = garage->currentCar;
    Car *car = &garage->cars[slot];
    MachineTestRecords *records;
    int one;
    int zero;

    memset(setup, 0, sizeof(RaceSetup));
    *(RaceOptions *)setup->options = *(RaceOptions *)(base + 1);
    one = 1; // in a register, as zero below
    setup->frameStep = 2;
    setup->game = one;
    setup->raceType = raceType;
    setup->showIntro = one;
    setup->unk0e = 0;
    RaceSetup_SetEventCode(setup, GameData_GetString((GameData *)data, event->codeString));
    RaceSetup_SetCourseByName(setup, GameData_GetString((GameData *)data, event->courseString));
    strcpy(setup->eventName, GameData_GetString((GameData *)data, event->nameString));
    setup->licenseId = 5;
    setup->eventIndex = -1;
    setup->unk582 = -1;
    setup->garageSlot = -1;
    setup->licenseTest = 0;
    setup->lapCount = one;
    setup->carCount = one;
    setup->maxPower = 0;
    setup->worldLeague = 0;
    setup->flags = (setup->flags | one) & ~6;
    zero = 0;
    g_RaceEventParams = *(RaceEventParams *)((char *)event + 0x44);
    GTMode_PreparePlayerCar(slot, zero, Event_IsDirt(raceCode));
    RaceSetup_SetCarFromGarage(setup, 0, -1, 3, 0, car, zero, slot);
    {
        GameState *gs2 = &g_GameState;
        char *base2 = (char *)gs2;
        int offset2 = 0xc6c0;
        MachineTestCarList *list = (MachineTestCarList *)((int)(base2 + offset2) & ~3);

        switch (raceType) {
        case 7:
            records = &gs2->machineTests[0];
            break;
        case 8:
            records = &gs2->machineTests[1];
            break;
        case 9:
            records = &gs2->machineTests[2];
            break;
        }
        MachineTest_BuildCarList(list, records, car->baseCarId);
    }
    {
        // g_RaceCarWork, rounded down to a word.
        GameState *gs3 = &g_GameState;
        char *base3 = (char *)gs3;
        int offset3 = 0x10bd8;
        CarWork *work = (CarWork *)((int)(base3 + offset3) & ~3);

        RaceStart_CarWorkClear(work);
        RaceStart_CarWorkLoadParts(work, car->carId);
        RaceStart_CarWorkSetStages(work, &car->setup);
    }
}

// Fills car as an AI car (driver type 1) on gridSlot: a car drawn from pool
// until one is available, with its car entry's setup and a random colour.
void RaceCar_InitFromPool(RaceCar *car, int gridSlot, unsigned int *seed, OpponentPool *pool)
{
    unsigned int count = pool->count;
    unsigned int carId;
    unsigned short *rgb;
    char *codes;

    memset(car, 0, sizeof(RaceCar));
    do {
        int index = pool->carIndex[Rand_Next(seed) % count % count];
        char *gt;
        GameDataTable *table;

        CarSetup_InitFromCarEntry(GameData_GetCarEntry(index + 1), &car->setup);
        gt = g_GtModeRaceDataBuf;
        if (g_GameDataMode == 2) {
            table = (GameDataTable *)(g_GtModeRaceData + 0x10);
        } else {
            table = (GameDataTable *)(gt + 0x100);
        }
        carId = *(unsigned int *)GameDataTable_At(table, index);
    } while (!CarInfoA_IsAvailable(carId));
    {
        int idx = car->setup.idx_t05;

        if (idx > 0) {
            carId = ((Table5Entry *)GameData_GetEntry(5, idx))->carId;
        }
    }
    {
        unsigned int colors = CarInfoA_GetColors(carId, &rgb, &codes);
        char color = codes[Rand_Next(seed) % colors];

        car->carId = carId;
        car->colorCode = (signed char)color;
    }
    car->active = 1;
    car->gridSlot = gridSlot;
    car->driverType = 1;
    car->unk8f = 0;
    strcpy(car->name, CarInfoA_GetName(carId));
    {
        CarSetup *cs = &car->setup;

        cs->flags_7a |= 0x40;
    }
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmode", GTMode_SetupEventRace);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmode", GTMode_Run);
}
