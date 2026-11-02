// OVL2 80010000-80010074 (arcadecar): Arcade car entries.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern void *g_EventDataBuf;

// Car entry of carId in the arcade game data (g_EventDataBuf table +0x108, or +0x110
// for the racing modification), through GameDataTable_FindId.
void *ArcadeCar_GetEntry(unsigned int carId, int racingMod)
{
    void *pvVar1;
    GameDataTable *table;

    table = (GameDataTable *)((int)g_EventDataBuf + 0x108);
    if (racingMod == 1) {
        table = (GameDataTable *)((int)g_EventDataBuf + 0x110);
    }
    pvVar1 = GameDataTable_FindId(table, carId);
    return pvVar1;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcadecar", ArcadeCar_GetColorCode);
}
