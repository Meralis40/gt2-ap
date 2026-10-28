// Resident code 80081c5c-8008264c (cdstream). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"
#include "gte.h"

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

// dst = m * v >> 12 + add, with v's components taken as 12-bit parts so that large
// vectors keep their precision (m's translation isn't used).
#ifdef NON_MATCHING
// The part of a vector's components selected by shift (0 and 12: 12 bits, 24: the rest,
// signed) in a GTE vector register pair.
#define VECTOR_PART_XY(v, shift, mask) \
    ((((v)->vx >> (shift)) & (mask)) | ((v)->vy >> (shift) & (mask)) << 16)

// Handwritten. The top part's product goes through IR1-IR3, which saturate at 16 bits:
// components past about 2^27 give wrong results.
static inline void Matrix_ApplyParts(MATRIX *m, VECTOR *v, int *out)
{
    int lo[3];
    int hi[3];

    gte_SetRotMatrixWords(m);
    gte_SetDataReg(GTE_VXY0, VECTOR_PART_XY(v, 0, 0xfff));
    gte_SetDataReg(GTE_VZ0, v->vz & 0xfff);
    gte_mvmva(1, GTE_MX_RT, GTE_V_V0, GTE_CV_NONE, 0);
    lo[0] = gte_GetDataReg(GTE_IR1);
    lo[1] = gte_GetDataReg(GTE_IR2);
    lo[2] = gte_GetDataReg(GTE_IR3);
    gte_SetDataReg(GTE_VXY2, (v->vx >> 24 & 0xffff) | v->vy >> 24 << 16);
    gte_SetDataReg(GTE_VZ2, v->vz >> 24);
    gte_mvmva(0, GTE_MX_RT, GTE_V_V2, GTE_CV_NONE, 0);
    hi[0] = gte_GetDataReg(GTE_IR1);
    hi[1] = gte_GetDataReg(GTE_IR2);
    hi[2] = gte_GetDataReg(GTE_IR3);
    gte_SetDataReg(GTE_VXY1, VECTOR_PART_XY(v, 12, 0xfff));
    gte_SetDataReg(GTE_VZ1, v->vz >> 12 & 0xfff);
    gte_mvmva(0, GTE_MX_RT, GTE_V_V1, GTE_CV_NONE, 0);
    out[0] = lo[0] + (hi[0] << 12) + gte_GetDataReg(GTE_MAC1);
    out[1] = lo[1] + (hi[1] << 12) + gte_GetDataReg(GTE_MAC2);
    out[2] = lo[2] + (hi[2] << 12) + gte_GetDataReg(GTE_MAC3);
}

VECTOR *Matrix_ApplyAdd(VECTOR *dst, MATRIX *m, VECTOR *v, VECTOR *add)
{
    int r[3];

    Matrix_ApplyParts(m, v, r);
    dst->vx = r[0] + add->vx;
    dst->vy = r[1] + add->vy;
    dst->vz = r[2] + add->vz;
    return dst;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ApplyAdd);
#endif

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_Transpose);

// dst = m * v >> 12, as Matrix_ApplyAdd.
#ifdef NON_MATCHING
VECTOR *Matrix_ApplyLV(VECTOR *dst, MATRIX *m, VECTOR *v)
{
    int r[3];

    Matrix_ApplyParts(m, v, r);
    dst->vx = r[0];
    dst->vy = r[1];
    dst->vz = r[2];
    return dst;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ApplyLV);
#endif

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", FUN_800824ac);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ShiftRot);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_OnReadSector);
}
