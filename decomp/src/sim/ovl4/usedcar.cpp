// OVL4 800224b0-800226a0 (usedcar): Used car tables.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
extern UsedCarPeriod g_OVL4_StorageUsedCar;

UsedCarEntry *UsedCar_GetMakerCars(UsedCarPeriod *period, int makerId)
{
    return (UsedCarEntry *)((int)&period->makers[0].offset + (unsigned int)period->makers[makerId].offset);
}

unsigned short *UsedCar_GetMakerCount(UsedCarPeriod *period, int makerId)
{
    return &period->makers[makerId].count;
}

UsedCarPeriod *UsedCar_GetPeriod(void)
{
    return &g_OVL4_StorageUsedCar;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/usedcar", UsedCar_LoadPeriod);

// Used cars of makerId for the current period (UsedCar_GetPeriod).
void UsedCarList_GetCars(int makerId, unsigned int *count, UsedCarEntry **cars)
{
    UsedCarPeriod *period;
    unsigned short *puVar1;
    UsedCarEntry *pUVar2;

    period = UsedCar_GetPeriod();
    puVar1 = UsedCar_GetMakerCount(period, makerId);
    *count = (unsigned int)*puVar1;
    pUVar2 = UsedCar_GetMakerCars(period, makerId);
    *cars = pUVar2;
}
}
