// Fills g_RaceSetup for an event race (a copy of the OVL4 function).
#include "common.h"
#include "gamestate.h"

extern "C" {
extern void *g_EventDataBuf;
extern void *g_GtModeRaceDataBuf;
extern RaceEventParams g_RaceEventParams;

char *strcpy(char *dst, const char *src);
unsigned int Irq_WaitVSyncs(int count);
char *GameData_GetString(void *data, unsigned short id);
RaceEvent *GameData_FindEvent(void *data, char *code);
void *GameData_GetCarEntry(unsigned short index);
void *GameData_GetEntry(int table, int index);
void CarSetup_InitFromCarEntry(void *carEntry, CarSetup *setup);
void CarSetup_FromCarEntry(void *carEntry, CarSetup *setup);
void CarParams_BuildFromSetup(CarSetup *setup, CarParams *params);
void RaceSetup_SetEventCode(RaceSetup *setup, char *code);
void RaceSetup_SetCourseByName(RaceSetup *setup, char *name);
char *CarInfoA_GetName(unsigned int carId);
int IdList_Count(IdList *list);
unsigned int RaceCarPick_Random(RaceCarPick *out, void *data, IdList *idList, unsigned int *seed,
                                RaceCarPick *picked, int pickedCount);
void RaceCarPick_SetColor(RaceCarPick *pick, unsigned int carId, unsigned int colorIdx, unsigned int *seed);
}

// Sets up the race of event (or, with keep set, the event of the current
// g_RaceSetup) from the event data (the GT mode data with gtMode set), then
// builds the g_RaceCarParams of every car. The race has 6 cars: the player
// car (pick1, set up from playerEntry), the rival car (pick2, from
// rivalEntry), then random cars from the event's car list.
extern "C" RaceSetup *RaceSetup_InitEvent(int keep, int gtMode, RaceEvent *event, void *playerEntry,
                                          RaceCarPick *pick1, void *rivalEntry, RaceCarPick *pick2)
{
    void *d = g_EventDataBuf;
    GameState *gs = &g_GameState;
    char *base = (char *)gs;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    void *gt = g_GtModeRaceDataBuf;
    void *data;
    unsigned int seed;
    RaceCarPick picks[6];
    int i;
    int j;

    if (gtMode) {
        d = gt;
    }
    data = d;
    if (keep == 0) {
        IdList *list;
        int n;
        int carCount;
        int randomCount;

        setup->dirt = 0;
        list = (IdList *)event;
        seed = Irq_WaitVSyncs(0);
        memset(setup, 0, sizeof(RaceSetup));
        *(RaceOptions *)setup->options = *(RaceOptions *)(base + 1);
        setup->frameStep = 2;
        setup->game = 2;
        setup->raceType = 4;
        setup->licenseId = 5;
        setup->licenseTest = 0;
        setup->showIntro = event->noIntro == 0;
        setup->unk0e = 0;
        setup->lapCount = event->lapCount;
        RaceSetup_SetEventCode(setup, GameData_GetString(data, event->codeString));
        RaceSetup_SetCourseByName(setup, GameData_GetString(data, event->courseString));
        strcpy(setup->eventName, GameData_GetString(data, event->nameString));
        setup->eventIndex = -1;
        setup->unk582 = -1;
        setup->garageSlot = -1;
        setup->maxPower = 0;
        setup->worldLeague = 0;
        setup->flags = (setup->flags | 1) & ~6;
        // n: the cars picked so far, the given ones first.
        n = 0;
        if (pick1) {
            n = 1;
            picks[0] = *pick1;
            picks[0].entryIndex = 0;
        }
        if (pick2) {
            picks[n] = *pick2;
            picks[n].entryIndex = 0;
            n++;
        }
        IdList_Count(list);
        randomCount = 6 - n;
        carCount = n + randomCount;
        i = 0;
        if (carCount >= 6) {
            carCount = 6;
        }
        setup->carCount = carCount;
        for (; i < carCount; i++) {
            RaceCar *car = &setup->cars[i];
            RaceCarPick *pick = &picks[i];
            unsigned int colorIdx = 0;
            int driverType = 1;
            int active = 1;
            void *entry;
            int color;
            CarSetup *cs;

            switch (i) {
            case 0:
                if (pick1) {
                    driverType = 3;
                }
                break;
            case 1:
                if (pick2) {
                    driverType = 4;
                }
                break;
            }
            memset(car, 0, sizeof(RaceCar));
            if (i >= n) {
                colorIdx = RaceCarPick_Random(pick, data, list, &seed, picks, n++);
                pick->flags = 0;
            }
            car->carId = pick->carId;
            color = pick->colorCode;
            car->active = active;
            entry = rivalEntry;
            car->gridSlot = carCount - (i + 1);
            car->colorCode = color;
            switch (driverType) {
            case 2:
            case 3:
                entry = playerEntry;
            case 4:
                CarSetup_FromCarEntry(entry, &car->setup);
                break;
            default: {
                // A random car: its setup from the car table, with the
                // car (and a colour of it) replaced for some entries.
                Table5Entry *e;
                CarSetup_InitFromCarEntry(GameData_GetCarEntry(pick->entryIndex), &car->setup);
                e = (Table5Entry *)GameData_GetEntry(5, car->setup.idx_t05);
                if (e->fixedCar) {
                    car->carId = e->carId;
                    RaceCarPick_SetColor(pick, car->carId, colorIdx, &seed);
                    car->colorCode = pick->colorCode;
                }
                break;
            }
            }
            cs = &car->setup;
            if (gtMode) {
                cs->flags_7a |= 0x40;
            }
            car->driverType = driverType;
            {
                int flags = pick->flags;
                car->unk8f = flags;
            }
            strcpy(car->name, CarInfoA_GetName(car->carId));
        }
    } else {
        event = GameData_FindEvent(data, setup->eventCode);
    }
    g_RaceEventParams = *(RaceEventParams *)((char *)event + 0x44);
    for (j = 0; j < setup->carCount; j++) {
        // g_RaceCarParams[j], past the end of g_GameState.
        char *base2 = (char *)&g_GameState;
        int offset2 = 0x14fda;
        CarParams_BuildFromSetup(&setup->cars[j].setup, (CarParams *)(base2 + (offset2 + j * 0x1c0)));
    }
    return setup;
}
