// OVL4 80016fec-80017750 (carwork2): CarWork helpers: setup export, racing
// modification variants.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void CarWork_LoadFromGarage(int slot, int garage);
extern CarWork g_CarWork;
int CarWork_CountRacingModStages(CarWork *work);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", CarWork_ExportSetup);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", Part_GetSlot);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", FUN_OVL4__8001719c);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", CarWork_RebuildParams);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", ListCarWork_Load);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", Tune_DrawInstalledParts);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", CarWork_LoadFromGarage);

// Loads garage car slot into g_CarWork (CarWork_LoadGarageCar).
void Garage_LoadCar(int slot, int garage)
{
    CarWork_LoadFromGarage(slot, garage);
}

/* &g_CarWork. */

CarSetup *CarWork_Get(void)
{
    return &g_CarWork.setup;
}

/* Whether the current car has racing modification variants. */

int RacingMod_HasVariants(void)
{
    int iVar1;

    iVar1 = CarWork_CountRacingModStages(&g_CarWork);
    return (unsigned int)(0 < iVar1);
}

/* CarWork_CountRacingModStages(&g_CarWork): number of leading non-negative ints at g_CarWork
      +0x15e0 (max 4). Kind 0x9d hides its element when < 2 (tentative: options with a single choice).
        */

int CarWork_CountSlots(void)
{
    int iVar1;

    iVar1 = CarWork_CountRacingModStages(&g_CarWork);
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", RacingMod_FirstVariant);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carwork2", RacingMod_NextVariant);

/* g_CarWork.turbo == 0 */

int CarWork_HasNoTurbo(void)
{
    return (unsigned int)(g_CarWork.setup.turbo == 0);
}

/* Part slot 0x13 (racing modification) fitted. */

int CarWork_HasRacingMod(void)
{
    return (unsigned int)(0 < g_CarWork.partSlots[0x13]);
}

/* No part fitted in any tuning slot (slots 3-5 ignored). */

int CarWork_IsStock(void)
{
    unsigned int uVar1;

    uVar1 = (unsigned int)(g_CarWork.partSlots[0] < 1);
    if (0 < g_CarWork.partSlots[1]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[2]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[6]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[7]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[8]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[9]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[10]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xb]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xc]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xd]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xe]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xf]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x10]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x11]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x12]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x13]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x14]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x15]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x16]) {
        uVar1 = 0;
    }
    return uVar1;
}
}
