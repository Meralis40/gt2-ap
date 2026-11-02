// OVL2 800136f0-80013844 (carlogoarc): Car logo archive.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


// state = 0.
void CarLogoArc_Init(short *state)
{
    *state = 0;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carlogoarc", CarLogoArc_Update);
}
