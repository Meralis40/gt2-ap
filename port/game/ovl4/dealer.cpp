// OVL4 80017750-800183ec (dealer): Dealer, tuning shop and garage sales.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"
#include "ap.h"

extern "C" unsigned int (&g_MenuPageAttr);

// CarSpec_FromCarWorkWithPart's output, a resident type that ovl4.h doesn't have.
struct CarSpec;

extern "C" {
extern GameState (&g_GameState);
extern unsigned short (&g_GarageCar_NoMark)[2];
extern unsigned short (&g_GarageCar_Mark)[2];
extern short (&g_PartNameStrIds);
extern CarWork (&g_CarWork);
extern short (&g_RacingMod_Variant);
extern short (&g_IncludedTireParts)[];
extern game_pointer<signed char> (&g_Car_ExcludedCodes)[];
extern unsigned short (&u_NoName)[];

// Car_HasPart(...) == 1, kept as a value: the game tests it with xori.
inline int Car_PartFitted(Car *car, int part)
{
    return Car_HasPart(car, part) == 1;
}

// Completes a car's setup through g_RaceCarWork (g_GameState + 0x10bd8, rounded
// down to a word): loads the car's parts, fits the setup's stages and exports the
// setup back. Returns whether the racing modification is fitted.
int CarSetup_Finalize(unsigned int carId, CarSetup *setup)
{
    GameState *gs = &g_GameState;
    char *base = (char *)gs;
    int offset = 0x10bd8;
    CarWork *work = (CarWork *)rt::from_guest(((int)rt::to_guest((base + offset)) & ~3));

    CarWork_Clear(work);
    CarWork_LoadParts(work, carId);
    CarWork_SetStagesFromSetup(work, setup);
    CarWork_ExportSetup(work, setup);
    return work->partSlots[0x13] > 0;
}

// The price of a new car (word 0x11 of its table 0x1e entry), 0 when it isn't there.
int Car_GetNewPrice(unsigned int carCode)
{
    unsigned int *entry = (unsigned int *)GameData_FindEntryById((GameData *)GameData_GetGtMode(), 0x1e, carCode);

    if (entry)
        return entry[0x11];
    return 0;
}

// The tyres that come with a new car, as part bits in partsOwned: the tyre parts
// (g_IncludedTireParts) available for it. Null when the car's entry has none.
unsigned char *Car_GetIncludedTires(unsigned int carCode, unsigned char *partsOwned)
{
    unsigned char *entry = (unsigned char *)GameData_FindEntryById((GameData *)GameData_GetGtMode(), 0x1e, carCode);
    short *part;
    int id;
    int i = 6;

    if (entry[0x40] == 0)
        return 0;
    {
        unsigned char *p = partsOwned + i;

        do {
            *p-- = 0;
        } while (--i >= 0);
    }
    part = g_IncludedTireParts;
    id = *part;
    while (id >= 0) {
        if (Part_Lookup(id, carCode, 0) >= 0)
            PartsOwned_Set(id, partsOwned);
        part++;
        id = *part;
    }
    return partsOwned;
}

// Byte +9 of the stock transmission entry (table 0x11) < 3. Gives Car.flags98 bit 14
// of a new car; meaning unknown.
int Car_GetTransFlag14(unsigned int carId)
{
    void *pvVar1;
    CarSetup CStack_90;

    Car_GetDefaultSetup(carId, &CStack_90);
    pvVar1 = GameData_GetEntry(0x11, (unsigned int)(unsigned short)CStack_90.idx_t11);
    return (unsigned int)(*(unsigned char *)rt::from_guest(((int)rt::to_guest(pvVar1) + 9)) < 3);
}

// 1, or -1 without the credits, -2 with a full garage.
int Dealer_CanBuyNewCar(int carId, int price, int garageIdx)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garageIdx];

    // A used car lot is part of its manufacturer's dealership, so this is set there too.
    if (Ap_Active() && !Ap_HasManufacturer(g_MenuPageAttr & 0xff)) {
        Ap_SayManufacturerLocked(g_MenuPageAttr & 0xff);
        return 0;
    }
    if (g->credits < price)
        return -1;
    if (g->carCount == 100)
        return -2;
    return 1;
}

// Buys a new car into the garage: 1, or -1 without the credits, -2 when the garage
// can't take it.
int Dealer_BuyNewCar(int carId, int color, int price, int garageIdx)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garageIdx];
    CarSetup setup;
    unsigned char parts[8];
    int racingMod;
    int flag14;
    int added;

    Car_GetDefaultSetup(carId, &setup);
    racingMod = CarSetup_Finalize(carId, &setup);
    if (g->credits < price)
        return -1;
    flag14 = Car_GetTransFlag14(carId);
    added = Garage_AddNewCar(g, carId, color, &setup, price, racingMod, flag14, Car_GetIncludedTires(carId, parts)) == 1;
    if (!added)
        return -2;
    g->credits -= price;
    return 1;
}

// Always 0.
int Car_GetSellPrice(unsigned int carId)
{
    return 0;
}

// Sells a garage car for a quarter of its new price.
void Garage_SellCar(int carSlot, int garageIdx)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garageIdx];
    int price = Car_GetNewPrice((g->cars + carSlot)->carId);

    Garage_RemoveCar(g, carSlot);
    Garage_AddCredits(g, price / 4);
}

// -1 when the part is not available for the car.
int Part_GetPrice(unsigned int carCode, PartId partId)
{
    int iVar1;
    int local_10 [2];

    iVar1 = Part_Lookup(partId, carCode, local_10);
    if (iVar1 < 0) {
        local_10[0] = -1;
    }
    return local_10[0];
}

// 1, or -8 when the part isn't available for the car, -1 without the credits, -3
// when it is fitted already, -5, -6 and -7 when weight reduction stages 2 and 3 and
// the racing modification lack the previous stage.
int Tune_CanBuyPart(int carSlot, int partId, int garageIdx)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garageIdx];
    Car *car = &g->cars[carSlot];
    int price = Part_GetPrice(car->carId, partId);

    if (Part_Lookup(partId, car->carId, 0) < 0)
        return -8;
    if (Ap_Active() && !Ap_HasPart(partId)) {
        Ap_SayPartLocked(partId);
        return -8;
    }
    if (g->credits < price)
        return -1;
    if (Car_HasPart(car, partId))
        return -3;
    switch (partId) {
    case 19:
        if (!Car_PartFitted(car, 18))
            return -5;
        break;
    case 20:
        if (!Car_PartFitted(car, 19))
            return -6;
        break;
    case 34:
        if (!Car_PartFitted(car, 20))
            return -7;
        break;
    }
    return 1;
}

// Buys a part for a garage car: Tune_CanBuyPart's result.
int Tune_BuyPart(int carSlot, int partId, int garageIdx)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garageIdx];
    Car *car = &g->cars[carSlot];
    int cost = Part_GetPrice(car->carId, partId);
    int result = Tune_CanBuyPart(carSlot, partId, garageIdx);

    if (result != 1)
        return result;
    Car_InstallPart(car, partId, cost);
    g->credits -= cost;
    return 1;
}

// Fits an owned part to a garage car through g_CarWork: 1, or -4 when the car doesn't
// have it, -8 when it isn't available. The racing modification also sets the car's
// colour and model; tyres (39-45) go in both tyre slots.
int Tune_InstallPart(int carSlot, int partId, int garageIdx, unsigned int color)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garageIdx];
    Car *car = &g->cars[carSlot];

    if (!Car_PartFitted(car, partId))
        return -4;
    if (Part_Lookup(partId, car->carId, 0) < 0)
        return -8;
    if (partId == 34) {
        CarWork *work = &g_CarWork;
        unsigned int model;

        CarWork_SetPartSlot(work, 0x13, g_RacingMod_Variant);
        model = *(int *)(work->racingMod[g_RacingMod_Variant].bytes + 8);
        car->color = color;
        car->baseCarId = model;
        Car_StoreSetup(car, work);
    } else {
        short slot[2];

        if (partId >= 34) {
            if (partId < 46) {
                if (partId >= 39) {
                    CarWork_SetPartSlot(&g_CarWork, 3, partId - 38);
                    CarWork_SetPartSlot(&g_CarWork, 4, partId - 38);
                    Car_StoreSetup(car, &g_CarWork);
                    return 1;
                }
            }
        }
        Part_GetSlot(partId, slot);
        CarWork_SetPartSlot(&g_CarWork, slot[0], slot[1]);
        Car_StoreSetup(&g->cars[carSlot], &g_CarWork);
    }
    return 1;
}

// g_CarWork's CarSpec with part fitted (tyres not), for the tuning shop's preview.
void CarWork_BuildSpecWithPart(int slot, int part, int garage, CarSpec *out)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garage];

    CarSpec_FromCarWorkWithPart(&g_CarWork, out, -1, 0);
    if (Part_Lookup(part, (g->cars + slot)->carId, 0) >= 0) {
        short s[2];

        if (part < 46) {
            if (part >= 39)
                return;
        }
        Part_GetSlot(part, s);
        CarSpec_FromCarWorkWithPart(&g_CarWork, out, s[0], s[1]);
    }
}

// Loads the player's car for a race: fits the dirt tyres (part 45) for a dirt race
// when the car has them, else takes them off.
void GTMode_PreparePlayerCar(int slot, int garage, int dirtRace)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garage];
    Car *car;

    CarWork_LoadFromGarage(slot, garage);
    car = &g->cars[slot];
    if (dirtRace & Car_HasPart(car, 45)) {
        Tune_InstallPart(slot, 45, 0, 0);
    } else {
        CarWork *work = &g_CarWork;

        if ((work->partSlots[3] == 7) | (work->partSlots[4] == 7)) {
            CarWork_SetPartSlot(work, 3, 0);
            CarWork_SetPartSlot(work, 4, 0);
            Car_StoreSetup(car, work);
        }
    }
}

// Buys wheels for a garage car: 1, or Garage_CanAfford's -1.
int WheelShop_BuyWheels(int carSlot, int wheelCode, int price, int garageIdx)
{
    GameState *gs = &g_GameState;
    Garage *g = &gs->garages[garageIdx];
    int result = Garage_CanAfford(price, garageIdx);

    if (result != 1)
        return result;
    CarSetup_SetWheels(&g_CarWork.setup, Wheel_FindIndex(wheelCode));
    Car_StoreSetup(&g->cars[carSlot], &g_CarWork);
    g->credits -= price;
    return 1;
}

// 1, or -1 without the credits.
int Garage_CanAfford(int price, int garageIdx)
{
    if (g_GameState.garages[garageIdx].credits < price)
        return -1;
    return 1;
}

// 0 when the car's code is in g_Car_ExcludedCodes.
int Car_IsNotExcluded(unsigned int carId)
{
    game_pointer<signed char> *p = g_Car_ExcludedCodes;
    signed char *code = *p;

    while (code) {
        if (carId == CarCode_Encode(code))
            return 0;
        code = *++p;
    }
    return 1;
}

/* Credits >= 50. */

int CarWash_CanAfford(void)
{
    return g_GameState.garages[0].credits < 0x32 ^ 1;
}

// Mark shown before a garage car name: the glyph 0x7f when flag (bit 15 of flags98) is
// set, else an empty string.
unsigned short *GarageCar_GetMark(int flag)
{
    unsigned short *puVar1;

    puVar1 = g_GarageCar_NoMark;
    if (flag != 0) {
        puVar1 = g_GarageCar_Mark;
    }
    return puVar1;
}

// The car's maker name (string id at +0x3c of its table 0x1e entry).
unsigned short *Car_GetMakerName(unsigned int carId)
{
    unsigned char *entry = (unsigned char *)GameData_FindEntryById((GameData *)GameData_GetGtMode(), 0x1e, carId);

    if (entry)
        return (unsigned short *)UniStr_Get(*(unsigned short *)(entry + 0x3c));
    return u_NoName;
}

// The car's model name (string id at +0x3e of its table 0x1e entry).
unsigned short *Car_GetModelName(unsigned int carId)
{
    unsigned char *entry = (unsigned char *)GameData_FindEntryById((GameData *)GameData_GetGtMode(), 0x1e, carId);

    if (entry)
        return (unsigned short *)UniStr_Get(*(unsigned short *)(entry + 0x3e));
    return u_NoName;
}

// The index of colorCode among the car's colours, 0 when it isn't one.
int CarColor_FindIndex(unsigned int carId, int colorCode)
{
    unsigned short *rgb;
    signed char *codes;
    int count = CarInfoA_GetColors(carId, &rgb, (char **)&codes);
    int i;

    for (i = 0; i < count; i++) {
        if (codes[i] == colorCode)
            return i;
    }
    return 0;
}

// String g_PartNameStrIds[index] from the game string table (UniStr_Get).
unsigned short *PartName_GetW(int index)
{
    unsigned short *puVar1;

    puVar1 = (unsigned short *)UniStr_Get((int)(&g_PartNameStrIds)[index]);
    return puVar1;
}
}
