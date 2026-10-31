// OVL4 80019f5c-8001a454 (battletable): Battle opponent table.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
extern void *g_GtModeRaceData;

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/battletable", Battle_WeightPowerScore);

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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/battletable", BattleTable_LoadNext);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/battletable", BattleTable_InsertSorted);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/battletable", BattleTable_Sort);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/battletable", BattleTable_PickOpponents);
}
