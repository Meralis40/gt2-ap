// psxMovieLoop virtuals, the three movies and the overlay entries.
#include "common.h"
#include "movie.h"

extern "C" {
void psxMovieLoop__Leave(psxMovieLoop *self)
{
    Movie_Stop(&self->player);
    Movie_Shutdown(&self->player);
    psxViewLoop__Leave(&self->base);
}

void psxMovieLoop__BeginDraw()
{
}

void psxMovieLoop__Draw()
{
}

void psxMovieLoop__EndDraw()
{
}

void psxMovieLoop__Virtual15()
{
}

int psxMovieLoop__RestartRead(psxMovieLoop *self)
{
    Movie_RestartRead(&self->player);
    return 1;
}

void psxMovieLoop__OnFrameDecoded(psxMovieLoop *self)
{
    self->framesReady++;
}

void Movie_Play18()
{
    psxMovieLoop loop;
    psxMovieLoop *p;

    psxMovieLoop__psxMovieLoop(&loop, 0x18, 1);
    Movie_ClearVram();
    Irq_WaitVSyncs(-1);
    Gpu_SetDisplayMode(0x31);
    Irq_WaitVSyncs(2);
    p = &loop;
    p->dispOffY = 0x18;
    p->dispOffX = 0;
    p->base.vsyncInterval = 2;
    ScreenViewLoop__Execute(p);
    Movie_DisplayOn();
    psxMovieLoop__psxMovieLoop_dtor(p, 2);
}

void Movie_Play1A()
{
    psxMovieLoop loop;
    psxMovieLoop *p;

    psxMovieLoop__psxMovieLoop(&loop, 0x1a, 0);
    Movie_ClearVram();
    Irq_WaitVSyncs(-1);
    Gpu_SetDisplayMode(0x13);
    Irq_WaitVSyncs(2);
    p = &loop;
    p->dispOffY = 8;
    p->base.vsyncInterval = 4;
    p->dispOffX = 0;
    p->noSkip = 1;
    ScreenViewLoop__Execute(p);
    Movie_DisplayOn();
    psxMovieLoop__psxMovieLoop_dtor(p, 2);
}

void Movie_Play19()
{
    psxMovieLoop loop;
    psxMovieLoop *p;

    psxMovieLoop__psxMovieLoop(&loop, 0x19, 0);
    Movie_ClearVram();
    Irq_WaitVSyncs(-1);
    Gpu_SetDisplayMode(0x33);
    Irq_WaitVSyncs(2);
    p = &loop;
    p->dispOffY = 0xc;
    p->base.vsyncInterval = 4;
    p->dispOffX = 0;
    p->noSkip = 1;
    ScreenViewLoop__Execute(p);
    Movie_DisplayOn();
    psxMovieLoop__psxMovieLoop_dtor(p, 2);
}

void Movie_DefaultEntry()
{
    Movie_Play18();
    Overlay_Start(1);
}

void Movie_ArcadeEntry(int which)
{
    if (which == 0) {
        Movie_Play1A();
    } else {
        Movie_Play19();
    }
    Overlay_Start(1);
}

void *psxMovieLoop____tf()
{
    if (psxMovieLoop__typeinfo == 0) {
        psxViewLoop____tf();
        __rtti_si(&psxMovieLoop__typeinfo, D_800115BC, &psxViewLoop__typeinfo);
    }
    return &psxMovieLoop__typeinfo;
}
}
