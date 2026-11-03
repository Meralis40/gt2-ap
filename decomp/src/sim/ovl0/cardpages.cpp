// OVL0 8005019c-800505ac (cardpages): Memory card pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
// g_Page_SaveReplay: CardMenu_StartSaveReplay.
void SaveReplay_Enter(int back)
{
    CardMenu_StartSaveReplay();
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/cardpages", SaveReplay_Update);

// CardMenu_Draw.
void SaveReplay_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// g_Page_SaveGhost: CardMenu_StartSaveGhost.
void SaveGhost_Enter(int back)
{
    CardMenu_StartSaveGhost();
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/cardpages", SaveGhost_Update);

// CardMenu_Draw.
void SaveGhost_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// g_Page_LoadGhost: CardMenu_StartLoadGhost.
void LoadGhost_Enter(int back)
{
    CardMenu_StartLoadGhost();
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/cardpages", LoadGhost_Update);

// CardMenu_Draw.
void LoadGhost_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

// g_Page_SaveGame: CardMenu_StartSave (save the game after a prize or license).
void SaveGame_Enter(int back)
{
    CardMenu_StartSave((GameSaveFile *)0x0);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/cardpages", SaveGame_Update);

// CardMenu_Draw.
void SaveGame_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/cardpages", QuickMenu_SetupCardUi);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/cardpages", QuickMenu_SetupCardUi2);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/cardpages", QuickMenu_SetupCardUi3);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/cardpages", QuickMenu_SetupCardUiThen);
}
