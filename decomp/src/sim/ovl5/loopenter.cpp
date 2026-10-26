// psxMovieLoop constructor, destructor, Enter and Update.
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

// One step of the screen loop: decodes the next frame when the display has a
// free buffer. Returns 0 to leave (START pressed, the stream ended, or the
// read can't restart), 1 after a frame was started.
int psxMovieLoop__Update(psxMovieLoop *self)
{
    int start = Irq_WaitVSyncs(0);
    int now;
    int status;
    int busy;

    busy = 1;
    for (;;) {
        now = Irq_WaitVSyncs(0);
        PadButtons_Read(self->pad.buttons, &self->padState);
        if (self->noSkip == 0 && (self->padState.pressed & PAD_START)) {
            while (Movie_GetStatus(&self->player) == busy) {
            }
            return 0;
        }
        status = Movie_GetStatus(&self->player);
        // Waiting for 900 vsyncs (15 seconds at 60 Hz): treat it as a
        // stopped CD.
        if (status == busy && now - start > 899) {
            start = now;
            status = 3;
        }
        if (status == 0) {
            if (self->startDelay != 0) {
                // The CD callback changes queued: read it as volatile.
                if (((volatile MovieStream *)&self->player.stream)->queued < self->startDelay) {
                    continue;
                }
                self->startDelay = 0;
            }
            if (self->framesReady < self->base.bufferCount - 1) {
                break;
            }
        }
        if (status == 2) {
            return 0;
        }
        // RestartRead (vptr[18]) returns 1 to keep playing. The original
        // tests the result with xori, which only `^ 1` gives.
        if (status == 3 && (((int (*)(psxMovieLoop *))self->base.vptr[18])(self) ^ 1) != 0) {
            return 0;
        }
    }
    {
        short *pos = (short *)(self->decodeBuf * 4 + (int)self->dispStarts);
        Movie_Step(&self->player, pos[0] + self->dispOffX, pos[1] + self->dispOffY);
    }
    if (++self->decodeBuf >= self->base.bufferCount) {
        self->decodeBuf = 0;
    }
    return 1;
}
}
