// OVL1 80016254-80016394 (mainstrings): Main menu strings.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/mainstrings", MainMenu_LoadStringsA);

// Inflates the menu string tables for the current language (g_GameState, first byte)
// to 0x801b9630 and 0x801ef6b0. MainMenu_Run calls it again after the save autoload,
// which can change the language.
void MainMenu_LoadStrings(void)
{
    MainMenu_LoadStringsA();
    MainMenu_LoadStringsB();
}
}
