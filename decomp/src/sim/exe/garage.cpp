// Resident code 8005e7b0-80060924 (garage). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int CarParams_Build(CarSpec *spec, void *params);
void CarSpec_FromCarWork(CarWork *work, CarSpec *spec);
void Gearbox_ComputeRatios(CarParams *params, int topSpeedKmh, short *ratios);
extern char D_80091620[];

void Garage_AddCredits(Garage *garage, int amount)
{
    int iVar1;

    iVar1 = garage->credits + amount;
    garage->credits = iVar1;
    if (99999999 < iVar1) {
        garage->credits = 99999999;
    }
    if (garage->credits < 0) {
        garage->credits = 0;
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", Garage_AddCar);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", Car_HasPart);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", Car_InstallPart);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", PartsOwned_Set);

void CarWork_ComputeGearRatios(CarWork *work, short *ratios)
{
    CarSpec CStack_298;
    CarParams CStack_1d0;

    CarSpec_FromCarWork(work, &CStack_298);
    CarParams_Build(&CStack_298, &CStack_1d0);
    CStack_1d0.unk1aa = '\0';
    Gearbox_ComputeRatios(&CStack_1d0, (unsigned int)CStack_1d0.gearTopSpeedDiv10 * 10, ratios);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_GetGearRange);

int CarWork_TransIsCustom(CarWork *work, int stage)
{
    return (unsigned int)(work->transEntries[stage][0x20] != '\0');
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_SetPartSlot);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_GetPartIdx);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarSpec_FromCarWorkWithPart);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarSpec_FromCarWork);

void FUN_8005f788(void)
{
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_HasNaTune);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_HasTurboKits);

int CarWork_HasFullSuspension(CarWork *work)
{
    return (unsigned int)(work->partSlots[0] == 3);
}

int CarWork_HasBrakeBalance(CarWork *work)
{
    return (unsigned int)(0 < work->partSlots[2]);
}

int CarWork_HasFullTransmission(CarWork *work)
{
    return (unsigned int)(work->partSlots[6] == 3);
}

int CarWork_GetGearCount(CarWork *work)
{
    return (unsigned int)work->transEntries[work->partSlots[6]][9];
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_CountRacingModStages);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_HasLsdSettings);

int CarWork_HasYawControl(CarWork *work)
{
    return (unsigned int)(work->partSlots[0x16] == 5);
}

int CarWork_HasAsc(CarWork *work)
{
    return (unsigned int)(work->partSlots[0x14] == 1);
}

int CarWork_HasTcs(CarWork *work)
{
    return (unsigned int)(work->partSlots[0x15] == 1);
}

int CarWork_SuspHasSetting0(CarWork *work)
{
    return (unsigned int)(work->suspEntries[work->partSlots[0]][0x4b] == '\0');
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_ComputeStats);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_ApplySetting);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_GetSettingRange);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_ResetSetting);

INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", Sfx_PlayMenu);

int FUN_800608e8(unsigned int param_1, int param_2)
{
    unsigned int uVar1;
    char *pcVar2;
    int iVar3;

    iVar3 = 4;
    do {
        uVar1 = param_1 & 0x3f;
        param_1 = param_1 >> 6;
        pcVar2 = (char *)(param_2 + iVar3);
        iVar3 = iVar3 + -1;
        *pcVar2 = D_80091620[uVar1];
    } while (iVar3 != -1);
    *(unsigned char *)(param_2 + 5) = 0;
    return param_2;
}
}
