// Fills g_RaceSetup for the race (copies of the OVL2 and OVL4 functions).
#include "common.h"
#include "gamestate.h"

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

// Sets car index of setup from a garage car, and builds its g_RaceCarParams.
// gridSlot < 0 keeps the rest of the car.
extern "C" RaceCar *RaceSetup_SetCarFromGarage(RaceSetup *setup, int index, int gridSlot, int driverType,
                                              int unk8f, Car *car, int unk582, int garageSlot)
{
    RaceCar *rc = &setup->cars[index];
    // g_RaceCarParams[index], past the end of g_GameState.
    char *base = (char *)&g_GameState;
    int offset = 0x14fda;
    CarParams *params = (CarParams *)(base + (offset + index * 0x1c0));

    if (gridSlot >= 0) {
        memset(rc, 0, sizeof(RaceCar));
        rc->active = 1;
        rc->gridSlot = gridSlot;
    }
    rc->driverType = driverType;
    rc->unk8f = unk8f;
    rc->carId = car->baseCarId;
    rc->colorCode = car->color;
    rc->setup = car->setup;
    CarSetup *cs = &rc->setup;
    cs->flags_7a |= 0x40;
    strcpy(rc->name, CarInfoA_GetName(rc->carId));
    setup->unk582 = unk582;
    setup->garageSlot = garageSlot;
    CarParams_BuildFromSetup(cs, params);
    return rc;
}
