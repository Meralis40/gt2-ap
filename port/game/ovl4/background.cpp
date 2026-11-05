// OVL4 80021c38-800222e4 (background): Display lists and the page background.
// A display list is a chain of chunks of up to 16 words, each starting with an OT
// tag (length, 24-bit link to the next chunk).
#include "common.h"

extern "C" {
extern unsigned int *g_DisplayList_Chunk;
extern unsigned char g_DisplayList_ChunkLen;
extern unsigned int g_DisplayList_DrawMode;
extern unsigned int g_MenuPageAttr;
extern unsigned int *g_MenuTileMap;
extern unsigned int *g_MenuOverlayTiles;
extern int *g_MenuSpriteGroups;
}

// The tile lists are read through a cursor, which the game keeps in memory (the
// argument's stack slot) and moves on after each read.
inline unsigned int Bg_ReadWord(void **cursor)
{
    unsigned int v = *(unsigned int *)*cursor;

    *cursor = (unsigned int *)*cursor + 1;
    return v;
}

inline unsigned short Bg_ReadHalf(void **cursor)
{
    unsigned short v = *(unsigned short *)*cursor;

    *cursor = (unsigned short *)*cursor + 1;
    return v;
}

extern "C" {
// Starts a packet chain at buf (one chunk, no draw mode yet). Returns the first free
// word.
void *DisplayList_Begin(void *buf)
{
    g_DisplayList_Chunk = (unsigned int *)buf;
    g_DisplayList_ChunkLen = '\0';
    g_DisplayList_DrawMode = 0xffffffff;
    return (void *)((int)buf + 4);
}

// Reserves words in the current chunk; when it would exceed 16 words, closes the
// chunk (length, link to p) and starts a new one at p. Returns where to write.
unsigned int *DisplayList_Reserve(unsigned int *p, int words)
{
    unsigned int len = g_DisplayList_ChunkLen;
    // those two registers variables with the forced registers are necessary for correct compilation.
    register unsigned int link asm("a0");
    register unsigned int *temp asm("v0");
    unsigned int *a0 = p;
    
    if ((int)(len + words) > 16) {
        link = (unsigned int)a0 << 8;
        ((unsigned char*)g_DisplayList_Chunk)[3] = len;
        temp = g_DisplayList_Chunk;
        __asm__("swl\t%0,2(%1)" : : "r"(link), "r"(temp) : "memory");
        len = 0;
        g_DisplayList_Chunk = a0;
        a0++;
        g_DisplayList_ChunkLen = 0;
    }
    g_DisplayList_ChunkLen = len + words;
    return a0;
}

// Adds a draw mode word only when it differs from the last one.
unsigned int *DisplayList_SetDrawMode(unsigned int *p, unsigned int mode)
{
    unsigned int *w;

    if (g_DisplayList_DrawMode == mode) {
        return p;
    }
    g_DisplayList_DrawMode = mode;
    w = DisplayList_Reserve(p, 1);
    *w = mode;
    return w + 1;
}

// Closes the last chunk with the end-of-list link. Returns that chunk.
void *DisplayList_End(void)
{
    ((unsigned char *)g_DisplayList_Chunk)[3] = g_DisplayList_ChunkLen;
    unsigned int *chunk = g_DisplayList_Chunk;  // a local: the game loads it into $v0

    __asm__ volatile("swl\t%0,2(%1)" : : "r"(0xffffff00), "r"(chunk) : "memory");
    return g_DisplayList_Chunk;
}

// A 16x8 TILE coloured from the 15-bit colour in the top half of tile. Black tiles
// are skipped when page attr bit 9 is set.
// Scheduling: the game computes the blue part (sra, andi) before the green
// part's andi and or. Tried: every order of the three parts, the parts as
// locals or separate |= statements, the colour as signed/unsigned short, the
// green part as (c >> 5 & 0x1f) << 11 or (c & 0x3e0) << 6, the page attr test
// in a local, and the declarations of x and y after the colour.
unsigned int *Background_AddSolidTile(unsigned int *p, unsigned int tile)
{
    short x = (tile & 0x3f) << 4;
    short y = (tile >> 5) & 0x7f8;
    int c = (tile >> 16) & 0x7fff;
    unsigned int rgb = (c & 0x1f) << 3 | ((c << 6) & 0xf800) | ((c >> 7) & 0xf8) << 16;
    unsigned int *w;
    BgTile *t;

    if ((g_MenuPageAttr & 0x200) && rgb == 0) {
        return p;
    }
    w = DisplayList_Reserve(p, 3);
    t = (BgTile *)(w - 1);
    t->rgbc = rgb | 0x60000000;
    t->w = 0x10;
    t->x = x;
    t->y = y;
    t->h = 8;
    return w + 3;
}

// A 16x8 textured SPRT from the page tile sheet uploaded by MenuPage_UploadTiles.
unsigned int *Background_AddOverlayTile(unsigned int *p, unsigned int tile)
{
    unsigned int hi = tile >> 16;
    unsigned int *w;
    BgSprite *s;

    p = DisplayList_SetDrawMode(p, (hi & 0x10) >> 4 | 0xe100001a);
    w = DisplayList_Reserve(p, 4);
    s = (BgSprite *)(w - 1);
    s->rgbc = 0x64808080;
    s->x = (tile & 0xff) << 4;
    s->y = (tile >> 5) & 0x7f8;
    s->u = (hi << 4) & 0xf0;
    s->v = (tile >> 18) & 0xf8;
    unsigned int cx = ((tile >> 27) & 3) | 0x24;
    unsigned int cy = ((tile >> 27) & 0x1c) | 0x3e0;

    s->clut = cx | cy << 4;
    s->w = 0x10;
    s->h = 8;
    return w + 4;
}

// A 16x8 textured SPRT from the 8bpp background picture page: UV and CLUT row from
// tile.
// The draw mode: the game ORs 0xe1000080 into the 0x10 bit, then the 0x400 bit,
// then 0xc (in the call's delay slot); GCC folds the two constants into
// 0xe100008c. Tried: the 0xc in a separate statement, a local or an inline
// function, added instead of ORed, and PsyQ's getTPage with x = 768 + ....
unsigned int *Background_AddTile(unsigned int *p, unsigned int tile)
{
    unsigned int hi = tile >> 16;
    unsigned int *w;
    BgSprite *s;

    p = DisplayList_SetDrawMode(p, (hi & 0x400) >> 6 | (hi & 0x10) >> 3 | 0xe1000080 | 0xc);
    w = DisplayList_Reserve(p, 4);
    s = (BgSprite *)(w - 1);
    s->rgbc = 0x64808080;
    s->x = (tile & 0xff) << 4;
    s->y = (tile >> 5) & 0x7f8;
    s->u = (hi << 4) & 0xf0;
    s->v = (tile >> 18) & 0xf8;
    s->clut = ((tile >> 28) & 1) << 4 | (((tile >> 28) & 0xe) | 0x3f0) << 5;
    s->w = 0x10;
    s->h = 8;
    return w + 4;
}

// 63 rows x 32 tiles: bit 7 set = solid tile (Background_AddSolidTile), else a
// textured tile (Background_AddTile).
unsigned int *Background_AddTileGrid(void *tiles, unsigned int *p)
{
    int row;
    int col;

    for (row = 0; row < 63; row++) {
        for (col = 0; col < 32; col++) {
            unsigned int tile = Bg_ReadWord(&tiles);

            if (tile & 0x80) {
                p = Background_AddSolidTile(p, tile);
            } else {
                p = Background_AddTile(p, tile);
            }
        }
    }
    return p;
}

// A count, then tiles: solid (bit 7) or textured from the page tile sheet
// (Background_AddOverlayTile).
unsigned int *Background_AddOverlayTiles(void *tiles, unsigned int *p)
{
    int i;
    int count = Bg_ReadWord(&tiles);

    for (i = 0; i < count; i++) {
        unsigned int tile = Bg_ReadWord(&tiles);

        if (tile & 0x80) {
            p = Background_AddSolidTile(p, tile);
        } else {
            p = Background_AddOverlayTile(p, tile);
        }
    }
    return p;
}

// A count, then for each group (u16 sprite count, u16 element count): the 12-byte
// sprites (xy, uv, wh, tpage, clut) as SPRTs; the group's 76-byte elements are
// skipped.
unsigned int *Background_AddSpriteGroups(void *groups, unsigned int *p)
{
    int count;
    int i = 0;

    count = Bg_ReadWord(&groups);
    for (; i < count; i++) {
        unsigned int sprites = Bg_ReadHalf(&groups);
        unsigned int elements = Bg_ReadHalf(&groups);

        for (; sprites != 0; sprites--) {
            unsigned short x;
            unsigned short y;
            unsigned short uv;
            unsigned short wh;
            unsigned short tpage;
            unsigned short clut;
            BgSprite *s;

            x = Bg_ReadHalf(&groups);
            y = Bg_ReadHalf(&groups);
            uv = Bg_ReadHalf(&groups);
            wh = Bg_ReadHalf(&groups);
            tpage = Bg_ReadHalf(&groups);
            clut = Bg_ReadHalf(&groups);
            p = DisplayList_SetDrawMode(p, (tpage & 0x1f) | 0xe1000000);
            p = DisplayList_Reserve(p, 4);
            s = (BgSprite *)(p - 1);
            s->rgbc = 0x64808080;
            p += 4;
            *(unsigned int *)&s->x = y << 16 | x;
            *(unsigned short *)&s->u = uv;
            s->clut = clut;
            *(unsigned int *)&s->w = (wh & 0xff00) << 8 | (wh & 0xff);
        }
        groups = (char *)groups + elements * 0x4c;
    }
    return p;
}

// Builds the page background into buf as a chain of 16-word packets: the 63x32 grid
// of 16x8 tiles (g_MenuTileMap), the overlay tiles (g_MenuOverlayTiles) and the
// sprite groups (g_MenuSpriteGroups). The tail goes to view.bgListTail.
void *MenuPage_BuildBackground(void *unused, void *buf)
{
    unsigned int *p;

    p = (unsigned int *)DisplayList_Begin(buf);
    p = Background_AddTileGrid(g_MenuTileMap, p);
    p = Background_AddOverlayTiles(g_MenuOverlayTiles, p);
    if (*g_MenuSpriteGroups != 0) {
        Background_AddSpriteGroups(g_MenuSpriteGroups, p);
    }
    return DisplayList_End();
}
}
