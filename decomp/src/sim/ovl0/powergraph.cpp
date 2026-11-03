// OVL0 80057628-80057a70 (powergraph): Power graph.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
// Clears the computed-curve flags; buffer receives the curves (0x434 bytes per
// candidate).
void PowerGraph_Reset(void *g, void *buffer)
{
    int iVar1;
    int iVar2;

    iVar2 = 9;
    iVar1 = (int)g + 9;
    *(unsigned char *)((int)g + 8) = 0;
    *(unsigned char *)((int)g + 9) = 0;
    *(void **)((int)g + 4) = buffer;
    do {
        *(unsigned char *)(iVar1 + 10) = 0;
        iVar2 = iVar2 + -1;
        iVar1 = iVar1 + -1;
    } while (-1 < iVar2);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/powergraph", PowerGraph_Start);

// Drops the candidates (count 0, flags cleared).
void PowerGraph_Clear(void *g)
{
    int iVar1;
    int iVar2;

    *(unsigned char *)((int)g + 9) = 0;
    iVar1 = 9;
    iVar2 = (int)g + 9;
    do {
        *(unsigned char *)(iVar2 + 10) = 0;
        iVar1 = iVar1 + -1;
        iVar2 = iVar2 + -1;
    } while (-1 < iVar1);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/powergraph", PowerGraph_Step);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/powergraph", PowerGraph_GetMax);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/powergraph", PowerGraph_GetPowerCurve);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/powergraph", PowerGraph_GetTorqueCurve);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/powergraph", PowerGraph_GetPeak);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/powergraph", PowerGraph_GetPower);
}
