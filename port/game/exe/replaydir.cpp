// Resident code 80068e2c-80069890 (replaydir). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "common.h"

extern "C" {
extern char D_8008FA70[];
extern char D_80091AB8[];
extern char D_80091ACC[];
extern unsigned char D_80091AEC[];
extern unsigned char D_80091B0C[];

#define SECTOR_FREE 0x3c0

void SectorMap_Init(SectorMap *map, short count)
{
    int i;
    short free = SECTOR_FREE;

    for (i = 959; i >= 0; i--) {
        map->next[i] = free;
    }
    map->count = count;
}

// Links chain's sectors in the map, the last one to -1. The game keeps a second copy of s
// for the else branch (4 words); permuter got it only with a variable that copies s.
void SectorMap_Link(SectorMap *map, SectorChain *chain)
{
    int n = chain->count;
    int i;

    for (i = 0; i < n; i++) {
        int s = chain->sectors[i];

        if (i == n - 1) {
            map->next[s] = -1;
        } else {
            int next = chain->sectors[i + 1];

            map->next[s] = next;
        }
    }
}

// 3 words: the game computes the map plus twice s and loads and stores at offset 2; GCC adds
// the 2 into p. p = &map[s] with p[1] matches on a short * (the count at [0]).
void SectorMap_FreeChain(SectorMap *map, int s)
{
    short *p;

    do {
        p = &map->next[s];
        s = *p;
        *p = SECTOR_FREE;
    } while (s != -1);
}

// Takes count free sectors into chain. Returns whether there were enough.
int SectorMap_Alloc(SectorMap *map, int count, SectorChain *chain)
{
    int i = 0;
    int n = 0;

    chain->count = count;
    while (i < map->count) {
        int s = map->next[i];

        if (s == SECTOR_FREE) {
            n++;
            chain->sectors[n - 1] = i;
            if (--count == 0) {
                return 1;
            }
        }
        i++;
    }
    chain->count = 0;
    return 0;
}

// 6 words: the game increments the sector offset at the top of the loop, from -2.
// Follows the chain from sector s into chain. Returns its length, 0 if it reaches a free
// sector.
int SectorMap_GetChain(SectorMap *map, int s, SectorChain *chain)
{
    int n = 1;

    while (1) {
        int i = n - 1;

        chain->sectors[i] = s;
        s = map->next[s];
        if (s == SECTOR_FREE) {
            break;
        }
        if (s == -1) {
            chain->count = n;
            return n;
        }
        n++;
    }
    chain->count = 0;
    return 0;
}

int SectorMap_CountFree(SectorMap *map)
{
    int n = 0;
    int i;

    for (i = 0; i < map->count; i++) {
        n += map->next[i] == SECTOR_FREE;
    }
    return n;
}

int SectorMap_CountUsed(SectorMap *map)
{
    int n = 0;
    int i;

    for (i = 0; i < map->count; i++) {
        n += map->next[i] != SECTOR_FREE;
    }
    return n;
}

void ReplayEntry_SetTitle(char *dst, char *title)
{
    strcpy(dst, title);
}

// Copies the entry's info to dst, ending a truncated one in D_8008FA70.
void ReplayEntry_GetInfo(ReplayDirEntry *entry, char *dst)
{
    if (entry->infoTruncated) {
        strncpy(dst, entry->info, 0x1f);
        *(Str3 *)(dst + 0x1f) = *(Str3 *)D_8008FA70;
    } else {
        strcpy(dst, entry->info);
    }
}

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

// An empty directory for a file of blocks card blocks (64 sectors each; the directory
// and the file's header take 0x2b).
void ReplayDir_Init(ReplayDir *dir, int blocks)
{
    short sectors;

    McHeader_Init(&dir->header);
    dir->header.blockCount = blocks;
    dir->header.iconType = 0x13;
    McHeader_SetTitle(&dir->header, D_80091ACC);
    memcpy(dir->header.clut, D_80091AEC, sizeof(dir->header.clut));
    memcpy(dir->header.icon[0], D_80091B0C, 0x80);
    memcpy(dir->header.icon[1], D_80091B0C + 0x80, 0x80);
    memcpy(dir->header.icon[2], D_80091B0C + 0x100, 0x80);
    sectors = blocks * 64 - 0x2b;
    dir->count = 0;
    dir->totalSectors = sectors;
    SectorMap_Init(&dir->sectorMap, sectors);
}

// 10 words: the game tests the count with lb, then an unsigned compare, where GCC loads it
// with lbu; and in the loop it keeps the index times 0x5c and adds the base at each read,
// where GCC steps a pointer.
// Whether a directory read from the card is sound: its block count, entry count, sectors
// and CRC.
int ReplayDir_IsValid(ReplayDir *dir)
{
    int ok = dir->header.blockCount < 16;
    int used;
    int i;

    ok &= dir->header.blockCount >= 2;
    if (dir->count < 0 || dir->count > 32) {
        return 0;
    }
    ok &= dir->totalSectors == dir->sectorMap.count;
    used = 0;
    for (i = 0; i < dir->count; i++) {
        short first = dir->entries[i].firstSector;

        if (first >= dir->totalSectors) {
            return 0;
        }
        used += dir->entries[i].sectorCount;
    }
    ok &= SectorMap_CountUsed(&dir->sectorMap) == used;
    ok &= dir->crc == Crc32((unsigned char *)dir, 0x1508);
    return ok;
}

// The game computes the entry's address once, base after the index, and loads the size
// through a copy of it (3 words). permuter got it only by reading the size through a pointer
// to it: `int *p = &dir->entries[i].size`, with `(&dir->entries[i])->crc`.
int ReplayDir_CheckEntryCrc(ReplayDir *dir, int i, unsigned char *data)
{
    unsigned int crc = dir->entries[i].crc;

    return crc == Crc32(data, dir->entries[i].size);
}

int ReplayDir_IsPlayable(ReplayDir *dir, int i)
{
    ReplayDirEntry *e = &dir->entries[i];

    return e->kind != 1;
}

// Whether a replay of size bytes fits, in place of entry i or (i < 0) as a new entry:
// 0 if it does, 1 if the directory is full, 2 if the card is.
int ReplayDir_CheckFits(ReplayDir *dir, int i, int size)
{
    int free = SectorMap_CountFree(&dir->sectorMap);
    int need = (unsigned int)(size + 0x7f) >> 7;

    if (i < 0) {
        if (dir->count == 32) {
            return 1;
        }
    } else {
        ReplayDirEntry *e = &dir->entries[i];

        free += e->sectorCount;
    }
    if (free < need) {
        return 2;
    }
    return 0;
}

void ReplayDir_UpdateCrc(ReplayDir *dir)
{
    unsigned int uVar1;

    uVar1 = Crc32((unsigned char *)dir, 0x1508);
    dir->crc = uVar1;
}

// 44 words off: the game gives sectors and chain each other's saved registers, adds the base
// after the index (as in ReplayDir_CheckEntryCrc) and computes the entry's address again
// after the copy, with a copy of it for each field.
// Saves entry and its data's sectors in place of entry i or (i < 0) as a new entry;
// chain gets the sectors. Returns whether it fits.
int ReplayDir_AddEntry(ReplayDir *dir, int i, ReplayDirEntry *entry, unsigned char *data, int size, SectorChain *chain)
{
    int added = 1;
    int sectors = (unsigned int)(size + 0x7f) >> 7;

    if (ReplayDir_CheckFits(dir, i, size)) {
        return 0;
    }
    if (i >= 0) {
        SectorMap_FreeChain(&dir->sectorMap, dir->entries[i].firstSector);
        added = 0;
    } else {
        i = dir->count;
    }
    if (!SectorMap_Alloc(&dir->sectorMap, sectors, chain)) {
        return 0;
    }
    SectorMap_Link(&dir->sectorMap, chain);
    dir->entries[i] = *entry;
    dir->entries[i].firstSector = chain->sectors[0];
    dir->entries[i].sectorCount = chain->count;
    dir->entries[i].size = size;
    dir->entries[i].crc = Crc32(data, size);
    dir->count += added;
    dir->crc = Crc32((unsigned char *)dir, 0x1508);
    return 1;
}

// 2 words: the game adds the base to the index once, after the FreeChain call, and the
// entries' offset in the loop; GCC adds the offset to the index and the base in the loop.
void ReplayDir_DeleteEntry(ReplayDir *dir, int i)
{
    SectorMap_FreeChain(&dir->sectorMap, (&dir->entries[i])->firstSector);
    while (i < dir->count - 1) {
        ReplayDirEntry *e = &dir->entries[i];

        *e = dir->entries[i + 1];
        i++;
    }
    dir->count--;
}

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

    iVar1 = McCard_StartTransfer('\a',port,D_80091AB8,(unsigned char *)dir,0,0x1580,0);
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

    iVar1 = McCard_StartTransfer('\x06',port,D_80091AB8,(unsigned char *)dir,0,0x1580,0);
    return iVar1;
}

// The game adds the base after the index and reads the two fields through two copies
// of the address, as in ReplayDir_CheckEntryCrc (8 words).
// Reads entry i's data into buf; chain gets its sectors.
int ReplayFile_StartReadEntry(ReplayDir *dir, int port, int i, void *buf, SectorChain *chain)
{
    ReplayDirEntry *e = &dir->entries[i];
    int size = e->size;

    SectorMap_GetChain(&dir->sectorMap, e->firstSector, chain);
    return McCard_StartTransfer(6, port, D_80091AB8, buf, 0x1580, size + 0x7f & ~0x7f, chain->sectors);
}
}
