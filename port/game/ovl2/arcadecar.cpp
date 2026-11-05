// OVL2 80010000-80010074 (arcadecar): Arcade car entries.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
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

// The code of a colour of carId.
int ArcadeCar_GetColorCode(unsigned int carId, int color)
{
    unsigned short *rgb;
    char *codes;

    CarInfoA_GetColors(carId, &rgb, &codes);
    return ((signed char *)codes)[color];
}
}
