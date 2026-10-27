// Resident code 8007f5a8-8007f7f4 (mcformat). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
unsigned char McFrame_Checksum(unsigned int *frame);
void *memset(void *__s, int __c, size_t __n);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcformat", McCard_StartFormat);

void McFrame_InitHeader(void *frame)
{
    unsigned char uVar1;

    memset(frame, 0, 0x80);
    *(unsigned short *)frame = 0x434d;
    uVar1 = McFrame_Checksum(frame);
    *(unsigned char *)((int)frame + 0x7f) = uVar1;
}

void McFrame_InitBadSector(int *frame, int sector)
{
    unsigned char uVar1;

    memset(frame, 0, 0x80);
    *frame = sector;
    uVar1 = McFrame_Checksum((unsigned int *)frame);
    *(unsigned char *)((int)frame + 0x7f) = uVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcformat", McOp_Format);
}
