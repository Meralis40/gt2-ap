// Starts the MDEC callback and the CD stream.
#include "common.h"
#include "movie.h"

extern "C" {
void Movie_Start(MoviePlayer *mp)
{
    mp->vlcReady = 0;
    DecDCTReset(1);
    DecDCToutCallback(Movie_MdecOutCb);
    MovieStream_Start(&mp->stream);
}
}
