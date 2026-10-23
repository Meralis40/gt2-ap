// The arcade menus (OVL2) and their garage races (OVL3), docs/arcade.md.
#ifndef ARCADE_H
#define ARCADE_H

// The arcade race selection that OVL2 leaves at 0x801d5fa0 (g_RaceCarWorkAlt).
struct ArcadeRaceSel {
    char unk0[2];
    signed char raceType;     // 0: 2-player battle, 4: single race, 6: time trial
    char unk3[3];
    short garage;             // 0x6
    short slot;               // 0x8
    char unkA[0x18 - 0xa];
    signed char unk18;        // RaceCar.unk8f of the player's car
    char unk19[0xa0 - 0x19];
    short garages[2];         // 0xa0: 2-player battle, per player
    short slots[2];           // 0xa4
    char unkA8[0xb6 - 0xa8];
    signed char unkB6[2];     // 0xb6: RaceCar.unk8f per player
    char unkB8[0x2c8 - 0xb8];
    int useGarageCars;        // 0x2c8
    int dirt;                 // 0x2cc: dirt course (dirt tyres)
    char unk2D0[0x2d4 - 0x2d0];
};

#endif
