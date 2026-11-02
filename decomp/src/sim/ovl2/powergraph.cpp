// OVL2 800121e0-80013218 (powergraph): Power graph: another version of the resident
// PowerGraphView.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/powergraph", PowerGraphView_Init_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/powergraph", PowerGraphView_Show_OVL2);

// Hidden at once (counter -1).
void PowerGraphView_Hide_OVL2(short *v)
{
    v[0x18] = -1;
}

// Copy of PowerGraphView_Close.
void PowerGraphView_Close_OVL2(short *v)
{
    if (0 < v[0x18]) {
        v[0x18] = -0x10;
    }
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/powergraph", PowerGraphView_Update_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/powergraph", PowerGraphView_ComputeScale_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/powergraph", PowerGraphView_DrawAxis_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/powergraph", PowerGraphView_DrawRpmAxis_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/powergraph", PowerGraphView_DrawCurve_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/powergraph", PowerGraphView_Draw_OVL2);
}
