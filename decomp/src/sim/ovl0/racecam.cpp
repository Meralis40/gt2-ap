// OVL0 80010000-80011ec8 (racecam): Race cameras.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern short g_RaceCam_FovTable[4];

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_SetProjection);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_CopyTargetCar);

// Projection distance from the field-of-view option (g_RaceCam_FovTable[options2[4]]).
void RaceCam_SetDefaultProjection(RaceCam *cam)
{
    RaceCam_SetProjection(cam, (unsigned int)(unsigned short)g_RaceCam_FovTable[g_GameState.options2[4]], 0, 0);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_UpdateLive);

// RaceCam_SetProjection(projH) then RaceCam_SetChase(chaseView).
void RaceCam_SetChaseProjection(RaceCam *cam, int projH)
{
    RaceCam_SetProjection(cam, projH, 0, 0);
    RaceCam_SetChase(cam, (unsigned int)cam->chaseView);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_SetChase);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", Math_LerpDiv);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", Math_Lerp12);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_UpdateIntro);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", Race_FindCarByPlace);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_UpdateReplay);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", Matrix_LookAlong);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", Vector_Lerp);

// Length of v (components >> 10 before squaring).
int Vector_Length(VECTOR *v)
{
    int iVar1;
    int iVar2;
    int iVar3;

    iVar1 = v->vx >> 10;
    iVar2 = v->vy >> 10;
    iVar3 = v->vz >> 10;
    iVar1 = Math_ISqrt(iVar1 *iVar1 + iVar2 *iVar2 + iVar3 *iVar3, 0);
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_TvPath);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_TvFixed);

// Track-side camera type 2: car-mounted view entry +0x10 (RaceCam_SetCarView) with
// projection +0x12.
void RaceCam_TvCarView(RaceCam *cam, void *entry)
{
    RaceCam_SetProjection(cam, (unsigned int)*(unsigned short *)((int)entry + 0x12), 0, 0);
    RaceCam_SetCarView(cam, (unsigned int)*(unsigned short *)((int)entry + 0x10));
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_SetCarView);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_TvZoom);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", RaceCam_SetOnboard);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", TrackCam_Find);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", TrackCam_EvalPath);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", TrackCam_EvalLinear);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racecam", TrackCam_EvalSpline);
}
