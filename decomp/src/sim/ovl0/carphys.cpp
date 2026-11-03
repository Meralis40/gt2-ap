// OVL0 8002f92c-80035714 (carphys): Car controls and physics setup.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char g_Damper_RateMin;
extern unsigned char g_Damper_RateMax;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Car_UpdateClutchManual);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Car_UpdateClutchAuto);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Car_LaunchAssist);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Car_ApplyControls);

// Sets aiMessage unless a held message is showing; a non-zero hold makes this_ one stick.
void CarAi_SetMessage(CarPhys *phys, unsigned char msg, int hold)
{
    if ((hold != 0) || (phys->aiMessageHold == 0)) {
        phys->aiMessage = msg;
        phys->aiMessageHold = (unsigned char)hold;
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Cars_ComputeFrameDelta);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_ScaleFrameDelta);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_UpdateLocalVelocity);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Cars_UpdateWheelRotation);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_ComputeTopSpeed);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_EffectiveMass);

// max(x, 12000). Used for CarParams.unk57[0] * 400 (identical to CarPhys_ClampMin12000B).
int CarPhys_ClampMin12000A(int x)
{
    if (x < 12000) {
        x = 12000;
    }
    return x;
}

// max(x, 12000). Used for CarParams.unk57[2] * 400.
int CarPhys_ClampMin12000B(int x)
{
    if (x < 12000) {
        x = 12000;
    }
    return x;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_InitSpeedLimits);

// phys->unk1c = v.
void CarPhys_SetUnk1c(CarPhys *phys, unsigned char v)
{
    phys->unk1c = v;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Drivetrain_GetClass);

// control = control (0 player, 2 CPU), aiMode = 0.
void CarPhys_SetControl(CarPhys *phys, unsigned char control)
{
    phys->control = control;
    phys->aiMode = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_SetUnk642);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Damper_SolveCurve);

// Linear map of level 10..255 between the two bytes at 0x80046dc8/9.
int Damper_LevelToRate(int level)
{
    return (unsigned int)g_Damper_RateMin +
           (int)((level + -10) * ((unsigned int)g_Damper_RateMax - (unsigned int)g_Damper_RateMin)) / 0xf5;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Damper_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Suspension_InitAxleTravel);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_LinkTables);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Tyre_BuildSlipCurves);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Tyre_BuildCurveB);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Tyre_BuildLoadCurves);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_ServiceTyres);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_ResetWheels);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_PlaceOnTrack);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_ResetOrientationHistory);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_ResetState);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_SetRollingStart);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_Setup);

// Clears proximity[8], followSlowdown and pushed.
void CarPhys_ClearProximity(CarPhys *phys)
{
    unsigned char *puVar1;
    int iVar2;

    iVar2 = 7;
    puVar1 = (unsigned char *)((int)&phys->lenRear + 3);
    phys->pushed = 0;
    phys->followSlowdown = 0;
    do {
        puVar1[0x76c] = 0;
        iVar2 = iVar2 + -1;
        puVar1 = puVar1 + -1;
    } while (-1 < iVar2);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_SetProximityFlags);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_CarPairInteraction);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_WallDeflect);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_MoveAndCollide);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_WallBounce);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Cars_MoveAll);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Cars_MoveAndCollide);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", FUN_OVL0__80034688);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", FUN_OVL0__800346ec);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Gearbox_ApplyTopSpeedSetting);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Drivetrain_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_SaveState);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", CarPhys_RestoreState);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Engine_ApplyRevLimiter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carphys", Engine_UpdateTurbo);
}
