// Resident code 800680a0-800683fc (spufile). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
extern SpuFileLoad g_SpuFileLoad;

INCLUDE_ASM("asm/sim/exe/nonmatchings/spufile", SpuFile_ProcessSector);

INCLUDE_ASM("asm/sim/exe/nonmatchings/spufile", SpuFile_OnSector);

void SpuFile_OnRetry(void)
{
    g_SpuFileLoad.phase = '\0';
    g_SpuFileLoad.ramCur = g_SpuFileLoad.ramBase;
    g_SpuFileLoad.spuCur = g_SpuFileLoad.spuBase;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/spufile", SpuFile_LoadNegId);

INCLUDE_ASM("asm/sim/exe/nonmatchings/spufile", SpuFile_Load);
}
