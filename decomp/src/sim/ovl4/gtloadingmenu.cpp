// OVL4 80014e60-8001508c (gtloadingmenu): GTLoadingMenu, and the two classes' __tf
// functions.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(psxViewLoopDoubleBuffer *this_, int in_chrg);
extern psxViewLoop_vtbl GTLoadingMenu__vtable;
int psxViewLoop__Update(psxViewLoop *this_);
void psxViewLoop__Draw(psxViewLoop *this_);
void *Prim_AddDrawMode(void *ot, unsigned int tpage);
int *Prim_AddTile(void *ot, unsigned int rgb);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtloadingmenu", GTLoadingMenu__GTLoadingMenu);

// Destructor (GCC 2): resets the vptr; deletes this_ if (in_chrg & 1).
void GTLoadingMenu__GTLoadingMenu_dtor(psxViewLoopDoubleBuffer *param_1, int param_2)
{
    (param_1->base).vptr = &GTLoadingMenu__vtable;
    psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(param_1, param_2);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtloadingmenu", GTLoadingMenu__Enter);

// Counts down; returns 0 (leave) when the counter goes below 0.
unsigned int GTLoadingMenu__Update(psxViewLoop *param_1)
{
    short sVar1;

    psxViewLoop__Update(param_1);
    sVar1 = (short)param_1[1].vsync.priority + -1;
    *(short *)&param_1[1].vsync.priority = sVar1;
    return (unsigned int)~(int)sVar1 >> 0x1f;
}

// Full-screen tile 0x202020.
void GTLoadingMenu__Draw(psxViewLoop *param_1)
{
    int *piVar1;

    psxViewLoop__Draw(param_1);
    piVar1 = Prim_AddTile(param_1[1].vsync.next, 0x2202020);
    *(unsigned short *)(piVar1 + 1) = 0x200;
    *(unsigned short *)piVar1 = 0;
    *(unsigned short *)((int)piVar1 + 2) = 0;
    *(unsigned short *)((int)piVar1 + 6) = 0x1e0;
    Prim_AddDrawMode(param_1[1].vsync.next, 0x40);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtloadingmenu", GTLoadingMenu____tf);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtloadingmenu", GTMenu____tf);
}
