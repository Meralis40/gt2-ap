// Resident code 80068e2c-80069890 (replaydir). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
unsigned int Crc32(unsigned char *data, int size);
void *McCard_FindFile(int port, char *name);
int McCard_StartCreate(int port, char *name, int size);
int McCard_StartTransfer(unsigned char cmd, int port, char *name, void *buf, int offset, int size, short *sectorMap);
char *strcpy(char *__dest, char *__src);
size_t strlen(char *__s);
char *strncpy(char *__dest, char *__src, size_t __n);
extern char D_80091AB8[];

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", SectorMap_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", SectorMap_Link);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", SectorMap_FreeChain);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", SectorMap_Alloc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", SectorMap_GetChain);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", SectorMap_CountFree);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", SectorMap_CountUsed);

void ReplayEntry_SetTitle(char *dst, char *title)
{
    strcpy(dst, title);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayEntry_GetInfo);

void ReplayEntry_SetCarName(void *entry, char *name)
{
    size_t sVar1;

    sVar1 = strlen(name);
    if ((int)sVar1 < 0x20) {
        strcpy((char *)((int)entry + 0x20), name);
    }
    else {
        strncpy((char *)((int)entry + 0x20), name, 0x1f);
        *(unsigned char *)((int)entry + 0x3f) = 0x3f;
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayDir_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayDir_IsValid);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayDir_CheckEntryCrc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayDir_IsPlayable);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayDir_CheckFits);

void ReplayDir_UpdateCrc(ReplayDir *dir)
{
    unsigned int uVar1;

    uVar1 = Crc32((unsigned char *)dir, 0x1508);
    dir->crc = uVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayDir_AddEntry);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayDir_DeleteEntry);

void *ReplayFile_Find(ReplayDir *dir, int port)
{
    void *pvVar1;

    pvVar1 = McCard_FindFile(port, D_80091AB8);
    return pvVar1;
}

int ReplayFile_Create(ReplayDir *dir, int port)
{
    int iVar1;

    iVar1 = McCard_StartCreate(port, D_80091AB8,
                                                          (unsigned int)(dir->header).blockCount << 0xd);
    return iVar1;
}

int ReplayFile_StartWriteDir(ReplayDir *dir, int port)
{
    int iVar1;

    iVar1 = McCard_StartTransfer('\a',port,D_80091AB8,dir,0,0x1580,0);
    return iVar1;
}

int ReplayFile_StartWriteData(ReplayDir *dir, int port, void *data, int size, short *sectors)
{
    int iVar1;

    iVar1 = McCard_StartTransfer
                                        ('\a',port,D_80091AB8,data,0x1580,size + 0x7fU & 0xffffff80,
                                          sectors + 1);
    return iVar1;
}

int ReplayFile_StartReadDir(ReplayDir *dir, int port)
{
    int iVar1;

    iVar1 = McCard_StartTransfer('\x06',port,D_80091AB8,dir,0,0x1580,0);
    return iVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydir", ReplayFile_StartReadEntry);
}
