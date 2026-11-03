// OVL0 80043388-80046bac (suspension): Suspension and drivetrain.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", RaceCars_PrepareWheelProbes);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", RaceCars_ProbeGround);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", RaceCars_ApplyGroundProbes);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Wheel_ComputeSpringForce);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Body_LiftToHeight);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", RaceCars_UpdateSuspension);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Gearbox_GearForSpeed);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Gearbox_SelectGear);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Matrix_FromAngles);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Angle_Wrap);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Angle_Diff);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Drivetrain_ClutchTorque);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Drivetrain_IdleTorque);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", RaceCars_ComputeBrakeTorque);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", RaceCars_ApplyLsdBias);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", Diff_GetCouplingTorque);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", RaceCars_UpdateDrivetrain);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/suspension", RaceCars_UpdateDifferentials);

// RaceCars_ComputeBrakeTorque, RaceCars_ApplyLsdBias, RaceCars_UpdateDrivetrain,
// RaceCars_UpdateDifferentials.
void RaceCars_StepDrivetrain(RaceVehicle *cars, int count)
{
    RaceCars_ComputeBrakeTorque(cars, count);
    RaceCars_ApplyLsdBias(cars, count);
    RaceCars_UpdateDrivetrain(cars, count);
    RaceCars_UpdateDifferentials(cars, count);
}
}
