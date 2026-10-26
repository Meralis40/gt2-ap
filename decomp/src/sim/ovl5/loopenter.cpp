// psxMovieLoop destructor and Enter.
#include "common.h"
#include "movie.h"

extern "C" {
void psxMovieLoop__psxMovieLoop_dtor(psxMovieLoop *self, int flags)
{
    self->base.vptr = psxMovieLoop__vtable;
    PadDevice_Close(&self->pad);
    psxViewLoop__psxViewLoop_dtor(&self->base, flags);
}

void psxMovieLoop__Enter(psxMovieLoop *self)
{
    MoviePlayer *mp;

    psxViewLoop__Enter(&self->base);
    self->framesReady = 0;
    self->base.videoModePending = 1;
    self->base.bufferIndex = 0;
    self->decodeBuf = 0;
    Movie_InitRing();
    mp = &self->player;
    Movie_Init(mp, self->rgb24);
    mp->onFrameDone = Movie_OnFrameDone;
    Movie_SetRing(mp, 0x20, g_MovieRing);
    Movie_SetVlcBuffers(mp, g_MovieVlcBufA, g_MovieVlcBufB);
    Movie_SetSliceBuffers(mp, (unsigned long *)g_MovieSliceBufA, (unsigned long *)(g_MovieSliceBufA + 0x3000));
    Movie_Open(mp, self->movieIndex);
    Movie_Start(mp);
}
}
