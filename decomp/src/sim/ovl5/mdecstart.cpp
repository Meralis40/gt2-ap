// Starts and stops the MDEC output of a frame.
#include "common.h"
#include "movie.h"

extern "C" {
// Feeds the run-length codes to the MDEC and starts the output of the first
// column at (x, y). In 24-bit mode, x is in pixels and the VRAM rect in
// 16-bit units, so x is scaled by 1.5.
void MdecOut_Start(MdecOut *m, int x, short y, unsigned long *vlc)
{
    int mode = m->rgb24;
    unsigned short count = m->sliceCount;

    m->state = 0;
    m->slicesLeft = count;
    if (mode != 0) {
        m->rect.x = (x * 3) >> 1;
        m->rect.y = y;
    } else {
        m->rect.x = x;
        m->rect.y = y;
    }
    DecDCTReset(1);
    if (mode == 0) {
        Gpu_DrawSync();
    }
    DecDCTin(vlc, mode);
    DecDCTout(m->slice[count & 1], m->sliceWords);
}

void MdecOut_Stop(MdecOut *m)
{
    DecDCTReset(1);
    m->state = 3;
}
}
