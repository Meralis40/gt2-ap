// The memory card driver's state (docs/decomp.md, McCard).
#ifndef MCCARD_H
#define MCCARD_H

#include "resident.h"

// A memory card port of the card driver (g_Card.ports, 0x264 bytes).
struct McPort {
    int (*op)(int event, int *sector, void **buf); // the running operation, 0 when idle
    unsigned char unk004[8];
    McDirEntry dir[15];
    int badSectors[20]; // the bad sectors remapped to frames 36-55, -1 for none
    signed char dirState[15]; // 0 for a free directory frame
    signed char freeList[15]; // the free directory frames, freeBlocks of them
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

// The card driver state, through an accessor: with &g_Card written directly
// the compiler folds the port offset into the loop pointer (CardSlots_Update).
static inline CardSystem *Card_Get()
{
    return &g_Card;
}

static inline McOpState *McOp_Get()
{
    return &g_McOp_Port;
}

// exe/mccard.cpp

// The 128-byte header frame of a save file.
struct McHeader {
    unsigned short magic; // "SC"
    unsigned char iconFlag;
    unsigned char blocks;
    unsigned char title[0x40]; // Shift-JIS
    unsigned char unk44[0x3c];
};

// exe/mcport.cpp

// A directory frame of the card, as it is written (128 bytes).
struct McDirFrame {
    unsigned int state;
    int size;
    short next;
    char name[0x74];
    unsigned char unk7e;
    unsigned char checksum;
};

#endif
