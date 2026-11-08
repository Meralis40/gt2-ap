// Resident code 8005d718-8005d9bc (vol): reading GT2.VOL's files by their index in
// the header's table, or by boot id through g_VolBootIds, and finding a car's files.
// A file's table entry is its byte offset in GT2.VOL; its size runs to the next
// entry's sector.
#include "common.h"

extern "C" {
extern unsigned short (&g_VolBootIds)[248];
extern VolHeader (&g_VolHeader);
extern unsigned int (&g_VolLba);
extern CarAsset (&g_CarAssets)[];
extern short (&g_CarAssetCount);

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

int Vol_GetSize(int file)
{
    unsigned int start = g_VolHeader.toc[file];

    return (g_VolHeader.toc[file + 1] & ~0x7ff) - start;
}

// sector is relative to the start of GT2.VOL.
// The game turns the sector into a byte offset and keeps the offset's low bits
// (andi, or), which GCC knows are 0 after the shift and drops. A local, an unsigned
// offset, a shift of the parameter and an inline Vol_ReadAt didn't keep them.
void Vol_ReadSectors(void *dst, int sector, int size)
{
    int offset = sector << 11;

    Cd_ReadBytes(dst, (g_VolLba + (offset >> 11)) * 0x800 | offset & 0x7ff, size);
}

// The game's version has no return: it turns the sector into a byte offset and
// falls through into Vol_ReadAtAsync, the next function.
void Vol_ReadSectorsAsync(void *dst, int sector, int size)
{
    Vol_ReadAtAsync(dst, sector << 11, size);
}

void Vol_ReadAtAsync(void *dst, unsigned int offset, int size)
{
    Cd_ReadAtAsync(dst, (g_VolLba + ((int)offset >> 0xb)) * 0x800 | offset & 0x7ff, size);
}

// Reads the file with reader, from its sector, its size in bytes.
void Vol_ReadFileWith(int file, void *dst, VolReader reader)
{
    unsigned int start = g_VolHeader.toc[file];

    reader(dst, start >> 11, (g_VolHeader.toc[file + 1] & ~0x7ff) - start);
}

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

// A binary search of g_CarAssets for carCode. Returns 0 if it isn't there.
// The game computes g_CarAssets' address first, before lo and the count, and adds
// it after the index. With the array's address in a local, GCC adds it first; with
// g_CarAssets indexed directly, the loop optimizer moves its address after the count's
// load. A goto loop keeps the address first but changes the registers; a do-while,
// an inline search function and carinfo.cpp's binary-search form gave the same
// code as the for loop.
CarAsset *CarAsset_Find(unsigned int carCode)
{
    int lo = 0;
    int hi = g_CarAssetCount;

    for (;;) {
        int mid = (lo + hi) >> 1;
        CarAsset *a = &g_CarAssets[mid];

        if (carCode == a->carCode) {
            return a;
        }
        if (lo >= hi) {
            return 0;
        }
        if (a->carCode < carCode) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
}
}
