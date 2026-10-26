// onFrameDone callback of the player.
#include "common.h"
#include "movie.h"

extern "C" {
void Movie_OnFrameDone()
{
    ((void (*)(psxViewLoop *))g_CurrentViewLoop->vptr[19])(g_CurrentViewLoop);
}
}
