// psxMovieLoop constructor, destructor and Enter.
#include "common.h"
#include "movie.h"

// The constructor is real C++: in a g++ constructor, the stores of the last
// block go through the return register (the copy of this), which the
// extern "C" version doesn't give. So psxMovieLoop is declared here again as a
// class derived from psxViewLoop, with the layout of movie.h, and the
// constructors are aliased to the symbols the other code calls.
struct ViewLoopClass : psxViewLoop {
    ViewLoopClass();
};

struct MovieLoopClass : ViewLoopClass {
    PadDevice pad;
    PadState padState;
    int noSkip;
    int rgb24;
    MoviePlayer player;
    int movieIndex;
    int startDelay;
    int framesReady;
    int decodeBuf;
    int vsyncTick;
    short *dispStarts;
    int dispOffX;
    int dispOffY;
    MovieLoopClass(int movie, int triple);
};

typedef char MovieLoopClass_size[sizeof(MovieLoopClass) == sizeof(psxMovieLoop) ? 1 : -1];

__asm__(".globl psxMovieLoop__psxMovieLoop
"
        ".set psxMovieLoop__psxMovieLoop, __14MovieLoopClassii
"
        ".set __13ViewLoopClass, psxViewLoop__psxViewLoop
");

// triple: triple buffered display (and two frames queued before the first
// decode).
MovieLoopClass::MovieLoopClass(int movie, int triple)
{
    vptr = psxMovieLoop__vtable;
    PadDevice_Open(&pad, 0, g_PadHandlers);
    noSkip = 0;
    rgb24 = 1;
    movieIndex = movie;
    if (triple != 0) {
        startDelay = 2;
    } else {
        startDelay = 0;
    }
    g_LastBufferIndex = 0;
    bufferCount = triple != 0 ? 3 : 2;
    vsyncTick = 0;
    dispStarts = triple != 0 ? D_800120D4 : D_800120CC;
    dispOffX = 0;
    dispOffY = 0;
}

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
