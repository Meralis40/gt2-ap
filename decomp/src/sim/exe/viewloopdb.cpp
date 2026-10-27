// Resident code 8007fe34-80080088 (viewloopdb). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Gpu_SetDisplayStart(unsigned int x, int y);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloopdb", Gpu_Reset);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloopdb", psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloopdb", psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloopdb", psxViewLoopDoubleBuffer__Leave);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloopdb", psxViewLoopDoubleBuffer__SetVideoMode);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloopdb", psxViewLoopDoubleBuffer__BeginDraw);

void psxViewLoopDoubleBuffer__SwapDisplay(psxViewLoopDoubleBuffer *this_)
{
    Gpu_SetDisplayStart(0, (int)this_->bufferY[(this_->base).bufferIndex]);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloopdb", psxViewLoopDoubleBuffer____tf);
}
