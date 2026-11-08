// Resident code 800680a0-800683fc (spufile): loading a sound file into RAM and SPU RAM.
#include "common.h"

extern "C" {
extern CdAudio (&g_CdAudio);
extern SpuFileLoad (&g_SpuFileLoad);
extern unsigned short (&g_VolBootIds)[248];
extern unsigned int (&g_VolLba);

// One sector of a sound file: the header goes to RAM (relocated once it is all
// there), the rest to SPU RAM in multiples of 16 bytes, with the remainder carried
// to the front of the sector buffer for the next one. Returns 1 once the file is done.
// 1 word: the game computes the DMA'd part (left - carry) after copying left to the
// size argument, and from that copy; GCC computes it first.
int SpuFile_ProcessSector(SpuFileLoad *l)
{
    uchar *buf = l->sectorBuf;
    unsigned int left = 0x800;
    unsigned int rem;
    unsigned int n;
    unsigned int carry;
    uchar *ram;
    int spu;

    l->started = 1;
    switch (l->phase) {
    case 0:
        l->headerSize = l->headerLeft = ((int *)buf)[5];
        l->spuLeft = ((int *)buf)[4];
        l->carry = 0;
        l->phase = 1;
    case 1:
        n = l->headerLeft;
        ram = l->ramCur;
        rem = n;
        if (n > left) {
            n = left;
        }
        l->ramCur = ram + n;
        l->headerLeft = rem - n;
        bcopy(buf, ram, n);
        if (l->headerLeft != 0) {
            return 0;
        }
        if (!l->isEngine) {
            Inst_Relocate((SeqBank *)l->ramBase, l->spuCur);
        } else {
            Engn_Relocate((EngnFile *)l->ramBase, l->spuCur);
        }
        buf += n;
        left -= n;
        l->phase = 2;
    case 2:
        carry = l->carry;
        rem = l->spuLeft;
        spu = l->spuCur;
        left += carry;
        buf -= carry;
        if (left > rem) {
            left = rem;
        }
        carry = left & 0xf;
        l->carry = carry;
        n = left - carry;
        l->spuCur = spu + n;
        l->spuLeft = rem - n;
        Spu_DmaTransfer(buf, spu, left, 0);
        Spu_DmaWait();
        bcopy(buf + n, l->sectorBuf - carry, carry);
        if (l->spuLeft != 0) {
            return 0;
        }
        l->phase = 3;
    case 3:
        return 1;
    }
    return 0;
}

// Sector function of the sound file read.
// 3 words: as RaceFont_OnSector in font.cpp, the game moves its arguments up a register
// and loads &g_CdAudio into $a0 before the prologue, as for an inlined function of
// g_CdAudio. An inline helper and an inline member function don't give it.
int SpuFile_OnSector(unsigned int sector, void *header)
{
    CdAudio *cd = &g_CdAudio;
    SpuFileLoad *l = &g_SpuFileLoad;
    int ret = 8;

    cd->curSector = cd->nextSector;
    if (sector == cd->nextSector) {
        cd->nextSector = sector + 1;
        ret = 0;
        CdGetSector(g_SpuFileLoad.sectorBuf, 0x200);
        if (SpuFile_ProcessSector(l)) {
            cd->unk167 = 0;
            ret = 2;
        }
    }
    return ret;
}

void SpuFile_OnRetry(void)
{
    g_SpuFileLoad.phase = '\0';
    g_SpuFileLoad.ramCur = g_SpuFileLoad.ramBase;
    g_SpuFileLoad.spuCur = g_SpuFileLoad.spuBase;
}

// SpuFile_Load for boot file index id.
// A handwritten neg $a0 that runs on into SpuFile_Load: GCC falls into no function.
int SpuFile_LoadNegId(int id, SeqBank *ram, int spu, int isEngine)
{
    return SpuFile_Load(-id, (uchar *)ram, spu, isEngine);
}

// Reads sound file id (a boot file index when negative) through a sector buffer on
// the stack: its header to ram, its samples to SPU RAM at spu. Returns the header
// size.
int SpuFile_Load(int id, uchar *ram, int spu, int isEngine)
{
    uchar buf[0x10 + 0x800]; /* room for a sector's remainder in front of the next */
    SpuFileLoad *l = &g_SpuFileLoad;
    unsigned int start;
    unsigned int size;

    if (id < 0) {
        id = g_VolBootIds[-id];
    }
    start = g_VolLba + Vol_GetSector(id);
    size = Vol_GetSize(id);
    g_SpuFileLoad.sectorBuf = buf + 0x10;
    l->ramBase = ram;
    l->ramCur = ram;
    l->spuBase = spu;
    l->spuCur = spu;
    l->phase = 0;
    l->isEngine = isEngine;
    CdAudio_SetupRead(0, start, size, 0);
    g_CdAudio.onSector = (CdSectorFn *)SpuFile_OnSector;
    CdAudio_WaitRead(SpuFile_OnRetry);
    return l->headerSize;
}
}
