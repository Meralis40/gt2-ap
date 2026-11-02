// OVL4 80019f5c-8001a454 (battletable): Battle opponent table. GTBattleMenu
// evaluates every GT race data car (one per frame), sorts them by score, then
// picks the opponent pool around the player's car.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern void *g_GtModeRaceData;

// weight * 300000 / power^2 (0x88b9 when power is 0). Lower is faster.
int Battle_WeightPowerScore(int power, int weight)
{
    int score = 0x88b9;
    int p2 = power * power;
    int w = weight * 300000;

    if (p2 > 0) {
        score = w / p2;
    }
    return score;
}

// Clamped to 0xfffe.
void BattleCarStat_SetScore(BattleCarStat *s, unsigned int score)
{
    if (0xfffe < score) {
        score = 0xfffe;
    }
    s->score = (unsigned short)score;
}

// Sets flag bit 0, bits 1..3 and bit 4.
void BattleCarStat_SetFlags(BattleCarStat *s, int racingMod, int drivetrain, int noTurbo)
{
    s->flags = ((s->flags & 0xfe | (unsigned char)racingMod) & 0xf1 | (unsigned char)(drivetrain << 1)) & 0xef |
                          (unsigned char)(noTurbo << 4);
}

// loaded = 0, count = GT race data +0x16.
void BattleTable_Init(BattleCarTable *t)
{
    void *pvVar1;

    pvVar1 = g_GtModeRaceData;
    t->loaded = 0;
    t->count = *(short *)((int)pvVar1 + 0x16);
}

// Evaluates the next car with its default setup: power / 10, the score and the
// flags (racing modification, drivetrain, no turbo). Returns 1 when all are done.
int BattleTable_LoadNext(BattleCarTable *t)
{
    int i = t->loaded;
    int done = 1;

    if (i < t->count) {
        BattleCarStat *s = &t->cars[i];
        CarSetup setup;
        CarParams params;
        EnginePerf perf;
        int racingMod;

        CarSetup_InitFromCarEntry(GameData_GetCarEntry(i + 1), &setup);
        CarParams_BuildFromSetup(&setup, &params);
        CarParams_GetPerformance(&params, &perf);
        racingMod = ((unsigned char *)GameData_GetEntry(5, (unsigned short)setup.idx_t05))[0xe] != 0;
        int power = perf.maxPower;

        s->powerDiv10 = power / 10;
        BattleCarStat_SetScore(s, Battle_WeightPowerScore(perf.maxPower, params.weight));
        BattleCarStat_SetFlags(s, racingMod, params.drivetrain, setup.turbo == 0);
        t->loaded++;
        done = 0;
    }
    return done;
}

// Binary-search insert of car into sorted[0..count).
void BattleTable_InsertSorted(BattleCarTable *t, int car, int count)
{
    int lo = 0;
    int hi = count;
    int pos = count;
    int score = (t->cars + car)->score;

    if (count > 0) {
        while (1) {
            pos = (lo + hi) >> 1;
            if (score < (t->cars + t->sorted[pos])->score) {
                hi = pos;
                if (lo == pos) {
                    break;
                }
                continue;
            }
            if (lo == pos) {
                pos = hi;
                break;
            }
            lo = pos;
        }
    }
    for (; pos < count; count--) {
        t->sorted[count] = t->sorted[count - 1];
    }
    t->sorted[pos] = car;
}

// Insertion sort of the car indices by score.
void BattleTable_Sort(BattleCarTable *t)
{
    int i = 0;
    int count = i;
    int j;

    for (; i < t->count; i++) {
        t->sorted[i] = -1;
    }
    for (j = 0; j < t->count; j++) {
        BattleTable_InsertSorted(t, j, count);
        count++;
    }
}

// Finds the player's score in the sorted list and steps below places down, then
// takes poolSize - below cars going down from there, then going up, keeping only
// the cars whose racing modification flag is the player's.
#ifdef NON_MATCHING
// Registers: the game keeps the count of cars taken in $a0 and the pool pointer
// in $a1, GCC the other way round. Tried: n declared first, last or at its first
// use, the count of cars in a local or not, a pointer to carIndex, n++ on its own,
// the two branches joined with a goto, and a while loop for the first pass.
void BattleTable_PickOpponents(BattleCarTable *t, BattlePlayer *player, OpponentPool *pool, int poolSize,
                               int below)
{
    int score = Battle_WeightPowerScore(player->power, player->weight);
    int want = poolSize - below;
    int start = 0;
    int racingMod = player->racingMod != 0;
    int count = t->count;
    int n;
    int i;

    while (1) {
        if (start >= count) {
            break;
        }
        if ((t->cars + t->sorted[start])->score > score) {
            break;
        }
        start++;
    }
    start -= below;
    if (start < 0) {
        start = 0;
    }
    n = 0;
    if (start >= t->count) {
        start = t->count - 1;
    }
    for (i = start; i >= 0; i--) {
        int car = t->sorted[i];

        if (!racingMod) {
            if (!((t->cars + car)->flags & 1)) {
                pool->carIndex[n++] = car;
            }
        } else {
            if ((t->cars + car)->flags & 1) {
                pool->carIndex[n++] = car;
            }
        }
        if (n >= want) {
            goto done;
        }
    }
    for (i = start; i < t->count; i++) {
        int car = t->sorted[i];

        if (!racingMod) {
            if (!((t->cars + car)->flags & 1)) {
                pool->carIndex[n++] = car;
            }
        } else {
            if ((t->cars + car)->flags & 1) {
                pool->carIndex[n++] = car;
            }
        }
        if (n >= want) {
            break;
        }
    }
done:
    pool->count = n;
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/battletable", BattleTable_PickOpponents);
#endif
}
