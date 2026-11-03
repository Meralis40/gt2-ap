// OVL0 80011ec8-800122c4 (racedev): RaceDevelopment and the race entries.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racedev", RaceDevelopment__RaceDevelopment);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racedev", RaceDevelopment__RaceDevelopment_dtor);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racedev", RaceDevelopment_Run);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racedev", Race_Entry);

// Default entry of the race overlay (Overlay_Start(0)): goes straight back to OVL1.
void Race_DefaultEntry(void)
{
    Overlay_Start(1);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racedev", RaceDevelopment____tf);
}
