// OVL1 80013230-80016254 (replaycopy): Replay copy between memory cards.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern GameSaveFile g_SaveFileBuf;
extern void *g_ReplayCopy;

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_CacheKanjiCommon);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_CacheKanji);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_DrawKanji);

// Japanese only: caches the common strings and both message lines.
void ReplayCopy_CacheMessages(void)
{
    void *pvVar1;

    pvVar1 = g_ReplayCopy;
    if (g_GameState.language == '\0') {
        ReplayCopy_CacheKanjiCommon();
        ReplayCopy_CacheKanji(*(char **)((int)pvVar1 + 0x24));
        ReplayCopy_CacheKanji(*(char **)((int)pvVar1 + 0x28));
    }
}

// Japanese only: caches the common strings and the error text.
void ReplayCopy_CacheErrorText(void)
{
    void *pvVar1;

    pvVar1 = g_ReplayCopy;
    if (g_GameState.language == '\0') {
        ReplayCopy_CacheKanjiCommon();
        ReplayCopy_CacheKanji(*(char **)((int)pvVar1 + 0x20));
    }
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_GetSelectedSectors);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_CountSelected);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_GetCountAfterCopy);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_GetFreeAfterCopy);

// Capacity of the card 2 replay file.
int ReplayCopy_GetCapacity(void)
{
    return (int)*(short *)((int)g_ReplayCopy + 0x68e);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_CanToggle);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_ToggleSelected);

void ReplayCopy_ClearSelection(void)
{
    int iVar1;
    int iVar2;

    iVar2 = 0x20;
    iVar1 = (int)g_ReplayCopy + 0x20;
    do {
        *(unsigned char *)(iVar1 + 0x2fa8) = 0;
        iVar2 = iVar2 + -1;
        iVar1 = iVar1 + -1;
    } while (-1 < iVar2);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_InitSpaceGauge);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_DrawCard2Space);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_DrawSelectedCount);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_ListCallback);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_CardsReady);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_GetSlotMessages);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State0_Delay);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State1_Error);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State2_CheckSlots);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State3_AskFormat);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State4_Format);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State5_SelectBlocks);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State6_NoSpace);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State7_CreateFile);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State8_WriteEmptyDir);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State9_LoadCard2Dir);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State10_LoadCard1Dir);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State11_SelectReplays);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State12_PrepareCopy);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State13_ReadReplay);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State14_NextRead);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State15_StartWrite);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State16_WriteReplay);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State17_NextWrite);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State18_WriteDir);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State19_Complete);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_State20_NoReplays);

// Returns -1.
int ReplayCopy_StateIdle(int event, void *ot)
{
    return -1;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_SetState);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_InitWidgets);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_Setup);

// Builds the widgets and enters state 0.
void ReplayCopy_Start(void)
{
    ReplayCopy_InitWidgets();
    ReplayCopy_SetState(0);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaycopy", ReplayCopy_Draw);
}
