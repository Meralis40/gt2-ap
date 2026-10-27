// Resident code 80081c5c-8008264c (cdstream). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int CdAudio_RunScript(int script);
int Sys_CriticalSection(int leave);
int Timer_Sample(...);
extern unsigned short D_801C9520;
extern int D_801C9524;
extern int D_801C9528;
extern int D_801C952C;

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_ResetRead);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_GetReadStatus);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", FUN_80081cc8);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Sys_CriticalSection);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Timer_Sample);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Timer_StartFrame);

int Timer_SinceFrameStart(void)
{
    int iVar1;

    iVar1 = Timer_Sample();
    return iVar1 - D_801C9528;
}

int Timer_HblanksSinceFrameStart(void)
{
    unsigned int uVar1;

    uVar1 = (unsigned int)D_801C9520;
    if ((int)uVar1 < D_801C952C) {
        uVar1 = uVar1 + 0xffff;
    }
    return uVar1 - D_801C952C;
}

int FUN_80081efc(void)
{
    int iVar1;
    int iVar2;

    iVar1 = D_801C9524;
    iVar2 = Timer_Sample();
    return iVar2 - iVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Gpu_SetDisplayStart);

void CdAudio_StartRead(void)
{
    int iVar1;

    do {
        iVar1 = CdAudio_RunScript(7);
    } while (iVar1 == 0);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_ResetStream);

void CdStream_AddFillCount(CdAudio *cd, short delta)
{
    int leave;

    leave = Sys_CriticalSection(0);
    cd->ringFill = cd->ringFill + delta;
    Sys_CriticalSection(leave);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", FUN_80081ff8);

unsigned char *CdStream_GetReadPtr(CdAudio *cd)
{
    return (unsigned char *)((int)cd->dst + (unsigned int)cd->ringRead * 0x800);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdStream_ReleaseSector);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdStream_WaitSector);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Stack_Check);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_SetupStream);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Gpu_WaitReady);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ApplyAdd);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_Transpose);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ApplyLV);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", FUN_800824ac);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ShiftRot);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_OnReadSector);
}
