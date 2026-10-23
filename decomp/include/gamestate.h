// The saved game (g_GameState) and the data placed after it.
#ifndef GAMESTATE_H
#define GAMESTATE_H

struct CarSetup {
    unsigned int wheel;
    char unk4[0x84 - 4];
};

// A car in a garage.
struct Car {
    unsigned int carId;
    unsigned int color;
    CarSetup setup;           // 0x8
    unsigned int baseCarId;   // 0x8c
    int value;                // 0x90
    unsigned short stat94;
    unsigned short stat96;
    unsigned short flags98;
    unsigned char partsOwned[7];
    unsigned char pad_a1;
    unsigned short dirt;      // 0xa2
};

struct Garage {
    short carCount;
    unsigned short pad_02;
    Car cars[100];            // 0x4
    int credits;              // 0x4014
    short currentCar;         // 0x4018
    unsigned char unk_401a;
    char driverName[12];      // 0x401b
    char pad_4027;
};

struct GameState {
    char unk0[0x3c74];
    Garage garages[2];        // 0x3c74
};

extern "C" GameState g_GameState;

#endif
