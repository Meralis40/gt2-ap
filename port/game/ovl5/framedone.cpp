// onFrameDone callback of the player.
#include "common.h"

extern "C" {
void Movie_OnFrameDone()
{
    ((psxMovieLoop *)g_CurrentViewLoop)->OnFrameDecoded();
}
}
