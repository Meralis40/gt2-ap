// OVL4 800183ec-80018f6c (events): Event classification, event codes and prize info.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_IsProLeague);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Events_AllRegionalLeaguesWon);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Events_AllProLeaguesWon);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_IsWorldLeague);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_IsLicense);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_GetMachineTestType);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_IsFreeRace);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_GetFreeRaceIndex);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_BuildLetterIndex);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_IndexFromCode);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Char_DigitValue);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", EventCode_GetBase);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", EventCode_Format);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", EventCode_GetRaceCount);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", ChampRaceList_Build);

// Clears the 6 place prizes and the prize car count.
void EventPrize_Clear(EventPrizeInfo *info)
{
    int *piVar1;
    int iVar2;

    iVar2 = 5;
    piVar1 = info->prizes + 4;
    do {
        piVar1[1] = 0;
        iVar2 = iVar2 + -1;
        piVar1 = piVar1 + -1;
    } while (-1 < iVar2);
    info->prizeCarCount = '\0';
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", License_IndexFromCode);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/events", Event_LoadPrizeInfo);
}
