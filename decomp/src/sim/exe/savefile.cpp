// Resident code 80069ff8-8006a3fc (savefile): the game save on the memory card.
#include "resident.h"

extern "C" {
unsigned int Crc32(unsigned char *data, int size);
void *McCard_FindFile(int port, char *name);
int McCard_StartCreate(int port, char *name, int size);
int McCard_StartTransfer(unsigned char cmd, int port, char *name, void *buf, int offset, int size, short *sectorMap);
void Options_ApplyNeGconCalib(GameState *state);
int SaveFile_GetDataSize(void);
void *memcpy(void *__dest, void *__src, size_t __n);
void bcopy(const void *src, void *dst, size_t n);
void McHeader_Init(McFileHeader *h);
void McHeader_SetTitle(McFileHeader *h, char *title);
extern char D_80091C94[];
extern char D_80091CA8[];
extern char D_80091CC4[];
extern char D_80091CE4[];
extern GameState g_GameState;
extern NeGconCalib g_NeGconCalib[2];

int SaveFile_GetDataSize(void)
{
    return 0x7ea0;
}

int SaveFile_GetBlockCount(GameSaveFile *file)
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

// The card header: the icon and title, and the blocks the save takes.
void SaveFile_InitHeader(GameSaveFile *file)
{
    McHeader_Init(&file->header);
    file->header.blockCount = SaveFile_GetBlockCount(file);
    file->header.iconType = 0x13;
    McHeader_SetTitle(&file->header, D_80091CA8);
    memcpy(file->header.clut, D_80091CC4, sizeof(file->header.clut));
    memcpy(file->header.icon[0], D_80091CE4, 0x80);
    memcpy(file->header.icon[1], D_80091CE4 + 0x80, 0x80);
    memcpy(file->header.icon[2], D_80091CE4 + 0x100, 0x80);
}

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

// The save image: the header, the game state, and a CRC of both.
void GameState_WriteSave(GameSaveFile *header, GameSaveFile *file)
{
    GameState *state = &g_GameState;

    memcpy(file, header, sizeof(file->header));
    memcpy(file->state, state, sizeof(file->state));
    file->crc = Crc32((unsigned char *)file, 0x7e9c);
}

void GameState_ReadSave(unsigned int param_1, int param_2)
{
    memcpy(&g_GameState, (void *)(param_2 + 0x200), 0x7c9c);
    Options_ApplyNeGconCalib(&g_GameState);
}

// The save's garage into garages[1], which the save doesn't hold.
void SaveFile_CopyGarageToPlayer2(GameSaveFile *self, GameSaveFile *file)
{
    GameState *state = &g_GameState;

    bcopy(&((GameState *)file->state)->garages[0], &state->garages[1], sizeof(Garage));
}

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

unsigned char SaveFile_CheckCrc(GameSaveFile *file, GameSaveFile *data)
{
    unsigned int uVar1;
    unsigned int uVar2;

    uVar2 = data->crc;
    uVar1 = Crc32((unsigned char *)data, 0x7e9c);
    return uVar2 == uVar1;
}

void Options_ApplyNeGconCalib(GameState *state)
{
    g_NeGconCalib[0] = state->ports[0].negconCalib;
    g_NeGconCalib[1] = state->ports[1].negconCalib;
}
}
