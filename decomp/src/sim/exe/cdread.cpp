// Resident code 8007ab14-8007ad58 (cdread): blocking CD reads, plain or through
// Inflate. Each read has a byte-offset entry and, just before it, a sector entry
// that is a single shift falling through into it.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern CdAudio g_CdAudio;

// Restarts the read until the drive reports no error, calling onRetry (if any)
// between attempts.
int CdAudio_WaitRead(void (*onRetry)(void))
{
    for (;;) {
        CdAudio_ResetRead();
        CdAudio_StartRead();
        CdAudio_WaitIdle();
        if (CdAudio_GetReadStatus() == 0) {
            return 1;
        }
        if (onRetry != 0) {
            onRetry();
        }
    }
}

#ifdef NON_MATCHING
// The game's version has no return: it turns the sector into a byte offset and
// falls through into Cd_ReadBytes, the next function.
int Cd_ReadSync(void *dst, int lba, int size)
{
    return Cd_ReadBytes(dst, lba << 11, size);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", Cd_ReadSync);
#endif

int Cd_ReadBytes(void *dst, int byteOffset, int size)
{
    CdAudio_SetupRead(dst, byteOffset >> 11, size, 0);
    byteOffset &= 0x7ff;
    g_CdAudio.reqSkip = byteOffset;
    return CdAudio_WaitRead(0);
}

InflateState *Cd_InflateRefill(InflateState *state)
{
    unsigned short *puVar1;

    CdStream_ReleaseSector();
    puVar1 = (unsigned short *)CdStream_WaitSector();
    state->in = puVar1;
    state->inEnd = puVar1 + 0x400;
    return state;
}

#ifdef NON_MATCHING
// Same as Cd_ReadSync: a shift that falls through into Cd_ReadInflate.
int FUN_8007abf8(void *dst, int lba, int size, void *sectorBuf, int bufSectors, void *heap, int heapSize)
{
    return Cd_ReadInflate(dst, lba << 11, size, sectorBuf, bufSectors, heap, heapSize);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", FUN_8007abf8);
#endif

#ifdef NON_MATCHING
// Matches but for $t0 and $t1, which hold the input pointer and the byte count
// the other way round (permuter didn't find it).
static inline void Cd_InflateInit(InflateState *s, InflateState *(*refill)(InflateState *), int avail, unsigned char *in,
                                  unsigned char *out, void *heap, int heapSize)
{
    s->yield = 0;
    if (refill == 0 || avail == 0) {
        avail = -1;
    } else {
        avail = (avail + 1) & ~1;
    }
    s->inEnd = (ushort *)(in + avail);
    s->refill = refill;
    s->inBase = in;
    s->in = (ushort *)in;
    s->flush = 0;
    s->outBase = out;
    s->out = out;
    s->outEnd = out - 1;
    s->heapBase = heap;
    s->heapEnd = (unsigned char *)heap + heapSize;
}

// Streams the sectors holding [fileOffset, fileOffset + size) through sectorBuf
// (bufSectors sectors) and inflates them into dst; the first sector's data starts
// at its offset in sectorBuf.
int Cd_ReadInflate(void *dst, int fileOffset, int size, void *sectorBuf, int bufSectors, void *heap, int heapSize)
{
    InflateState state;
    int skip;
    int avail;
    int start;
    unsigned int end;

    Stack_Check();
    skip = fileOffset & 0x7ff;
    avail = 0x800 - skip;
    start = fileOffset >> 11;
    end = (unsigned int)(fileOffset + size + 0x7ff) >> 11;
    Cd_InflateInit(&state, Cd_InflateRefill, avail, (unsigned char *)sectorBuf + skip, (unsigned char *)dst,
                   heap, heapSize);
    g_CdAudio.reqFlag = 0;
    CdAudio_SetupStream(sectorBuf, bufSectors, start, end);
    CdAudio_ResetStream();
    CdAudio_StartRead();
    CdStream_WaitSector();
    Inflate_Run(&state);
    Music_Stop();
    CdAudio_WaitIdle();
    return 1;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", Cd_ReadInflate);
#endif

#ifdef NON_MATCHING
// Same as Cd_ReadSync: a shift that falls through into Cd_ReadAtAsync.
void FUN_8007ad1c(void *dst, unsigned int lba, int size)
{
    Cd_ReadAtAsync(dst, lba << 11, size);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdread", FUN_8007ad1c);
#endif

void Cd_ReadAtAsync(void *dst, unsigned int byteOffset, int size)
{
    unsigned char auStack_7008 [16384];
    unsigned char auStack_3008 [12288];

    Cd_ReadInflate(dst, byteOffset, size, auStack_7008, 8, auStack_3008, 0x3000);
}
}
