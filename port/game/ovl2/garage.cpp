// OVL2 80013218-8001363c (garage): Garage sorting and the car buffer.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern GameState (&g_GameState);
extern game_pointer<void> (&g_EventDataBuf);
extern int (&g_GameDataMode);
extern game_pointer<void> (&g_GtModeRaceData);
extern unsigned char (&g_OVL2_CarGzBuf);

// order: the cars of garage sorted by name, by binary insertion.
void Garage_SortByName(Garage *garage, short *order)
{
    int sorted = 0;
    int i;

    for (i = 0; i < garage->carCount; i++) {
        char *name = CarInfoA_GetName((i + garage->cars)->baseCarId);
        int lo = 0;
        int hi = sorted;
        int pos = sorted;
        int j;
        short *p;

        if (sorted > 0) {
            while (1) {
                pos = (hi + lo) >> 1;
                if (strcmp(name, CarInfoA_GetName(garage->cars[order[pos]].baseCarId)) < 0) {
                    if (lo == pos) {
                        break;
                    }
                    hi = pos;
                } else {
                    if (lo == pos) {
                        pos = hi;
                        break;
                    }
                    lo = pos;
                }
            }
        }
        for (j = sorted, p = &order[sorted]; j > pos; j--, p--) {
            p[0] = p[-1];
        }
        order[pos] = i;
        sorted++;
    }
}

// Returns g_OVL2_CarGzBuf.
void *ArcadeMenu_GetCarGzBuf(void)
{
    return &g_OVL2_CarGzBuf;
}

// Sets a car from its id, colour and setup.
void Car_Init_OVL2(Car *car, unsigned int carId, int color, CarSetup *setup)
{
    car->carId = carId;
    car->color = color;
    car->setup = *setup;
}

// Adds a car when the garage has room: 1 when added.
int Garage_AppendCar(Garage *garage, unsigned int carId, int color, CarSetup *setup)
{
    int result;

    if (garage->carCount < 100) {
        Car_Init_OVL2(&garage->cars[garage->carCount], carId, color, setup);
        result = 1;
        garage->carCount += result;
    } else {
        result = 0;
    }
    return result;
}

// When the first garage is empty, fills both with cars of the game data.
void Garage_FillDebug(void)
{
    GameState *state = &g_GameState;
    Garage *guest;
    GameDataTable *table;
    CarSetup setup;
    unsigned short *rgb;
    char *codes;
    int count;
    int i;

    int cars = state->garages[0].carCount;

    guest = &state->garages[1];
    if (cars != 0) {
        return;
    }
    table = (GameDataTable *)((char *)g_EventDataBuf + 0x100);
    if (g_GameDataMode == 2) {
        table = (GameDataTable *)((char *)g_GtModeRaceData + 0x10);
    }
    count = table->count;
    for (i = 1; i < 0x66; i++) {
        int index = i % count;
        int code = index + 1;
        unsigned int carId = *(unsigned int *)GameDataTable_At(table, index);
        int colors = CarInfoA_GetColors(carId, &rgb, &codes);
        char *name = CarInfoA_GetName(carId);

        if (!((name == 0) | (colors <= 0))) {
            unsigned char color = codes[i % colors];

            CarSetup_FromCarEntry(GameData_GetCarEntry(code), &setup);
            Garage_AppendCar(&g_GameState.garages[0], carId, (signed char)color, &setup);
        }
        if (i < 4) {
            code = 0x12 - i % 18;
            carId = *(unsigned int *)GameDataTable_At(table, code - 1);
            colors = CarInfoA_GetColors(carId, &rgb, &codes);
            name = CarInfoA_GetName(carId);
            if (!((name == 0) | (colors <= 0))) {
                unsigned char color = codes[i % colors];

                CarSetup_FromCarEntry(GameData_GetCarEntry(code), &setup);
                Garage_AppendCar(guest, carId, (signed char)color, &setup);
            }
        }
    }
}
}
