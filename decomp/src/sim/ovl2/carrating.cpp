// OVL2 80019e60-80019fe4 (carrating): Car ratings.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carrating", CarRating_WeightPowerScore);

// Rating of one arcade car: dirt tyres fitted, stat94 >> 13, power (flags98 & 0x3fff),
// stat96, weight (stat94 & 0x1fff), and pwrClass 0..3 from CarRating_WeightPowerScore
// (< 0xe10 -> 0, < 0x170c -> 1, < 0x2ae5 -> 2, else 3).
void CarRating_Fill(ArcCarRating *r, Car *car)
{
    int iVar1;

    iVar1 = Car_HasPart(car, PART_TIRES_DIRT);
    r->dirtTires = (unsigned char)iVar1;
    r->unk03 = (unsigned char)(car->stat94 >> 0xd);
    r->power = car->flags98 & 0x3fff;
    r->unk06 = car->stat96;
    r->weight = car->stat94 & 0x1fff;
    r->pwrClass = '\x03';
    iVar1 = CarRating_WeightPowerScore((int)r->power, (int)r->weight);
    if (iVar1 < 0x2ae5) {
        r->pwrClass = '\x02';
    }
    if (iVar1 < 0x170c) {
        r->pwrClass = '\x01';
    }
    if (iVar1 < 0xe10) {
        r->pwrClass = '\0';
    }
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carrating", CarRatingTable_Build);
}
