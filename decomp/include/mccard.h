// The memory card driver's state (docs/decomp.md, McCard).
#ifndef MCCARD_H
#define MCCARD_H

#include "resident.h"

// A memory card port of the card driver (g_Card.ports, 0x264 bytes).
struct McPort {
    int (*op)(int event, int *sector, void **buf); // the running operation, 0 when idle
    unsigned char unk004[8];
    McDirEntry dir[15];
    unsigned char unk1ec[0x50];
    signed char dirState[15]; // 0 for a free directory frame
    unsigned char unk24b[0xf];
    unsigned char freeBlocks;
    unsigned char unk25b[2];
    unsigned char state; // 0 ready
    unsigned char unk25e;
    unsigned char dirty;
    unsigned char unk260;
    unsigned char unk261[3];
};

// The card driver state.
struct CardSystem {
    unsigned char unk00[0x44];
    McPort ports[2];
};

// The running operation's state (g_McOp_Port and the g_McOp_ variables after it).
struct McOpState {
    McPort *port;
    signed char cmd;           // the transfer command the port runs (6 read, 7 write)
    signed char blocks[15];    // the file's block chain
    unsigned short *sectorMap; // file sectors in order, or 0 for 0, 1, 2...
    short index;
    unsigned short cardSector;
    unsigned char *buffer;
    unsigned int offset;
    int size;
    unsigned int remaining;
    unsigned char retries;
    unsigned char phase;
    unsigned char result;
};

extern "C" {
extern McOpState g_McOp_Port;
extern CardSystem g_Card;
extern unsigned char g_McFrame[];
}

static inline CardSystem *Card_Get()
{
    return &g_Card;
}

static inline McOpState *McOp_Get()
{
    return &g_McOp_Port;
}

#endif
