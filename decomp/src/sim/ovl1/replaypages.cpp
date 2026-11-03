// OVL1 800124a4-80013230 (replaypages): Replay theater, demo, save and load pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern BaseMenu *g_CurrentMenu;
extern pointer g_OVL1_EndFontInfo;

// Root page of the Replay Theater: stores back, waits 20 frames, starts music track 0.
// On first entry also calls DemoReplay_Count on the replay buffer.
void ReplayTheaterRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].enter = (MenuPageEnterFn *)0x14;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
    if (back == 0) {
        DemoReplay_Count(g_OVL1_EndFontInfo);
    }
    MainMenu_PlayMusic(0);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", ReplayTheaterRoot_Update);

// Empty.
void ReplayTheaterRoot_Draw(DrawLayer *layer)
{
}

// Page pushed after a replay was chosen (BaseMenu.result already set): sets a 20-frame
// delay and stops the music (MainMenu_StopMusic).
void StartReplayWait_Enter(int back)
{
    g_CurrentMenu->activePage[1].enter = (MenuPageEnterFn *)0x14;
    MainMenu_StopMusic();
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", StartReplayWait_Update);

// Empty.
void StartReplayWait_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", ReplayTheaterMenu_ItemCallback);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", ReplayTheaterMenu_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", ReplayTheaterMenu_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", ReplayTheaterMenu_Draw);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", LoadReplay_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", LoadReplay_Update);

// CardMenu_Draw.
void LoadReplay_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", RenameDeleteReplay_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", RenameDeleteReplay_Update);

// CardMenu_Draw.
void RenameDeleteReplay_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", CopyReplay_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", CopyReplay_Update);

// ReplayCopy_Draw.
void CopyReplay_Draw(DrawLayer *layer)
{
    ReplayCopy_Draw((unsigned int *)((int)layer->ot + 0x10));
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", DemoSelect_ItemCallback);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", DemoSelect_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", DemoSelect_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", DemoSelect_Draw);

// Sets BaseMenu.result (start a replay) and a 20-frame delay.
void DemoStart_Enter(int back)
{
    BaseMenu *pBVar1;

    pBVar1 = g_CurrentMenu;
    g_CurrentMenu->activePage[1].enter = (MenuPageEnterFn *)0x14;
    pBVar1->result = '\x01';
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", DemoStart_Update);

// Empty.
void DemoStart_Draw(DrawLayer *layer)
{
}

// Root page of the Save Game menu: stores back, waits 20 frames, starts music track 7.
void SaveGameRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
    pMVar1[1].enter = (MenuPageEnterFn *)0x14;
    MainMenu_PlayMusic(7);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", SaveGameRoot_Update);

// Empty.
void SaveGameRoot_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", SaveGameCard_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", SaveGameCard_Update);

// CardMenu_Draw.
void SaveGameCard_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// Root page of the Load Game menu: stores back, waits 20 frames, starts music track 7.
void LoadGameRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    pMVar1[1].update = (MenuPageUpdateFn *)back;
    pMVar1[1].enter = (MenuPageEnterFn *)0x14;
    MainMenu_PlayMusic(7);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", LoadGameRoot_Update);

// Empty.
void LoadGameRoot_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", LoadGameCard_Enter);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/replaypages", LoadGameCard_Update);

// CardMenu_Draw.
void LoadGameCard_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}
}
