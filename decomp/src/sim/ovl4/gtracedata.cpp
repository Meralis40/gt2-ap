// OVL4 80021a40-80021c38 (gtracedata): Race data events and wheels.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void *GameData_GetEntryIn(void *data, int table, int index);
extern void *g_GtModeRaceData;
int GameData_BinarySearch(void *data, int table, unsigned int carCode);
extern unsigned int g_GtModeData;

// Entry index of table 0 of g_GtModeRaceData.
unsigned short *GtRaceData_GetEvent(int index)
{
    unsigned short *puVar1;

    puVar1 = GameData_GetEntryIn(g_GtModeRaceData, 0, index);
    return puVar1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtracedata", GtRaceData_FindEvent);

int Wheel_FindIndex(int wheelCode)
{
    int iVar1;

    iVar1 = GameData_BinarySearch(g_GtModeData, 0x1d, wheelCode);
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtracedata", CarSetup_SetWheels);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtracedata", Wheel_GetVariant);
}
