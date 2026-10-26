// Reports the player's status, stops and steps the player (the MDEC outputs
// frame n while the CPU decodes frame n + 1), and the CD sector and MDEC
// callbacks.
#include "common.h"
#include "movie.h"

extern "C" {
// 0: a frame is ready to decode, 1: waiting, 2: the stream ended, 3: the CD stopped.
int Movie_GetStatus(MoviePlayer *mp)
{
    int leave = Sys_CriticalSection(0);
    int status = 1;

    if ((unsigned short)mp->stream.queued == 0) {
        if (mp->stream.flags & 4) {
            status = 2;
        } else if (g_CdAudio.state == 0) {
            status = 3;
        }
    } else if (mp->mdec.state == 2) {
        status = 0;
    }
    Sys_CriticalSection(leave);
    return status;
}

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

int Movie_CdSectorCb(void *a, void *b)
{
    return Movie_OnSector(g_Movie, a, b);
}

// While the MDEC is busy, defers the sector to Movie_OnMdecSlice (flag 8);
// otherwise reads it. Returns 2 once the stream has ended.
int Movie_OnSector(MoviePlayer *mp, void *a, void *b)
{
    MovieStream *s = &mp->stream;
    unsigned int flags = s->flags;

    // The MDEC callbacks change the state: read it as volatile.
    if (((volatile MdecOut *)&mp->mdec)->state == 0) {
        s->flags = flags | 8;
        return (flags >> 1) & 2;
    }
    return (MovieStream_ReadSector(s) != 0) << 1;
}

void Movie_MdecOutCb()
{
    Movie_OnMdecSlice(g_Movie);
}
}
