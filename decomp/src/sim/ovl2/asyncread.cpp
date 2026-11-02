// OVL2 80015834-80015ab4 (asyncread): Asynchronous CD reads.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


// Copy of AsyncRead_Init.
void AsyncRead_Init_OVL2(AsyncRead *r)
{
    r->busy = '\0';
    r->state = '\0';
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/asyncread", AsyncRead_Start_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/asyncread", AsyncRead_Stop);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/asyncread", AsyncRead_Poll_OVL2);
}
