// OVL1 8001da08-8001e808 (datatransfer): Data transfer pages, GT1 record merging and
// conversion.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern BaseMenu *g_CurrentMenu;

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", MainMenu_LoadStringsB);

// Root page of Data Transfer: stores back, waits 20 frames, starts music track 7.
void DataTransferRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
    pMVar1[1].enter = (MenuPageEnterFn *)0x14;
    MainMenu_PlayMusic(7);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferRoot_Update);

// Empty.
void DataTransferRoot_Draw(DrawLayer *layer)
{
}

// Unreferenced page: 20-frame delay, MainMenu_StopMusic.
void DataTransferUnusedWait_Enter(int back)
{
    g_CurrentMenu->activePage[1].enter = (MenuPageEnterFn *)0x14;
    MainMenu_StopMusic();
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferUnusedWait_Update);

// Empty.
void DataTransferUnusedWait_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferMenu_ItemCallback);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferMenu_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferMenu_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferMenu_Draw);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferCarList_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferCarList_Update);

// DataXfer_Draw.
void DataTransferCarList_Draw(DrawLayer *layer)
{
    DataXfer_Draw((unsigned int *)((int)layer->ot + 0x10));
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", Mix_MergeCourseRecords);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", Mix_FindTimeRank);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", Mix_MergeLicenseTimes);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", Mix_MergeMachineTest0);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", Mix_MergeMachineTest1);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", Mix_MergeMachineTest2);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferMix_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferMix_Update);

// DataXfer_Draw.
void DataTransferMix_Draw(DrawLayer *layer)
{
    DataXfer_Draw((unsigned int *)((int)layer->ot + 0x10));
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", Convert_ApplyGt1Licenses);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferConvert_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/datatransfer", DataTransferConvert_Update);

// DataXfer_Draw.
void DataTransferConvert_Draw(DrawLayer *layer)
{
    DataXfer_Draw((unsigned int *)((int)layer->ot + 0x10));
}
}
