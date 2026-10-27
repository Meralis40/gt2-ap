// Resident code 80069ff8-8006a3fc (savefile). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
unsigned int Crc32(unsigned char *data, int size);
void *McCard_FindFile(int port, char *name);
int McCard_StartCreate(int port, char *name, int size);
int McCard_StartTransfer(unsigned char cmd, int port, char *name, void *buf, int offset, int size, short *sectorMap);
void Options_ApplyNeGconCalib(GameState *state);
int SaveFile_GetDataSize(void);
void *memcpy(void *__dest, void *__src, size_t __n);
extern char D_80091C94[];
extern GameState g_GameState;

int SaveFile_GetDataSize(void)
{
    return 0x7ea0;
}

int SaveFile_GetBlockCount(void)
{
    int iVar1;
    unsigned int uVar2;

    iVar1 = SaveFile_GetDataSize();
    uVar2 = iVar1 + 0x1fffU & 0xffffe000;
    if ((int)uVar2 < 0) {
        uVar2 = uVar2 + 0x1fff;
    }
    return (int)uVar2 >> 0xd;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/savefile", SaveFile_InitHeader);

void *SaveFile_Find(GameSaveFile *file, int port)
{
    void *pvVar1;

    pvVar1 = McCard_FindFile(port, D_80091C94);
    return pvVar1;
}

int SaveFile_Create(GameSaveFile *file, int port)
{
    int iVar1;

    iVar1 = McCard_StartCreate(port, D_80091C94, (unsigned int)(file->header).blockCount << 0xd
                                                        );
    return iVar1;
}

int SaveFile_GetFileSize(void)
{
    int iVar1;

    iVar1 = SaveFile_GetDataSize();
    return iVar1 + 0x7fU & 0xffffff80;
}

int SaveFile_StartWrite(GameSaveFile *file, int port, GameSaveFile *src)
{
    int iVar1;

    iVar1 = SaveFile_GetDataSize();
    iVar1 = McCard_StartTransfer
                                        ('\a',port,D_80091C94,src,0,iVar1 + 0x7fU & 0xffffff80,
                                          0);
    return iVar1;
}

int SaveFile_StartRead(GameSaveFile *file, int port, GameSaveFile *dst)
{
    int iVar1;

    iVar1 = SaveFile_GetDataSize();
    iVar1 = McCard_StartTransfer
                                        ('\x06',port,D_80091C94,dst,0,iVar1 + 0x7fU & 0xffffff80,
                                          0);
    return iVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/savefile", GameState_WriteSave);

void GameState_ReadSave(unsigned int param_1, int param_2)
{
    memcpy(&g_GameState, (void *)(param_2 + 0x200), 0x7c9c);
    Options_ApplyNeGconCalib(&g_GameState);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/savefile", SaveFile_CopyGarageToPlayer2);

void *SaveFile_GetGarages(GameSaveFile *self, GameSaveFile *file)
{
    return file->state + 0x3c74;
}

void *SaveFile_GetCourseRecords(GameSaveFile *self, GameSaveFile *file)
{
    return file->state + 0x218;
}

void *SaveFile_GetLicenses(GameSaveFile *self, GameSaveFile *file)
{
    return file->state + 0x1418;
}

void *SaveFile_GetMachineTest0(GameSaveFile *self, GameSaveFile *file)
{
    return file->state + 0x3a88;
}

void *SaveFile_GetMachineTest1(GameSaveFile *self, GameSaveFile *file)
{
    return file->state + 0x3b2c;
}

void *SaveFile_GetMachineTest2(GameSaveFile *self, GameSaveFile *file)
{
    return file->state + 0x3bd0;
}

bool SaveFile_CheckCrc(GameSaveFile *file, GameSaveFile *data)
{
    unsigned int uVar1;
    unsigned int uVar2;

    uVar2 = data->crc;
    uVar1 = Crc32((unsigned char *)data, 0x7e9c);
    return uVar2 == uVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/savefile", Options_ApplyNeGconCalib);
}
