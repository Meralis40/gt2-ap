// OVL0 800505ac-80052838 (raceresults): Race and machine test result pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern void *g_OVL0_EndFontInfo;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", LapList_ItemCallback);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", RaceResults_SetupCar);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", RaceResults_InitView);

// Turns the car 0xc per frame; after frame 0xfb90 also pulls the camera back.
void RaceResults_SpinView(ResultCarView *view, int frame)
{
    view->rotY = view->rotY + 0xcU & 0x3fff;
    if (0xfb90 < frame) {
        view->pos[2] = view->pos[2] + 0x8000;
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", RaceResults_CanSave);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", MachineTest_StoreResult);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", RaceResultsRoot_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", RaceResultsRoot_Update);

// Empty.
void RaceResultsRoot_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", RaceResults_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", RaceResults_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", RaceResults_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", MachineTestName_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", MachineTestName_Update);

// TextEntry_Draw.
void MachineTestName_Draw(DrawLayer *layer)
{
    TextEntry_Draw((TextEntry *)((int)g_OVL0_EndFontInfo + 0x5c8), layer->ot);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", MachineTestResult_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", MachineTestResult_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceresults", MachineTestResult_Draw);
}
