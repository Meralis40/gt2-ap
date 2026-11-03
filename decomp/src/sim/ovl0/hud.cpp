// OVL0 8002bcf0-8002ee98 (hud): HUD.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern RECT g_Hud_TimClutRect0;
extern RECT g_Hud_TimImageRect0;
extern RECT g_Hud_TimClutRect1;
extern RECT g_Hud_TimImageRect1;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_RotatePoint);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Tacho_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawNeedle);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Tacho_DrawDial);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Tacho_DrawNeedle);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawBoostGauge);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawLapPanel);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawTimeSpeed);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawTargetTime);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawMachineTestRecord);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawRecordPanel);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawLapPopup);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawPedals);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawSteering);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawTyres);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawMessage);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawFull);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawReplayLabel);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Hud_DrawSplit);

// Loads two TIMs of pack (offsets +0x1c, +0x28) to fixed CLUT and image rects
// (Tim_LoadClutAndImage).
void Hud_LoadCluts(void *view, int pack)
{
    Tim_LoadClutAndImage((void *)(pack + *(int *)(pack + 0x1c)), &g_Hud_TimClutRect0, &g_Hud_TimImageRect0);
    Tim_LoadClutAndImage((void *)(pack + *(int *)(pack + 0x28)), &g_Hud_TimClutRect1, &g_Hud_TimImageRect1);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/hud", Particles_Draw);
}
