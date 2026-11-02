// Resident code 8007f5a8-8007f7f4 (mcformat). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

// Starts formatting the card. Returns 0, or why not: 5 the card isn't ready (or
// unformatted, state 3), 1 busy.
int McCard_StartFormat(int port)
{
    int leave = Sys_CriticalSection(0);
    int result = 5;
    McPort *p = &Card_Get()->ports[port];
    int state = p->state;

    if (state == 0 || state == 3) {
        result = 1;
        if (p->op == 0) {
            result = 0;
            g_McOp_Port.port = p;
            g_McOp_Port.index = 0;
            g_McOp_Port.result = 5;
            p->op = McOp_Format;
        }
    }
    Sys_CriticalSection(leave);
    return result;
}

void McFrame_InitHeader(void *frame)
{
    unsigned char uVar1;

    memset(frame, 0, 0x80);
    *(unsigned short *)frame = 0x434d;
    uVar1 = McFrame_Checksum((unsigned int *)frame);
    *(unsigned char *)((int)frame + 0x7f) = uVar1;
}

void McFrame_InitBadSector(int *frame, int sector)
{
    unsigned char uVar1;

    memset(frame, 0, 0x80);
    *frame = sector;
    uVar1 = McFrame_Checksum((unsigned int *)frame);
    *(unsigned char *)((int)frame + 0x7f) = uVar1;
}

// Writes the card's frames a step at a time: the header in frame 0 cleared, the 15
// directory frames, the 20 bad sector frames, then the header.
#ifdef NON_MATCHING
// The game loads &g_McOp_Port into $a0 first, moving the arguments up to $a1-$a3, then
// copies it to a saved register (as in McOp_Create); the rest is the game's code.
int McOp_Format(int event, int *sector, void **buf)
{
    McOpState *op = McOp_Get();
    McPort *port = op->port;
    int i = op->index;
    int j;

    if (event == 0) {
        *buf = g_McFrame;
        op->index = i + 1;
        *sector = 0;
        if (i == 0) {
            memset(g_McFrame, 0, 0x80);
            port->unk25e = 0;
            McPort_ResetDir(port);
            return 7;
        }
        j = i - 1;
        *sector = i;
        if (j < 15) {
            McFrame_FromDirEntry(&port->dir[j], (McDirFrame *)g_McFrame);
            return 7;
        }
        j = i - 16;
        if (j < 20) {
            McFrame_InitBadSector((int *)g_McFrame, -1);
            return 7;
        }
        j = i - 36;
        *sector = 0;
        if (j <= 0) {
            McFrame_InitHeader(g_McFrame);
            return 7;
        }
        McPort_RebuildBlockUse(port);
        port->unk25e = 1;
        port->dirty = 1;
        op->result = 0;
    }
    port->op = 0;
    return 0;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/mcformat", McOp_Format);
#endif
}
