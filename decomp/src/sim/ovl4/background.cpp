// OVL4 80021c38-800222e4 (background): Display lists and the page background.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
extern unsigned int *g_DisplayList_Chunk;
extern unsigned char g_DisplayList_ChunkLen;
extern unsigned int g_DisplayList_DrawMode;
unsigned int *Background_AddOverlayTiles(unsigned int *tiles, unsigned int *p);
unsigned int *Background_AddSpriteGroups(int *groups, unsigned int *p);
void *DisplayList_End(void);
unsigned int *Background_AddTileGrid(unsigned int *tiles, unsigned int *p);
extern unsigned int *g_MenuTileMap;
extern unsigned int *g_MenuOverlayTiles;
extern int *g_MenuSpriteGroups;

// Starts a packet chain at buf (one chunk, no draw mode yet). Returns the first free
// word.
void *DisplayList_Begin(void *buf)
{
    g_DisplayList_Chunk = buf;
    g_DisplayList_ChunkLen = '\0';
    g_DisplayList_DrawMode = 0xffffffff;
    return (void *)((int)buf + 4);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", DisplayList_Reserve);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", DisplayList_SetDrawMode);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", DisplayList_End);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", Background_AddSolidTile);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", Background_AddOverlayTile);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", Background_AddTile);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", Background_AddTileGrid);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", Background_AddOverlayTiles);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/background", Background_AddSpriteGroups);

// Builds the page background into buf as a chain of 16-word packets: the 63x32 grid of
// 16x8 tiles (g_MenuTileMap), the overlay tiles (g_MenuOverlayTiles) and the sprite
// groups (g_MenuSpriteGroups). The tail goes to view.bgListTail.
void MenuPage_BuildBackground(void *unused, void *buf)
{
    unsigned int *puVar1;

    puVar1 = DisplayList_Begin(buf);
    puVar1 = Background_AddTileGrid(g_MenuTileMap, puVar1);
    puVar1 = Background_AddOverlayTiles(g_MenuOverlayTiles, puVar1);
    if (*g_MenuSpriteGroups != 0) {
        Background_AddSpriteGroups(g_MenuSpriteGroups, puVar1);
    }
    DisplayList_End();
}
}
