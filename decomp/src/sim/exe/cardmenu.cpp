// Resident code 8006ebf0-8007306c (cardmenu). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CardMenu_CacheKanji(char *sjis);
void CardMenu_CacheKanjiSlotNames(...);
void CardMenu_InitWidgets(...);
void CardMenu_PrepareGhostEntry(...);
void CardMenu_PrepareReplayEntry(...);
void CardMenu_SetState(int state);
void KanjiFont_UploadClut(void *font);
void KanjiFont_UploadTexture(void *font);
void SaveFile_InitHeader(GameSaveFile *file);
extern CardMenu *g_CardMenu;
extern unsigned char g_CardMenu_GhostLoaded;
extern GameState g_GameState;

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", KanjiFont_Init);

void KanjiFont_Upload(void *font)
{
    KanjiFont_UploadClut(font);
    KanjiFont_UploadTexture(font);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", KanjiFont_CacheString);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_CacheKanjiSlotNames);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_CacheKanji);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_DrawKanji);

void CardMenu_CacheKanjiLines(void)
{
    CardMenu *pCVar1;

    pCVar1 = g_CardMenu;
    if (g_GameState.language == '\0') {
        CardMenu_CacheKanjiSlotNames();
        CardMenu_CacheKanji(pCVar1->line1);
        CardMenu_CacheKanji(pCVar1->line2);
    }
}

void CardMenu_CacheKanjiError(void)
{
    CardMenu *pCVar1;

    pCVar1 = g_CardMenu;
    if (g_GameState.language == '\0') {
        CardMenu_CacheKanjiSlotNames();
        CardMenu_CacheKanji(pCVar1->errorMsg);
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_CacheKanjiListHints);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_EntryListCallback);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_PickReplayFileSize);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_ReadReplayDir);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_DiscardChanges);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_ReplayEntryList);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_EditTitle);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_CreateReplayFile);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_WriteReplayData);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_WriteReplayDir);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_SaveFailed);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_SaveComplete);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_PlayList);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_LoadReplay);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_GhostList);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_LoadGhost);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_ConfirmSave);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_CreateGameFile);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_ConfirmOverwrite);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_WriteGameSave);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_SaveError);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_ConfirmLoad);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_ReadGameSave);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_LoadComplete);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_Start);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_Error);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_SelectSlot);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_CheckSlot);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_NoCard);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_NotFormatted);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_Format);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_NoSpace);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_NoReplayData);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardState_NoGameFile);

int CardState_Idle(int event, void *arg)
{
    return -1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_SetState);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_PrepareReplayEntry);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_PrepareGhostEntry);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_InitWidgets);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_Setup);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_Draw);

void CardMenu_StartSaveReplay(void)
{
    g_CardMenu->mode = CARDMODE_SAVE_REPLAY;
    CardMenu_PrepareReplayEntry();
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartSaveGhost(void)
{
    g_CardMenu->mode = CARDMODE_SAVE_REPLAY;
    CardMenu_PrepareGhostEntry();
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartPlayReplay(void)
{
    g_CardMenu->mode = CARDMODE_PLAY_REPLAY;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartEditReplay(void)
{
    g_CardMenu->mode = CARDMODE_EDIT_REPLAY;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}

void CardMenu_StartLoadGhost(void)
{
    g_CardMenu->mode = CARDMODE_LOAD_GHOST;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
    g_CardMenu_GhostLoaded = '\0';
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_GhostLoaded);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cardmenu", CardMenu_StartSave);

void CardMenu_StartLoad(GameSaveFile *file)
{
    CardMenu *pCVar1;

    pCVar1 = g_CardMenu;
    g_CardMenu->saveFile = (GameSaveFile *)g_CardMenu->replayData;
    if (file != 0) {
        pCVar1->saveFile = file;
    }
    SaveFile_InitHeader(pCVar1->saveFile);
    pCVar1->mode = CARDMODE_LOAD_GAME;
    CardMenu_InitWidgets();
    CardMenu_SetState(0);
}
}
