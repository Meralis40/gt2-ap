// OVL0 800392d8-8003b69c (drivetrain): Engine, gearbox and tyre slip.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
// 1.0 + (|speed| - 30 km/h) x 30 (4.12, speed in m/s 4.12), at least 1.0, at most 1.125.
int Speed_FactorAbove30Kmh(int speed)
{
    int iVar1;
    longlong lVar2;

    if (speed < 0) {
        speed = -speed;
    }
    iVar1 = speed + -0x8555;
    if (iVar1 < 0) {
        iVar1 = 0;
    }
    lVar2 = Fixed_MulFloor(iVar1, 0x1e);
    iVar1 = (int)lVar2 + 0x1000;
    if (0x11ff < iVar1) {
        iVar1 = 0x1200;
    }
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Drivetrain_EngineSpeedFromWheels);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Engine_UpdateRpmDisplay);

// When +0x372 is 1: clutchIn +0x619 = clutch +0x60a != 0.
void Clutch_UpdateShiftState(CarPhys *car)
{
    if (car->clutch == 0) {
        car->clutchIn = 0;
        return;
    }
    car->clutchIn = 1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Tyre_ComputeSlipRatio);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Tyres_ComputeSlipRatios);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Tyres_LookupSlipCurve);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Gearbox_ApplyShiftRequest);

// Rpm display +0x6ac = engine speed +0x624 x 60.
void Engine_SetRpmDisplay(CarPhys *car)
{
    longlong lVar1;

    lVar1 = Fixed_MulFloor(car->engineSpeed, 0x3c);
    car->rpm = (short)lVar1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Gearbox_AutoSelectDirection);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Tyres_ComputeGripLimit);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Tyres_ComputeSlipAngle);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", TyreCurve_EvalAngle);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", CarPhys_UpdateDrivetrain);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", TyreCurve_FindPeakAngle);
}
