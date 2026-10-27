// Resident code 80069890-80069ff8 (replaydata). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void *memcpy(void *__dest, void *__src, size_t __n);
extern unsigned char D_80091C8C;
extern unsigned short g_VolBootIds[248];

void *Buf_Append(void *dst, void *src, int size)
{
    memcpy(dst, src, size);
    return (void *)((int)dst + (size + 3U & 0xfffffffc));
}

void *Buf_Read(void *src, void *dst, unsigned int size)
{
    memcpy(dst, src, size);
    return (void *)((int)src + (size + 3 & 0xfffffffc));
}

void *Buf_AppendByte(unsigned char *dst, unsigned char *src)
{
    *dst = *src;
    return dst + 4;
}

signed char *Buf_ReadCharAsShort(signed char *src, short *dst)
{
    *dst = (short)*src;
    return src + 4;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydata", ReplayData_BuildRace);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydata", Replay_Unpack);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydata", ReplayData_BuildGhost);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydata", GhostReplay_Unpack);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydata", GhostEvent_ApplyFile);

int License_DemoFileId(int license, int test)
{
    return (unsigned int)g_VolBootIds[0xe1] + (signed char)(&D_80091C8C)[license] * 10 + test;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydata", Replay_RestoreCarParams);
}
