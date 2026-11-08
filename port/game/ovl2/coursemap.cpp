// OVL2 80017b7c-8001802c (coursemap): Course map.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern unsigned int (&g_VolLba);
unsigned char FUN_OVL2__80017a14(void *cm, int a1, int a2);


// Hashes the names of the course maps of info.
void CourseMap_Init(ArcCourseMap *cm, short tpage, ArcCourseMapEntry *info, void *buf, void *buf2)
{
    int i;

    cm->tpage = tpage;
    cm->info = info;
    cm->buf = buf;
    cm->state = 0;
    cm->ready = 0;
    cm->hookActive = 0;
    cm->hookPending = 0;
    cm->cdClaimed = 0;
    cm->stopRequested = 0;
    cm->buf2 = buf2;
    for (i = 0; i < info->nameOffset; i++) {
        unsigned int hash = Str_Hash((char *)info + info[i + 1].nameOffset);

        cm->hashes[i] = hash;
    }
}

// Cancels any load (CourseMap_Cancel), then requests the map whose name hash is
// nameHash (+0x14) and sets state 1.
void CourseMap_Start(ArcCourseMap *cm, ArcadeMenu *menu, unsigned int nameHash)
{
    CourseMap_Cancel(cm, menu);
    cm->hash = nameHash;
    cm->state = 1;
    cm->loaded = 0;
    cm->ready = 0;
}

// Thunk to CourseMap_Start.
void CourseMap_Request(ArcCourseMap *cm, ArcadeMenu *menu, unsigned int nameHash)
{
    CourseMap_Start(cm, menu, nameHash);
}

// Stops the map's load and hides it.
void CourseMap_Cancel(ArcCourseMap *cm, ArcadeMenu *menu)
{
    cm->ready = 0;
    if (cm->hookActive != 0) {
        ArcadeMenu_CancelHook(menu);
    }
    cm->hookPending = 0;
    if (cm->cdClaimed != 0) {
        cm->stopRequested = 1;
    }
    cm->state = 0;
}

// The index of the map whose name hash is hash; 0 if there is none.
int CourseMap_FindIndex(ArcCourseMap *cm, unsigned int hash)
{
    int count = cm->info->nameOffset;
    int i;

    for (i = 0; i < count; i++) {
        unsigned int cur = cm->hashes[i];

        if (cur == hash) {
            return i;
        }
    }
    return 0;
}

// Reads the requested map when the menu's CD read is free, and waits for the hook
// to decode it.
void CourseMap_Update(ArcCourseMap *cm, ArcadeMenu *menu)
{
    AsyncRead *read = &menu->cdRead;
    int status;

    if (cm->cdClaimed & cm->stopRequested) {
        read->busy = 0;
        read->state = 0;
        cm->stopRequested = 0;
        menu->cdBusy = 0;
        cm->cdClaimed = 0;
        return;
    }
    if (cm->hookPending != 0 && ArcadeMenu_SetHook(menu, FUN_OVL2__80017a14, cm) != 0) {
        cm->hookActive = 1;
        cm->hookPending = 0;
    }
    switch (cm->state) {
    case 1:
        if (cm->stopRequested == 0 && menu->cdBusy == 0) {
            menu->cdBusy = 1;
            cm->cdClaimed = 1;
            cm->hookPending = 1;
            cm->decoded = 0;
            cm->unk1d = 0;
            cm->state++;
        }
        return;
    case 2: {
        int index = CourseMap_FindIndex(cm, cm->hash);
        int sector = g_VolLba + VolId_GetSector((VolBootId)0x21);
        ArcCourseMapEntry *entry = &cm->info[index];

        if (AsyncRead_Start_OVL2(read, cm->buf, sector + entry->sector, entry->size) == 0) {
            return;
        }
        cm->state++;
    }
    case 3:
        switch (AsyncRead_Poll_OVL2(read, &status)) {
        case 0:
            cm->state = 2;
            break;
        case 2:
            menu->cdBusy = 0;
            cm->cdClaimed = 0;
            cm->loaded = 1;
            cm->state++;
            break;
        }
        return;
    case 4:
        if (cm->decoded == 0) {
            return;
        }
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        cm->state++;
        return;
    case 11:
        cm->ready = 1;
    default:
        cm->state = 0;
        return;
    }
}

// The map, once it is ready.
void CourseMap_Draw(ArcCourseMap *cm, void *ot, int x, int y, int brightness)
{
    int tpage = cm->tpage;

    if (cm->ready != 0) {
        PrimSprite *sprite = Prim_AddSprite(ot, brightness | (brightness << 8) | (brightness << 16));
        int clut;

        clut = ((tpage & 0x10) << 10) | ((tpage & 0xf) << 2);
        sprite->x = x - 0x1c;
        sprite->y = y - 0x64;
        sprite->v = 1;
        sprite->w = 0xbc;
        sprite->clut = clut;
        sprite->u = 0;
        sprite->h = 0xc8;
        Prim_AddDrawMode(ot, tpage);
    }
}

// The sector of the map whose name hash is hash, in the course map file.
int FUN_OVL2__80017fc4(ArcCourseMap *cm, unsigned int hash)
{
    int index = CourseMap_FindIndex(cm, hash);
    ArcCourseMapEntry *entry = &cm->info[index];

    return entry->sector;
}

// Its size.
int FUN_OVL2__80017ff8(ArcCourseMap *cm, unsigned int hash)
{
    int index = CourseMap_FindIndex(cm, hash);
    ArcCourseMapEntry *entry = &cm->info[index];

    return entry->size;
}
}
