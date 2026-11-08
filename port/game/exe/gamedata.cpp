// Resident code 80077d5c-80078370 (gamedata). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "gptr.h"
#include "common.h"

// The table of race events, in the base data and the GT mode race data.
#define GAMEDATA_EVENTS 30

extern "C" {

extern int (&g_GameDataMode);
extern game_pointer<GameData> (&g_GtModeRaceData);

// Entry index of table.
void *GameData_GetEntryIn(GameData *data, int table, int index)
{
    return GameDataTable_GetEntry(&data->tables[table], index);
}

// String index of table's string pool.
char *GameData_GetTableString(GameData *data, int table, int index)
{
    return StrPool_Get((char *)data->tables[table + (data->tableCount >> 1)].entries, index);
}

// The entry of table whose first word is id (the table is sorted by it), or 0.
void *GameData_FindEntryById(GameData *data, int table, unsigned int id)
{
    int lo = -1;
    int hi = data->tables[table].count;
    int mid;
    unsigned int *e;

    do {
        mid = (lo + hi) >> 1;
        e = (unsigned int *)GameData_GetEntryIn(data, table, mid);
        if (*e == id) return e;
        if (*e < id) {
            lo = mid;
        } else {
            hi = mid;
        }
    } while (lo + 1 != hi);
    return 0;
}

// The index of that entry, or -1.
int GameData_BinarySearch(GameData *data, int table, unsigned int id)
{
    int lo = -1;
    int hi = data->tables[table].count;
    int mid;
    unsigned int v;

    do {
        mid = (lo + hi) >> 1;
        v = *(unsigned int *)GameData_GetEntryIn(data, table, mid);
        if (v == id) return mid;
        if (v < id) {
            lo = mid;
        } else {
            hi = mid;
        }
    } while (lo + 1 != hi);
    return -1;
}

void *GameDataTable_GetEntry(GameDataTable *table, int index)
{
    return (void *)rt::from_guest(((int)table->entries + (unsigned int)table->entrySize * index));
}

// GameData_FindEntryById for a table on its own.
void *GameDataTable_FindById(GameDataTable *table, unsigned int id)
{
    int lo = -1;
    int hi = table->count;
    int mid;
    unsigned int *e;

    do {
        mid = (lo + hi) >> 1;
        e = (unsigned int *)GameDataTable_GetEntry(table, mid);
        if (*e == id) return e;
        if (*e < id) {
            lo = mid;
        } else {
            hi = mid;
        }
    } while (lo + 1 != hi);
    return 0;
}

void Car_SetupFromEntry(void *entry, CarSetup *out)
{
    CarSetup_FromCarEntry(entry, out);
}

void FUN_80078018(void *param_1, CarSetup *param_2)
{
    CarSetup_InitFromCarEntry(param_1, param_2);
}

void *GameDataTable_At(GameDataTable *table, int index)
{
    return (void *)rt::from_guest(((int)table->entries + (unsigned int)table->entrySize * index));
}

// The index of the entry whose first word is id, or -1. Like GameData_BinarySearch,
// but the bounds start at 0 and count.
int GameDataTable_SearchId(GameDataTable *table, unsigned int id)
{
    int lo = 0;
    int hi = table->count;
    int mid;
    unsigned int v;

    for (;;) {
        mid = (lo + hi) >> 1;
        v = *(unsigned int *)GameDataTable_At(table, mid);
        if (id == v) return mid;
        if (lo == hi) return -1;
        if (v < id) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
}

void *GameDataTable_FindId(GameDataTable *table, unsigned int id)
{
    int i = GameDataTable_SearchId(table, id);

    if (i < 0) return 0;
    return GameDataTable_At(table, i);
}

// Number of ids before the first 0 (at most 16).
int IdList_Count(IdList *list)
{
    int n;
    unsigned int *id;

    n = 0;
    id = list->ids;
loop:
    if (n < 16 && *id++ != 0) {
        n++;
        goto loop;
    }
    return n;
}

// The race event string pool: the GT mode race data's first table's, in GT mode.
static inline void *GameData_EventStrings(GameData *data)
{
    if (g_GameDataMode == 2) {
        return g_GtModeRaceData->tables[g_GtModeRaceData->tableCount >> 1].entries;
    }
    return (&data->tables[GAMEDATA_EVENTS])[data->tableCount >> 1].entries;
}

static inline GameDataTable *GameData_Events(GameData *data)
{
    if (g_GameDataMode == 2) {
        return &g_GtModeRaceData->tables[0];
    }
    return &data->tables[GAMEDATA_EVENTS];
}

// String id of the race event string pool.
char *GameData_GetString(GameData *data, int id)
{
    return StrPool_Get((char *)GameData_EventStrings(data), id);
}

// The index of the race event whose code is code, or -1. The events are sorted by
// the code's string id.
int GameData_FindEventIndex(GameData *data, char *code)
{
    int str = StrPool_Find((unsigned short *)GameData_EventStrings(data), code);
    GameDataTable *t;
    GameDataTable *events;
    int lo, hi, mid;
    int v;

    if (str < 0) return -1;
    // A copy of the table pointer, made after lo is set (found with permuter).
    t = GameData_Events(data);
    lo = 0;
    events = t;
    hi = events->count;
    for (;;) {
        mid = (lo + hi) >> 1;
        v = *(unsigned short *)GameDataTable_At(events, mid);
        if (str == v) return mid;
        if (lo == hi) return -1;
        if (v < str) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
}

void *GameData_FindEvent(GameData *data, char *code)
{
    int i = GameData_FindEventIndex(data, code);

    if (i >= 0) return GameDataTable_At(GameData_Events(data), i);
    return 0;
}
}
