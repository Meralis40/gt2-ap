// OVL0 8005019c-800505ac (cardpages): Memory card pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
void QuickMenu_PushPage(QuickMenu *menu, QuickMenuPage *page);
extern QuickMenu *g_QuickMenu;
extern CardMenuConfig g_QuickMenu_CardConfig;
extern short g_OVL0_CardSlotStatus[];
extern void *g_OVL0_KanjiFont;
extern QuickMenuPage *g_SaveGame_NextPage;
extern GameState g_GameState;
extern unsigned char g_TT_GhostReady;
// g_Page_SaveReplay: CardMenu_StartSaveReplay.
void SaveReplay_Enter(int back)
{
    CardMenu_StartSaveReplay();
}

// Goes back when the card menu is done.
int SaveReplay_Update(int active)
{
    int result = 0;
    PadState *pad = &g_QuickMenu->padState[0];

    if (active == 0) {
        pad = 0;
    }

    switch (CardMenu_Update(pad)) {
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

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

int SaveGhost_Update(int active)
{
    int result = 0;
    PadState *pad = &g_QuickMenu->padState[0];

    if (active == 0) {
        pad = 0;
    }

    switch (CardMenu_Update(pad)) {
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

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

// As SaveReplay_Update, noting a ghost that was loaded.
int LoadGhost_Update(int active)
{
    int result = 0;
    PadState *pad = &g_QuickMenu->padState[0];

    if (active == 0) {
        pad = 0;
    }
    int done;

    done = CardMenu_Update(pad);
    switch (done) {
    case 0:
        break;
    case 1:
        if (CardMenu_GhostLoaded() != 0) {
            char *base = (char *)&g_GameState;
            int offset = 0xbcc4;

            (base + offset)[6] = done;
            g_TT_GhostReady = done;
        }
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

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

// When done goes to g_SaveGame_NextPage, or back without one.
int SaveGame_Update(int active)
{
    QuickMenu &menu = *g_QuickMenu;
    int result = 0;
    PadState *pad = &menu.padState[0];

    if (active == 0) {
        pad = 0;
    }
    switch (CardMenu_Update(pad)) {
    case 0:
        break;
    case 1:
        if (g_SaveGame_NextPage == 0) {
            Sfx_PlayMenu(4);
            result = 2;
        } else {
            QuickMenu_PushPage(&menu, g_SaveGame_NextPage);
            Sfx_PlayMenu(3);
            result = 1;
        }
        break;
    }
    return result;
}

// CardMenu_Draw.
void SaveGame_Draw(DrawLayer *layer)
{
    CardMenu_Draw((unsigned int *)((int)layer->ot + 0x10));
}

void QuickMenu_SetupCardUi(void *work)
{
    g_QuickMenu_CardConfig.work = (CardMenu *)work;
    g_QuickMenu_CardConfig.slotStatus = g_OVL0_CardSlotStatus;
    g_QuickMenu_CardConfig.kanjiFont = g_OVL0_KanjiFont;
    CardMenu_Setup(&g_QuickMenu_CardConfig);
}

void QuickMenu_SetupCardUi2(void *work)
{
    g_QuickMenu_CardConfig.work = (CardMenu *)work;
    g_QuickMenu_CardConfig.slotStatus = g_OVL0_CardSlotStatus;
    g_QuickMenu_CardConfig.kanjiFont = g_OVL0_KanjiFont;
    CardMenu_Setup(&g_QuickMenu_CardConfig);
}

void QuickMenu_SetupCardUi3(void *work)
{
    g_QuickMenu_CardConfig.work = (CardMenu *)work;
    g_QuickMenu_CardConfig.slotStatus = g_OVL0_CardSlotStatus;
    g_QuickMenu_CardConfig.kanjiFont = g_OVL0_KanjiFont;
    CardMenu_Setup(&g_QuickMenu_CardConfig);
}

void QuickMenu_SetupCardUiThen(QuickMenuPage *next, void *work)
{
    g_QuickMenu_CardConfig.work = (CardMenu *)work;
    g_QuickMenu_CardConfig.slotStatus = g_OVL0_CardSlotStatus;
    g_SaveGame_NextPage = next;
    g_QuickMenu_CardConfig.kanjiFont = g_OVL0_KanjiFont;
    CardMenu_Setup(&g_QuickMenu_CardConfig);
}
}
