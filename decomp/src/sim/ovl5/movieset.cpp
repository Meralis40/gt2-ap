// Player setup: GPU DMA hook, frame ring and buffers.
#include "common.h"
#include "movie.h"

extern "C" {
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
}
