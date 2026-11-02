// OVL4 800183ec-80018f6c (events): Event classification, event codes and prize info.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern char *g_EventCodes[];
extern int g_EventCount;
extern short g_EventLetterIndex[256];
extern char *g_RegionalLeagueCodes[];
extern char *g_ProLeagueCodes[];
extern char g_EventCodeFmt[];
extern char *g_LicensePrizeEventFmt[6];
extern char *g_LicensePrefixes[6];
extern void *g_GtModeRaceDataBuf;
extern char D_80022E64[];   // "PFL"
extern char D_80022E68[];   // "EPL"
extern char D_80022F1C[];   // "GTW"
extern char D_80022F20[];   // "G400"
extern char D_80022F28[];   // "G1000"
extern char D_80022F30[];   // "GMAX"
extern char D_80022F38[];   // "FREERACE"
extern char D_80022F44[];   // "FREECHAMP"
extern char D_80022F50[];   // "FREERACE01"
extern char D_80022F5C[];   // "FREERACE02"
extern char D_80022F68[];   // "FREERACE03"
extern char D_80022F74[];   // "none"

// Seeds *seed from the vblank count. Taking the address here gives seed the
// first stack slot in Event_LoadPrizeInfo, before the colour pointers.
static inline void Rand_Seed(unsigned int *seed)
{
    *seed = Irq_WaitVSyncs(0);
}

// Whether raceCode is a professional league event ("PFL..." or "EPL...").
int Event_IsProLeague(char *raceCode)
{
    char prefix[8];
    int i;

    for (i = 0; i < 3; i++) {
        prefix[i] = raceCode[i];
        if (((signed char *)raceCode)[i] == 0)
            break;
    }
    prefix[i] = 0;
    if (strcmp(prefix, D_80022E64) == 0)
        return 1;
    return strcmp(prefix, D_80022E68) == 0;
}

// Whether every regional league event has been won.
int Events_AllRegionalLeaguesWon(void)
{
    GameState *gs = &g_GameState;
    int all = 1;
    PlayerStats *stats = &gs->stats;
    char *code;
    int i;

    for (i = 0; (code = g_RegionalLeagueCodes[i]) != 0; i++) {
        if (PlayerStats_GetEventResult(stats, Event_IndexFromCode(code)) != 1)
            all = 0;
    }
    return all;
}

// Whether every professional league event has been won.
int Events_AllProLeaguesWon(void)
{
    GameState *gs = &g_GameState;
    int all = 1;
    PlayerStats *stats = &gs->stats;
    char *code;
    int i;

    for (i = 0; (code = g_ProLeagueCodes[i]) != 0; i++) {
        if (PlayerStats_GetEventResult(stats, Event_IndexFromCode(code)) != 1)
            all = 0;
    }
    return all;
}

// Whether raceCode is a GT World League event ("GTW...").
int Event_IsWorldLeague(char *raceCode)
{
    char prefix[8];
    int i;

    for (i = 0; i < 3; i++) {
        prefix[i] = raceCode[i];
        if (((signed char *)raceCode)[i] == 0)
            break;
    }
    prefix[i] = 0;
    return strcmp(prefix, D_80022F1C) == 0;
}

// Whether raceCode is a license test.
int Event_IsLicense(char *raceCode)
{
    return *(signed char *)raceCode == 'L';
}

// The machine test of eventCode: 7 (400 m), 8 (1000 m), 9 (max speed) or -1.
int Event_GetMachineTestType(char *eventCode)
{
    if (strcmp(eventCode, D_80022F20) == 0)
        return 7;
    if (strcmp(eventCode, D_80022F28) == 0)
        return 8;
    if (strcmp(eventCode, D_80022F30) == 0)
        return 9;
    return -1;
}

// Whether eventCode is a free race ("FREERACE..." or "FREECHAMP...").
int Event_IsFreeRace(char *eventCode)
{
    char prefix[64];
    int i;

    for (i = 0; i < 8; i++) {
        prefix[i] = eventCode[i];
        if (((signed char *)eventCode)[i] == 0)
            break;
    }
    prefix[i] = 0;
    if (strcmp(prefix, D_80022F38) == 0)
        return 1;
    for (i = 0; i < 9; i++) {
        prefix[i] = eventCode[i];
        if (((signed char *)eventCode)[i] == 0)
            break;
    }
    prefix[i] = 0;
    return strcmp(prefix, D_80022F44) == 0;
}

// The free race of eventCode, 0-2 ("FREERACE01" to "FREERACE03"), or 3.
int Event_GetFreeRaceIndex(char *eventCode)
{
    if (strcmp(eventCode, D_80022F50) == 0)
        return 0;
    if (strcmp(eventCode, D_80022F5C) == 0)
        return 1;
    if (strcmp(eventCode, D_80022F68) == 0)
        return 2;
    return 3;
}

// Fills g_EventLetterIndex with the first event of each first letter.
void Event_BuildLetterIndex(void)
{
    int i;

    for (i = 0; i < 256; i++)
        g_EventLetterIndex[i] = g_EventCount;
    for (i = g_EventCount - 1; i >= 0; i--) {
        if (i < g_EventLetterIndex[*(signed char *)g_EventCodes[i]])
            g_EventLetterIndex[*(signed char *)g_EventCodes[i]] = i;
    }
    for (i = 0; i < 256; i++) {
        if (g_EventLetterIndex[i] >= g_EventCount)
            g_EventLetterIndex[i] = 0;
    }
}

// The index of raceCode in g_EventCodes, or 0.
int Event_IndexFromCode(char *raceCode)
{
    int i;

    for (i = g_EventLetterIndex[*(signed char *)raceCode]; i < g_EventCount; i++) {
        if (strcmp(g_EventCodes[i], raceCode) == 0)
            return i;
    }
    return 0;
}

// The value of the digit c, or 0.
int Char_DigitValue(int c)
{
    unsigned int d = c - '0';

    if (d >= 10)
        return 0;
    return d;
}

// The event code without its last 2 digits.
void EventCode_GetBase(char *out, char *code)
{
    int n = strlen(code) - 2;
    int i;

    for (i = 0; i < n; i++)
        *out++ = *code++;
    *out = 0;
}

// The code of race race of the event base ("%s%02d").
void EventCode_Format(char *out, char *base, int race)
{
    sprintf(out, g_EventCodeFmt, base, race);
}

// The race count of an event, from the last 2 digits of base.
int EventCode_GetRaceCount(char *base)
{
    int len = strlen(base);
    signed char *p;
    int count;
    int i;

    if (len < 2)
        return 0;
    p = (signed char *)base + len - 2;
    count = 0;
    for (i = 0; i < 2; i++) {
        count *= 10;
        count += Char_DigitValue(*p++);
    }
    return count;
}

// Fills list with the races of championship eventCode. Doesn't match: i and the
// param45 pointer get $s2 and $s3 the other way round, and the "none" address
// is built in $v1 instead of $a1.
#ifdef NON_MATCHING
void ChampRaceList_Build(ChampRaceList *list, char *eventCode)
{
    char base[64];
    char code[64];
    unsigned int seed;
    void *data;
    int count;
    int single;
    int i;

    ChampRaceList_Clear(list);
    data = g_GtModeRaceDataBuf;
    Rand_Seed(&seed);
    EventCode_GetBase(base, eventCode);
    count = EventCode_GetRaceCount(base);
    i = 0;
    list->raceCount = count;
    if (count > 0) {
        single = count < 2;
        do {
            RaceEvent *event;
            char *course;

            if (!single)
                EventCode_Format(code, base, i + 1);
            else
                strcpy(code, eventCode);
            event = GameData_FindEvent(data, code);
            course = GameData_GetString(data, event->courseString);
            if (strcmp(course, D_80022F74) == 0)
                course = Course_PickRandom(&seed);
            list->param45[i] = event->lapCount;
            list->courseHash[i] = Str_Hash(course);
            strcpy(list->raceCode[i], code);
            i++;
        } while (i < count);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", ChampRaceList_Build);
#endif

// Clears the 6 place prizes and the prize car count.
void EventPrize_Clear(EventPrizeInfo *info)
{
    int i;

    for (i = 5; i >= 0; i--)
        info->prizes[i] = 0;
    info->prizeCarCount = 0;
}

// The license of a license test code (0 S to 5 JB), from its first 3 letters.
int License_IndexFromCode(char *code)
{
    char prefix[16];
    int found = 0;
    int i;

    strcpy(prefix, code);
    prefix[3] = 0;
    for (i = 0; i < 6; i++) {
        if (strcmp(g_LicensePrefixes[i], prefix) == 0)
            found = i;
    }
    return found;
}

// Fills info with the prizes and prize cars of eventCode.
void Event_LoadPrizeInfo(EventPrizeInfo *info, char *eventCode)
{
    char base[64];
    char code[64];
    CarSetup setup;
    unsigned char tires[8];
    unsigned int seed;
    unsigned short *rgb;
    signed char *colors;
    void *gtMode;
    int j;
    RaceEvent *event;
    unsigned int *cars;
    int i;

    Rand_Seed(&seed);
    EventPrize_Clear(info);
    if (Event_IsLicense(eventCode)) {
        int license = License_IndexFromCode(eventCode);

        sprintf(code, g_LicensePrizeEventFmt[license], 0);
    } else {
        EventCode_GetBase(base, eventCode);
        if (EventCode_GetRaceCount(base) >= 2)
            EventCode_Format(code, base, 1);
        else
            strcpy(code, eventCode);
    }
    event = (RaceEvent *)GtRaceData_FindEvent(code);
    info->championshipPrize = event->championshipPrize * 100;
    info->prizes[0] = event->prizes[0] * 100;
    info->prizes[1] = event->prizes[1] * 100;
    info->prizes[2] = event->prizes[2] * 100;
    info->prizes[3] = event->prizes[3] * 100;
    info->prizes[4] = event->prizes[4] * 100;
    info->prizes[5] = event->prizes[5] * 100;
    // An empty loop the game keeps.
    for (j = 5; j >= 0; j--)
        ;
    cars = event->prizeCars;
    gtMode = GameData_GetGtMode();
    for (i = 0; i < 4; i++) {
        unsigned int carId = *cars++;
        int n;
        int color;
        unsigned int value;
        int racingMod;
        int flag14;
        unsigned char *carTires;

        if (carId == 0)
            break;
        n = CarInfoA_GetColors(carId, &rgb, (char **)&colors);
        color = colors[Rand_Next(&seed) % n];
        Car_GetDefaultSetup(carId, &setup);
        value = ((unsigned int *)GameData_FindEntryById(gtMode, 0x1e, carId))[0x11];
        racingMod = CarSetup_Finalize(carId, &setup);
        flag14 = Car_GetTransFlag14(carId);
        carTires = Car_GetIncludedTires(carId, tires);
        Car_Init(&info->prizeCars[i], carId, carId, color, &setup, value, racingMod, flag14, carTires);
        info->prizeCarCount++;
    }
}
}
