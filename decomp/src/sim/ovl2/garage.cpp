// OVL2 80013218-8001363c (garage): Garage sorting and the car buffer.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char g_OVL2_CarGzBuf;

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garage", Garage_SortByName);

// Returns g_OVL2_CarGzBuf.
void *ArcadeMenu_GetCarGzBuf(void)
{
    return &g_OVL2_CarGzBuf;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garage", Car_Init_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garage", Garage_AppendCar);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garage", Garage_FillDebug);
}
