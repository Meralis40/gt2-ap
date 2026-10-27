// Resident code 80081084-800810f8 (vsynccb). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void VSyncList_Remove(VSyncCallback **list, VSyncCallback *node);
extern GfxIrqState g_GfxIrq;

INCLUDE_ASM("asm/sim/exe/nonmatchings/vsynccb", VSyncCallback_Install);

void VSyncCallback_Uninstall(VSyncCallback *cb)
{
    VSyncList_Remove(&g_GfxIrq.vsyncCallbacks, cb);
}
}
