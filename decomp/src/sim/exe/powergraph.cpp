// Resident code 80073ce4-80074844 (powergraph). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void PowerGraphView_ComputeScale(short *v);

void PowerGraphView_Init(short *v)
{
    v[0x13] = -1;
    v[0x1c] = *v + 0x44;
}

void PowerGraphView_Show(short *v)
{
    PowerGraphView_ComputeScale(v);
    v[0x13] = 0;
    v[0x1c] = *v + 0x44;
}

void PowerGraphView_Close(short *v)
{
    if (-1 < v[0x13]) {
        v[0x13] = -0x10;
    }
}

void PowerGraphView_Update(short *v)
{
    short sVar1;

    if (v[0x13] < 0) {
        if (v[0x13] < -1) {
            v[0x13] = v[0x13] + 1;
            return;
        }
    }
    else {
        sVar1 = v[0x13] + 1;
        v[0x13] = sVar1;
        if (v[0x1c] < sVar1) {
            v[0x13] = *v + 8;
        }
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/powergraph", PowerGraphView_ComputeScale);

INCLUDE_ASM("asm/sim/exe/nonmatchings/powergraph", PowerGraphView_DrawAxis);

INCLUDE_ASM("asm/sim/exe/nonmatchings/powergraph", PowerGraphView_DrawRpmAxis);

INCLUDE_ASM("asm/sim/exe/nonmatchings/powergraph", PowerGraphView_DrawCurve);

INCLUDE_ASM("asm/sim/exe/nonmatchings/powergraph", PowerGraphView_DrawGrid);
}
