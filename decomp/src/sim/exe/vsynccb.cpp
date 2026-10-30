// Resident code 80081084-800810f8 (vsynccb): adding and removing a callback node on
// the vsync list.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void *memset(void *dst, int c, unsigned int n);
void func_8008107C(void);
void VSyncList_Add(VSyncCallback **list, VSyncCallback *node);
void VSyncList_Remove(VSyncCallback **list, VSyncCallback *node);
extern GfxIrqState g_GfxIrq;

// Clears cb and adds it to the vsync list with an empty function, first (priority 0).
void VSyncCallback_Install(VSyncCallback *cb)
{
    memset(cb, 0, sizeof(VSyncCallback));
    cb->func = func_8008107C;
    cb->priority = 0;
    VSyncList_Add(&g_GfxIrq.vsyncCallbacks, cb);
}

void VSyncCallback_Uninstall(VSyncCallback *cb)
{
    VSyncList_Remove(&g_GfxIrq.vsyncCallbacks, cb);
}
}
