// OVL1 8001f508-80020dcc (dataxfer): GT1 and GT2 save data transfer.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern GameSaveFile g_SaveFileBuf;
extern void *g_DataXfer;

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_CacheKanjiCommon);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_CacheKanji);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_DrawKanji);

// Japanese only: common strings and both message lines.
void DataXfer_CacheMessages(void)
{
    void *pvVar1;

    pvVar1 = g_DataXfer;
    if (g_GameState.language == '\0') {
        DataXfer_CacheKanjiCommon();
        DataXfer_CacheKanji(*(char **)((int)pvVar1 + 0x28));
        DataXfer_CacheKanji(*(char **)((int)pvVar1 + 0x2c));
    }
}

// Japanese only: common strings and the error text.
void DataXfer_CacheErrorText(void)
{
    void *pvVar1;

    pvVar1 = g_DataXfer;
    if (g_GameState.language == '\0') {
        DataXfer_CacheKanjiCommon();
        DataXfer_CacheKanji(*(char **)((int)pvVar1 + 0x24));
    }
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State6_ConfirmGt2);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State7_LoadGt2);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State8_Complete);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State9_NoCars);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", Gt1Save_Checksum);

// Checksum of the first 0x6ba4 bytes matches the stored one.
unsigned char Gt1Save_Check(unsigned char *data)
{
    unsigned int uVar1;

    uVar1 = Gt1Save_Checksum(data, 0x6ba4);
    return *(unsigned int *)(data + 0x6ba4) == uVar1;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State10_ConfirmGt1);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State11_LoadGt1);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State0_Delay);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State1_Error);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State2_SelectSlot);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_FindGt1File);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State3_CheckSlot);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State4_NoCard);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_State5_NoFile);

// Returns -1.
int DataXfer_StateIdle(int event, void *ot)
{
    return -1;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_SetState);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_InitWidgets);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_Setup);

// mode 0 trade, 1 mix records, 2 convert GT1. Prepares the save header template, the
// widgets, then state 0.
void DataXfer_Start(short mode)
{
    void *pvVar1;
    GameSaveFile *file;

    pvVar1 = g_DataXfer;
    *(short *)g_DataXfer = mode;
    file = (GameSaveFile *)((int)pvVar1 + 0x41c);
    *(GameSaveFile **)((int)pvVar1 + 0x418) = file;
    SaveFile_InitHeader(file);
    DataXfer_InitWidgets();
    DataXfer_SetState(0);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/dataxfer", DataXfer_Draw);

// Cars of the loaded GT2 file (count, then Car records of 0xa4 from +4), via
// SaveFile_GetGarages.
void *DataXfer_GetCarList(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetGarages(*(GameSaveFile **)((int)g_DataXfer + 0x418),
                                                              *(GameSaveFile **)((int)g_DataXfer + 0x418));
    return pvVar1;
}

// Course records of the loaded file (0x80 x 0x24).
void *DataXfer_GetCourseRecords(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetCourseRecords
                                          (*(GameSaveFile **)((int)g_DataXfer + 0x418),
                                            *(GameSaveFile **)((int)g_DataXfer + 0x418));
    return pvVar1;
}

// License tests of the loaded file (6 x 10 x 0xa4).
void *DataXfer_GetLicenseTests(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetLicenses
                                          (*(GameSaveFile **)((int)g_DataXfer + 0x418),
                                            *(GameSaveFile **)((int)g_DataXfer + 0x418));
    return pvVar1;
}

void *DataXfer_GetMachineTest0(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetMachineTest0
                                          (*(GameSaveFile **)((int)g_DataXfer + 0x418),
                                            *(GameSaveFile **)((int)g_DataXfer + 0x418));
    return pvVar1;
}

void *DataXfer_GetMachineTest1(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetMachineTest1
                                          (*(GameSaveFile **)((int)g_DataXfer + 0x418),
                                            *(GameSaveFile **)((int)g_DataXfer + 0x418));
    return pvVar1;
}

void *DataXfer_GetMachineTest2(void)
{
    void *pvVar1;

    pvVar1 = SaveFile_GetMachineTest2
                                          (*(GameSaveFile **)((int)g_DataXfer + 0x418),
                                            *(GameSaveFile **)((int)g_DataXfer + 0x418));
    return pvVar1;
}

// GT1 save data (+0x61c of the work area, after the card header).
unsigned char *DataXfer_GetGt1Data(void)
{
    return (unsigned char *)((int)g_DataXfer + 0x61c);
}
}
