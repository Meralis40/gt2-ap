// Loads a garage car into g_RaceCarWorkAlt for a time trial (docs/arcade.md).
#include "common.h"
#include "carwork.h"

extern "C" void CarWork_LoadGarageCarAlt(int garage, int slot)
{
    GameState *gs = &g_GameState;
    // g_RaceCarWorkAlt (0x801d5fa0), past the end of g_GameState.
    char *base = (char *)gs;
    int offset = 0xc6c0;
    CarWork *work = (CarWork *)((int)(base + offset) & ~3);
    Garage *g = &gs->garages[garage];
    Car *car = &g->cars[slot];

    CarWork_Clear(work);
    CarWork_LoadParts(work, car->carId);
    CarWork_SetStagesFromSetup(work, &car->setup);
}
