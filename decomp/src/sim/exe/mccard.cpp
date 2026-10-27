// Resident code 8007d32c-8007da44 (mccard). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
extern void *g_McOp_Port;

INCLUDE_ASM("asm/sim/exe/nonmatchings/mccard", McHeader_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mccard", McHeader_SetTitle);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mccard", McCard_FindFile);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mccard", McCard_StartCreate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mccard", McOp_Create);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mccard", McCard_StartTransfer);

void FUN_8007d7a0(void)
{
    *(unsigned int *)g_McOp_Port = 0;
}

unsigned int McOp_FileToCardSector(void *op, unsigned int fileSector)
{
    return (int)*(signed char *)((int)op + ((int)fileSector >> 6) + 5) << 6 | fileSector & 0x3f;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/mccard", McCard_GetResult);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mccard", McOp_Transfer);
}
