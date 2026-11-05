// OVL1 80016254-80016394 (mainstrings): Main menu strings.
#include "common.h"
#include "gamestate.h"

// One language's strings of the first table, as a block for the struct copy.
struct MainStringsBlock {
    char data[0xe47];
};

extern "C" {
extern unsigned char D_80021104[];
extern char s_CannotDetectMemoryCard1[];

// Inflates the first string table and copies the current language's strings to
// 0x801b9630.
unsigned char MainMenu_LoadStringsA(void)
{
    MainStringsBlock strings[8];

    Gzip_Inflate(D_80021104, strings);
    *(MainStringsBlock *)s_CannotDetectMemoryCard1 = strings[g_GameState.language];
}

// Inflates the menu string tables for the current language (g_GameState, first byte)
// to 0x801b9630 and 0x801ef6b0. MainMenu_Run calls it again after the save autoload,
// which can change the language.
void MainMenu_LoadStrings(void)
{
    MainMenu_LoadStringsA();
    MainMenu_LoadStringsB();
}
}
