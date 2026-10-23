// Sets the part slots of a car being built from its setup (copy of the OVL4
// function).
#include "common.h"
#include "carwork.h"

extern "C" void *GameData_GetEntry(int table, int index);

// The stage byte at offset off of a game data entry.
#define STAGE(entry, off) (((unsigned char *)(entry))[off])

// Copies setup into work and sets each part slot to the stage of the fitted part.
extern "C" void CarWork_SetStagesFromSetup(CarWork *work, CarSetup *setup)
{
    work->setup = *setup;
    work->partSlots[0] = STAGE(GameData_GetEntry(0x12, setup->idx_t12), 0x8);
    work->partSlots[1] = STAGE(GameData_GetEntry(0x0, setup->idx_t00), 0x8);
    work->partSlots[2] = STAGE(GameData_GetEntry(0x1, setup->idx_t01), 0x8);
    work->partSlots[3] = STAGE(GameData_GetEntry(0x16, setup->idx_t16_front), 0x8);
    work->partSlots[4] = STAGE(GameData_GetEntry(0x17, setup->idx_t16_rear), 0x4);
    work->partSlots[6] = STAGE(GameData_GetEntry(0x11, setup->idx_t11), 0x8);
    work->partSlots[7] = STAGE(GameData_GetEntry(0xf, setup->idx_t0f), 0x8);
    work->partSlots[8] = STAGE(GameData_GetEntry(0xe, setup->idx_t0e), 0x8);
    work->partSlots[9] = STAGE(GameData_GetEntry(0x10, setup->idx_t10), 0x8);
    work->partSlots[10] = STAGE(GameData_GetEntry(0xa, setup->idx_t0a), 0x8);
    work->partSlots[11] = STAGE(GameData_GetEntry(0x7, setup->idx_t07), 0x8);
    work->partSlots[12] = STAGE(GameData_GetEntry(0x8, setup->idx_t08), 0x8);
    work->partSlots[13] = STAGE(GameData_GetEntry(0xb, setup->idx_t0b), 0x8);
    work->partSlots[14] = STAGE(GameData_GetEntry(0x9, setup->idx_t09), 0x8);
    work->partSlots[15] = STAGE(GameData_GetEntry(0xc, setup->idx_t0c), 0x8);
    work->partSlots[16] = STAGE(GameData_GetEntry(0x13, setup->idx_t13), 0x8);
    work->partSlots[17] = STAGE(GameData_GetEntry(0x14, setup->idx_t14), 0x8);
    work->partSlots[18] = STAGE(GameData_GetEntry(0x4, setup->idx_t04), 0xb);
    work->partSlots[19] = STAGE(GameData_GetEntry(0x5, setup->idx_t05), 0xe);
    work->partSlots[20] = STAGE(GameData_GetEntry(0x1b, setup->idx_t1b), 0x8);
    work->partSlots[21] = STAGE(GameData_GetEntry(0x1c, setup->idx_t1c), 0x8);
    work->partSlots[22] = STAGE(GameData_GetEntry(0x15, setup->idx_t15), 0x8);
    work->partSlots[23] = STAGE(GameData_GetEntry(0x2, setup->idx_08), 0x8);
    work->partSlots[24] = 0;
    work->partSlots[25] = 0;
    work->partSlots[26] = STAGE(GameData_GetEntry(0xd, setup->idx_t0d), 0x4);
}
