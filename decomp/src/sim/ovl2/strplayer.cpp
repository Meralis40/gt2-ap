// OVL2 800265d4-8002678c (strplayer): Course preview stream player.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "copies.h"
#include "common.h"

extern "C" {
extern unsigned short D_80053508[];
#ifdef NON_MATCHING
// Sets up the player of movie in work: the ring of 2 frames from +8, the VLC buffer
// and the two slice buffers.
void Movie_InitRing_OVL2(ArcStrPlayer *p, int movie, void *buffer)
{
    unsigned char **work = (unsigned char **)buffer;

    p->work = buffer;
    p->movie = D_80053508[movie];
    p->started = 0;
    p->frameReady = 0;
    p->decoding = 0;
    p->frameDone = 0;
    work[0] = (unsigned char *)work + 8;
    work[1] = (unsigned char *)work + 0x2768;
    Movie_Init(p, 0);
    Movie_SetRing(p, 2, work);
    Movie_SetVlcBuffers(p, (unsigned char *)work + 0x4ec8, (unsigned char *)work + 0x4ec8);
    Movie_SetSliceBuffers(p, (unsigned long *)((unsigned char *)work + 0xcec8),
                          (unsigned long *)((unsigned char *)work + 0xe0c8));
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/strplayer", Movie_InitRing_OVL2);
#endif

// One frame of the player: starts it, restarts it after its end, and decodes the next
// frame.
void StrPlayer_Poll(ArcStrPlayer *p)
{
    int status;

    if (p->started == 0) {
        Movie_Open(p, p->movie);
        Movie_Start(p);
        p->started = 1;
    }
    status = Movie_GetStatus(p);
    switch (status) {
    case 0:
        p->frameReady = 1;
        break;
    case 2:
    case 3:
        p->started = 0;
        break;
    }
    if (p->decoding != 0) {
        p->decoding = 0;
        Movie_StartMdec(p);
        do {
        } while (*(volatile unsigned char *)&p->mdec.state != 2);
        p->frameDone = 1;
    } else if (p->frameReady != 0) {
        p->frameReady = 0;
        p->decoding = 1;
        Movie_DecodeNext(p, 0x280, 0x100);
    }
}

// Stops and shuts down OVL2's copy of the movie player.
void StrPlayer_Close(ArcStrPlayer *p)
{
    Movie_Stop(p);
    Movie_Shutdown(p);
}
}
