// The car of a race being built from its parts (docs/gt-mode.md).
#ifndef CARWORK_H
#define CARWORK_H

#include "gamestate.h"

struct CarWork {
    CarSetup setup;
    char unk84[0x17b0 - 0x84];
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
