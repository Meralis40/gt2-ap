// The car of a race being built from its parts (docs/gt-mode.md).
#ifndef CARWORK_H
#define CARWORK_H

#include "gamestate.h"

// Game data entries, by size. Most hold their stage at byte 8.
struct PartEntry12 { unsigned int data[3]; };
struct PartEntry16 { unsigned int data[4]; };
struct PartEntry20 { unsigned int data[5]; };
struct PartEntry24 { unsigned int data[6]; };
struct PartEntry28 { unsigned int data[7]; };
struct PartEntry32 { unsigned int data[8]; };
struct PartEntry36 { unsigned int data[9]; };
struct PartEntry76 { unsigned int data[19]; };

// A front tyre (table 0x16), with its entries of tables 0x18 to 0x1a.
struct FrontTyreEntry {
    unsigned int unk0[2];
    unsigned char stage;          // 0x8
    unsigned char unk9;
    unsigned short idx_t18;       // 0xa
    unsigned short idx_t19;       // 0xc
    unsigned short idx_t1a;       // 0xe
};

// A rear tyre (table 0x17), with its entries of tables 0x18 and 0x19.
struct RearTyreEntry {
    unsigned int unk0;
    unsigned char stage;          // 0x4
    unsigned char unk5;
    unsigned short idx_t18;       // 0x6
    unsigned short idx_t19;       // 0x8
    unsigned short unka;
};

struct TyreEntry18 { unsigned char data[4]; };
struct TyreEntry19 { unsigned char data[64]; };
struct TyreEntry1a { unsigned char data[8]; };

// Each tuning category keeps the game data entry of every stage the car can
// use (stage 0 = stock) and its entry index (-1 = not available).
struct CarWork {
    CarSetup setup;
    char unk84[0x988 - 0x84];
    PartEntry76 susp[4];               // 0x988: table 0x12
    int suspIdx[4];
    PartEntry12 brakes[2];             // 0xac8: table 0x00
    int brakesIdx[2];
    PartEntry16 brakeBal[2];           // 0xae8: table 0x01
    int brakeBalIdx[2];
    FrontTyreEntry frontTyres[10];     // 0xb10: table 0x16
    int frontTyresIdx[10];
    TyreEntry18 frontTyres18[10];      // 0xbd8
    TyreEntry19 frontTyres19[10];      // 0xc00
    TyreEntry1a frontTyres1a[10];      // 0xe80
    RearTyreEntry rearTyres[10];       // 0xed0: table 0x17
    int rearTyresIdx[10];
    TyreEntry18 rearTyres18[10];       // 0xf70
    TyreEntry19 rearTyres19[10];       // 0xf98
    PartEntry36 trans[4];              // 0x1218: table 0x11
    int transIdx[4];
    PartEntry16 clutch[4];             // 0x12b8: table 0x0f
    int clutchIdx[4];
    PartEntry12 flywheel[4];           // 0x1308: table 0x0e
    int flywheelIdx[4];
    PartEntry12 driveshaft[2];         // 0x1348: table 0x10
    int driveshaftIdx[2];
    PartEntry12 chip[2];               // 0x1368: table 0x0a
    int chipIdx[2];
    PartEntry12 portPolish[2];         // 0x1388: table 0x07
    int portPolishIdx[2];
    PartEntry12 balance[2];            // 0x13a8: table 0x08
    int balanceIdx[2];
    PartEntry12 naTune[4];             // 0x13c8: table 0x0b
    int naTuneIdx[4];
    PartEntry12 displ[2];              // 0x1408: table 0x09
    int displIdx[2];
    PartEntry20 turbo[5];              // 0x1428: table 0x0c
    int turboIdx[5];
    PartEntry12 intercooler[3];        // 0x14a0: table 0x13
    int intercoolerIdx[3];
    PartEntry12 muffler[4];            // 0x14d0: table 0x14
    int mufflerIdx[4];
    PartEntry12 weight[4];             // 0x1510: table 0x04, stage at 0xb
    int weightIdx[4];
    PartEntry28 racingMod[5];          // 0x1550: table 0x05, stage at 0xe
    int racingModIdx[5];
    PartEntry16 asc[2];                // 0x15f0: table 0x1b
    int ascIdx[2];
    PartEntry16 tcs[2];                // 0x1618: table 0x1c
    int tcsIdx[2];
    PartEntry32 lsd[6];                // 0x1640: table 0x15
    int lsdIdx[6];
    PartEntry24 t02;                   // 0x1718: table 0x02 (no tune part)
    int t02Idx;
    PartEntry20 t03;                   // 0x1734: table 0x03
    int t03Idx;
    PartEntry76 t06;                   // 0x174c: table 0x06
    int t06Idx;
    PartEntry16 t0d;                   // 0x179c: table 0x0d
    int t0dIdx;
    short partSlots[0x1c];    // 0x17b0: stage fitted per part slot (3, 4: tyres)
};

extern "C" {
void CarWork_Clear(CarWork *work);
void CarWork_LoadParts(CarWork *work, unsigned int carId);
void CarWork_SetStagesFromSetup(CarWork *work, CarSetup *setup);
void CarWork_SetPartSlot(CarWork *work, int slot, int index);
void CarWork_ComputeStats(CarWork *work, unsigned short *out);
}

#endif
