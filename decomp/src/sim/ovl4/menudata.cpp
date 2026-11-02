// OVL4 80020c50-80021a40 (menudata): Menu data files, pages and the background picture
// stream.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
extern int *g_SoloData_CarPages;
int Gpu_TransferImage(RECT *rect, unsigned int *data, int store);
void Gpu_DrawSync(void);
void VolId_ReadFile(VolBootId id, void *dst);
void VolId_ReadFileAsync(VolBootId id, void *dst);
extern int g_MenuElementCount;
extern int g_MenuLanguage;
extern LangMenuFiles g_LangMenuFiles[7];
extern unsigned int g_MenuPageAttr;
extern int g_MenuPageBackPage;
int PackFile_StreamEntry(int volBootIdx, int entry, int *index, void *header);
extern int *g_CommonPicIdx;
extern int *g_GtMenuDatIdx;
void MenuPage_Load(void *pageData);
int GtMenuDat_ReadPage(int pageId, void *dst);
extern int *g_SoloData;
extern void *g_MenuPageBuf;
extern void *g_MenuScratch;
extern unsigned int *g_MenuTileMap;
extern unsigned int *g_MenuOverlayTiles;
extern int *g_MenuSpriteGroups;
extern MenuElement g_MenuElements[64];
extern int g_MenuBgPicId;
extern BgPicStream g_BgPicStream;
extern GameState g_GameState;
extern unsigned char g_CarSpecStringsGz[];
extern CarSpecStrings g_CarSpecStrings;
extern unsigned short g_VolBootIds[248];
extern VolHeader g_VolHeader;
extern unsigned int g_VolLba;
extern CdAudio g_CdAudio;
void Gzip_Inflate(void *src, void *dst);
void *memcpy(void *dst, const void *src, unsigned int n);
void bcopy(const void *src, void *dst, unsigned int n);
void Vol_ReadAtAsync(void *dst, unsigned int offset, int size);
void CdAudio_SetupRead(void *dst, unsigned int sector, int size, int mode);
int CdAudio_RunScript(int script);
void CdAudio_WaitRead(void (*onRetry)(void));
void CdGetSector(void *dst, int words);
int MenuData_LoadSoloData(int *buf, int size);
int PackFile_ReadEntry(int volBootIdx, int entry, int *index, void *dst);
void PackFile_SeekEntry(int volBootIdx, int entry, int *index);
int BgPic_StreamRead(void *header, unsigned int offset, int size);
void MenuPage_LoadBgPic(int picId, void *header);
unsigned int *MenuPage_UploadTiles(int *data);
unsigned int *MenuPage_UploadBgClut(void *header);
void MenuPage_AddElements(MenuElement *src, int count);
int BgPicStream_ProcessSector(BgPicStream *s);
int BgPicStream_OnSector(unsigned int sector, void *arg);
void BgPicStream_Restart(void);

// Inflates the car spec strings of all the languages and keeps the game
// language's.
void CarSpecStrings_Load(void)
{
    CarSpecRecord tables[8];

    Gzip_Inflate(g_CarSpecStringsGz, tables);
    *(CarSpecRecord *)&g_CarSpecStrings = tables[g_GameState.language];
}

// Possible GS_125.OBJ/GsGetWorkBase Possible SSVM_2.OBJ/SsGetVoiceMask
int *FUN_OVL4__80020d48(void)
{
    return g_SoloData_CarPages;
}

// Sets the page buffer and the scratch buffer, which holds the tile map, the
// overlay tiles and the sprite groups (and the background picture header).
void MenuData_SetBuffers(void *pageBuf, void *scratch)
{
    g_MenuScratch = scratch;
    g_MenuPageBuf = pageBuf;
    g_MenuTileMap = (unsigned int *)((char *)scratch + 4);
    g_MenuOverlayTiles = (unsigned int *)((char *)scratch + 0x6000);
    g_MenuSpriteGroups = (int *)((char *)g_MenuOverlayTiles + 0x2800);
}

// Reads solodata, the commonpic index and the gtmenudat index for the game
// language into buf, one after the other; returns the bytes used.
int MenuData_Load(void *buf, int size)
{
    int used;
    int *idx;
    int n;

    g_MenuBgPicId = -1;
    g_MenuLanguage = g_GameState.language;
    used = MenuData_LoadSoloData((int *)buf, 0x1400);
    idx = (int *)((char *)buf + used);
    g_CommonPicIdx = idx;
    VolId_ReadFile(VOLID_GTMENU_COMMONPIC_IDX, idx);
    n = *idx * 4 + 8;
    used += n;
    idx = (int *)((char *)idx + n);
    g_GtMenuDatIdx = idx;
    VolId_ReadFile(g_LangMenuFiles[g_MenuLanguage].gtmenudatIdx, idx);
    n = *idx * 4 + 8;
    return used + n;
}

// Reads solodata for the menu language into buf: the message page table (a u16
// count rounded up to even, then the ids), then the car page table. Returns its
// size.
#ifdef NON_MATCHING
// 10 lines: the game rounds the count up in other registers (the count in $v0, the
// sum in $v1, the mask in $v0), so the mask loads after the add.
int MenuData_LoadSoloData(int *buf, int size)
{
    int n;
    int total;

    VolId_ReadFileAsync(g_LangMenuFiles[g_MenuLanguage].solodata, buf);
    g_SoloData = buf;
    n = *buf;
    n = (n + 1) & ~1;
    n <<= 1;
    buf = (int *)((char *)buf + (n + 4));
    total = n + 8;
    g_SoloData_CarPages = buf;
    return total + *buf * 8;
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuData_LoadSoloData);
#endif

// Clears the page, reads iconimg for the menu language into buf and uploads it to VRAM
// (0x2c0, 0) 64x256.
void MenuData_LoadIcons(unsigned int *buf)
{
    RECT local_10;

    g_MenuElementCount = 0;
    g_MenuPageAttr = 0;
    VolId_ReadFileAsync((unsigned int)g_LangMenuFiles[g_MenuLanguage].iconimg, buf);
    local_10.x = 0x2c0;
    local_10.w = 0x40;
    local_10.y = 0;
    local_10.h = 0x100;
    Gpu_TransferImage(&local_10, buf, 0);
    Gpu_DrawSync();
}

// Finds carId's page in solodata's car page table by binary search; 0 if it has
// none. The table pointer steps past the count (*table++), as the game does.
int MenuPage_FindCarPage(unsigned int carId)
{
    int *table = g_SoloData_CarPages;
    int lo = -1;
    int hi = *table++;
    int mid;
    CarPage *page;

    do {
        mid = (lo + hi) >> 1;
        page = &((CarPage *)table)[mid];
        if (page->carId == carId)
            return page->pageId;
        if (page->carId < carId)
            lo = mid;
        else
            hi = mid;
    } while (lo + 1 != hi);
    return 0;
}

// Streams background picture picId of commonpic (volume file 0xc3) into header and VRAM.
void MenuPage_LoadBgPic(int picId, void *header)
{
    PackFile_StreamEntry(0xc3, picId, g_CommonPicIdx, header);
}

// Reads page pageId of gtmenudat into dst, or with no dst only seeks the drive to it.
int GtMenuDat_ReadPage(int pageId, void *dst)
{
    if (dst == 0) {
        PackFile_SeekEntry(g_LangMenuFiles[g_MenuLanguage].gtmenudatDat, pageId, g_GtMenuDatIdx);
        return 0;
    }
    return PackFile_ReadEntry(g_LangMenuFiles[g_MenuLanguage].gtmenudatDat, pageId, g_GtMenuDatIdx, dst);
}

// Reads entry of a pack file (index: a count, then the byte offsets) into dst;
// returns its size.
int PackFile_ReadEntry(int volBootIdx, int entry, int *index, void *dst)
{
    int *offsets = index + 1;
    unsigned int start = offsets[entry];
    int size = offsets[entry + 1] - start;

    Vol_ReadAtAsync(dst, (start & ~3) + (g_VolHeader.toc[g_VolBootIds[volBootIdx]] & ~0x7ff), size);
    return size;
}

// Seeks the drive to entry of a pack file.
void PackFile_SeekEntry(int volBootIdx, int entry, int *index)
{
    int *e = index + entry;

    CdAudio_SetupRead(0, ((e[1] & ~3) + (g_VolHeader.toc[g_VolBootIds[volBootIdx]] & ~0x7ff)) >> 11, 0, 0);
    g_CdAudio.reqSkip = 0;
    CdAudio_RunScript(9);
}

// Streams entry of a pack file, a background picture, into header and VRAM;
// returns its size.
int PackFile_StreamEntry(int volBootIdx, int entry, int *index, void *header)
{
    int *offsets = index + 1;
    unsigned int start = offsets[entry];
    int size = offsets[entry + 1] - start;

    BgPic_StreamRead(header, (start & ~3) + (g_VolHeader.toc[g_VolBootIds[volBootIdx]] & ~0x7ff), size);
    return size;
}

// Page ids with bit 31 set are message pages: the low bits index the u16 table of
// solodata (g_SoloData + 4). Reads the page from gtmenudat into g_MenuPageBuf and
// parses it (MenuPage_Load); if that changed the background picture, seeks the drive
// back to the page.
void MenuPage_LoadById(int pageId)
{
    int iVar1;

    if (pageId < 0) {
        pageId = (int)*(unsigned short *)((int)g_SoloData + pageId *2 + 4);
    }
    Gpu_DrawSync();
    GtMenuDat_ReadPage(pageId, g_MenuPageBuf);
    iVar1 = g_MenuBgPicId;
    MenuPage_Load(g_MenuPageBuf);
    if (iVar1 != g_MenuBgPicId) {
        GtMenuDat_ReadPage(pageId, (void *)0x0);
    }
}

// Copies the page's overlay tiles (a count, then the tiles) to g_MenuOverlayTiles,
// then uploads the tile CLUTs to VRAM (0x240, 0xf8) 64x8 and the tile pixels to
// (0x280, 0x100), 128 wide, in rows of 8 for 32 tiles. Returns the CLUTs. The
// count is read through a copy of data from before its increment, and the rect
// through a pointer, as in the game.
unsigned int *MenuPage_UploadTiles(int *data)
{
    int size;
    unsigned int *clut;
    unsigned int n;
    RECT rect;
    RECT *r = &rect;
    int *p;

    p = data++;
    size = (p[1] + 1) * 4;
    memcpy(g_MenuOverlayTiles, data, size);
    clut = (unsigned int *)((char *)data + size);
    rect.x = 0x240;
    rect.y = 0xf8;
    rect.w = 0x40;
    rect.h = 8;
    Gpu_TransferImage(r, clut, 0);
    data = (int *)(clut + 0x101);
    n = clut[0x100];
    rect.x = 0x280;
    rect.y = 0x100;
    rect.w = 0x80;
    rect.h = (n >> 5) * 8;
    if (n & 0x1f)
        rect.h += 8;
    Gpu_TransferImage(r, (unsigned int *)data, 0);
    Gpu_DrawSync();
    return clut;
}

// Uploads the background picture CLUTs (header + 0x1f84) to VRAM (0, 0x1f8), 512x8.
unsigned int *MenuPage_UploadBgClut(void *header)
{
    RECT local_10;

    local_10.y = 0x1f8;
    local_10.w = 0x200;
    local_10.x = 0;
    local_10.h = 8;
    Gpu_TransferImage(&local_10, (unsigned int *)((int)header + 0x1f84), 0);
    Gpu_DrawSync();
    return (unsigned int *)((int)header + 0x1f84);
}

// Parses a gtmenudat page: the sprite groups (each with its own elements),
// which go to g_MenuSpriteGroups, the page's other elements, its attributes,
// back page and background picture, then the overlay tiles. Loads the
// background picture if it changed. The group loop tests at the top, as in the
// game, only written as while (1) with a break.
void MenuPage_Load(void *pageData)
{
    unsigned int count;
    unsigned int i;
    char *p = (char *)pageData;
    char *groups = p + 4;
    unsigned short sprites;
    unsigned short elements;
    int n;
    int picId;

    g_MenuElementCount = 0;
    count = *(unsigned int *)(p + 4);
    p += 8;
    i = 0;
    while (1) {
        if (i >= count)
            break;
        i++;
        sprites = ((unsigned short *)p)[0];
        elements = ((unsigned short *)p)[1];
        p += sprites * 12 + 4;
        MenuPage_AddElements((MenuElement *)p, elements);
        p += elements * sizeof(MenuElement);
    }
    memcpy(g_MenuSpriteGroups, groups, p - groups);
    n = *(int *)p;
    p += 4;
    MenuPage_AddElements((MenuElement *)p, n);
    p += n * sizeof(MenuElement);
    g_MenuPageAttr = *(unsigned int *)p;
    p += 4;
    g_MenuPageBackPage = *(int *)p;
    p += 4;
    picId = *(int *)p;
    MenuPage_UploadTiles((int *)(p + 4));
    if (picId != g_MenuBgPicId) {
        MenuPage_LoadBgPic(picId, g_MenuScratch);
        MenuPage_UploadBgClut(g_MenuScratch);
    }
    g_MenuBgPicId = picId;
}

// Appends count elements to g_MenuElements, up to 64.
void MenuPage_AddElements(MenuElement *src, int count)
{
    for (; count > 0 && g_MenuElementCount < 64; count--) {
        g_MenuElements[g_MenuElementCount] = *src;
        src++;
        g_MenuElementCount++;
    }
}

// Links the chain head..tail in after ot. The 24-bit links are copied with lwl
// and swl, one asm statement each; GNU as moves the last into the return's delay
// slot, as in the game.
void OT_InsertChain(OtTag *ot, OtTag *head, OtTag *tail)
{
    unsigned int link;

    __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(tail) : "memory");
    __asm__ volatile("swl\t%0,2(%1)" : : "r"((unsigned int)head << 8), "r"(ot) : "memory");
}

// The page's element index, or null past the last one.
MenuElement *MenuPage_GetElement(int index)
{
    if (index >= g_MenuElementCount)
        return 0;
    return &g_MenuElements[index];
}

// Takes one sector of a background picture: the first gives the header from
// startOffset, the next ones fill the 0x4000-byte header, and the rest go to the
// line buffer, which goes to VRAM 8 lines (0x1000 bytes) at a time. Returns 1
// once the picture is done.
int BgPicStream_ProcessSector(BgPicStream *s)
{
    char *sector = (char *)s->sectorBuf;
    char *header = (char *)s->header;
    int state = s->state;
    char *line = (char *)s->lineBuf;
    unsigned int done;
    unsigned int n;

    s->started = 1;
    switch (state) {
    case 0:
        sector += s->startOffset;
        s->bytesDone = 0x800 - s->startOffset;
        bcopy(sector, header, 0x800 - s->startOffset);
        s->state = 1;
        break;
    case 1:
        *(SectorData *)(header + s->bytesDone) = *(SectorData *)sector;
        done = s->bytesDone;
        if ((s->bytesDone = done + 0x800) < 0x4000)
            return 0;
        n = done - 0x3800;
        bcopy(sector + (0x800 - n), line, n);
        s->lineFill = n;
        s->state = 2;
        break;
    case 2:
        *(SectorData *)(line + s->lineFill) = *(SectorData *)sector;
        s->bytesDone += 0x800;
        s->lineFill += 0x800;
        if (s->lineFill >= 0x1000) {
            Gpu_TransferImage((RECT *)&s->vramX, (unsigned int *)s->lineBuf, 0);
            Gpu_DrawSync();
            s->vramY += 8;
            s->lineFill -= 0x1000;
            bcopy((char *)s->lineBuf + 0x1000, s->lineBuf, s->lineFill);
        }
        if (s->bytesDone < s->size)
            return 0;
        s->state = 3;
        break;
    case 3:
        return 1;
    default:
        return 0;
    }
    return 0;
}

/* CdAudio retry callback: resets the stream state. */

void BgPicStream_Restart(void)
{
    g_BgPicStream.bytesDone = 0;
    g_BgPicStream.state = '\0';
    g_BgPicStream.started = '\0';
    g_BgPicStream.lineFill = 0;
    g_BgPicStream.vramY = 0;
}

// The CD reader's sector callback while a background picture streams: takes the
// expected sector (8 otherwise) and returns 2 once the picture is done.
#ifdef NON_MATCHING
// The game has the &g_CdAudio-in-$a0 pattern (like RaceFont_OnSector): it moves
// the arguments up a register and builds the address in $a0 before the prologue.
int BgPicStream_OnSector(unsigned int sector, void *arg)
{
    CdAudio *cd = &g_CdAudio;
    int result = 8;
    BgPicStream *s = &g_BgPicStream;

    cd->curSector = cd->nextSector;
    if (sector == cd->nextSector) {
        result = 0;
        cd->nextSector = sector + 1;
        CdGetSector(g_BgPicStream.sectorBuf, 0x200);
        if (BgPicStream_ProcessSector(s)) {
            cd->unk167 = 0;
            result = 2;
        }
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", BgPicStream_OnSector);
#endif

// Streams a background picture of size bytes at byte offset of the volume into
// header and VRAM (0x300, 0), 256 wide, through two stack buffers.
int BgPic_StreamRead(void *header, unsigned int offset, int size)
{
    char sectorBuf[0x800];
    char lineBuf[0x1800];
    BgPicStream *s = &g_BgPicStream;

    s->header = header;
    g_BgPicStream.sectorBuf = sectorBuf;
    s->lineBuf = lineBuf;
    s->vramX = 0x300;
    s->vramW = 0x100;
    s->vramH = 8;
    s->state = 0;
    s->started = 0;
    s->lineFill = 0;
    s->vramY = 0;
    s->size = size;
    offset = ((g_VolLba + (offset >> 11)) << 11) | (offset & 0x7ff);
    CdAudio_SetupRead(0, offset >> 11, size, 0);
    {
        unsigned int skip = offset & 0x7ff;
        CdAudio *cd = &g_CdAudio;

        cd->reqSkip = skip;
        cd->onSector = (void *)BgPicStream_OnSector;
        s->startOffset = skip;
    }
    CdAudio_WaitRead(BgPicStream_Restart);
    return size;
}
}
