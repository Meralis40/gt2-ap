// Resident code 8007ab14-8007ad58 (cdread). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CdStream_ReleaseSector(...);
unsigned char *CdStream_WaitSector(...);
int Cd_ReadInflate(void *dst, int fileOffset, int size, void *sectorBuf, int arg4, void *heap, int heapSize);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", CdAudio_WaitRead);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", Cd_ReadSync);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", Cd_ReadBytes);

InflateState *Cd_InflateRefill(InflateState *state)
{
    unsigned short *puVar1;

    CdStream_ReleaseSector();
    puVar1 = (unsigned short *)CdStream_WaitSector();
    state->in = puVar1;
    state->inEnd = puVar1 + 0x400;
    return state;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", FUN_8007abf8);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", Cd_ReadInflate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", FUN_8007ad1c);

void Cd_ReadAtAsync(void *dst, unsigned int byteOffset, int size)
{
    unsigned char auStack_7008 [16384];
    unsigned char auStack_3008 [12288];

    Cd_ReadInflate(dst, byteOffset, size, auStack_7008, 8, auStack_3008, 0x3000);
}
}
