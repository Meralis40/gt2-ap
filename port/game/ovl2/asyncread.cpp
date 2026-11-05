// OVL2 80015834-80015ab4 (asyncread): Asynchronous CD reads.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern CdAudio g_CdAudio;


static inline int CdAudio_GetState(volatile CdAudio *cd)
{
    return cd->state;
}

// Copy of AsyncRead_Init.
void AsyncRead_Init_OVL2(AsyncRead *r)
{
    r->busy = '\0';
    r->state = '\0';
}

// Starts a CD read if none is running (state 3): 1 when started.
int AsyncRead_Start_OVL2(AsyncRead *r, void *dst, int sector, int count)
{
    if (r->busy != 0) {
        return 0;
    }
    CdAudio_SetupRead(dst, sector, count, 0);
    r->busy = 1;
    r->state = 3;
    return 1;
}

// Stops the read: 1 once it is stopped. States 5 and 6 are the stop in progress.
int AsyncRead_Stop(AsyncRead *r)
{
    int result = 0;
    int stopped;

    switch (r->state) {
    case 5:
        stopped = Music_Stop();
        if (stopped != 1) {
            break;
        }
        r->state += stopped;
    case 6:
        result = 1;
        r->state = 0;
        r->busy = 0;
        break;
    default:
        if (r->busy == 0 && CdAudio_GetState(&g_CdAudio) == 0) {
            r->state = 0;
            return 1;
        }
        r->busy = 1;
        r->state = 5;
        break;
    }
    return result;
}

// Steps the read; remaining: the bytes left, or -1. 0: idle, 1: reading, 2: done.
int AsyncRead_Poll_OVL2(AsyncRead *r, int *remaining)
{
    int result = 1;
    int stopped;

    *remaining = -1;
    switch (r->state) {
    case 1:
        if (CdAudio_GetState(&g_CdAudio) == 0) {
            goto reset;
        }
        stopped = Music_Stop();
        if (stopped != 1) {
            break;
        }
        r->state += stopped;
    case 2:
        r->state++;
    case 3:
    reset:
        CdAudio_ResetRead();
        if (CdAudio_RunScript(7) != 0) {
            r->state = 4;
        }
        break;
    case 4: {
        CdAudio *cd = &g_CdAudio;

        *remaining = cd->remaining;
        if (cd->state != 0) {
            break;
        }
        if (CdAudio_GetReadStatus() != 0) {
            r->state = 3;
        } else {
            result = 2;
            r->state = 0;
            r->busy = 0;
        }
        break;
    }
    case 5:
        stopped = Music_Stop();
        if (stopped != 1) {
            break;
        }
        r->state += stopped;
    case 6:
        result = 2;
        r->state = 0;
        r->busy = 0;
        break;
    case 0:
        result = 0;
        r->busy = 0;
        break;
    }
    return result;
}
}
