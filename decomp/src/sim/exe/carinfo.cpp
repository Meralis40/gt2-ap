// Resident code 80060924-80061010 (carinfo). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int CarInfoA_FindIndex(unsigned int carId);
int CarInfoA_GetColors(unsigned int carId, unsigned short **outRgb, char **outCodes);
int Str_FindCharNoCase(int ch, char *str, int len);
void VolId_ReadFile(VolBootId id, void *dst);
extern CarInfoA *g_CurrentCarInfoA;

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CarCode_Encode);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", FUN_8006098c);

void LoadCarInfoA(CarInfoA *buffer)
{
    g_CurrentCarInfoA = buffer;
    VolId_ReadFile(VOLID_CARINFOA, buffer);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CarInfoA_FindIndex);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CarInfoA_GetData);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CarInfoA_GetName);

unsigned int CarInfoA_GetHiddenRegions(unsigned int carId)
{
    CarInfoA *pCVar1;
    int iVar2;

    pCVar1 = g_CurrentCarInfoA;
    iVar2 = CarInfoA_FindIndex(carId);
    return pCVar1->entries[iVar2].hiddenRegions;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CarInfoA_IsAvailable);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CarInfoA_GetColors);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CarInfoA_GetFields);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", Str_FindCharNoCase);

int CarInfoA_FindColorIndex(unsigned int carId, int colorCode)
{
    int iVar1;
    unsigned short *puStack_10;
    char *local_c;

    iVar1 = CarInfoA_GetColors(carId, &puStack_10, &local_c);
    iVar1 = Str_FindCharNoCase(colorCode, local_c, iVar1);
    return iVar1;
}

unsigned short CarInfoA_GetColorRgb(unsigned int carId, int colorCode)
{
    int iVar1;
    unsigned short *local_10;
    char *local_c;

    iVar1 = CarInfoA_GetColors(carId, &local_10, &local_c);
    iVar1 = Str_FindCharNoCase(colorCode, local_c, iVar1);
    return local_10[iVar1];
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", Wheel_GetFileId);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", EngineSound_FindIndex);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", DemoRace_LoadSetup);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", Course_Get);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", Course_FindByHash);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", FUN_80060efc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CourseMap_GetFileId);
}
