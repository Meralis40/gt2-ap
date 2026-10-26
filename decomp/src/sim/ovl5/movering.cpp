// Frame ring and display helpers of the movie screen.
#include "common.h"
#include "movie.h"

extern "C" {
void Movie_InitRing()
{
    int i;

    for (i = 31; i >= 0; i--) {
        g_MovieRing[i] = g_MovieFrames[i];
    }
}

void Movie_DisplayOn()
{
    Irq_WaitVSyncs(-1);
    EnableDisplay(1);
    Irq_WaitVSyncs(3);
}
}
