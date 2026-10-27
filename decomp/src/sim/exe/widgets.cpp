// Resident code 8006bcb8-8006c460 (widgets). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {

void GrowBar_Update(unsigned char *bar)
{
    short sVar1;

    if (*(short *)(bar + 0x12) < 0) {
        if (*(short *)(bar + 0x12) < -1) {
            *(short *)(bar + 0x12) = *(short *)(bar + 0x12) + 1;
            return;
        }
    }
    else {
        sVar1 = *(short *)(bar + 0x12) + 1;
        *(short *)(bar + 0x12) = sVar1;
        if (*(short *)(bar + 0x10) < sVar1) {
            *(unsigned short *)(bar + 0x12) = *(unsigned short *)(bar + 0x10);
        }
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", GrowBar_Draw);

void FadeCounter_Update(short *c)
{
    short sVar1;

    if (c[1] < 0) {
        if (c[1] < -1) {
            c[1] = c[1] + 1;
            return;
        }
    }
    else {
        sVar1 = c[1] + 1;
        c[1] = sVar1;
        if (*c < sVar1) {
            c[1] = *c;
        }
    }
}

int FadeCounter_GetLevel(short *c)
{
    int iVar1;
    unsigned int uVar2;

    uVar2 = (unsigned int)c[1];
    iVar1 = 0;
    if (-1 < (int)uVar2) {
        iVar1 = (int)(uVar2 << 7) / (int)*c;
    }
    if ((int)uVar2 < -1) {
        iVar1 = (int)(~uVar2 << 7) / (int)*c;
    }
    return iVar1;
}

void WipeBar_Update(WipeBar *bar)
{
    short sVar1;

    if (bar->t < 0) {
        if (bar->t < -1) {
            bar->t = bar->t + 1;
            return;
        }
    }
    else {
        sVar1 = bar->t + 1;
        bar->t = sVar1;
        if (bar->duration < sVar1) {
            bar->t = bar->duration;
        }
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", WipeBar_GetLevel);

INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", WipeBar_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", CardProgress_Reset);

INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", CardProgress_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", CardProgress_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", GroundDisc_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/widgets", GroundDisc_Draw);
}
