// Fills g_RaceSetup for the race (copies of the OVL2 and OVL4 functions).
#include "common.h"
#include "race.h"

extern "C" {
char *strcpy(char *dst, const char *src);
char *CarInfoA_GetName(unsigned int carId);
void CarSetup_FromCarEntry(void *carEntry, CarSetup *setup);
void CarParams_BuildFromSetup(CarSetup *setup, CarParams *params);
}

// Sets car index of setup from a car table entry instead of a garage car, and
// builds its g_RaceCarParams. gridSlot < 0 keeps the rest of the car.
extern "C" RaceCar *RaceSetup_SetCarFromEntry(RaceSetup *setup, int index, int gridSlot, int driverType,
                                             int unk8f, unsigned int carId, unsigned int color,
                                             void *carEntry)
{
    RaceCar *car = &setup->cars[index];

    if (gridSlot >= 0) {
        memset(car, 0, sizeof(RaceCar));
        car->active = 1;
        car->gridSlot = gridSlot;
    }
    car->driverType = driverType;
    car->carId = carId;
    car->unk8f = unk8f;
    car->colorCode = color;
    strcpy(car->name, CarInfoA_GetName(car->carId));
    CarSetup_FromCarEntry(carEntry, &car->setup);
    // g_RaceCarParams[index], past the end of g_GameState.
    char *base = (char *)&g_GameState;
    int offset = 0x14fda;
    CarParams_BuildFromSetup(&car->setup, (CarParams *)(base + (offset + index * 0x1c0)));
    return car;
}
