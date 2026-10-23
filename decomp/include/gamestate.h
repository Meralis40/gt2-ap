// The saved game (g_GameState) and the data placed after it.
#ifndef GAMESTATE_H
#define GAMESTATE_H

// The parts fitted to a car and their settings. idx_tNN is the entry of the
// part in game data table NN (GameData_GetEntry).
struct CarSetup {
    unsigned int wheel;
    unsigned short idx_t00;       // 0x4
    unsigned short idx_t01;
    unsigned short idx_08;        // 0x8: table 2
    unsigned short idx_0a;
    unsigned short idx_0c;        // 0xc
    unsigned short idx_t0d;
    unsigned short idx_t11;       // 0x10
    unsigned short idx_t12;
    unsigned short idx_t15;       // 0x14
    unsigned short idx_t16_front;
    unsigned short idx_t16_rear;  // 0x18: table 0x17
    unsigned short idx_t04;
    unsigned short idx_t05;       // 0x1c
    unsigned short idx_t07;
    unsigned short idx_t08;       // 0x20
    unsigned short idx_t09;
    unsigned short idx_t0a;       // 0x24
    unsigned short idx_t0b;
    unsigned short idx_t0c;       // 0x28
    unsigned short idx_t0e;
    unsigned short idx_t0f;       // 0x2c
    unsigned short idx_t10;
    unsigned short idx_t14;       // 0x30
    unsigned short idx_t13;
    unsigned short idx_t1b;       // 0x34
    unsigned short idx_t1c;
    unsigned short idx_t1d;       // 0x38
    unsigned short pad_3a;
    char unk3c[0x7a - 0x3c];
    unsigned char flags_7a;       // 0x7a
    char unk7b[0x84 - 0x7b];
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
