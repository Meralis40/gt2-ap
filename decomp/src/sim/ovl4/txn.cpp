// OVL4 8001da24-8001e22c (txn): Shop transactions.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
int Car_GetNewPrice(unsigned int carCode);
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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/txn", Txn_SetSellCar);

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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/txn", Txn_SetBuyPart);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/txn", Txn_PreviewPart);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/txn", Txn_Back);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/txn", Txn_Decline);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/txn", Shop_ExecuteTransaction);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/txn", Shop_CheckTransaction);
}
