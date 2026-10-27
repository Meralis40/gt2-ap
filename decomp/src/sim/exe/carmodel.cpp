// Resident code 80061010-800680a0 (carmodel). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CarModel_Init(CarModelHeader *model, unsigned int slot);
unsigned char FUN_800608e8(...);
void Vol_ReadFileAsync(int file, void *dst);
extern unsigned char D_801C946C;

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_InitStub);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", WheelMesh_Scale);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_BuildWheels);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarInst_InitWheels);

void CarModel_GetWheelDims(CarModelHeader *model, WheelDims *front, WheelDims *rear)
{
    short sVar1;

    front->radius = model->frontWheel[0];
    sVar1 = model->frontWheel[1];
    front->scale = 0xc00;
    front->offset = 0x7a;
    front->width = sVar1;
    rear->radius = model->rearWheel[0];
    sVar1 = model->rearWheel[1];
    rear->scale = 0xc00;
    rear->offset = 0x7a;
    rear->width = sVar1;
}

int CarModel_GetRideHeight(CarModelHeader *model)
{
    return ((int)model->frontWheel[0] - (int)model->wheelPos[0].y) * 0x10;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_SetColor);

void CarModel_Load(int fileId, CarModelHeader *dst, unsigned int slot)
{
    Vol_ReadFileAsync(fileId, dst);
    CarModel_Init(dst, slot);
}

void CarModel_ReadTim(int fileId, void *dst)
{
    Vol_ReadFileAsync(fileId + 1, dst);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Wheel_LoadTim);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarTex_FindColorIndex);

void FUN_800616a0(unsigned int param_1)
{
    FUN_800608e8(param_1, &D_801C946C);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Mesh_NormalsToEnvUV);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarBody_DrawUnclipped);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarBody_DrawClipped);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", WheelMesh_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_DrawWheel);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Clut_UploadToSlot);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Vram_UploadCarSlot);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Tim_LoadToSlot);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Clut_Blend);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", ShadowMesh_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_DrawShadow);
}
