// The car of a race being built from its parts (docs/gt-mode.md). CarWork is
// in resident.h.
#ifndef CARWORK_H
#define CARWORK_H

#include "gamestate.h"

extern "C" {
void CarWork_Clear(CarWork *work);
void CarWork_LoadParts(CarWork *work, unsigned int carId);
void CarWork_SetStagesFromSetup(CarWork *work, CarSetup *setup);
void CarWork_SetPartSlot(CarWork *work, int slot, int index);
void CarWork_ComputeStats(CarWork *work, unsigned short *out);
}

#endif
