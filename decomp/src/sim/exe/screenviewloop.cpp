// Resident code 800833a0-8008371c (screenviewloop). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
extern unsigned char D_800A6F28;

INCLUDE_ASM("asm/sim/exe/nonmatchings/screenviewloop", ScreenViewLoop__ScreenViewLoop);

INCLUDE_ASM("asm/sim/exe/nonmatchings/screenviewloop", ScreenViewLoop__ScreenViewLoop_dtor);

int ScreenViewLoop__Execute(ScreenViewLoop *this_)
{
    (*this_->vptr->Run)(this_);
    return 0;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/screenviewloop", ScreenViewLoop__Run);

void ScreenViewLoop__Enter(ScreenViewLoop *this_)
{
}

int ScreenViewLoop__Step(ScreenViewLoop *this_)
{
    return 1;
}

void ScreenViewLoop__Flip(ScreenViewLoop *this_)
{
}

void ScreenViewLoop__Leave(ScreenViewLoop *this_)
{
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/screenviewloop", ScreenViewLoop____tf);

INCLUDE_ASM("asm/sim/exe/nonmatchings/screenviewloop", FUN_800834fc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/screenviewloop", FUN_800835dc);

void FUN_80083708(int param_1, unsigned char param_2)
{
    (&D_800A6F28)[param_1] = param_2;
}
}
