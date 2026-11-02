// GPU DMA hook (end of the last column upload) and the CD stream start and stop.
#include "common.h"

extern "C" {
void Movie_GpuDmaIrq()
{
    Movie_OnGpuDma(g_Movie);
}

void Movie_OnGpuDma(MoviePlayer *mp)
{
    void (*cb)() = mp->onFrameDone;

    if (mp->mdec.state == 1) {
        mp->mdec.state = 2;
        if (cb != 0) {
            cb();
        }
    }
}

void MovieStream_Clear(MovieStream *s)
{
    memset(s, 0, 0x1c);
}

int MovieStream_Start(MovieStream *s)
{
    int leave = Sys_CriticalSection(0);
    int ret;

    s->readIdx = 0;
    s->writeIdx = 0;
    s->queued = 0;
    s->flags &= 1;
    s->dst = s->ring[0];
    s->frameNo = 0;
    s->sectorIdx = 0;
    ret = CdAudio_RunScript(5);
    Sys_CriticalSection(leave);
    return ret;
}

void MovieStream_Stop(MovieStream *s)
{
    int leave = Sys_CriticalSection(0);

    s->flags |= 4;
    Music_Stop();
    Sys_CriticalSection(leave);
}
}
