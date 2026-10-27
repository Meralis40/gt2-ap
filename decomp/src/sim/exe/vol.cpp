// Resident code 8005d718-8005d9bc (vol). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Cd_ReadAtAsync(void *dst, unsigned int byteOffset, int size);
unsigned int Vol_GetSector(int file);
int Vol_GetSize(int file);
void Vol_ReadFile(int file, void *dst);
void Vol_ReadFileAsync(int file, void *dst);
void Vol_ReadFileWith(int file, void *dst, void *reader);
void Vol_ReadSectors(void *dst, int sector, int size);
void Vol_ReadSectorsAsync(void *dst, int sector, int size);
extern unsigned short g_VolBootIds[248];
extern VolHeader g_VolHeader;
extern unsigned int g_VolLba;

unsigned int VolId_GetSector(VolBootId id)
{
    unsigned int uVar1;

    uVar1 = Vol_GetSector((unsigned int)g_VolBootIds[id]);
    return uVar1;
}

unsigned int Vol_GetSector(int file)
{
    return g_VolHeader.toc[file] >> 0xb;
}

int VolId_GetSize(VolBootId id)
{
    int iVar1;

    iVar1 = Vol_GetSize((unsigned int)g_VolBootIds[id]);
    return iVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/vol", Vol_GetSize);

INCLUDE_ASM("asm/sim/exe/nonmatchings/vol", Vol_ReadSectors);

INCLUDE_ASM("asm/sim/exe/nonmatchings/vol", Vol_ReadSectorsAsync);

void Vol_ReadAtAsync(void *dst, unsigned int offset, int size)
{
    Cd_ReadAtAsync(dst, (g_VolLba + ((int)offset >> 0xb)) * 0x800 | offset & 0x7ff, size);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/vol", Vol_ReadFileWith);

void VolId_ReadFile(VolBootId id, void *dst)
{
    Vol_ReadFile((unsigned int)g_VolBootIds[id], dst);
}

void VolId_ReadFileAsync(VolBootId id, void *dst)
{
    Vol_ReadFileAsync((unsigned int)g_VolBootIds[id], dst);
}

void Vol_ReadFile(int file, void *dst)
{
    Vol_ReadFileWith(file, dst, Vol_ReadSectors);
}

void Vol_ReadFileAsync(int file, void *dst)
{
    Vol_ReadFileWith(file, dst, Vol_ReadSectorsAsync);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/vol", CarAsset_Find);
}
