// OVL4 80017750-800183ec (dealer): Dealer, tuning shop and garage sales.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void *GameData_GetEntry(int table, int index);
int Car_GetDefaultSetup(int carId, CarSetup *out);
int Part_Lookup(PartId partId, unsigned int carCode, int *outPrice);
extern GameState g_GameState;
extern unsigned short g_GarageCar_NoMark[2];
extern unsigned short g_GarageCar_Mark[2];
short *UniStr_Get(int index);
extern short g_PartNameStrIds;

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", CarSetup_Finalize);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Car_GetNewPrice);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Car_GetIncludedTires);

// Byte +9 of the stock transmission entry (table 0x11) < 3. Gives Car.flags98 bit 14
// of a new car; meaning unknown.
int Car_GetTransFlag14(unsigned int carId)
{
    void *pvVar1;
    CarSetup CStack_90;

    Car_GetDefaultSetup(carId, &CStack_90);
    pvVar1 = GameData_GetEntry(0x11, (unsigned int)(unsigned short)CStack_90.idx_t11);
    return (unsigned int)(*(unsigned char *)((int)pvVar1 + 9) < 3);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Dealer_CanBuyNewCar);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Dealer_BuyNewCar);

// Always 0.
int Car_GetSellPrice(unsigned int carId)
{
    return 0;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Garage_SellCar);

// -1 when the part is not available for the car.
int Part_GetPrice(unsigned int carCode, PartId partId)
{
    int iVar1;
    int local_10 [2];

    iVar1 = Part_Lookup(partId, carCode, local_10);
    if (iVar1 < 0) {
        local_10[0] = -1;
    }
    return local_10[0];
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Tune_CanBuyPart);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Tune_BuyPart);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Tune_InstallPart);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", CarWork_BuildSpecWithPart);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", GTMode_PreparePlayerCar);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", WheelShop_BuyWheels);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Garage_CanAfford);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Car_IsNotExcluded);

/* Credits >= 50. */

int CarWash_CanAfford(void)
{
    return g_GameState.garages[0].credits < 0x32 ^ 1;
}

// Mark shown before a garage car name: the glyph 0x7f when flag (bit 15 of flags98) is
// set, else an empty string.
unsigned short *GarageCar_GetMark(int flag)
{
    unsigned short *puVar1;

    puVar1 = g_GarageCar_NoMark;
    if (flag != 0) {
        puVar1 = g_GarageCar_Mark;
    }
    return puVar1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Car_GetMakerName);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", Car_GetModelName);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/dealer", CarColor_FindIndex);

// String g_PartNameStrIds[index] from the game string table (UniStr_Get).
unsigned short *PartName_GetW(int index)
{
    unsigned short *puVar1;

    puVar1 = (unsigned short *)UniStr_Get((int)(&g_PartNameStrIds)[index]);
    return puVar1;
}
}
