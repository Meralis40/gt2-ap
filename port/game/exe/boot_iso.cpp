// Boot code 80010f1c-80011d30 (boot_iso): an ISO 9660 reader that finds the game's files on
// the disc, and the indexes of the cars, wheels, engine sounds and courses built from
// GT2.VOL's directory.
#include "common.h"

extern "C" {
extern char D_80011D98[];       // "CD001"
extern char D_80011DA0[];       // ";1"
extern char D_80011DA4[];       // "gt2.vol"
extern char D_80011DAC[];       // "music.dat"
extern char D_80011DB8[];       // "stream.dat"
extern char D_80011DC4[];       // "/carobj"
extern char D_80011DCC[];       // "/carlogo"
extern char D_80011DD8[];       // "/carwheel"
extern char D_80011DE4[];       // "/engine"
extern char D_80011DEC[];       // "/crsmap"
extern char D_80091620[];       // "-0123456789abcdefghijklmnopqrstuvwxyz"
extern char *D_800959C0[];      // the music's track names
extern unsigned char D_801EF650[];
extern unsigned char D_801E2CF0[];
extern IsoContext g_IsoCtx;
extern unsigned int g_VolLba;
extern unsigned short g_WheelBrandCodes[];
extern VolDirEntry g_VolDir[];
extern CarAsset g_CarAssets[];
extern short g_CarAssetCount;
extern unsigned int g_WheelCodes[];
extern short g_WheelCodeCount;
extern unsigned short g_EngineSoundIds[];
extern short g_EngineSoundCount;
extern unsigned int g_CourseMapIds[];
extern unsigned short g_CourseMapCount;
extern unsigned short g_CourseMapFirstFile;
extern CourseInfo g_CourseInfo;
extern SpuBank g_CommonSoundBank;

// The first file of a directory of GT2.VOL: past its ".." and its subdirectories.
#define VOL_DIR_FILES(entry, path)                  \
    entry = Vol_FindEntry(path, 0);                 \
    {                                               \
        VolDirEntry *dir = g_VolDir;                \
                                                    \
        entry = dir + entry->index;                 \
    }                                               \
    while ((entry->flags & 3) == 1) {               \
        entry++;                                    \
    }

// Reads a sector into ctx->sector, unless it is already there.
unsigned char *Iso_ReadSector(IsoContext *ctx, int lba)
{
    if (ctx->cachedLba != lba) {
        ctx->cachedLba = lba;
        Cd_ReadSync(ctx->sector, lba, 0x800);
    }
    return ctx->sector;
}

// Scans the volume descriptors from sector 16 for the primary one, and takes its root
// directory record as the current and the root directory. Always returns -1.
// One instruction: the game sets the -1 before the saved registers are restored, not after.
int Iso_Mount(IsoContext *ctx)
{
    int lba = 0x10;
    int bad = 2;

    ctx->cachedLba = -1;
    ctx->error = 0;
    while (1) {
        unsigned char *sector = Iso_ReadSector(ctx, lba);

        if (sector == 0) {
            break;
        }
        if (strncmp((char *)sector + 1, D_80011D98, 5) != 0) {
        fail:
            ctx->error = bad;
            break;
        }
        if (*sector == 0xff) {
            goto fail;
        }
        if (*sector == 1) {
            ctx->curDir = *(IsoDirRecord *)(sector + 0x9c);
            ctx->root = ctx->curDir;
            break;
        }
        lba++;
    }
    return -1;
}

// Resolves a '/'-separated path from the current directory. Returns the record, or 0.
// Registers only: the game keeps n in $s0 and path in $s1, and builds name + n in $v0.
IsoDirRecord *Iso_FindPath(IsoContext *ctx, char *path)
{
    char name[256];
    IsoDirRecord *rec = &ctx->curDir;
    char *slash;
    int n;
    int second;

    ctx->error = 0;
    if (*(signed char *)path == '/') {
        second = ((signed char *)path)[1];
        path++;
        if (second == 0) {
            return &ctx->root;
        }
    }
next:
    slash = strchr(path, '/');
    n = slash - path;
    if (slash == 0) {
        n = strlen(path);
    }
    strncpy(name, path, n);
    name[n] = 0;
    rec = Iso_FindInDir(ctx, rec->lba, name);
    if (slash == 0 || rec == 0) {
        return rec;
    }
    path = slash + 1;
    if (rec->flags & 2) {
        goto next;
    }
    ctx->error = 3;
    return 0;
}

// Looks for name + ";1" in the directory at dirLba. A record that crosses a sector is put
// back together in ctx->spillRecord. The next sector read is dirLba again, never the one
// after it, so a directory is only searched in its first sector.
// The game hoists the 3 into a saved register, and its blocks are in another order.
IsoDirRecord *Iso_FindInDir(IsoContext *ctx, int dirLba, char *name)
{
    char fullName[0x108];
    int copied;
    int rest;

    strcpy(fullName, name);
    strcat(fullName, D_80011DA0);
    copied = 0;
    rest = copied;
    ctx->error = 0;
    while (1) {
        unsigned char *sector = Iso_ReadSector(ctx, dirLba);
        unsigned char *rec;
        unsigned char *end;
        unsigned char *next;

        if (sector == 0) {
            return 0;
        }
        rec = sector + rest;
        end = sector + 0x800;
        if (rest != 0) {
            bcopy(sector, ctx->spillRecord + copied, rest);
            if (strncasecmp((char *)ctx->spillRecord + 0x21, fullName, ctx->spillRecord[0x20]) == 0) {
                return (IsoDirRecord *)ctx->spillRecord;
            }
        }
        while (1) {
            int length = rec[0];

            if (length == 0) {
                ctx->error = 3;
                return 0;
            }
            next = rec + length;
            copied = end - rec;
            if (end < next) {
                break;
            }
            if (strncasecmp(((IsoDirRecord *)rec)->name, fullName, ((IsoDirRecord *)rec)->nameLength) == 0) {
                return (IsoDirRecord *)rec;
            }
            rec = next;
        }
        rest = next - end;
        bcopy(rec, ctx->spillRecord, copied);
    }
}

// Makes path the current directory. Returns -1 if it isn't there.
int Iso_ChangeDir(IsoContext *ctx, char *path)
{
    IsoDirRecord *rec = Iso_FindPath(ctx, path);

    if (rec == 0) {
        return -1;
    }
    ctx->curDir = *rec;
    return 0;
}

IsoDirRecord *Iso_FindBootPath(char *path)
{
    return Iso_FindPath(&g_IsoCtx, path);
}

// Mounts the disc, goes to dir, and notes where GT2.VOL, MUSIC.DAT and STREAM.DAT start.
// STREAM.DAT is not on the NTSC-U discs.
// The game keeps rec in $a0, which moves its registers and the Music_Init branch.
void Disc_LocateDataFiles(char *dir)
{
    IsoContext *ctx = &g_IsoCtx;
    IsoDirRecord *rec;

    Iso_Mount(ctx);
    Iso_ChangeDir(ctx, dir);
    rec = Iso_FindPath(ctx, D_80011DA4);
    g_VolLba = rec->lba;
    rec = Iso_FindPath(ctx, D_80011DAC);
    if (rec != 0) {
        Music_Init(D_800959C0, rec->lba);
    } else {
        Music_Init(D_800959C0, 0);
    }
    rec = Iso_FindPath(&g_IsoCtx, D_80011DB8);
    if (rec != 0) {
        g_StreamLba = rec->lba;
    }
}

// A /carwheel file's name as a wheel code: the brand's index in g_WheelBrandCodes from
// name[0..1], a number from name[2..4], the size from name[6] ('4'-'6' to 1-3), name[7].
unsigned int WheelCode_FromName(char *name)
{
    signed char *s = (signed char *)name;
    unsigned int code = 0;
    int brand = 0;
    unsigned int entry = g_WheelBrandCodes[0];
    unsigned int key = s[0] | s[1] << 8;
    int number;
    int size;

    while (entry != 0) {
        if (entry == key) {
            break;
        }
        brand++;
        entry = g_WheelBrandCodes[brand];
    }
    number = (s[2] - '0') * 100;
    number += (s[3] - '0') * 10;
    number += s[4] - '0';
    key = brand;
    switch (s[6]) {
    case '4':
        size = 1;
        break;
    case '5':
        size = 2;
        break;
    case '6':
        size = 3;
        break;
    default:
        size = 0;
        break;
    }
    code = code << 4 | key;
    code = code << 12 | number;
    code = code << 3 | size;
    return code << 13 | s[7];
}

// The decimal value of the digits s starts with.
int Str_ParseUInt(char *s)
{
    register int value asm("a1") = 0;
    int c;
    int tens;

    while ((unsigned int)((c = *(signed char *)s++) - '0') < 10) {
        tens = value * 10 - '0';
        value = tens + c;
    }
    return value;
}

// A table from ' ' on, of each character's index in "-0123456789abc...xyz", upper case as
// lower case.
// Registers: the game keeps the masked character apart from the loaded one, in $a0.
void Str_InitBase37Table(void)
{
    int i = 0;
    char *s = D_80091620;
    unsigned char c = *s;

    while (c != 0) {
        D_801EF650[c - 0x20] = i;
        if ((unsigned int)(c - 'a') < 26) {
            D_801EF650[c - 0x40] = i;
        }
        s++;
        c = *s;
        i++;
    }
}

void emptyFunc(void) {
}

// Fills g_CarAssets from /carobj, where a car has 4 files.
// The tail differs: the game stores the count first and shifts each index by itself.
void Vol_IndexCarObjects(void)
{
    VolDirEntry *entry;
    CarAsset *assets;
    int count;
    int n;
    unsigned short file;

    VOL_DIR_FILES(entry, D_80011DC4);
    count = 0;
    assets = g_CarAssets;
    while (1) {
        assets[count].carCode = CarCode_Encode((signed char *)entry->name);
        file = entry->index;
        assets[count].logoFile = 0;
        assets[count].file = file;
        count++;
        if (entry[3].flags & 0x80) {
            break;
        }
        entry += 4;
    }
    g_CarAssetCount = count;
    n = g_CarAssetCount;
    assets[n].file = assets[n - 1].file + 4;
}

// Sets the cars' logoFile from /carlogo, which has two files for each maker; the first
// file for the cars without one.
void Vol_IndexCarLogos(void)
{
    VolDirEntry *entry;
    VolDirEntry *first;
    int count;
    int i;
    int fallback;

    VOL_DIR_FILES(entry, D_80011DCC);
    first = entry;
    while (1) {
        unsigned int carCode = CarCode_Encode((signed char *)entry->name);
        int c = (unsigned char)entry->name[5];

        if (c < 0x72) {
            if (c >= 0x70) {
                goto next;
            }
        }
        {
            CarAsset *asset = CarAsset_Find(carCode);

            if (asset != 0) {
                asset->logoFile = entry->index;
            }
        }
    next:
        if (entry[1].flags & 0x80) {
            break;
        }
        entry += 2;
    }
    count = g_CarAssetCount;
    fallback = first->index;
    i = 0;
    if (count > 0) {
        int n = count;

        do {
            if (g_CarAssets[i].logoFile == 0) {
                g_CarAssets[i].logoFile = fallback;
            }
            i++;
        } while (i < n);
    }
}

// Fills g_WheelCodes from /carwheel's file names.
void Vol_IndexCarWheels(void)
{
    VolDirEntry *entry;
    char *name;
    unsigned int *codes;
    int count;

    VOL_DIR_FILES(entry, D_80011DD8);
    count = 0;
    name = entry->name;
    codes = g_WheelCodes;
    while (1) {
        *codes = WheelCode_FromName(name);
        codes++;
        count++;
        if (name[-1] & 0x80) {
            break;
        }
        name += sizeof(VolDirEntry);
    }
    g_WheelCodeCount = count;
}

// Fills g_EngineSoundIds from /engine's numbered names; each sound has 9 files.
void Vol_IndexEngineSounds(void)
{
    VolDirEntry *entry;
    char *name;
    unsigned short *ids;
    short count;

    VOL_DIR_FILES(entry, D_80011DE4);
    count = 0;
    if ((unsigned char)entry->name[0] < 0x3a) {
        name = entry->name;
        ids = g_EngineSoundIds;
        do {
            *ids = Str_ParseUInt(name);
            ids++;
            count++;
            if (name[0xff] & 0x80) {    // the flags of the sound's last entry
                break;
            }
            name += 9 * sizeof(VolDirEntry);
        } while ((unsigned char)name[0] < 0x3a);
    }
    g_EngineSoundCount = count;
}

void Vol_IndexCarData(void)
{
    Str_InitBase37Table();
    emptyFunc();
    Vol_IndexCarObjects();
    Vol_IndexCarLogos();
    Vol_IndexCarWheels();
    Vol_IndexEngineSounds();
}

char *strchr(char *s, int c)
{
    if (*(signed char *)s == 0) {
        return 0;
    }
    if (((c ^ *(signed char *)s) & 0xff) == 0) {
        return s;
    }
    return strchr(s + 1, c);
}

// Fills g_CourseMapIds with the hash of each /crsmap file's name, without its extension.
void Vol_IndexCourseMaps(void)
{
    VolDirEntry *entry;
    char *fileName;
    unsigned int *ids;
    short count;
    int dirFlag;

    VOL_DIR_FILES(entry, D_80011DEC);
    count = 0;
    dirFlag = 1;
    ids = g_CourseMapIds;
    fileName = entry->name;
    g_CourseMapFirstFile = entry->index;
    while ((fileName[-1] & 3) != dirFlag) {
        char name[32];
        char *dot;

        strcpy(name, fileName);
        dot = strchr(name, '.');
        if (dot != 0) {
            *dot = 0;
        }
        *ids = Str_Hash(name);
        fileName += sizeof(VolDirEntry);
        ids++;
        count++;
    }
    g_CourseMapCount = count;
}

// Reads the course table and turns its name offsets into pointers.
void Vol_LoadCourseInfo(void)
{
    unsigned int i;

    Vol_IndexCourseMaps();
    CourseInfo *base = &g_CourseInfo;
    CourseInfo *info = base;

    VolId_ReadFile(VOLID_CRSINFO, base);
    for (i = 0; i < info->count; i++) {
        CourseEntry *entry = &info->entries[i];

        entry->name += (int)base;
    }
}

void Sound_LoadCommonBank(void)
{
    SpuBank *bank = &g_CommonSoundBank;

    SpuBank_Init(bank, (SeqBank *)D_801E2CF0, 0x200);
    SpuBank_Load(bank, 0xf7, 0, 1);
}

}
