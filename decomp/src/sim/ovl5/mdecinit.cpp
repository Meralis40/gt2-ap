// MDEC output setup and frame size.
#include "common.h"

extern "C" {
void MdecOut_Init(MdecOut *m, int rgb24)
{
    memset(m, 0, 0x18);
    m->state = 2;
    m->rgb24 = rgb24;
}

// Sets the frame size: w / 16 slices (16-pixel columns) of h pixels, rounded
// up to a whole macroblock. A slice is 16 or 24 VRAM pixels wide (16 or 24 bits
// per pixel).
void MdecOut_SetSize(MdecOut *m, int w, int h)
{
    int words;
    int h2;

    w = (w + 15) >> 4;
    m->sliceCount = w;
    h2 = (h + 15) & ~15;
    if (m->rgb24 == 0) {
        words = h2 * 8;
        w = 16;
    } else {
        words = h2 * 12;
        w = 24;
    }
    m->rect.w = w;
    m->rect.h = h;
    m->sliceWords = words;
}
}
