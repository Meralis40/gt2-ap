// OVL4 8001907c-80019c60 (eventrules): Licenses and event entry rules.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
int License_IsEarned(LicenseId license);
LicenseId License_IdFromRaceCode2(char *raceCode);
extern unsigned int g_License_EntryMsgPages[6];

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", Course_PickRandom);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", License_IdFromRaceCode2);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", License_IsEarned);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", License_GetHighest);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", GtRaceData_GetAllowedCars);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", EventRules_Load);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", EventRules_BuildAll);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", Event_GetMaxPower);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", Event_IsDirt);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", Event_IsTarmac);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", Event_GetTurboRule);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", EventRules_GetRequiredLicense);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", EventRules_GetPowerLimit);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", EventRules_GetPrize);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", EventRules_GetChampionshipPrize);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", Race_CheckEntry);

// Gate for entering a license: test L* needs License_IsEarned(id+1) (B needs nothing,
// A needs B, IC needs A ... S needs IA). Returns 1 or -3.
int License_CheckEntry(char *raceCode, int *msgPage)
{
    LicenseId LVar1;
    int iVar2;

    LVar1 = License_IdFromRaceCode2(raceCode);
    *msgPage = g_License_EntryMsgPages[LVar1];
    if (((int)LVar1 < 5) && (iVar2 = License_IsEarned(LVar1 + LICENSE_IA), iVar2 != 1)) {
        return -3;
    }
    return 1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/eventrules", Event_GetGhostFileId);
}
