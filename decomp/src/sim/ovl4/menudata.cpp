// OVL4 80020c50-80021a40 (menudata): Menu data files, pages and the background picture
// stream.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
extern int *g_SoloData_CarPages;
int Gpu_TransferImage(RECT *rect, unsigned int *data, int store);
void Gpu_DrawSync(void);
void VolId_ReadFileAsync(VolBootId id, void *dst);
extern int g_MenuElementCount;
extern int g_MenuLanguage;
extern LangMenuFiles g_LangMenuFiles[7];
extern unsigned int g_MenuPageAttr;
int PackFile_StreamEntry(int volBootIdx, int entry, int *index, void *header);
extern int *g_CommonPicIdx;
void MenuPage_Load(void *pageData);
int GtMenuDat_ReadPage(int pageId, void *dst);
extern int *g_SoloData;
extern void *g_MenuPageBuf;
extern int g_MenuBgPicId;
extern BgPicStream g_BgPicStream;

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", CarSpecStrings_Load);

// Possible GS_125.OBJ/GsGetWorkBase Possible SSVM_2.OBJ/SsGetVoiceMask
int *FUN_OVL4__80020d48(void)
{
    return g_SoloData_CarPages;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuData_SetBuffers);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuData_Load);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuData_LoadSoloData);

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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuPage_FindCarPage);

// Streams background picture picId of commonpic (volume file 0xc3) into header and VRAM.
void MenuPage_LoadBgPic(int picId, void *header)
{
    PackFile_StreamEntry(0xc3, picId, g_CommonPicIdx, header);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", GtMenuDat_ReadPage);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", PackFile_ReadEntry);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", PackFile_SeekEntry);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", PackFile_StreamEntry);

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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuPage_UploadTiles);

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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuPage_Load);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuPage_AddElements);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", OT_InsertChain);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", MenuPage_GetElement);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", BgPicStream_ProcessSector);

/* CdAudio retry callback: resets the stream state. */

void BgPicStream_Restart(void)
{
    g_BgPicStream.bytesDone = 0;
    g_BgPicStream.state = '\0';
    g_BgPicStream.started = '\0';
    g_BgPicStream.lineFill = 0;
    g_BgPicStream.vramY = 0;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", BgPicStream_OnSector);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menudata", BgPic_StreamRead);
}
