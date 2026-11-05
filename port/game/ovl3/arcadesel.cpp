// Loads an arcade garage car with the right tyres (docs/arcade.md).
#include "common.h"
#include "gamestate.h"

extern "C" {
extern CarWork g_Ovl3_CarWork;
}

// Loads g_GameState.garages[garage].cars[slot] into g_Ovl3_CarWork, fits dirt
// tyres (stage 7 in the tyre slots 3 and 4) if dirt is set, else puts the
// stock tyres back, and stores the car back into the garage.
extern "C" void ArcadeSel_LoadGarageCar(int garage, int slot, int dirt)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garage];
    Car *car = &g->cars[slot];

    CarWork_Clear(&g_Ovl3_CarWork);
    CarWork_LoadParts(&g_Ovl3_CarWork, car->carId);
    CarWork_SetStagesFromSetup(&g_Ovl3_CarWork, &car->setup);
    if (dirt) {
        CarWork_SetPartSlot(&g_Ovl3_CarWork, 3, 7);
        CarWork_SetPartSlot(&g_Ovl3_CarWork, 4, 7);
    } else if ((g_Ovl3_CarWork.partSlots[3] == 7) | (g_Ovl3_CarWork.partSlots[4] == 7)) {
        CarWork_SetPartSlot(&g_Ovl3_CarWork, 3, 0);
        CarWork_SetPartSlot(&g_Ovl3_CarWork, 4, 0);
    }
    Car_StoreSetup(car, &g_Ovl3_CarWork);
}
