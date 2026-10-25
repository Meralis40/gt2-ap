// Race setup types (docs/race.md).
#ifndef RACE_H
#define RACE_H

#include "gamestate.h"

// A car of the race: what g_RaceSetup holds for each grid position.
struct RaceCar {
    unsigned int carId;
    int colorCode;              // 0x4
    CarSetup setup;             // 0x8
    unsigned char active;       // 0x8c
    unsigned char gridSlot;
    unsigned char driverType;
    unsigned char unk8f;
    char name[0xd0 - 0x90];     // 0x90
};

// The race to run (g_RaceSetup), filled by the menus for Race_Entry.
struct RaceSetup {
    unsigned char options[8];
    unsigned char frameStep;    // 0x8
    unsigned char game;
    unsigned char raceType;
    unsigned char licenseId;
    unsigned char licenseTest;  // 0xc
    unsigned char showIntro;
    unsigned char unk0e;
    unsigned char lapCount;
    char eventCode[16];         // 0x10
    char courseName[32];        // 0x20
    unsigned int courseHash;    // 0x40
    char eventName[20];         // 0x44
    short dirt;                 // 0x58
    unsigned char carCount;
    unsigned char pad5b;
    RaceCar cars[6];            // 0x5c
    unsigned char unk53c[64];
    short eventIndex;           // 0x57c
    char unk57e;
    unsigned char pad57f;
    char worldLeague;           // 0x580
    unsigned char pad581;
    short unk582;
    short garageSlot;           // 0x584
    short maxPower;
    unsigned int flags;         // 0x588
};

// The physics parameters of a race car (g_RaceCarParams, 0x1c0 bytes each).
struct CarParams;

// A car picked for a race: car, colour and the entry it came from.
struct RaceCarPick {
    unsigned int carId;
    signed char flags;            // 0x4
    signed char colorCode;        // 0x5
    unsigned short colorRgb;      // 0x6
    short entryIndex;             // 0x8
    short pad0a;
};

#endif
