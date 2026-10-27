// Resident code 8007adc8-8007c268 (gte). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Dma_WaitGpu(...);
void Gpu_DrawSync(void);
void *Gpu_LoadImageBlock(int *block, RECT *rect);
int Gpu_TransferImage(RECT *rect, unsigned int *data, int store);
void Gpu_WaitReady(...);
void Matrix_Mul(MATRIX *dst, MATRIX *a, MATRIX *b);
MATRIX *Matrix_Translate(MATRIX *m, VECTOR *v);
void *Tim_LoadClut(void *tim, RECT *rect);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", LodTable_SquareDistances);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", FUN_8007ae08);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Lod_DistanceSq);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Lod_Select);

void Gpu_DrawSync(void)
{
    Gpu_WaitReady();
    Dma_WaitGpu();
    Gpu_WaitReady();
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Identity);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_SetEuler);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Copy);

MATRIX *Matrix_Comp(MATRIX *m, MATRIX *n)
{
    Matrix_Translate(m, (VECTOR *)n->t);
    Matrix_Mul(m, m, n);
    return m;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Translate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_RotEuler);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_RotX);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_RotY);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_RotZ);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Scale);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gte_TransMsb);

void Camera_SetFrustum(Camera *cam, int left, int right, int top, int bottom, int h)
{
    cam->extentX = (short)right - (short)left;
    cam->extentY = (short)top - (short)bottom;
    cam->frustumH = (short)h;
    cam->centerX = (short)(((left + right >> 1) << 0xc) / h);
    cam->centerY = (short)(((bottom + top >> 1) << 0xc) / h);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Camera_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", FUN_8007b5ac);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", FUN_8007b5fc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Clip_TestPoints);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gte_LoadScaled);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gte_LoadTransform);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Clip_TestBox);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Mul);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gpu_TransferImage);

void *Gpu_LoadImageBlock(int *block, RECT *rect)
{
    if (rect == 0) {
        rect = (RECT *)(block + 1);
    }
    Gpu_TransferImage(rect, (unsigned int *)(block + 3), 0);
    Gpu_DrawSync();
    return (void *)((int)block + *block);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Tim_LoadClut);

void Tim_LoadImage(void *tim, RECT *rect)
{
    int *block;

    block = (int *)((int)tim + 8);
    if ((*(unsigned int *)((int)tim + 4) >> 3 & 1) != 0) {
        block = (int *)((int)block + *(int *)((int)tim + 8));
    }
    Gpu_LoadImageBlock(block, rect);
}

void Tim_LoadClutAndImage(void *tim, RECT *clutRect, RECT *imageRect)
{
    int *block;

    block = Tim_LoadClut(tim, clutRect);
    Gpu_LoadImageBlock(block, imageRect);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Poly_Clip);
}
