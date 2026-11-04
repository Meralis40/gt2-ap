// OVL2 80015ab4-80016ad4 (arccarview): Arcade 3D car preview.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern int g_ArcCarView_ModelStep;
extern int g_ArcCarView_TexStep;

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_DecodeHook);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_SetColor);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_ApplyColor);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_GetColorCount);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_CycleColor);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_Init);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_Begin);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_LoadGarageCar);

// Loads the car of car code code (CarCode_Encode) with colour color modulo its colour
// count, default setup and params (perf computed after the load).
void ArcCarView_LoadCarCode(ArcCarView *view, ArcadeMenu *menu, char *code, int color)
{
    unsigned int carId;
    int iVar1;
    unsigned short *puStack_18;
    char *pcStack_14;

    carId = CarCode_Encode(code);
    ArcCarView_Begin(view, menu, carId);
    iVar1 = CarInfoA_GetColors(carId, &puStack_18, &pcStack_14);
    view->colorIdx = (char)(color % iVar1);
    Car_GetDefaultSetup(carId, &view->setup);
    CarParams_BuildFromSetup(&view->setup, &view->params);
    view->wheelColor = 2;
    view->hasPerf = 1;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_Unload);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_Update);

// Inflate steps 0x17d0 (model) / 9000 (textures) bytes per frame.
void ArcCarView_SetFastDecode(ArcCarView *view)
{
    g_ArcCarView_ModelStep = 0x17d0;
    g_ArcCarView_TexStep = 9000;
}

// Inflate steps 3000 / 5000 bytes per frame.
void ArcCarView_SetSlowDecode(void)
{
    g_ArcCarView_ModelStep = 3000;
    g_ArcCarView_TexStep = 5000;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_GetDrivetrainIcon);

// out: perf words 0-3 (max power, its rpm, max torque, its rpm) and the weight (params
// +0x5a).
void ArcCarView_GetStats(ArcCarView *view, unsigned short *out)
{
    out[4] = (view->params).weight;
    out[1] = (view->perf).maxPowerRpm;
    *out = (view->perf).maxPower;
    out[3] = (view->perf).maxTorqueRpm;
    out[2] = (view->perf).maxTorque;
}
}
