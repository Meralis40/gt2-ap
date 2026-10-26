// DecDCToutCallback of the player.
#include "common.h"
#include "movie.h"

extern "C" {
void Movie_MdecOutCb()
{
    Movie_OnMdecSlice(g_Movie);
}
}
