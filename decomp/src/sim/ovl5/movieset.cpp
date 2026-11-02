// Player setup: GPU DMA hook, frame ring, buffers and the movie's CD sectors.
#include "common.h"
#include "movie.h"

extern "C" {
// The player being played, for the callbacks. The first variable of the
// overlay's .bss.
MoviePlayer *g_Movie OVL5_BSS;

void Movie_Init(MoviePlayer *mp, int rgb24)
{
    g_Movie = mp;
    mp->onFrameDone = 0;
    MovieStream_Clear(&mp->stream);
    MdecOut_Init(&mp->mdec, rgb24);
    mp->savedGpuDmaHook = Gfx_SetGpuDmaHook(Movie_GpuDmaIrq);
}

void Movie_Shutdown(MoviePlayer *mp)
{
    g_GfxIrq.gpuDmaHook = mp->savedGpuDmaHook;
}

void Movie_SetRing(MoviePlayer *mp, short count, unsigned char **ring)
{
    MovieStream *s = &mp->stream;

    s->ringSize = count;
    s->ring = ring;
}

void Movie_SetVlcBuffers(MoviePlayer *mp, void *a, void *b)
{
    mp->vlcNext = a;
    mp->vlcOther = b;
}

void Movie_SetSliceBuffers(MoviePlayer *mp, unsigned long *a, unsigned long *b)
{
    MdecOut *m = &mp->mdec;

    m->slice[0] = a;
    m->slice[1] = b;
}

void Movie_Open(MoviePlayer *mp, int movie)
{
    int start = g_MovieSectors[movie];
    int sectors = g_MovieSectors[movie + 1] - start;

    start += g_StreamLba;
    Movie_SetCdRange(mp, start, sectors - 0x19);
}

// Prepares the streaming read of sectors lba to lba + sectors - 1, mode 0xc0,
// XA file 1 channel 7, with Movie_CdSectorCb called for each sector.
void Movie_SetCdRange(MoviePlayer *mp, unsigned int lba, int sectors)
{
    unsigned int end;

    CdAudio_ResetRead();
    end = lba + sectors;
    end--;
    g_CdAudio.reqMode = 0xc0;
    g_CdAudio.masterVol = 0x8000;
    g_CdAudio.reqFlag = 0;
    g_CdAudio.reqStart = lba;
    g_CdAudio.reqEnd = end;
    CdReq_SetFilter(1, 7, (CdReq *)&g_CdAudio.reqFlag);
    g_CdAudio.onSector = (void *)Movie_CdSectorCb;
}
}
