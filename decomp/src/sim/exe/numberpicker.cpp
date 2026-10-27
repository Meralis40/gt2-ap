// Resident code 8006d9c8-8006dcb8 (numberpicker). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {

void NumberPicker_Init(NumberPicker *p)
{
    p->anim = -1;
    p->value = p->min;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/numberpicker", NumberPicker_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/numberpicker", NumberPicker_Draw);
}
