// OVL4 80019c60-80019f5c (playerstats): Player statistics and garage totals.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
extern GameState g_GameState;

/* g_GameState.stats.racesEntered. */

int PlayerStats_GetRacesEntered(void)
{
    return g_GameState.stats.racesEntered;
}

/* g_GameState.stats.racesWon. */

int PlayerStats_GetRacesWon(void)
{
    return g_GameState.stats.racesWon;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/playerstats", PlayerStats_GetWinRate);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/playerstats", PlayerStats_GetRatio50);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/playerstats", Events_GetWinPercent);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/playerstats", PlayerStats_GetPrizeMoney);

/* g_GameState.garages[0].carCount. */

int Garage_GetCarCount(void)
{
    return (int)g_GameState.garages[0].carCount;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/playerstats", Garage_GetTotalValue);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/playerstats", Garage_FindMostPowerfulCar);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/playerstats", License_CountMedals);
}
