// Resident code 8005db90-8005e7b0 (records). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "common.h"

extern "C" {
extern GameStateRace g_GameState;

// The best finish of an event: a nibble of eventResults, the low one for even events.
int PlayerStats_GetEventResult(PlayerStats *stats, int eventIdx)
{
    int b = stats->eventResults[eventIdx / 2];

    if (eventIdx & 1) {
        return b >> 4;
    }
    return b & 0xf;
}

// Stores place if it is better than the event's result; returns 1 when stored.
int PlayerStats_SetEventResult(PlayerStats *stats, int eventIdx, int place)
{
    int cur = PlayerStats_GetEventResult(stats, eventIdx);
    int ret = 0;

    if (eventIdx < 0) {
        return ret; // through ret, where place == 0 returns 0 itself
    }
    if (place == 0) {
        return 0;
    }
    if ((cur == 0) | (place < cur)) { // both tests, then an or
        int i = eventIdx / 2;
        int b = stats->eventResults[i];

        if ((eventIdx & 1) == 0) {
            b = (b & 0xf0) | place;
        } else {
            b = (b & 0xf) | (place << 4);
        }
        stats->eventResults[i] = b;
        ret = 1;
    }
    return ret;
}

void PlayerStats_AddRaceResult(PlayerStats *stats, int place)
{
    stats->finishPlaceSum = stats->finishPlaceSum + place;
    stats->finishCount = stats->finishCount + 1;
    if (place == 1) {
        stats->racesWon = stats->racesWon + 1;
    }
}

void Stats_AddWinnings(void *stats, int amount)
{
    unsigned int uVar1;
    int iVar2;

    uVar1 = *(int *)((int)stats + 0x5c) + amount;
    *(unsigned int *)((int)stats + 0x5c) = uVar1;
    if (100000000 < uVar1) {
        iVar2 = *(int *)((int)stats + 0x58) + 1;
        *(int *)((int)stats + 0x58) = iVar2;
        if (iVar2 < 0) {
            *(unsigned int *)((int)stats + 0x58) = 0x7fffffff;
        }
        *(int *)((int)stats + 0x5c) = *(int *)((int)stats + 0x5c) + -100000000;
    }
}

// Flag 4 not set: a win with flags 2 still counts.
static inline int ArcadeCourse_CanCount(unsigned int v)
{
    return (v >> 2 & 1) ^ 1; // an xori; ! or == 0 is folded into the branch
}

// An arcade race won: ORs flags into the course's byte. Flags 2 also counts the win
// in bits 3-5, up to 5, which sets flags 2 and 4.
void PlayerStats_SetArcadeCourseFlags(PlayerStats *stats, int course, unsigned int flags)
{
    if (course >= 0) {
        if (course < 32) { // nested: in one condition GCC makes it one unsigned test
            unsigned char *p = &stats->arcadeCourses[course];
            int old = *p;
            unsigned int v = old | flags;

            *p = v;
            if (ArcadeCourse_CanCount(v)) {
                if (flags == 2) {
                    int n = (v & 0x38) >> 3;

                    n++; // in one expression, n is computed in v0
                    if (n >= 5) {
                        n = 5;
                        *p = old | 6;
                    }
                    *p = (*p & 0xc7) | (n << 3);
                }
            }
        }
    }
}

void TimeRecord_Init(TimeRecord *rec)
{
    int i;
    unsigned int empty;

    rec->time = -1;
    empty = -1; // set here, its copy comes before the loop count (found with permuter)
    for (i = 3; i >= 0; i--) {
        rec->splits[i] = empty; // splits[3] is unk10 and unk12
    }
    rec->unk10 = 0;
}

// The time of sector 0-3: from the previous sector line to this one, the last one
// ending at time; -1 when either is missing.
int Records_GetSectorTime(TimeRecord *t, int sector)
{
    unsigned int end = t->time;
    unsigned int start = 0;

    if (sector < 3) {
        end = t->splits[sector];
    }
    sector--;
    if (sector >= 0) {
        start = t->splits[sector];
    }
    if ((end == (unsigned int)-1) & (start != (unsigned int)-1)) { // both tests, then an and
        end = t->time;
    }
    if (end == start) {
        end = -1;
    }
    if ((end == (unsigned int)-1) | (start == (unsigned int)-1)) { // and an or
        return -1;
    }
    return end - start;
}

void LicenseTest_Init(LicenseTest *test)
{
    int i;

    test->unk0 = 0;
    test->medal = 0;
    test->nearMisses = 0;
    for (i = 0; i < 5; i++) {
        TimeRecord_Init(&test->bestTimes[i]);
        test->bestNames[i][0] = 0;
    }
}

// The inline helpers give the game's code: the test of -1 as a nor, and the
// record's address added to the index (not a pointer stepped through the array).
static inline int TimeRecord_IsEmpty(TimeRecord *rec)
{
    return rec->time == (unsigned int)-1;
}

// a is faster than b. Written as b > a, b's time is loaded first.
static inline int TimeRecord_IsFaster(TimeRecord *a, TimeRecord *b)
{
    return b->time > a->time;
}

// The rank 0-4 time would take in the top 5, or -1.
int LicenseTest_FindRank(LicenseTest *test, TimeRecord *time)
{
    int i;

    if (TimeRecord_IsEmpty(time)) {
        return -1;
    }
    for (i = 0; i < 5; i++) {
        if (TimeRecord_IsEmpty(test->bestTimes + i)) {
            return i;
        }
        if (TimeRecord_IsFaster(time, test->bestTimes + i)) {
            return i;
        }
    }
    return -1;
}

// Inserts time and driverName in the top 5, moving the lower ones down.
void LicenseTest_InsertTime(LicenseTest *test, TimeRecord *time, char *driverName)
{
    int rank = LicenseTest_FindRank(test, time);
    int i;

    for (i = 3; i >= rank; i--) {
        strcpy(test->bestNames[i + 1], test->bestNames[i]);
        test->bestTimes[i + 1] = test->bestTimes[i];
    }
    test->bestTimes[rank] = *time;
    strcpy(test->bestNames[rank], driverName);
}

void MachineTestRecords_SetName(MachineTestRecords *records, unsigned int rank, char *name)
{
    if (rank < 8) {
        strcpy(records->entries[rank].name, name);
    }
}

// Through a helper, the entry's address is added to the index, as in the game.
static inline void MachineTestEntry_Clear(MachineTestEntry *e)
{
    e->result = -1;
}

void MachineTestRecords_Init(MachineTestRecords *records)
{
    int i;

    memset(records, 0, sizeof(MachineTestRecords));
    for (i = 0; i < 8; i++) {
        MachineTestEntry_Clear(&records->entries[i]);
    }
}

static inline int MachineTest_IsBetter(unsigned int old, unsigned int value, int higherIsBetter)
{
    if (higherIsBetter) {
        return old < value;
    }
    return value < old;
}

// Inserts entry in the 8 places, replacing the entry of the same car if it is better.
// Returns its rank, or -1. GCC steps a pointer through the entries where the game adds
// them to an offset, and allocates other registers.
int MachineTest_InsertRecord(MachineTestRecords *records, MachineTestEntry *entry, int higherIsBetter)
{
    int i;
    int j;

    if ((unsigned char)records->count > 8) {
        records->count = 0;
    }
    for (i = 0; i < records->count; i++) {
        if (entry->carId == records->entries[i].carId) {
            if (!MachineTest_IsBetter(records->entries[i].result, entry->result, higherIsBetter)) {
                return -1;
            }
            for (j = i + 1; j < records->count; j++) {
                records->entries[j - 1] = records->entries[j];
            }
            records->entries[records->count - 1].result = -1;
            records->count--;
            break;
        }
    }
    for (i = 0; i < records->count; i++) {
        if (MachineTest_IsBetter(records->entries[i].result, entry->result, higherIsBetter)) {
            goto insert;
        }
    }
    if (i >= 8) {
        return -1;
    }
insert:
    for (j = 6; j >= i; j--) {
        records->entries[j + 1] = records->entries[j];
    }
    records->entries[i] = *entry;
    if (records->count < 8) {
        records->count++;
    }
    return i;
}

void RaceRecord_Init(RaceRecord *rec)
{
    int i;

    rec->unk00 = 0;
    rec->lapCount = 0;
    rec->lapsKept = 0;
    rec->bestLap = -1;
    TimeRecord_Init(&rec->best);
    TimeRecord_Init(&rec->current);
    for (i = 0; i < 10; i++) {
        TimeRecord_Init(&rec->laps[i]);
    }
    rec->unkf8 = -1;
}

// The time of lap, or -1 when it isn't kept.
unsigned int Records_GetLapTime(RaceRecord *rec, int lap)
{
    lap -= rec->lapCount - rec->lapsKept;
    if (lap < 0) {
        return -1;
    }
    if (lap >= rec->lapsKept) {
        return -1;
    }
    return (rec->laps + lap)->time;
}

// Adds lap: kept with the last 10, and as the best if it is. An invalid lap is -1.
void RaceRecord_AddLap(RaceRecord *rec, int lap, unsigned int lapTime, unsigned short extra, int invalid)
{
    int better;

    if (invalid) {
        lapTime = -1;
    }
    if (rec->lapCount == lap) {
        return;
    }
    rec->current.time = lapTime;
    rec->current.unk10 = extra;
    if (++rec->lapsKept >= 11) {
        *(TimeRecords9 *)&rec->laps[0] = *(TimeRecords9 *)&rec->laps[1];
        rec->lapsKept = 10;
    }
    better = 0;
    rec->laps[rec->lapsKept - 1] = rec->current;
    if (rec->current.time != (unsigned int)-1) {
        better = rec->current.time < rec->best.time;
    }
    if (better) {
        rec->bestLap = rec->lapCount;
        rec->best = rec->current;
    }
    if (++rec->lapCount >= 1000) {
        rec->lapCount = 999;
    }
    TimeRecord_Init(&rec->current);
}

void RaceSetup_SetEventCode(RaceSetup *setup, char *code)
{
    memset(setup->eventCode, 0, 0x10);
    strcpy(setup->eventCode, code);
}

void RaceSetup_SetCourse(RaceSetup *setup, unsigned int hash)
{
    unsigned int index;
    unsigned int *puVar1;

    setup->courseHash = hash;
    index = Course_FindByHash(hash);
    puVar1 = (unsigned int *)Course_Get(index);
    memset(setup->courseName, 0, 0x20);
    strcpy(setup->courseName, (char *)*puVar1);
}

void RaceSetup_SetCourseByName(RaceSetup *setup, char *name)
{
    unsigned int hash;

    hash = Str_Hash(name);
    RaceSetup_SetCourse(setup, hash);
}

void RaceResult_Init(void *res)
{
    memset(res, 0, 0xc);
    *(unsigned int *)((int)res + 4) = 0xffffffff;
}

void ChampRaceList_Clear(ChampRaceList *list)
{
    memset(list, 0, 0x94);
}

void ChampRaceList_AddPoints(ChampRaceList *list)
{
    int i;

    for (i = 0; i < 6; i++) {
        list->points[i] += list->racePoints[i];
    }
}

// Sorts the 6 cars in order by points, highest first (an insertion sort).
void ChampRaceList_SortStandings(ChampRaceList *list, signed char *order)
{
    int i;

    for (i = 0; i < 6; i++) {
        int p = list->points[i];
        int pos = i;
        int j;

        for (j = 0; j < i; j++) {
            if (list->points[order[j]] < p) {
                int k;

                for (k = i; k > j; k--) {
                    order[k] = order[k - 1];
                }
                pos = j;
                break;
            }
        }
        order[pos] = i;
    }
}

void FUN_8005e73c(void *param_1)
{
    memset(param_1, 0, 0x15a5c);
}

// The record of the race's course.
CourseRecord *CourseRecord_GetCurrent()
{
    GameStateRace *gs = &g_GameState;
    RaceSetup *race = &gs->race; // 0xbf7c, then the field: 0xbfbc is no valid offset

    return &gs->courseRecords[Course_FindByHash(race->courseHash)];
}
}
