// OVL0 80048418-80048528 (asyncread): Asynchronous reads.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern CdAudio (&g_CdAudio);
void AsyncRead_Init(AsyncRead *r)
{
    r->busy = 0;
    r->state = 0;
}

// Starts a CD read (CdAudio_SetupRead) and sets state 1.
void AsyncRead_Start(AsyncRead *r, void *dst, int sector, int count)
{
    CdAudio_SetupRead(dst, sector, count, 0);
    r->state = '\x01';
}

// Returns 2 when the read is done, 0 when none is running, 1 otherwise. A failed read
// starts again.
int AsyncRead_Poll(AsyncRead *r)
{
    int result = 1;

    switch (r->state) {
    case 1:
        CdAudio_ResetRead();
        if (CdAudio_RunScript(7) != 0) {
            r->state = 2;
        }
        break;
    case 2:
        if (((unsigned char *)&g_CdAudio)[0x166] != 0) {
            break;
        }
        if (CdAudio_GetReadStatus() != 0) {
            r->state = 1;
        } else {
            result = 2;
            r->state = 0;
            r->busy = 0;
        }
        break;
    case 0:
        result = 0;
        r->busy = 0;
        break;
    }
    return result;
}
}
