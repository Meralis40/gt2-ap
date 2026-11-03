// OVL1 8001779c-80017d74 (mainmenupage): Main menu page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern short g_MainMenu_Fade;
extern short g_MainMenu_FadeOut;
extern short g_MainMenu_State;
extern MenuList g_MainMenu_List;
extern short g_MainMenu_OpenDelay;
extern GameSaveFile g_SaveFileBuf;
extern int g_MainMenu_IdleFrames;

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/mainmenupage", MainMenu_ItemCallback);

// g_Page_MainMenu enter: resets the fades and the idle counter and builds
// g_MainMenu_List (rows are sprites, see MainMenu_ItemCallback).
void MainMenu_Enter(int back)
{
    g_MainMenu_Fade = 0;
    g_MainMenu_FadeOut = 0x10;
    g_MainMenu_State = 0;
    g_MainMenu_OpenDelay = 0x10;
    MenuList_Init(&g_MainMenu_List, MainMenu_ItemCallback, (void *)0x0);
    g_MainMenu_IdleFrames = 0;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/mainmenupage", MainMenu_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/mainmenupage", MainMenu_Draw);
}
