// Resident code 80074844-80075fac (enginecurve). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int EngineCurve_BoostFactor(EngineCurve *curve, short rpm1, short rpm2);
void EngineCurve_Build(CarParams *params, EngineCurve *curve, int unused);
void EngineCurve_GetPerf(EngineCurve *curve, EnginePerf *out);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Lerp_Clamped);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_Resample);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", CarNameTable_Find);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Gearbox_ComputeRatios);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Gearbox_ComputeLimits);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_BoostFactor);

int EngineCurve_BoostFactorClamped(EngineCurve *curve, short rpm1, short rpm2)
{
    int iVar1;
    int iVar2;

    iVar1 = EngineCurve_BoostFactor(curve, rpm1, rpm2);
    iVar2 = curve->maxBoost + 0x1000;
    if (iVar2 < iVar1) {
        iVar1 = iVar2;
    }
    return iVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_SetupTurbo);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_Build);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_GetPerf);

int CarParams_GetPerformance(CarParams *params, EnginePerf *out)
{
    EngineCurve EStack_b8;

    EngineCurve_Build(params, &EStack_b8, 0);
    EngineCurve_GetPerf(&EStack_b8, out);
    return (unsigned int)(unsigned short)out->maxPower;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul32);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul32_16);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_800759ac);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul32Shift);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_800759fc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_80075a2c);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul16);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_80075acc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_MulShiftTrunc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul16Shift);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_80075ba4);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_MulFloor);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_MulFloor16);

long long FUN_80075c34(int param_1, int param_2)
{
    return (long long)param_1 * (long long)param_2 >> 8;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_MulShift);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_80075c9c);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_80075ce4);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_TorqueAt);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Div);

INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Dot2Shift);
}
