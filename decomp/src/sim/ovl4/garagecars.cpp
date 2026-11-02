// OVL4 8001ec0c-8001f210 (garagecars): Garage cars and the machine test car list.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

// Fills car for a new car: its ids, colour, setup, value and fitted parts
// (tires, 7 bytes of part bits, or none), with the weight, drivetrain, torque
// and power computed from the setup in the scratchpad.
// Doesn't match: the game stores stat96 before reading flags98 back for the
// racing modification bit, where GCC stores it after.
#ifdef NON_MATCHING
void Car_Init(Car *car, unsigned int carId, unsigned int baseCarId, int color, CarSetup *setup, unsigned int value,
              int racingMod, int flag14, unsigned char *tires)
{
    CarParams *params = (CarParams *)0x1f800000;
    EnginePerf *perf = (EnginePerf *)0x1f8001c0;
    unsigned short kept;
    int weight;
    int i;

    CarParams_BuildFromSetup(setup, params);
    CarParams_GetPerformance(params, perf);
    car->carId = carId;
    car->baseCarId = baseCarId;
    car->color = color;
    car->setup = *setup;
    car->value = value;
    // GCC reverses the loop (bgez).
    for (i = 0; i < 7; i++) {
        car->partsOwned[i] = 0;
    }
    if (tires) {
        for (i = 0; i < 7; i++) {
            car->partsOwned[i] = tires[i];
        }
    }
    kept = car->flags98 & 0xc000;
    car->flags98 = kept | perf->maxPower;
    car->stat96 = perf->maxTorque;
    kept = car->stat94 & 0xe000;
    // The game reads the weight with lhu and sign-extends it.
    weight = (short)*(unsigned short *)&params->weight;
    car->stat94 = kept | weight;
    ((Hi3 *)&car->stat94)->hi = params->drivetrain;
    car->flags98 = (car->flags98 & 0x7fff) | (racingMod << 15);
    car->flags98 = (car->flags98 & 0xbfff) | (flag14 << 14);
    car->dirt = 0;
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/garagecars", Car_Init);
#endif

// Removes the car in slot, moving the next ones down, and keeps the current
// car's slot (-1 when it was the one removed).
void Garage_RemoveCar(Garage *garage, int slot)
{
    if (garage->currentCar == slot) {
        garage->currentCar = -1;
    }
    if (slot < garage->currentCar) {
        garage->currentCar--;
    }
    for (; slot < garage->carCount - 1; slot++) {
        int next = slot + 1; // a local keeps the test at the top

        garage->cars[slot] = garage->cars[next];
    }
    garage->carCount--;
}

// Adds a new car (Car_Init, with carId as its base car) if the garage has
// room for it; returns whether it was added.
int Garage_AddNewCar(Garage *garage, int carId, int color, CarSetup *setup, int price, int flag15, int flag14,
                     unsigned char *parts)
{
    if (garage->carCount >= 100) {
        return 0;
    }
    Car_Init(&garage->cars[garage->carCount], carId, carId, color, setup, price, flag15, flag14, parts);
    garage->carCount++;
    return 1;
}

// Moves the car in slot to the slot to, shifting the ones between. The current
// car follows when it is the one moved, or when a car moved down past it.
void Garage_MoveCar(Garage *garage, int slot, int to)
{
    Car car;
    int i;
    int current;

    car = garage->cars[slot];
    if (to < slot) {
        for (i = slot - 1; i >= to; i--) {
            garage->cars[i + 1] = garage->cars[i];
        }
    } else {
        for (i = slot; i < to; i++) {
            garage->cars[i] = garage->cars[i + 1];
        }
    }
    garage->cars[to] = car;
    current = garage->currentCar;
    if (current >= 0) {
        if ((current < slot) & (current >= to)) {
            garage->currentCar++;
        } else if (current == slot) {
            garage->currentCar = to;
        }
    }
}

// The cars of a machine test: the record cars, then the player car.
// Doesn't match: the game keeps records and the entries' offset apart (an
// offset stepped by 20 added to records), where GCC steps a pointer into the
// entries, and it gives list, records and playerCarId other registers.
#ifdef NON_MATCHING
void MachineTest_BuildCarList(MachineTestCarList *list, MachineTestRecords *records, unsigned int playerCarId)
{
    int count = records->count;
    int i;

    for (i = 0; i < count; i++) {
        MachineTestEntry *e = &records->entries[i];

        list->cars[i].carId = e->carId;
        strcpy(list->cars[i].name, CarInfoA_GetName(e->carId));
    }
    list->cars[count].carId = playerCarId;
    strcpy(list->cars[count].name, CarInfoA_GetName(playerCarId));
    list->count = count + 1;
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/garagecars", MachineTest_BuildCarList);
#endif
}
