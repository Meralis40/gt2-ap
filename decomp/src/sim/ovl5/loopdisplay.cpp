// psxMovieLoop::Flip and UpdateDisplay: the display shows the decoded frames.
#include "common.h"
#include "movie.h"

extern "C" {
void psxMovieLoop__Flip()
{
}

// Every vsyncInterval vblanks, if a decoded frame is waiting, shows the next
// display buffer, and turns the display on after the first frame.
void psxMovieLoop__UpdateDisplay(psxMovieLoop *self)
{
    if (++self->vsyncTick >= self->base.vsyncInterval) {
        self->vsyncTick = 0;
        if (self->framesReady > 0) {
            // idx is set twice and added to the address as an integer, which
            // gives the original instruction order and registers.
            int idx = self->base.bufferIndex;
            short *pos;

            self->framesReady--;
            idx <<= 2;
            pos = (short *)(idx + (int)self->dispStarts);
            Gpu_SetDisplayStart((pos[0] * 3) >> 1, pos[1]);
            if (++self->base.bufferIndex >= self->base.bufferCount) {
                self->base.bufferIndex = 0;
            }
            if (self->base.videoModePending != 0) {
                self->base.videoModePending = 0;
                EnableDisplay(0);
            }
        }
    }
}
}
