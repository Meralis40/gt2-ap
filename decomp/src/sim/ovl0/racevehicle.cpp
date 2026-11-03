// OVL0 80012cd4-80014b54 (racevehicle): Race vehicles: setup, transforms, input and
// sounds.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_Shutdown);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_InitLapGhosts);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_SelectGhostLap);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", Matrix_FromCarPose);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_UpdateTransform);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_OnLap);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_OnLapDone);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_GetInput);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_GetPlayer1Input);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_GetPlayer2Input);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_GetLoggedInput);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_StartSound);

// CarSound_Stop on the car sound.
void RaceVehicle_StopSound(RaceVehicle *v)
{
    CarSound_Stop((CarSound *)&v->soundStart);
}

// soundMuted = 1 and stops the sound (pause).
void RaceVehicle_MuteSound(RaceVehicle *v)
{
    v->soundMuted = 1;
    RaceVehicle_StopSound(v);
}

// soundMuted = 0 and restarts the sound.
void RaceVehicle_UnmuteSound(RaceVehicle *v)
{
    v->soundMuted = 0;
    RaceVehicle_StartSound(v);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racevehicle", RaceVehicle_UpdateSound);
}
