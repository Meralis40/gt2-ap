// OVL1 800169b0-8001721c (autoload): Kanji strings and the auto-load page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern char *g_AutoLoad_Line1;
extern char *g_AutoLoad_Line2;
extern int g_AutoLoad_State;
extern int g_AutoLoad_Port;
extern unsigned char g_AutoLoad_EmptyLine;
extern GameSaveFile g_SaveFileBuf;

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/autoload", Kanji_CacheSlotNames);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/autoload", Kanji_CacheString);

// Japanese only: resets the kanji glyph cache and caches the glyphs of the slot names
// and of the two current message lines.
void AutoLoad_CacheKanjiMessages(void)
{
    if (g_GameState.language == 0) {
        Kanji_CacheSlotNames();
        Kanji_CacheString(g_AutoLoad_Line1);
        Kanji_CacheString(g_AutoLoad_Line2);
    }
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/autoload", Kanji_DrawString);

// g_Page_AutoLoad enter: resets the state machine, shows the "Memory Card 1" header
// and builds the save-file header template in g_SaveFileBuf.
void AutoLoad_Enter(int back)
{
    g_AutoLoad_State = 0;
    g_AutoLoad_Port = 0;
    g_AutoLoad_Line1 = &g_AutoLoad_EmptyLine;
    g_AutoLoad_Line2 = &g_AutoLoad_EmptyLine;
    AutoLoad_CacheKanjiMessages();
    SaveFile_InitHeader(&g_SaveFileBuf);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/autoload", AutoLoad_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/autoload", AutoLoad_Draw);
}
