// OVL4 80016fec-80017750 (carwork2): CarWork helpers: setup export, part slots,
// loading garage cars, the installed parts summary, racing modification variants.
// Assembled by GNU as (GAS_CXX), for FUN_OVL4__8001719c's loop label.
#include "ovl4.h"

// CarSpec_FromCarWork's output and the two CarParams buffers, which ovl4.h doesn't
// have.
struct CarSpec {
    unsigned char data[0xc8];
};

struct CarParams {
    unsigned char data[0x1c0];
};

// A row of the installed parts summary: the tune slot (-1 ends the list), the part
// of its stage 1, and the text offset.
struct TuneSummaryRow {
    short slot;
    short nameBase;
    short dx;
    short dy;
};

extern "C" {
void CarWork_LoadFromGarage(int slot, int garage);
extern CarWork g_CarWork;
int CarWork_CountRacingModStages(CarWork *work);
void CarWork_SetPartSlot(CarWork *work, int slot, int stage);
void CarWork_Clear(CarWork *work);
void CarWork_LoadParts(CarWork *work, unsigned int carId);
void CarWork_SetStagesFromSetup(CarWork *work, CarSetup *setup);
void CarSpec_FromCarWork(CarWork *work, CarSpec *spec);
void CarParams_Build(CarSpec *spec, CarParams *params);
int CarParams_BuildFromSetup(CarSetup *setup, CarParams *params);
unsigned short *PartName_GetW(int index);
void GTFont_DrawW(void *ot, unsigned short *str, int x, int y, int rgb, int flags);
extern GameState g_GameState;
extern char g_RaceRecords[];
extern TuneSummaryRow g_TuneSummaryRows[];
extern short g_RacingMod_Variant;

// Refits the transmission stage, marks the setup and copies it to out.
void CarWork_ExportSetup(CarWork *work, CarSetup *out)
{
    CarWork_SetPartSlot(work, 6, work->partSlots[6]);
    work->setup.flags_7a |= 0xc0;
    *out = work->setup;
}

// The tune slot of a part and its stage in that slot (1 = the slot's first part).
void Part_GetSlot(PartId partId, short *out)
{
    short base = 0;
    short slot = 0;

    switch (partId) {
    case 35: // suspension
    case 36:
    case 37:
        base = 35;
        slot = 0;
        break;
    case 1: // sports brakes
        base = 1;
        slot = 1;
        break;
    case 2: // brake balance
        base = 2;
        slot = 2;
        break;
    case 13: // transmission
    case 14:
    case 15:
        base = 13;
        slot = 6;
        break;
    case 3: // clutch
    case 4:
    case 5:
        base = 3;
        slot = 7;
        break;
    case 10: // flywheel
    case 11:
    case 12:
        base = 10;
        slot = 8;
        break;
    case 33: // carbon driveshaft
        base = 33;
        slot = 9;
        break;
    case 6: // computer chip
        base = 6;
        slot = 10;
        break;
    case 32: // port polishing
        base = 32;
        slot = 11;
        break;
    case 9: // engine balancing
        base = 9;
        slot = 12;
        break;
    case 29: // NA tuning
    case 30:
    case 31:
        base = 29;
        slot = 13;
        break;
    case 7: // displacement
        base = 7;
        slot = 14;
        break;
    case 46: // turbo
    case 47:
    case 48:
    case 49:
        base = 46;
        slot = 15;
        break;
    case 16: // intercooler
    case 17:
        base = 16;
        slot = 16;
        break;
    case 26: // muffler
    case 27:
    case 28:
        base = 26;
        slot = 17;
        break;
    case 18: // weight reduction
    case 19:
    case 20:
        base = 18;
        slot = 18;
        break;
    case 34: // racing modification
        base = 34;
        slot = 19;
        break;
    case 0: // ASC
        base = 0;
        slot = 20;
        break;
    case 38: // TCS
        base = 38;
        slot = 21;
        break;
    case 21: // LSD and yaw control
    case 22:
    case 23:
    case 24:
    case 25:
        base = 21;
        slot = 22;
        break;
    }
    out[1] = partId - base + 1;
    out[0] = slot;
}

// An empty loop over the garage's cars.
void FUN_OVL4__8001719c(void)
{
    int i = 0;
    int more;

    do {
        more = i < g_GameState.garages[0].carCount;
        i++;
    } while (more);
}

// Builds the current car's parameters from g_CarWork twice (from its CarSpec and
// from its setup) and drops them. The empty loops count over the setup and the
// parameters.
void CarWork_RebuildParams(void)
{
    CarParams params;
    CarParams built;
    CarSpec spec;
    CarSetup setup;
    int i;

    if (g_GameState.garages[0].currentCar >= 0) {
        for (i = 0; i < (int)sizeof(CarSetup); i++)
            ;
        CarSpec_FromCarWork(&g_CarWork, &spec);
        CarParams_Build(&spec, &built);
        setup = g_CarWork.setup;
        CarParams_BuildFromSetup(&setup, &params);
        for (i = 0; i < (int)sizeof(CarParams); i++)
            ;
    }
}

// Loads garage car slot into g_RaceCarWorkAlt (0x801d5fa0, past the end of
// g_GameState).
void ListCarWork_Load(int slot)
{
    GameState *gs = &g_GameState;
    char *base = (char *)gs;
    int offset = 0xc6c0;
    CarWork *work = (CarWork *)((int)(base + offset) & ~3);
    Garage *g = &gs->garages[0];
    Car *car;

    CarWork_Clear(work);
    car = &g->cars[slot];
    CarWork_LoadParts(work, car->carId);
    CarWork_SetStagesFromSetup(work, &car->setup);
}

// Draws the name of every part fitted to g_RaceCarWorkAlt's car (0x801d5fa0, taken
// as g_RaceRecords + 0x118).
void Tune_DrawInstalledParts(void *ot, int x, int y)
{
    char *base = g_RaceRecords;
    int offset = 0x118;
    CarWork *work = (CarWork *)((int)(base + offset) & ~3);
    TuneSummaryRow *row;

    row = g_TuneSummaryRows;
loop:
    if (row->slot >= 0) {
        int stage = *(work->partSlots + row->slot);

        if (stage > 0) {
            GTFont_DrawW(ot, PartName_GetW(row->nameBase + stage - 1), x + row->dx, y + row->dy, 0x606060, 0);
        }
        row++;
        goto loop;
    }
}

// Loads car slot of garage into g_CarWork.
void CarWork_LoadFromGarage(int slot, int garage)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garage];
    Car *car;

    CarWork_Clear(&g_CarWork);
    car = &g->cars[slot];
    CarWork_LoadParts(&g_CarWork, car->carId);
    CarWork_SetStagesFromSetup(&g_CarWork, &car->setup);
}

// Loads garage car slot into g_CarWork (CarWork_LoadGarageCar).
void Garage_LoadCar(int slot, int garage)
{
    CarWork_LoadFromGarage(slot, garage);
}

/* &g_CarWork. */

CarSetup *CarWork_Get(void)
{
    return &g_CarWork.setup;
}

/* Whether the current car has racing modification variants. */

int RacingMod_HasVariants(void)
{
    int iVar1;

    iVar1 = CarWork_CountRacingModStages(&g_CarWork);
    return (unsigned int)(0 < iVar1);
}

/* CarWork_CountRacingModStages(&g_CarWork): number of leading non-negative ints at g_CarWork
      +0x15e0 (max 4). Kind 0x9d hides its element when < 2 (tentative: options with a single choice).
        */

int CarWork_CountSlots(void)
{
    int iVar1;

    iVar1 = CarWork_CountRacingModStages(&g_CarWork);
    return iVar1;
}

// Selects the first racing modification variant and returns its word at +8.
int RacingMod_FirstVariant(void)
{
    CarWork_CountRacingModStages(&g_CarWork);
    g_RacingMod_Variant = 1;
    return *(int *)(g_CarWork.racingModEntries[1] + 8);
}

// Selects the next racing modification variant (back to the first after the
// last) and returns its word at +8.
int RacingMod_NextVariant(void)
{
    CarWork *work = &g_CarWork;
    int count = CarWork_CountRacingModStages(work);

    if (count < ++g_RacingMod_Variant) {
        g_RacingMod_Variant = 1;
    }
    return *(int *)(work->racingModEntries[g_RacingMod_Variant] + 8);
}

/* g_CarWork.turbo == 0 */

int CarWork_HasNoTurbo(void)
{
    return (unsigned int)(g_CarWork.setup.turbo == 0);
}

/* Part slot 0x13 (racing modification) fitted. */

int CarWork_HasRacingMod(void)
{
    return (unsigned int)(0 < g_CarWork.partSlots[0x13]);
}

/* No part fitted in any tuning slot (slots 3-5 ignored). */

int CarWork_IsStock(void)
{
    unsigned int uVar1;

    uVar1 = (unsigned int)(g_CarWork.partSlots[0] < 1);
    if (0 < g_CarWork.partSlots[1]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[2]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[6]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[7]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[8]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[9]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[10]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xb]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xc]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xd]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xe]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0xf]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x10]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x11]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x12]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x13]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x14]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x15]) {
        uVar1 = 0;
    }
    if (0 < g_CarWork.partSlots[0x16]) {
        uVar1 = 0;
    }
    return uVar1;
}
}
