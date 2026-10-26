// Takes the next queued frame of the CD stream ring, and releases it.
#include "common.h"
#include "movie.h"

extern "C" {
// Next queued frame, 0 if none. Its slot stays in use until
// MovieStream_ReleaseFrame.
unsigned char *MovieStream_PopFrame(MovieStream *s)
{
    int leave = Sys_CriticalSection(0);
    int idx = s->readIdx;
    int size = s->ringSize;
    unsigned char *frame = 0;

    if (s->queued != 0) {
        frame = s->ring[idx];
        if (++idx == size) {
            idx = 0;
        }
        s->readIdx = idx;
    }
    Sys_CriticalSection(leave);
    return frame;
}

// Frees the slot of the frame taken by MovieStream_PopFrame.
void MovieStream_ReleaseFrame(MovieStream *s)
{
    int leave = Sys_CriticalSection(0);
    int queued = s->queued;

    if (queued != 0) {
        queued--;
    }
    s->queued = queued;
    Sys_CriticalSection(leave);
}
}
