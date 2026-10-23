// Race setup types (docs/race.md).
#ifndef RACE_H
#define RACE_H

#include "gamestate.h"

// A car of the race: what g_RaceSetup holds for each grid position.
struct RaceCar {
    unsigned int carId;
    unsigned int colorCode;     // 0x4
    CarSetup setup;             // 0x8
    unsigned char active;       // 0x8c
    unsigned char gridSlot;
    unsigned char driverType;
    unsigned char unk8f;
    char name[0xd0 - 0x90];     // 0x90
};

struct RaceSetup {
    char unk0[0x5c];
    RaceCar cars[1];            // 0x5c
};

// The physics parameters of a race car (g_RaceCarParams, 0x1c0 bytes each).
struct CarParams;

// A car picked for a race: car, colour and the entry it came from.
struct RaceCarPick {
    unsigned int carId;
    unsigned char flags;          // 0x4
    signed char colorCode;        // 0x5
    unsigned short colorRgb;      // 0x6
    short entryIndex;             // 0x8
    short pad0a;
};

#endif
