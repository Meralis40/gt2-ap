// OVL4 8001907c-80019bf0 (eventrules): Licenses and event entry rules.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern unsigned int g_License_EntryMsgPages[6];
extern GameState g_GameState;
extern char *g_RandomCourseNames[];
extern char *g_LicenseRaceCodes[6];
extern char *g_EventCodes[];
extern EventRules g_EventRules[];
extern unsigned int g_EventAllowedCars[];
extern void *g_GtModeRaceData;
extern void *g_GtModeRaceDataBuf;

// One of the random course names, picked with seed.
char *Course_PickRandom(unsigned int *seed)
{
    unsigned int count;
    unsigned int k;

    for (count = 0; g_RandomCourseNames[count]; count++)
        ;
    k = Rand_Next(seed) % count;
    return g_RandomCourseNames[k];
}

// The license of a license test code ("LIS..." to "LIB..."), from its first 3 letters.
LicenseId License_IdFromRaceCode2(char *raceCode)
{
    char prefix[16];
    LicenseId id = LICENSE_S;
    LicenseId i;

    strcpy(prefix, raceCode);
    prefix[3] = 0;
    for (i = id; (int)i < 6; i++) {
        if (strcmp(g_LicenseRaceCodes[i], prefix) == 0)
            id = i;
    }
    return id;
}

// 1 if every test of license has a medal.
int License_IsEarned(LicenseId license)
{
    GameState *gs = &g_GameState;
    int earned = 1;
    int i = 0;
    LicenseTest *test = gs->licenses[license];

loop:
    if (i < 10) {
        if ((signed char)test->medal == MEDAL_NONE)
            earned = 0;
        test++;
        i++;
        goto loop;
    }
    return earned;
}

// The best license earned, or LICENSE_NONE.
LicenseId License_GetHighest(void)
{
    if (License_IsEarned(LICENSE_S))
        return LICENSE_S;
    if (License_IsEarned(LICENSE_IA))
        return LICENSE_IA;
    if (License_IsEarned(LICENSE_IB))
        return LICENSE_IB;
    if (License_IsEarned(LICENSE_IC))
        return LICENSE_IC;
    if (License_IsEarned(LICENSE_A))
        return LICENSE_A;
    if (License_IsEarned(LICENSE_B))
        return LICENSE_B;
    return LICENSE_NONE;
}

// The car codes of allowed car list listId (1-based, 0 ended), or 0 for 0 (any car).
unsigned int *GtRaceData_GetAllowedCars(int listId)
{
    void *data = g_GtModeRaceData;
    int i = listId - 1;

    if (i < 0)
        return 0;
    return (unsigned int *)GameData_GetEntryIn(data, 2, i);
}

// Fills rules from the race data of event raceCode, its allowed cars (at most 32)
// going to allowedCars from allowedStart. Returns the number of allowed cars.
int EventRules_Load(EventRules *rules, char *raceCode, unsigned int *allowedCars, int allowedStart)
{
    RaceEvent *event = GameData_FindEvent(g_GtModeRaceDataBuf, raceCode);
    int count;
    unsigned int *cars;

    rules->championshipPrize = event->championshipPrize * 100;
    rules->prizes[0] = event->prizes[0] * 100;
    rules->prizes[1] = event->prizes[1] * 100;
    rules->prizes[2] = event->prizes[2] * 100;
    rules->prizes[3] = event->prizes[3] * 100;
    rules->prizes[4] = event->prizes[4] * 100;
    rules->prizes[5] = event->prizes[5] * 100;

    // Read before the stores to rules, which GCC can't move them past.
    int list = event->allowedCarsList;
    int dirt = event->dirt;
    int license = event->license;
    int drivetrain = event->drivetrain;
    short maxPower = event->maxPower;
    int turbo = event->turbo;
    int racingMod = event->racingMod;

    count = 0;
    rules->allowedCarsStart = allowedStart;
    rules->allowedCarsCount = 0;
    rules->maxPower = maxPower;
    rules->flags.bits = ((((rules->flags.bits & ~1 | dirt) & ~0xe | license << 1) & ~0x70
                          | drivetrain << 4) & ~0x180 | turbo << 7) & ~0x600
                        | racingMod << 9;
    cars = GtRaceData_GetAllowedCars(list);
    if (cars) {
        int i = 0;
        unsigned int *src = cars;
        // In int arithmetic: the game adds allowedStart * 4 first.
        unsigned int *out = (unsigned int *)(allowedStart * 4 + (int)allowedCars);

        for (; i < 32; i++) {
            if (*src == 0)
                break;
            *out++ = *src;
            count++;
            src++;
        }
        rules->allowedCarsCount = count;
    }
    return count;
}

// Builds the rules of the 248 events. Doesn't match: the game sets $s4 to the
// high half of g_EventAllowedCars before g_EventCodes and g_EventRules.
void EventRules_BuildAll(void)
{
    int allowedStart = 0;
    int i = 0;
    char **codes = g_EventCodes;
    EventRules *rules = g_EventRules;

    do {
        char *code = *codes++;
        EventRules *r = rules++;

        i++;
        allowedStart += EventRules_Load(r, code, g_EventAllowedCars, allowedStart);
    } while (i < 248);
}

int Event_GetMaxPower(char *raceCode)
{
    EventRules *rules = &g_EventRules[Event_IndexFromCode(raceCode)];

    return rules->maxPower;
}

int Event_IsDirt(char *eventCode)
{
    EventRules *rules = &g_EventRules[Event_IndexFromCode(eventCode)];
    EventFlags flags = rules->flags;

    return flags.Dirt();
}

int Event_IsTarmac(char *eventCode)
{
    EventRules *rules = &g_EventRules[Event_IndexFromCode(eventCode)];
    EventFlags flags = rules->flags;

    return flags.Dirt() ^ 1;
}

// 1 if the event is for cars without a turbo, 2 with one, else 0.
int Event_GetTurboRule(char *eventCode)
{
    EventRules *rules = &g_EventRules[Event_IndexFromCode(eventCode)];
    EventFlags flags = rules->flags;
    int rule = 0;

    switch (flags.Turbo()) {
    case 1:
        rule = 1;
        break;
    case 2:
        rule = 2;
        break;
    }
    return rule;
}

// The license needed for event eventIdx, or -1.
LicenseId EventRules_GetRequiredLicense(int eventIdx)
{
    EventRules *rules = &g_EventRules[eventIdx];
    EventFlags flags = rules->flags;
    LicenseId license = (LicenseId)-1;

    switch (flags.License()) {
    case 1:
        license = LICENSE_B;
        break;
    case 2:
        license = LICENSE_A;
        break;
    case 3:
        license = LICENSE_IC;
        break;
    case 4:
        license = LICENSE_IB;
        break;
    case 5:
        license = LICENSE_IA;
        break;
    case 6:
        license = LICENSE_S;
        break;
    }
    return license;
}

int EventRules_GetPowerLimit(int event)
{
    EventRules *rules = &g_EventRules[event];

    return rules->maxPower;
}

int EventRules_GetPrize(int event, int place)
{
    EventRules *rules = &g_EventRules[event];

    return rules->prizes[place];
}

int EventRules_GetChampionshipPrize(int event)
{
    EventRules *rules = &g_EventRules[event];

    return rules->championshipPrize;
}

// Whether the player's current car can enter event raceCode: 1, or a negative
// reason, with the message page to show in *msgPage.
int Race_CheckEntry(char *raceCode, int *msgPage)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[0];
    int slot = g->currentCar;
    EventRules *rules;
    Car *car;
    int power;
    unsigned int carId;
    unsigned int drivetrain;
    int licenses;
    int racingMod;
    int noTurbo;
    int stock;
    int hasDirt;
    unsigned int need;
    int count;

    *msgPage = (int)0x80000002;
    if (slot < 0)
        return -1;
    if (Event_IsProLeague(raceCode)) {
        *msgPage = (int)0x8000002f;
        int won = Events_AllRegionalLeaguesWon() == 1;

        if (!won)
            return -9;
    }
    if (Event_IsWorldLeague(raceCode)) {
        *msgPage = (int)0x80000030;
        int won = Events_AllProLeaguesWon() == 1;

        if (!won)
            return -10;
    }
    rules = &g_EventRules[Event_IndexFromCode(raceCode)];
    car = &g->cars[slot];
    power = car->flags98 & 0x3fff;
    carId = car->carId;
    drivetrain = car->stat94 >> 13;
    licenses = 6 - License_GetHighest();
    racingMod = CarWork_HasRacingMod();
    noTurbo = CarWork_HasNoTurbo();
    stock = CarWork_IsStock();
    hasDirt = Car_HasPart(car, PART_TIRES_DIRT);
    if (rules->Flags().Dirt() == 1 && !hasDirt) {
        *msgPage = (int)0x80000025;
        return -2;
    }

    switch (rules->Flags().License()) {
    case 0:
        break;
    case 1:
        if (licenses < 1) {
            *msgPage = (int)0x80000013;
            return -3;
        }
        break;
    case 2:
        if (licenses < 2) {
            *msgPage = (int)0x80000012;
            return -3;
        }
        break;
    case 3:
        if (licenses < 3) {
            *msgPage = (int)0x80000014;
            return -3;
        }
        break;
    case 4:
        if (licenses < 4) {
            *msgPage = (int)0x80000016;
            return -3;
        }
        break;
    case 5:
        if (licenses < 5) {
            *msgPage = (int)0x80000015;
            return -3;
        }
        break;
    case 6:
        if (licenses < 6) {
            *msgPage = (int)0x80000017;
            return -3;
        }
        break;
    }

    *msgPage = (int)0x80000023;
    switch (rules->Flags().Drivetrain()) {
    case 0:
        break;
    case 1:
        need = 1;
        goto check;
    case 2:
        if (drivetrain != 0)
            return -4;
        break;
    case 3:
        need = 3;
        goto check;
    case 4:
        need = 4;
        goto check;
    case 5:
        need = 2;
    check:
        if (drivetrain != need)
            return -4;
        break;
    }

    switch (rules->Flags().Turbo()) {
    case 0:
        break;
    case 1:
        *msgPage = (int)0x80000026;
        if (!noTurbo)
            return -5;
        break;
    case 2:
        *msgPage = (int)0x80000027;
        if (noTurbo)
            return -5;
        break;
    }

    *msgPage = (int)0x8000001e;
    switch (rules->Flags().RacingMod()) {
    case 0:
        break;
    case 1:
        *msgPage = (int)0x80000029;
        if (racingMod)
            return -6;
        break;
    case 2:
        *msgPage = (int)0x80000028;
        if (!racingMod)
            return -6;
        break;
    case 3:
        *msgPage = (int)0x8000002c;
        if (!stock)
            return -6;
        break;
    }

    *msgPage = (int)0x8000002b;
    if (rules->maxPower > 0 && power > rules->maxPower)
        return -7;

    *msgPage = (int)0x8000001e;
    count = rules->allowedCarsCount;
    if (count > 0) {
        int i = 0;
        int start = rules->allowedCarsStart;
        int n = count;

        while (i < n) {
            unsigned int id = g_EventAllowedCars[start + i];

            if (id == carId)
                goto allowed;
            i++;
        }
        return -8;
    }
allowed:
    *msgPage = (int)0x8000001e;
    return 1;
}

// Gate for entering a license: test L* needs License_IsEarned(id+1) (B needs nothing,
// A needs B, IC needs A ... S needs IA). Returns 1 or -3.
int License_CheckEntry(char *raceCode, int *msgPage)
{
    LicenseId id = License_IdFromRaceCode2(raceCode);

    *msgPage = g_License_EntryMsgPages[id];
    if ((int)id < 5) {
        int earned = License_IsEarned(id + 1) == 1;

        if (!earned)
            return -3;
    }
    return 1;
}
}
