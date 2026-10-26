// Stops the player; decodes a frame on the CPU while the MDEC outputs the previous one.
#include "common.h"
#include "movie.h"

extern "C" {
void Movie_Stop(MoviePlayer *mp)
{
    DecDCToutCallback(0);
    MdecOut_Stop(&mp->mdec);
    MovieStream_Stop(&mp->stream);
}

void Movie_RestartRead(MoviePlayer *mp)
{
    CdAudio_RunScript(6);
}

int Movie_StartMdec(MoviePlayer *mp)
{
    unsigned long *vlc = (unsigned long *)mp->vlcReady;

    if (vlc == 0) {
        return 0;
    }
    MdecOut_SetSize(&mp->mdec, mp->frameW, mp->frameH);
    MdecOut_Start(&mp->mdec, mp->dstX, mp->dstY, vlc);
    return 1;
}

void *Movie_DecodeNext(MoviePlayer *mp, int x, int y)
{
    unsigned short *frame;
    void *out;
    void *other;
    unsigned short h;

    frame = (unsigned short *)MovieStream_PopFrame(&mp->stream);
    mp->dstX = x;
    mp->dstY = y;
    out = mp->vlcNext;
    mp->frameW = frame[2];
    h = frame[3];
    other = mp->vlcOther;
    mp->vlcOther = out;
    mp->frameH = h;
    mp->vlcNext = other;
    Movie_DecodeVlc(frame, (unsigned short *)out);
    MovieStream_ReleaseFrame(&mp->stream);
    return mp->vlcReady = out;
}

void Movie_Step(MoviePlayer *mp, int x, int y)
{
    Movie_StartMdec(mp);
    Movie_DecodeNext(mp, x, y);
}

void Movie_DecodeThenStart(MoviePlayer *mp, int x, int y)
{
    Movie_DecodeNext(mp, x, y);
    Movie_StartMdec(mp);
}
}
