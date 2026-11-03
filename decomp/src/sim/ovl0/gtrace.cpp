// OVL0 80017098-80017e18 (gtrace): GT mode race loop and practice.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GTRace_BeginPractice);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GTRace_EndPractice);

// Runs the PreQuickMenuIO screen in mode 0 (from the race to the menus).
void RaceLoop_EnterMenus(RaceLoop *loop)
{
    int iVar1;

    (loop->menuIO).mode = '\0';
    do {
        do {
            iVar1 = ((ScreenViewLoop *)&loop->menuIO)->Execute();
        } while (1 < iVar1);
    } while (iVar1 < 0);
}

// Runs the PreQuickMenuIO screen in mode 1 (from the menus back to the race).
void RaceLoop_LeaveMenus(RaceLoop *loop)
{
    int iVar1;

    (loop->menuIO).mode = '\x01';
    do {
        do {
            iVar1 = ((ScreenViewLoop *)&loop->menuIO)->Execute();
        } while (1 < iVar1);
    } while (iVar1 < 0);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GTRace_RunMenus);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GranTurismoRaceLoop__GranTurismoRaceLoop);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GranTurismoRaceLoop__GranTurismoRaceLoop_dtor);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GranTurismoRaceLoop__State4);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GranTurismoRaceLoop__State8);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GranTurismoRaceLoop__State10);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GranTurismoRaceLoop__State6);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GranTurismoRaceLoop__State13);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GTRace_Run);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/gtrace", GranTurismoRaceLoop____tf);
}
