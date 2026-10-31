// OVL4 8001da24-8001e22c (txn): Shop transactions.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

// The resident CarParams (0x1c0 bytes), EnginePerf (0x6c bytes) and CarSpec
// (0xc4 bytes), of which only the power is read here.
struct TxnCarParams {
    unsigned char data[0x1c0];
};

struct TxnEnginePerf {
    unsigned short maxPower;
    unsigned char pad02[0x6a];
};

struct TxnCarSpec {
    unsigned char data[0xc4];
};

extern "C" {
int Car_GetNewPrice(unsigned int carCode);
int Car_GetSellPrice(unsigned int carId);
int Part_GetPrice(unsigned int carCode, PartId partId);
int Car_HasPart(Car *car, int part);
CarSetup *CarWork_Get(void);
int CarParams_BuildFromSetup(CarSetup *setup, TxnCarParams *params);
int CarParams_GetPerformance(TxnCarParams *params, TxnEnginePerf *out);
void CarWork_BuildSpecWithPart(int slot, int part, int garage, TxnCarSpec *out);
int CarParams_Build(TxnCarSpec *spec, TxnCarParams *params);
void CarWork_RebuildParams(void);
void Garage_SellCar(int carSlot, int garageIdx);
void Garage_LoadCar(int slot, int garage);
int Garage_CanAfford(int price, int garageIdx);
int Dealer_CanBuyNewCar(int carId, int price, int garageIdx);
int Dealer_BuyNewCar(int carId, int color, int price, int garageIdx);
int Tune_CanBuyPart(int carSlot, int partId, int garageIdx);
int Tune_BuyPart(int carSlot, int partId, int garageIdx);
int Tune_InstallPart(int carSlot, int partId, int garageIdx, unsigned int color);
int WheelShop_BuyWheels(int carSlot, int wheelCode, int price, int garageIdx);
extern GameState g_GameState;

// Type 0, price 0.
void Txn_Init(ShopTxn *txn)
{
    txn->type = 0;
    txn->price = 0;
}

// Type 0 (no transaction).
void Txn_Clear(ShopTxn *txn)
{
    txn->type = 0;
}

// Type 6: sell the car in garage slot slot, at its selling price.
// One operand order: the game adds the slot's offset to g_GameState's address
// (addu a0,a0,v0), where GCC adds the address to the offset. CSE puts an
// operand known to be a constant second (fold_rtx), so the game's base wasn't
// known to CSE; an empty asm("" : "=r"(gs) : "0"(&g_GameState)) gives the
// game's add, but its lui is then scheduled one store early. Reading the slot
// back from txn gives the game's copy of it. Tried: a GameState or Garage
// pointer, a Car pointer stepped by the slot, cars + slot, the address as a
// char pointer, a byte offset in a local, an inline returning &g_GameState or
// the car, the slot cast to unsigned or short, and the pointer pinned to a0.
#ifdef NON_MATCHING
void Txn_SetSellCar(ShopTxn *txn, int slot)
{
    txn->sellSlot = slot;
    txn->type = 6;
    txn->price = Car_GetSellPrice(g_GameState.garages[0].cars[txn->sellSlot].carId);
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/txn", Txn_SetSellCar);
#endif

// Type 7: make garage car slot the current car (garage "change car", element 0xab).
// Shop_CheckTransaction applies it at once.
void Txn_SetSelectCar(ShopTxn *txn, int slot)
{
    txn->selectSlot = slot;
    txn->type = 7;
}

// Transaction type 1; price 0 means Car_GetNewPrice(carCode). Called by SimMap_Update
// for menu element action 6 (dealer car tile, car code at element +0x10).
void Txn_SetBuyNewCar(ShopTxn *txn, unsigned int carCode, int price)
{
    int iVar1;

    txn->type = 1;
    txn->carCode = carCode;
    if (price == 0) {
        iVar1 = Car_GetNewPrice(carCode);
        txn->price = iVar1;
    }
    else {
        txn->price = price;
    }
}

// Type 5 (wheels) for the current car, no wheel chosen yet (price 0). Set on entering
// a page with wheel elements (bit 28).
void Txn_InitWheelShop(ShopTxn *txn)
{
    int iVar1;

    iVar1 = (int)g_GameState.garages[0].currentCar;
    txn->type = 5;
    txn->price = 0;
    txn->wheelSlot = iVar1;
}

// Type 5: wheels wheelCode at 2000 Cr.
void Txn_SetBuyWheels(ShopTxn *txn, int wheelCode)
{
    txn->type = 5;
    txn->wheelCode = wheelCode;
    txn->price = 2000;
}

// Type 3: buy part for the current car, at its price.
void Txn_SetBuyPart(ShopTxn *txn, int part)
{
    GameState *gs = &g_GameState;
    Garage *garage = &gs->garages[0];
    int slot = garage->currentCar;

    txn->part = part;
    txn->partSlot = slot;
    txn->type = 3;
    txn->price = Part_GetPrice(garage->cars[txn->partSlot].carId, part);
}

// Fills in a part's price, whether the current car has it, and the car's power
// now and with the part (-1 when the part has no price).
void Txn_PreviewPart(ShopTxn *txn, int part)
{
    GameState *gs = &g_GameState;
    Garage *garage = &gs->garages[0];
    int slot = garage->currentCar;
    Car *car = &garage->cars[slot];
    TxnCarParams params;
    TxnEnginePerf perf;
    CarSetup setup;
    TxnCarSpec spec;

    txn->price = Part_GetPrice(car->carId, part);
    txn->partOwned = Car_HasPart(car, part);
    setup = *CarWork_Get();
    CarParams_BuildFromSetup(&setup, &params);
    CarParams_GetPerformance(&params, &perf);
    txn->powerNow = perf.maxPower;
    txn->powerWithPart = -1;
    if (txn->price >= 0) {
        CarWork_BuildSpecWithPart(slot, part, 0, &spec);
        CarParams_Build(&spec, &params);
        CarParams_GetPerformance(&params, &perf);
        txn->powerWithPart = perf.maxPower;
    }
}

// Steps back from a transaction's confirmation: a bought car or an owned part
// goes back to buying, and the rest stay. 1 for a sale.
int Txn_Back(ShopTxn *txn)
{
    int r = 0;

    switch (txn->type) {
    case 0:
        break;
    case 6:
        txn->type = 6;
        r = 1;
        break;
    case 7:
        txn->type = 7;
        break;
    case 1:
    case 2:
        txn->type = 1;
        break;
    case 3:
    case 4:
        txn->type = 3;
        break;
    case 5:
        txn->type = 5;
        break;
    }
    return r;
}

// The same for a declined confirmation, with nothing returned.
void Txn_Decline(ShopTxn *txn)
{
    switch (txn->type) {
    case 6:
        txn->type = 6;
        break;
    case 1:
    case 2:
        txn->type = 1;
        break;
    case 3:
    case 4:
        txn->type = 3;
        break;
    case 5:
        txn->type = 5;
        break;
    }
}

// Carries out the transaction and sets the type that follows it. Returns the
// message page to show (top bit set), or 0.
int Shop_ExecuteTransaction(ShopTxn *txn)
{
    GameState *gs = &g_GameState;
    Garage *garage = &gs->garages[0];
    int type = 0;
    int msg = 0;
    int slot;

    switch (txn->type) {
    case 6:
        Garage_SellCar(txn->sellSlot, 0);
        type = 6;
        msg = 0x8000001c;
        break;
    case 1:
        Dealer_BuyNewCar(txn->carCode, txn->color, txn->price, 0);
        if (garage->currentCar >= 0) {
            type = 2;
            msg = 0x80000008;
        } else {
            slot = garage->carCount;
            slot--;
            garage->currentCar = slot;
            Garage_LoadCar(slot, 0);
            type = 1;
            msg = 0x80000004;
        }
        break;
    case 2:
        slot = garage->carCount;
        slot--;
        garage->currentCar = slot;
        Garage_LoadCar(slot, 0);
        type = 1;
        msg = 0x80000005;
        break;
    case 3:
        Tune_BuyPart(txn->partSlot, txn->part, 0);
        switch (txn->part) {
        case PART_DISPLACEMENT:
        case PART_ENGINE_BALANCING:
        case PART_PORT_POLISH:
            Tune_InstallPart(txn->partSlot, txn->part, 0, txn->color);
            type = 3;
            msg = 0x80000006;
            break;
        case PART_WEIGHT_RED_1:
        case PART_WEIGHT_RED_2:
        case PART_WEIGHT_RED_3:
        case PART_RACING_MOD:
            Tune_InstallPart(txn->partSlot, txn->part, 0, txn->color);
            type = 3;
            msg = 0x8000001f;
            break;
        default:
            type = 4;
            msg = 0x80000007;
            break;
        }
        break;
    case 4:
        Tune_InstallPart(txn->partSlot, txn->part, 0, txn->color);
        type = 3;
        switch (txn->part) {
        case PART_PERF_CHIP:
        case PART_INTERCOOLER_SPORTS:
        case PART_INTERCOOLER_RACING:
        case PART_MUFFLER_SPORTS:
        case PART_MUFFLER_SEMI:
        case PART_MUFFLER_RACING:
        case PART_NA_TUNE_1:
        case PART_NA_TUNE_2:
        case PART_NA_TUNE_3:
        case PART_TURBO_1:
        case PART_TURBO_2:
        case PART_TURBO_3:
        case PART_TURBO_4:
            msg = 0x80000009;
            break;
        default:
            msg = 0;
            break;
        }
        break;
    case 5:
        WheelShop_BuyWheels(txn->wheelSlot, txn->wheelCode, txn->price, 0);
        msg = 0;
        break;
    }
    CarWork_RebuildParams();
    txn->type = type;
    return msg;
}

// Makes slot the garage's current car.
static inline void Garage_SetCurrentCar(Garage *garage, int slot)
{
    garage->currentCar = slot;
}

// Checks whether the transaction can go ahead. Returns the message page to show
// (top bit set), or 0. A car selection is applied at once.
int Shop_CheckTransaction(ShopTxn *txn)
{
    GameState *gs = &g_GameState;
    Garage *garage = &gs->garages[0];
    int msg = 0;

    switch (txn->type) {
    case 5:
        if (txn->price == 0)
            return 0;
        if (txn->wheelSlot < 0) {
            msg = 0x80000002;
            break;
        }
        switch (Garage_CanAfford(txn->price, 0)) {
        case -1:
            msg = 0x80000001;
            break;
        case 1:
            msg = 0x80000020;
            break;
        }
        break;
    case 6:
        msg = 0x8000000d;
        if (txn->sellSlot < 0)
            msg = 0x80000002;
        break;
    case 7:
        msg = 0x80000005;
        Garage_SetCurrentCar(garage, txn->selectSlot);
        Garage_LoadCar(txn->selectSlot, 0);
        break;
    case 1:
        switch (Dealer_CanBuyNewCar(txn->carCode, txn->price, 0)) {
        case -1:
            msg = 0x80000001;
            break;
        case -2:
            msg = 0x8000000e;
            break;
        case 1:
            msg = 0x80000000;
            break;
        }
        break;
    case 3:
        if (txn->partSlot < 0) {
            msg = 0x80000002;
            break;
        }
        switch (Tune_CanBuyPart(txn->partSlot, txn->part, 0)) {
        case -5:
            msg = 0x8000000f;
            break;
        case -6:
            msg = 0x80000010;
            break;
        case -7:
            msg = 0x80000011;
            break;
        case -8:
            msg = 0x80000019;
            break;
        case -3:
            msg = 0x80000003;
            break;
        case -1:
            msg = 0x80000001;
            break;
        case 1:
            switch (txn->part) {
            case PART_DISPLACEMENT:
            case PART_ENGINE_BALANCING:
            case PART_PORT_POLISH:
            case PART_WEIGHT_RED_1:
            case PART_WEIGHT_RED_2:
            case PART_WEIGHT_RED_3:
            case PART_RACING_MOD:
                msg = 0x8000000b;
                break;
            default:
                msg = 0x80000020;
                break;
            }
            break;
        }
        break;
    }
    return msg;
}
}
