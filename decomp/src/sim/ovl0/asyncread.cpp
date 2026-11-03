// OVL0 80048418-80048528 (asyncread): Asynchronous reads.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/asyncread", AsyncRead_Init);

// Starts a CD read (CdAudio_SetupRead) and sets state 1.
void AsyncRead_Start(AsyncRead *r, void *dst, int sector, int count)
{
    CdAudio_SetupRead(dst, sector, count, 0);
    r->state = '\x01';
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/asyncread", AsyncRead_Poll);
}
