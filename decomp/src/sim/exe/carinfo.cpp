// Resident code 80060924-80061010 (carinfo): the car info (.carinfoa) lookups, the
// wheel, engine sound, course and course map tables. CarInfoA_GetData doesn't match
// yet and is the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int CarInfoA_FindIndex(unsigned int carId);
int CarInfoA_GetData(unsigned int carId, char **outData);
unsigned int CarInfoA_GetHiddenRegions(unsigned int carId);
int CarInfoA_GetColors(unsigned int carId, unsigned short **outRgb, char **outCodes);
int Str_FindCharNoCase(int ch, char *str, int len);
void VolId_ReadFile(VolBootId id, void *dst);
void Vol_ReadFile(int file, void *dst);
int strcmp(const char *a, const char *b);
extern CarInfoA *g_CurrentCarInfoA;
extern GameState g_GameState;

struct CourseEntry {
    char *name;
    unsigned int hash;
    unsigned short flags;
    char unkA[0xe];
};

struct CourseInfo {
    char magic[6];
    unsigned short count;
    CourseEntry entries[1];
};

extern unsigned char D_801EF630[];     // car code letter -> 6-bit value
extern CarAsset g_CarAssets[];
extern CourseInfo g_CourseInfo;
extern unsigned int g_WheelCodes[];
extern short g_WheelCodeCount;
extern unsigned short D_801E2FC6;      // first wheel file
extern unsigned short g_EngineSoundIds[];
extern short g_EngineSoundCount;
extern int g_DemoRaceCount;
extern unsigned short D_801E30B8;      // file before the first demo race setup
extern unsigned int g_CourseMapIds[];
extern unsigned short g_CourseMapCount;
extern unsigned short g_CourseMapFirstFile;

// Packs a five-letter car code into 6 bits a letter, the first letter highest.
unsigned int CarCode_Encode(signed char *code)
{
    unsigned char *t = D_801EF630;
    unsigned char *a = &t[code[0]], *b = &t[code[1]], *c = &t[code[2]], *d = &t[code[3]], *e = &t[code[4]];

    return *e | *d << 6 | *c << 12 | *b << 18 | *a << 24;
}

unsigned int FUN_8006098c(unsigned int index)
{
    int i;

    for (i = 0; i < 0x460; i++) {
        if (g_CarAssets[i].file <= index && index < g_CarAssets[i + 1].file) {
            return g_CarAssets[i].carCode;
        }
    }
    return g_CarAssets[0].carCode;
}

void LoadCarInfoA(CarInfoA *buffer)
{
    g_CurrentCarInfoA = buffer;
    VolId_ReadFile(VOLID_CARINFOA, buffer);
}

// Binary search of the entries (sorted by carId); 0 when not found.
int CarInfoA_FindIndex(unsigned int carId)
{
    CarInfoA *info = g_CurrentCarInfoA;
    int lo = 0;
    int hi = info->count;
    int mid;
    unsigned int id;

    while (1) {
        mid = (lo + hi) >> 1;
        id = info->entries[mid].carId;
        if (carId == id) {
            return mid;
        }
        if (lo >= hi) {
            return 0;
        }
        if (id < carId) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
}

// The entry's data (colours, codes, name); returns the colour count.
#ifdef NON_MATCHING
// The game schedules the dataOffset mask (and) before the colour count's andi.
int CarInfoA_GetData(unsigned int carId, char **outData)
{
    CarInfoA *info = g_CurrentCarInfoA;
    int i = CarInfoA_FindIndex(carId);
    unsigned int off = info->entries[i].dataOffset;
    int n = info->entries[i].colorCountMinus1;

    *outData = (char *)info + off;
    return n + 1;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carinfo", CarInfoA_GetData);
#endif

char *CarInfoA_GetName(unsigned int carId)
{
    char *data;
    int n = CarInfoA_GetData(carId, &data);

    return data != 0 ? data + n * 3 + 1 : 0;
}

unsigned int CarInfoA_GetHiddenRegions(unsigned int carId)
{
    CarInfoA *pCVar1;
    int iVar2;

    pCVar1 = g_CurrentCarInfoA;
    iVar2 = CarInfoA_FindIndex(carId);
    return pCVar1->entries[iVar2].hiddenRegions;
}

// Whether the car isn't hidden in the game's language.
int CarInfoA_IsAvailable(unsigned int carId)
{
    unsigned int hidden = CarInfoA_GetHiddenRegions(carId);

    switch (*(unsigned char *)&g_GameState) {
    case 0:
        return (hidden ^ 1) & 1;
    case 1:
        return (hidden >> 1 ^ 1) & 1;
    case 2:
        return (hidden >> 3 ^ 1) & 1;
    default:
        return (hidden >> 2 ^ 1) & 1;
    }
}

int CarInfoA_GetColors(unsigned int carId, unsigned short **outRgb, char **outCodes)
{
    char *data;
    int n = CarInfoA_GetData(carId, &data);

    unsigned short *rgb = (unsigned short *)data;

    *outRgb = rgb;
    *outCodes = (char *)(rgb + n);
    return n;
}

int CarInfoA_GetFields(unsigned int carId, unsigned short **outRgb, char **outCodes, char **outName)
{
    char *data;
    int n = CarInfoA_GetData(carId, &data);
    unsigned short *rgb = (unsigned short *)data;
    char *p;

    *outRgb = rgb;
    p = (char *)(rgb + n);
    *outCodes = p;
    *outName = p + n;
    return n;
}

// The index of ch in str (case-insensitive); 0 when not found.
int Str_FindCharNoCase(int ch, char *str, int len)
{
    int i;
    int c;

    if ((unsigned int)(ch - 'A') < 26) {
        ch += 0x20;
    }
    i = 0;
loop:
    if (i < len) {
        c = ((signed char *)str)[i];
        if ((unsigned int)(c - 'A') < 26) {
            c += 0x20;
        }
        if (ch == c) {
            return i;
        }
        i++;
        goto loop;
    }
    return 0;
}

int CarInfoA_FindColorIndex(unsigned int carId, int colorCode)
{
    int iVar1;
    unsigned short *puStack_10;
    char *local_c;

    iVar1 = CarInfoA_GetColors(carId, &puStack_10, &local_c);
    iVar1 = Str_FindCharNoCase(colorCode, local_c, iVar1);
    return iVar1;
}

unsigned short CarInfoA_GetColorRgb(unsigned int carId, int colorCode)
{
    int iVar1;
    unsigned short *local_10;
    char *local_c;

    iVar1 = CarInfoA_GetColors(carId, &local_10, &local_c);
    iVar1 = Str_FindCharNoCase(colorCode, local_c, iVar1);
    return local_10[iVar1];
}

// Binary search of the wheel codes; the first wheel file when not found.
int Wheel_GetFileId(unsigned int code)
{
    int lo = 0;
    int hi = g_WheelCodeCount;
    int base = D_801E2FC6;
    int mid;
    unsigned int c;

    while (1) {
        mid = (lo + hi) >> 1;
        c = g_WheelCodes[mid];
        if (code == c) {
            return base + mid;
        }
        if (lo >= hi) {
            return base;
        }
        if (c < code) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
}

int EngineSound_FindIndex(unsigned int id)
{
    int lo = 0;
    int hi = g_EngineSoundCount;
    int mid;
    unsigned int s;

    while (1) {
        mid = (lo + hi) >> 1;
        s = g_EngineSoundIds[mid];
        if (id == s) {
            return mid;
        }
        if (lo >= hi) {
            return 0;
        }
        if (s < id) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
}

void DemoRace_LoadSetup(int n)
{
    GameState *gs = &g_GameState;

    Vol_ReadFile(D_801E30B8 + n % g_DemoRaceCount + 1, (char *)gs + 0xbf7c);
}

// The code reads the course table through a local pointer and this accessor; folding
// the table's address with the entries' offset doesn't match.
static inline CourseEntry *CourseAt(CourseInfo *c, int i)
{
    return &c->entries[i];
}

CourseEntry *Course_Get(int index)
{
    CourseInfo *c = &g_CourseInfo;

    return CourseAt(c, index);
}

int Course_FindByHash(unsigned int hash)
{
    unsigned int i;
    CourseInfo *c = &g_CourseInfo;

    for (i = 0; i < g_CourseInfo.count; i++) {
        if (CourseAt(c, i)->hash == hash) {
            return i;
        }
    }
    return 0;
}

static inline int Course_Matches(CourseEntry *e, char *name, int kind)
{
    return (e->flags & 0x18) == kind && strcmp(name, e->name) == 0;
}

// The hash of the course named name whose flags & 0x18 are kind; 0 when none.
unsigned int FUN_80060efc(char *name, int kind)
{
    unsigned int i;
    CourseInfo *c = &g_CourseInfo;

    for (i = 0; i < c->count; i++) {
        if (Course_Matches(CourseAt(c, i), name, kind)) {
            return CourseAt(c, i)->hash;
        }
    }
    return 0;
}

int CourseMap_GetFileId(unsigned int id)
{
    unsigned int *ids = g_CourseMapIds;
    unsigned int *end = ids + g_CourseMapCount;
    unsigned int *p;

    for (p = ids; p != end; p++) {
        if (*p == id) {
            return g_CourseMapFirstFile + (p - ids);
        }
    }
    return g_CourseMapFirstFile;
}
}
