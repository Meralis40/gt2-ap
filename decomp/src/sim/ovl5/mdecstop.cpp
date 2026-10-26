// Stops the MDEC.
#include "common.h"
#include "movie.h"

extern "C" {
void MdecOut_Stop(MdecOut *m)
{
    DecDCTReset(1);
    m->state = 3;
}
}
