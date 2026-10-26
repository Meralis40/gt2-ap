// MDEC output setup.
#include "common.h"
#include "movie.h"

extern "C" {
void MdecOut_Init(MdecOut *m, int rgb24)
{
    memset(m, 0, 0x18);
    m->state = 2;
    m->rgb24 = rgb24;
}
}
