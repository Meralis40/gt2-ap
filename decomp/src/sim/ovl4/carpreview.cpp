// OVL4 8001a454-8001b3ac (carpreview): Car preview: colours, model and spec sheet.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
unsigned int *GameData_FindEntryById(void *data, int table, unsigned int id);
void *GameData_GetEntry(int table, int index);
void *GameData_GetGtMode(void);
int Car_GetDefaultSetup(int carId, CarSetup *out);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_GetColorCode);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_GetColorNameW);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_GetColorCount);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_CycleColor);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_SetColor);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_ApplyColor);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_DrawLogo);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_DrawColorChips);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_DrawModel);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_Init);

void FUN_OVL4__8001ab64(int param_1)
{
    *(unsigned char *)(param_1 + 0x1c) = 0;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_ShowCurrentCar);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_Load);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_SetWheel);

// Fills the spec sheet from the car's default setup (GameData tables 3, 5, 6, 0xd and
// the type 0x1e car entry).
void CarPreview_SpecFromDefault(CarSpecSheet *spec, int carId)
{
    void *pvVar1;
    unsigned int *puVar2;
    void *pvVar3;
    void *pvVar4;
    void *pvVar5;
    CarSetup CStack_a0;

    pvVar1 = GameData_GetGtMode();
    puVar2 = GameData_FindEntryById(pvVar1, 0x1e, carId);
    Car_GetDefaultSetup(carId, &CStack_a0);
    pvVar1 = GameData_GetEntry(3, (unsigned int)(unsigned short)CStack_a0.idx_0a);
    pvVar3 = GameData_GetEntry(5, (unsigned int)(unsigned short)CStack_a0.idx_t05);
    pvVar4 = GameData_GetEntry(6, (unsigned int)(unsigned short)CStack_a0.idx_0c);
    pvVar5 = GameData_GetEntry(0xd, (unsigned int)(unsigned short)CStack_a0.idx_t0d);
    spec->modelYear = (unsigned short)*(unsigned char *)((int)puVar2 + 0x41);
    spec->length = (unsigned int)*(unsigned short *)((int)pvVar1 + 8);
    spec->width = (unsigned int)*(unsigned short *)((int)pvVar1 + 10);
    spec->height = (unsigned int)*(unsigned short *)((int)pvVar3 + 0x1a);
    spec->weight = (unsigned int)*(unsigned short *)((int)pvVar1 + 0xe);
    spec->displacement = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x2c);
    spec->drivetrain = (unsigned int)*(unsigned char *)((int)pvVar5 + 8);
    spec->engineStrA = (unsigned int)*(unsigned short *)((int)pvVar4 + 4);
    spec->engineStrB = (unsigned int)*(unsigned short *)((int)pvVar4 + 8);
    spec->engineStrC = (unsigned int)*(unsigned short *)((int)pvVar4 + 6);
    spec->power = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x2e);
    spec->powerRpm = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x30) * 10;
    spec->torque = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x32);
    spec->torqueRpmStr = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x34);
    spec->price = puVar2[0x11];
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_SpecFromCar);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_GetEngineTextA);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", FUN_OVL4__8001b2f8);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_GetEngineTextC);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carpreview", CarPreview_GetTorqueRpmText);
}
