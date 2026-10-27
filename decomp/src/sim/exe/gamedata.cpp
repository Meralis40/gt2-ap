// Resident code 80077d5c-80078370 (gamedata). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CarSetup_FromCarEntry(void *carEntry, CarSetup *setup);
void CarSetup_InitFromCarEntry(void *carEntry, CarSetup *setup);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameData_GetEntryIn);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameData_GetTableString);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameData_FindEntryById);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameData_BinarySearch);

void *GameDataTable_GetEntry(GameDataTable *table, int index)
{
    return (void *)((int)table->entries + (unsigned int)table->entrySize * index);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameDataTable_FindById);

void Car_SetupFromEntry(void *entry, CarSetup *out)
{
    CarSetup_FromCarEntry(entry, out);
}

void FUN_80078018(void *param_1, CarSetup *param_2)
{
    CarSetup_InitFromCarEntry(param_1, param_2);
}

void *GameDataTable_At(GameDataTable *table, int index)
{
    return (void *)((int)table->entries + (unsigned int)table->entrySize * index);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameDataTable_SearchId);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameDataTable_FindId);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", IdList_Count);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameData_GetString);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameData_FindEventIndex);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gamedata", GameData_FindEvent);
}
