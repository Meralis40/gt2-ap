// OVL2 8002445c-80025614 (guestload): Guest garage loading.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern void *g_GuestLoad;

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_CacheKanjiCommon);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_CacheKanji);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_DrawKanji);

// Copy of DataXfer_CacheMessages.
void GuestLoad_CacheMessages(void)
{
    void *pvVar1;

    pvVar1 = g_GuestLoad;
    if (g_GameState.language == '\0') {
        GuestLoad_CacheKanjiCommon();
        GuestLoad_CacheKanji(*(char **)((int)pvVar1 + 0x28));
        GuestLoad_CacheKanji(*(char **)((int)pvVar1 + 0x2c));
    }
}

// Copy of DataXfer_CacheErrorText.
void GuestLoad_CacheErrorText(void)
{
    void *pvVar1;

    pvVar1 = g_GuestLoad;
    if (g_GameState.language == '\0') {
        GuestLoad_CacheKanjiCommon();
        GuestLoad_CacheKanji(*(char **)((int)pvVar1 + 0x24));
    }
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State6_Confirm);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State7_Read);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State8_Complete);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State0_Delay);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State1_Error);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State2_SelectSlot);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State3_CheckSlot);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State4_NoCard);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_State5_NoFile);

// Returns -1.
int GuestLoad_StateIdle(int event, void *ot)
{
    return -1;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_SetState);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_InitWidgets);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_Setup);

// Prepares the save file buffer (work +0x384), the widgets, then state 0.
void GuestLoad_Start(void)
{
    GameSaveFile *file;

    file = (GameSaveFile *)((int)g_GuestLoad + 900);
    *(GameSaveFile **)((int)g_GuestLoad + 0x380) = file;
    SaveFile_InitHeader(file);
    GuestLoad_InitWidgets();
    GuestLoad_SetState(0);
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/guestload", GuestLoad_Draw);
}
