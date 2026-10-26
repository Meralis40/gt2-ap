// Takes the next queued frame of the CD stream ring.
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
}
