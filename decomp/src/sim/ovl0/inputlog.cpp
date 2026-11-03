// OVL0 800163b8-8001681c (inputlog): Input logs.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
// Recording: clears the header. Playback: rewinds (framesLeft = framesWritten).
// runLength = -1.
void InputLog_Reset(InputLog *log, int forPlayback, unsigned short capacity)
{
    if (log != (InputLog *)0x0) {
        if (forPlayback == 0) {
            memset(log, 0, 0x1c);
        }
        else {
            memset(&log->header, 0, 5);
            log->ended = 0;
            log->pos = 0;
            log->framesLeft = log->framesWritten;
        }
        log->capacity = capacity;
        log->runLength = -1;
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/inputlog", InputLog_Read);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/inputlog", InputLog_Flush);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/inputlog", InputLog_Write);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/inputlog", InputLog_Close);
}
