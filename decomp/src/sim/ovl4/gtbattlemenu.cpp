// OVL4 8001508c-80015404 (gtbattlemenu): GTBattleMenu and the battle opponent pool.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
extern OpponentPool g_Battle_Pool;
void psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(psxViewLoopDoubleBuffer *this_, int in_chrg);
extern psxViewLoop_vtbl GTBattleMenu__vtable;

// &g_Battle_Pool.
OpponentPool *Battle_GetOpponentPool(void)
{
    return &g_Battle_Pool;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtbattlemenu", Battle_SetPlayer);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtbattlemenu", GTBattleMenu__GTBattleMenu);

// Destructor (GCC 2): resets the vptr; deletes this_ if (in_chrg & 1).
void GTBattleMenu__GTBattleMenu_dtor(psxViewLoopDoubleBuffer *param_1, int param_2)
{
    (param_1->base).vptr = &GTBattleMenu__vtable;
    psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(param_1, param_2);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtbattlemenu", GTBattleMenu__Enter);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtbattlemenu", GTBattleMenu__Update);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtbattlemenu", GTBattleMenu__Draw);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtbattlemenu", GTBattleMenu____tf);
}
