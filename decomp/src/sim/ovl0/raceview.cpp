// OVL0 800292a0-800299d8 (raceview): Race view.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceview", RaceView_DrawFrame);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceview", RaceView_DrawHud);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceview", RaceView_RenderView);

// RaceView_DrawHud, then RaceView_RenderView(0).
void RaceView_DrawSingle(void *view)
{
    RaceView_DrawHud(view);
    RaceView_RenderView(view, 0);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceview", RaceView_SetSplitArea);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceview", RaceView_DrawSplit);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceview", RaceView_UpdateCamera);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceview", RaceView_DrawWorld);
}
