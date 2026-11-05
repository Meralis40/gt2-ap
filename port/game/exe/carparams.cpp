// Resident code 80075fac-80077d5c (carparams). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "common.h"

extern "C" {
extern CarParamsPartDesc g_CarParamsPartDescs[];
extern unsigned char D_800AF230;
extern unsigned char D_80092888;
extern unsigned char D_80092890;
extern unsigned char D_80092898;
extern unsigned char D_800928A0;
extern unsigned char D_800928CC;
extern unsigned char D_800928D8;
extern unsigned char D_800928E4;
extern unsigned char D_80092900;
extern unsigned char D_8009294C;
extern unsigned char D_80092BB4;
extern unsigned char D_80092E24;
extern unsigned char D_80092968;
extern unsigned char D_80092970;
extern unsigned char D_80092978;
extern unsigned char D_80092980;
extern unsigned char D_800929AC;
extern unsigned char D_800929B8;
extern unsigned char D_800929C4;
extern unsigned char D_800929E0;
extern unsigned char D_80092A2C;
extern unsigned char D_80092820;
extern unsigned char D_80092D6C;
extern unsigned char D_80092D7C;
extern unsigned char D_80092D8C;
extern unsigned char D_80092DBC;
extern unsigned char D_80092DEC;
extern unsigned char D_80092DF4;
extern unsigned char D_80092DFC;
extern unsigned char D_80092C24;
extern unsigned char D_80092E08;
extern unsigned char D_80092E50;
extern unsigned int D_801C94E4;
extern unsigned char g_ArcadeDataEntrySizes;
extern unsigned char g_CarSpecFromSetupDesc;
extern CarSpecPartLink g_CarSpecPartLinks[];
extern unsigned int g_DataLanguage;
extern unsigned int g_GameDataBase;
extern int g_GameDataMode;
extern GameState g_GameState;
extern unsigned int g_GtModeData;
extern unsigned char g_GtModeDataEntrySizes;
extern void *g_GtModeRaceData;
extern unsigned char g_GtModeRaceEntrySizes;
extern LangCarParamFiles g_LangCarParamFiles[7];
extern unsigned char g_LicenseDataEntrySizes;
extern void *g_RaceGameData;
extern void *g_UniStrDb;

short CarParams_GetTyreRadius(CarParams *params, int rear, short *radius, short *width, short *sidewall)
{
    int rim;
    int w;
    int aspect;
    short tw;
    short s;

    if (rear == 0) {
        w = params->tyreB[0];
        aspect = params->tyreC[0];
        rim = params->tyreA[0];
    } else {
        w = params->tyreB[1];
        aspect = params->tyreC[1];
        rim = params->tyreA[1];
    }
    tw = (w * 5 * 0x2000 + 0x5000) / 1000;
    *width = tw;
    s = tw * aspect / 20;
    *sidewall = s;
    s = s + rim * 127 * 0x2000 / 20000;
    *radius = s;
    return s;
}

// The game keeps layout in $s2 and front in $s1, and lays out the course flag's
// decision tree with == 3 on the fall-through side.
unsigned char CarParams_GetDriveLayout(CarParams *params, unsigned char *layout, unsigned char *flag, short *front, short *rear)
{
    int mode;

    *front = 0;
    *rear = 0;
    switch (params->drivetrain) {
    case 0:
    case 3:
    case 4:
        *layout = 0;
        *flag = 1;
        break;
    case 1:
        *layout = 1;
        *flag = 0;
        break;
    case 2:
        mode = ((unsigned char *)params)[0x51];
        *front = (params->awdFrontPct << 12) / 100;
        *flag = 1;
        if (*(unsigned short *)((int)Course_Get(D_800AF230) + 8) & 4) {
            switch (mode) {
            case 2:
                break;
            case 0:
            case 1:
            case 3:
                *front = 0x800;
                break;
            }
            mode = 4;
        }
        switch (mode) {
        case 0:
            *layout = 2;
            *flag = 0;
            break;
        case 1:
            *layout = 3;
            break;
        case 2:
            *layout = 4;
            if (params->awdFrontPct >= 50) {
                *front = 0x1000;
            }
            *rear = 0xc40;
            break;
        case 3:
            *layout = 5;
            *front = Fixed_Div(*front, 0x2c74, 0);
            *rear = 0x16f8;
            break;
        case 4:
            *layout = 6;
            break;
        }
        break;
    }
    return *layout;
}

void CarSetup_ApplyPartEntry(int table, void *entry, CarSetup *setup)
{
    unsigned int *desc;

    switch (table) {
    case 13:
        desc = (unsigned int *)&D_80092888;
        break;
    case 27:
        desc = (unsigned int *)&D_80092890;
        break;
    case 28:
        desc = (unsigned int *)&D_80092898;
        break;
    case 17:
        desc = (unsigned int *)&D_800928A0;
        break;
    case 5:
        desc = (unsigned int *)&D_800928D8;
        break;
    case 12:
        desc = (unsigned int *)&D_800928E4;
        break;
    case 18:
        desc = (unsigned int *)&D_80092900;
        setup->setting_5e[0] = 0x80;
        setup->setting_5e[1] = 0x80;
        break;
    case 1:
        desc = (unsigned int *)&D_800928CC;
        break;
    case 21:
        desc = (unsigned int *)&D_8009294C;
        break;
    default:
        return;
    }
    CarSetup_ApplyDescriptors(desc, entry, setup);
}

void CarSpec_ApplyPartEntry(int table, void *entry, CarSpec *spec)
{
    unsigned int *desc;

    switch (table) {
    case 13:
        desc = (unsigned int *)&D_80092968;
        break;
    case 27:
        desc = (unsigned int *)&D_80092970;
        break;
    case 28:
        desc = (unsigned int *)&D_80092978;
        break;
    case 17:
        desc = (unsigned int *)&D_80092980;
        break;
    case 5:
        desc = (unsigned int *)&D_800929B8;
        break;
    case 12:
        desc = (unsigned int *)&D_800929C4;
        break;
    case 18:
        desc = (unsigned int *)&D_800929E0;
        *(unsigned char *)((int)spec + 0x9e) = 0x80;
        *(unsigned char *)((int)spec + 0x9f) = 0x80;
        break;
    case 1:
        desc = (unsigned int *)&D_800929AC;
        break;
    case 21:
        desc = (unsigned int *)&D_80092A2C;
        break;
    default:
        return;
    }
    CarSetup_ApplyDescriptors(desc, entry, (CarSetup *)spec);
}

void CarSpec_CopySetupFields(CarSpec *spec, CarSetup *setup)
{
    CarSetup_ApplyDescriptors((unsigned int *)&g_CarSpecFromSetupDesc, setup, (CarSetup *)spec);
}

// The game loads the three link fields and steps the pointer before the call, and
// computes the CarSpec address in the call's delay slot; no loop shape tried gives that.
void CarSpec_Resolve(CarSpec *spec, CarSetup *setup)
{
    CarSpecPartLink *link;
    unsigned short *tyre;

    CarSpec_Clear(spec);
    CarSetup_ApplyDescriptors((unsigned int *)&g_CarSpecFromSetupDesc, setup, (CarSetup *)spec);
    for (link = g_CarSpecPartLinks; link->specOffset != -1; link++) {
        *(void **)((int)spec + link->specOffset) =
            GameData_GetEntry(link->table, *(unsigned short *)((int)setup + link->setupOffset));
    }
    tyre = (unsigned short *)GameData_GetEntry(0x16, (unsigned short)setup->idx_t16_front);
    spec->frontTyre18 = GameData_GetEntry(0x18, tyre[5]);
    spec->frontTyre19 = GameData_GetEntry(0x19, tyre[6]);
    spec->frontTyre1a = GameData_GetEntry(0x1a, tyre[7]);
    tyre = (unsigned short *)GameData_GetEntry(0x17, (unsigned short)setup->idx_t16_rear);
    spec->rearTyre18 = GameData_GetEntry(0x18, tyre[3]);
    spec->rearTyre19 = GameData_GetEntry(0x19, tyre[4]);
}

int Part_LookupRacingModStage(unsigned int carCode, char stage, int *outPrice)
{
    int index;
    void *pvVar1;

    index = GameData_FindPart(5, stage, 0xe, carCode);
    if (outPrice != 0) {
        if (index == -1) {
            *outPrice = 0;
        }
        else {
            pvVar1 = GameData_GetEntry(5, index);
            *outPrice = *(int *)((int)pvVar1 + 4);
        }
    }
    return index;
}

int Part_Lookup(int part, unsigned int carCode, int *outPrice)
{
    int table;
    int offset;
    char stage;
    int index;

    table = -1;
    offset = -1;
    stage = 1;
    switch (part) {
    case 1:
        table = 0x0;
        offset = 8;
        break;
    case 2:
        table = 0x1;
        offset = 8;
        break;
    case 32:
        table = 0x7;
        offset = 8;
        break;
    case 9:
        table = 0x8;
        offset = 8;
        break;
    case 6:
        table = 0xa;
        offset = 8;
        break;
    case 33:
        table = 0x10;
        offset = 8;
        break;
    case 0:
        table = 0x1b;
        offset = 8;
        break;
    case 38:
        table = 0x1c;
        offset = 8;
        break;
    case 34:
        table = 0x5;
        stage = 1;
        offset = 0xe;
        break;
    case 35:
    case 36:
    case 37:
        table = 0x12;
        stage = part - 0x22;
        offset = 8;
        break;
    case 18:
    case 19:
    case 20:
        table = 0x4;
        stage = part - 0x11;
        offset = 0xb;
        break;
    case 13:
    case 14:
    case 15:
        table = 0x11;
        stage = part - 0xc;
        offset = 8;
        break;
    case 29:
    case 30:
    case 31:
        table = 0xb;
        stage = part - 0x1c;
        offset = 8;
        break;
    case 46:
    case 47:
    case 48:
    case 49:
        table = 0xc;
        stage = part - 0x2d;
        offset = 8;
        break;
    case 7:
    case 8:
        table = 0x9;
        stage = part - 0x6;
        offset = 8;
        break;
    case 26:
    case 27:
    case 28:
        table = 0x14;
        stage = part - 0x19;
        offset = 8;
        break;
    case 3:
    case 4:
    case 5:
        table = 0xf;
        stage = part - 0x2;
        offset = 8;
        break;
    case 16:
    case 17:
        table = 0x13;
        stage = part - 0xf;
        offset = 8;
        break;
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
        table = 0x15;
        stage = part - 0x14;
        offset = 8;
        break;
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
        table = 0x16;
        stage = part - 0x26;
        offset = 8;
        break;
    case 10:
    case 11:
    case 12:
        table = 0xe;
        stage = part - 0x9;
        offset = 8;
        break;
    }
    index = GameData_FindPart(table, stage, offset, carCode);
    if (index == -1) {
        index = GameData_FindPart(table, stage, offset, CarCode_Encode("00000"));
    }
    if (outPrice != 0) {
        if (index == -1) {
            *outPrice = 0;
        } else {
            *outPrice = *(int *)((int)GameData_GetEntry(table, index) + 4);
        }
    }
    return index;
}

int GameData_FindPart(int table, char stage, int stageOffset, unsigned int carCode)
{
    void *data;
    unsigned char *e;
    int i;
    int k;

    data = (void *)g_GtModeData;
    i = GameData_FindFirstForCar(table, carCode);
    if (i != -1) {
        for (k = 4;; i++) {
            e = (unsigned char *)GameData_GetEntryIn(data, table, i);
            if (*(unsigned int *)e != carCode) {
                break;
            }
            if (e[stageOffset] == stage && e[k] != 0) {
                return i;
            }
        }
    }
    return -1;
}

int GameData_FindFirstForCar(int table, unsigned int carCode)
{
    void *data;
    int i;

    data = (void *)g_GtModeData;
    i = GameData_BinarySearch(data, table, carCode);
    if (i == -1) {
        return i;
    }
    while (i > 0) {
        if (carCode != *(unsigned int *)GameData_GetEntryIn(data, table, i - 1)) {
            return i;
        }
        i--;
    }
    return i;
}

void *GameData_GetCarEntry(unsigned int carCode)
{
    void *data;
    unsigned int idx;

    idx = carCode & 0x3ffffff;
    data = 0;
    switch ((unsigned int)g_GameDataMode) {
    case 2:
        return GameData_GetEntryIn(g_GtModeRaceData, 1, idx - 1);
    case 1:
    case 3:
        data = g_RaceGameData;
    }
    return GameData_GetEntryIn(data, 0x1f, idx - 1);
}

int Car_GetDefaultSetup(unsigned int carId, CarSetup *setup)
{
    GameDataTable *table;
    int found;
    void *entry;
    char *data;

    table = 0;
    found = 1;
    switch ((unsigned int)g_GameDataMode) {
    case 2:
        data = (char *)g_GtModeData;
        table = (GameDataTable *)(data + 0xf8);
        break;
    case 1:
    case 3:
        data = (char *)g_RaceGameData;
        table = (GameDataTable *)(data + 0x108);
        break;
    }
    entry = GameDataTable_FindById(table, carId);
    if (entry == 0) {
        entry = GameDataTable_GetEntry(table, 0);
        found = 0;
    }
    Car_SetupFromEntry(entry, setup);
    return found;
}

void CarSetup_Clear(CarSetup *setup)
{
    memset(setup, 0, 0x84);
    setup->manufacturerId = 0xff;
}

void CarSpec_Clear(CarSpec *spec)
{
    memset(spec, 0, 0xc4);
    spec->manufacturerId = 0xff;
}

int UniStrDb_Load(void *buf, int size)
{
    g_DataLanguage = (unsigned int)g_GameState.language;
    VolId_ReadFileAsync((unsigned int)g_LangCarParamFiles[g_DataLanguage].unistrdb, buf);
    g_UniStrDb = buf;
    return *(int *)buf;
}

// The game keeps the entry sizes pointer in $s2 and the total in $s1; this swaps them.
int GameData_Relocate(void *data, unsigned int *entrySizes)
{
    int n;
    int i;
    int total;

    i = 0;
    total = 0;
    n = *(unsigned short *)((char *)data + 6) >> 1;
    for (; i < n; i++) {
        total += GameData_RelocateTable((char *)data, i, entrySizes[i]);
    }
    return total + n * 8;
}

// The game reads the header half-word before its stores and recomputes the second
// table's address; the order of the loads and stores differs.
unsigned int GameData_RelocateTable(char *data, int table, unsigned int entrySize)
{
    unsigned int *t;
    unsigned int bytes;
    unsigned int *s;
    unsigned int extra;

    t = (unsigned int *)(data + table * 8);
    bytes = t[3];
    ((unsigned short *)t)[6] = entrySize;
    t[2] += (unsigned int)data;
    ((unsigned short *)t)[7] = bytes / entrySize;
    table += *(unsigned short *)(data + 6) >> 1;
    s = (unsigned int *)(data + table * 8 + 8);
    extra = 0;
    if (*s != 0) {
        *s += (unsigned int)data;
        extra = *(unsigned int *)(data + table * 8 + 0xc);
    }
    return bytes + extra;
}

char *StrPool_Get(char *pool, int index)
{
    int i;

    pool += 2;
    for (i = 0; i < index; i++) {
        pool += *(signed char *)pool + 2;
    }
    return pool + 1;
}

short *UniStr_Get(int index)
{
    short *psVar1;

    psVar1 = UniStrDb_Get(g_UniStrDb, index);
    return psVar1;
}

short *UniStrDb_Get(void *db, int index)
{
    short *s;
    int i;

    s = (short *)((char *)db + 10);
    for (i = 0; i < index; i++) {
        s = (short *)((char *)s + (*s * 2 + 4));
    }
    return s + 1;
}

int StrPool_Find(unsigned short *pool, char *str)
{
    unsigned char *s;
    int i;
    int n;

    s = (unsigned char *)pool + 2;
    i = 0;
    if (*pool != 0) {
        n = *pool;
        do {
            if (strcmp(str, (char *)s + 1) == 0) {
                return i;
            }
            i++;
            s += *s + 2;
        } while (i < n);
    }
    return -1;
}

void *GameData_LoadGtModeRace(void *buffer, int size)
{
    g_DataLanguage = (unsigned int)g_GameState.language;
    VolId_ReadFileAsync((unsigned int)g_LangCarParamFiles[g_DataLanguage].gtmodeRace, buffer);
    g_GtModeRaceData = buffer;
    GameData_Relocate(buffer, (unsigned int *)&g_GtModeRaceEntrySizes);
    D_801C94E4 = (unsigned int)*(unsigned short *)((int)buffer + 4);
    return buffer;
}

void *GameData_LoadGtMode(void *buffer, int size)
{
    g_GameDataMode = 2;
    g_DataLanguage = (unsigned int)g_GameState.language;
    VolId_ReadFileAsync((unsigned int)g_LangCarParamFiles[g_DataLanguage].gtmodeData, buffer);
    g_GtModeData = buffer;
    g_GameDataBase = buffer;
    GameData_Relocate(buffer, (unsigned int *)&g_GtModeDataEntrySizes);
    D_801C94E4 = (unsigned int)*(unsigned short *)((int)buffer + 4);
    return buffer;
}

void *GameData_LoadArcade(void *buffer, int size)
{
    g_GameDataMode = 1;
    g_DataLanguage = (unsigned int)g_GameState.language;
    VolId_ReadFileAsync((unsigned int)g_LangCarParamFiles[g_DataLanguage].arcadeData, buffer);
    g_RaceGameData = buffer;
    g_GameDataBase = buffer;
    GameData_Relocate(buffer, (unsigned int *)&g_ArcadeDataEntrySizes);
    return buffer;
}

void *GameData_LoadLicense(void *buffer, int size)
{
    g_GameDataMode = 3;
    g_DataLanguage = (unsigned int)g_GameState.language;
    VolId_ReadFileAsync((unsigned int)g_LangCarParamFiles[g_DataLanguage].licenseData, buffer);
    g_RaceGameData = buffer;
    g_GameDataBase = buffer;
    GameData_Relocate(buffer, (unsigned int *)&g_LicenseDataEntrySizes);
    return buffer;
}

void *GameData_GetGtMode(void)
{
    return g_GtModeData;
}

void *FUN_80076f1c(void)
{
    return g_RaceGameData;
}

void *GameData_GetEntry(int table, int index)
{
    void *pvVar1;

    pvVar1 = GameData_GetEntryIn(g_GameDataBase, table, index);
    return pvVar1;
}

void CarSetup_InitFromCarEntry(void *carEntry, CarSetup *setup)
{
    CarSetup_Clear(setup);
    CarSetup_FromCarEntry(carEntry, setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092C24, carEntry, setup);
    setup->flags_7a = setup->flags_7a | 1;
}

void CarSetup_FromCarEntry(void *carEntry, CarSetup *setup)
{
    unsigned int *wheel;

    CarSetup_Clear(setup);
    if (g_GameDataMode == 2) {
        setup->flags_7a = setup->flags_7a | 0x40;
        setup->manufacturerId = *(unsigned char *)((int)carEntry + 0x3a);
    }
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092BB4, carEntry, setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_800928A0, GameData_GetEntry(0x11, (unsigned short)setup->idx_t11), setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_800928D8, GameData_GetEntry(5, (unsigned short)setup->idx_t05), setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_800928E4, GameData_GetEntry(0xc, (unsigned short)setup->idx_t0c), setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092900, GameData_GetEntry(0x12, (unsigned short)setup->idx_t12), setup);
    setup->setting_5e[0] = 0x80;
    setup->setting_5e[1] = 0x80;
    CarSetup_ApplyDescriptors((unsigned int *)&D_800928CC, GameData_GetEntry(1, (unsigned short)setup->idx_t01), setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_8009294C, GameData_GetEntry(0x15, (unsigned short)setup->idx_t15), setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092888, GameData_GetEntry(0xd, (unsigned short)setup->idx_t0d), setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092890, GameData_GetEntry(0x1b, (unsigned short)setup->idx_t1b), setup);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092898, GameData_GetEntry(0x1c, (unsigned short)setup->idx_t1c), setup);
    wheel = (unsigned int *)GameData_GetEntry(0x1d, (unsigned short)setup->idx_t1d);
    setup->wheel = *wheel | (((unsigned char *)wheel)[7] & 0x1f) << 8;
}

void CarSetup_CopySoundFromSpec(CarSetup *setup, CarSpec *spec)
{
    setup->engineSound = spec->outB6;
    setup->engineSoundVariant = spec->outB8;
    setup->flags_7a = spec->flags;
}

int CarParams_BuildFromSetup(CarSetup *setup, void *params)
{
    CarSpec spec;
    int ret;

    CarSpec_Resolve(&spec, setup);
    ret = CarParams_Build(&spec, params);
    setup->engineSound = spec.outB6;
    setup->engineSoundVariant = spec.outB8;
    setup->flags_7a = spec.flags;
    return ret;
}

// The game keeps the entry in $s0 and the factor percentage in $s1 (swapped here), and
// allocates the constants of the final stores differently.
#define P8(off) *(unsigned char *)((int)params + (off))
#define P16(off) *(unsigned short *)((int)params + (off))

int CarParams_Build(CarSpec *spec, void *params)
{
    CarParamsPartDesc *d;
    int factor;
    char *s;

    d = g_CarParamsPartDescs;
    factor = 1000;
    s = (char *)spec;
    memset(params, 0, 0x1c0);
    for (; d->desc != 0; d++) {
        unsigned char *e = *(unsigned char **)(s + d->specOffset);
        int pct;

        CarSetup_ApplyDescriptors(d->desc, e, (CarSetup *)params);
        pct = 0;
        switch (d->table) {
        case 0x10:
            CarSetup_ApplyDescriptors((unsigned int *)&D_80092DFC, spec->driveshaft, (CarSetup *)params);
            break;
        case 0xe:
            CarSetup_ApplyDescriptors((unsigned int *)&D_80092DF4, spec->flywheel, (CarSetup *)params);
            break;
        case 0xb:
            CarSetup_ApplyDescriptors((unsigned int *)&D_80092DEC, spec->naTune, (CarSetup *)params);
        case 8:
            pct = e[0xb];
            break;
        case 0xc:
            pct = e[0x12];
            break;
        case 7:
        case 9:
        case 10:
        case 0x13:
        case 0x14:
            pct = e[9];
            break;
        }
        if (pct == 0xff) {
            pct = 0x207;
        } else {
            pct += 100;
        }
        factor = factor * pct / 100;
    }
    {
        int w;

        w = P8(0x57);
        P8(0x57) = w * *(unsigned short *)((int)spec->weightReduction + 8) / 1000;
        w = *(short *)((int)params + 0x5a);
        *(short *)((int)params + 0x5a) = w * *(unsigned char *)((int)spec->racingMod + 0xc) / 100;
    }
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092D6C, spec->frontTyre18, (CarSetup *)params);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092D8C, spec->frontTyre19, (CarSetup *)params);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092820, spec->frontTyre1a, (CarSetup *)params);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092D7C, spec->rearTyre18, (CarSetup *)params);
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092DBC, spec->rearTyre19, (CarSetup *)params);
    CarParams_ApplySuspension(spec, params);
    CarParams_ApplyDesc8e08(spec, params);
    CarParams_CopyT05(spec, params);
    CarParams_CopyT11(spec, params);
    CarParams_ApplyBrakeBalance(spec, params);
    CarParams_Nop(spec, params);
    CarParams_ApplyDesc8e50(spec, params);
    CarParams_ApplySettingB5(spec, params);
    CarParams_ApplySettingB4(spec, params);
    if (spec->flags & 1) {
        P16(0x1ae) = spec->unk7a;
    }
    spec->outB6 = *(unsigned short *)((int)spec->t06 + 0xa);
    spec->outB8 = *(unsigned char *)((int)spec->muffler + 8);
    if (*(unsigned char *)((int)spec->turbo + 9) != 0) {
        spec->flags |= 2;
        spec->outB8 += 4;
    }
    P16(0x1b0) = factor;
    P16(0x1ae) = P16(0x1ae) * 10;
    if (spec->flags & 0x80) {
        P8(0x1aa) = 0;
    }
    P8(0x134) = 0x94;
    P8(0x13d) = 0x94;
    P8(0x133) = 0xa6;
    P8(0x13c) = 0xa6;
    P8(0x71) = 0x28;
    P8(0x7d) = 0x28;
    P8(0x74) = 0x46;
    P8(0x78) = 0x46;
    P8(0x80) = 0x46;
    P8(0x84) = 0x46;
    P8(0x72) = 0x1e;
    P8(0x76) = 0x1e;
    P8(0x7e) = 0x1e;
    P8(0x82) = 0x1e;
    P8(0x44) = 0x6;
    P8(0xb0) = 0x6;
    P8(0xc3) = 0x6;
    P8(0xd6) = 0x6;
    P8(0xe9) = 0x6;
    P8(0x12d) = 0x0;
    P8(0x12e) = 0x82;
    P8(0x12f) = 0xa4;
    P8(0x130) = 0xff;
    P8(0x131) = 0xc8;
    P8(0x132) = 0xac;
    P8(0x136) = 0x0;
    P8(0x137) = 0x82;
    P8(0x138) = 0xa4;
    P8(0x139) = 0xff;
    P8(0x13a) = 0xc8;
    P8(0x13b) = 0xac;
    P8(0x8e) = 0x8;
    P8(0x9f) = 0x8;
    P8(0x11a) = 0x4;
    P8(0x123) = 0x4;
    P8(0x12c) = 0x4;
    P8(0x135) = 0x4;
    P8(0x118) = 0x9;
    return 1;
}
#undef P8
#undef P16

void CarSetup_ApplyDescriptors(unsigned int *desc, void *entry, CarSetup *setup)
{
    int elemSize;
    int count;
    unsigned int d;
    void *src;
    void *dst;

    elemSize = 0;
    count = 0;
    while (*desc != 0) {
        d = *desc;
        src = (void *)((int)entry + (d >> 10 & 0x3ff));
        dst = (void *)((int)setup + (d & 0x3ff));
        switch (d >> 20 & 0xf) {
        case 0:
            elemSize = 1;
            count = 1;
            break;
        case 1:
            elemSize = 2;
            count = 1;
            break;
        case 2:
            elemSize = 4;
            count = 1;
            break;
        case 3:
            elemSize = 1;
            count = 4;
            break;
        case 4:
            elemSize = 1;
            count = 6;
            break;
        case 5:
            elemSize = 1;
            count = 8;
            break;
        case 6:
            elemSize = 1;
            count = 16;
            break;
        case 7:
            elemSize = 2;
            count = 8;
            break;
        case 8:
            elemSize = 2;
            count = 16;
            break;
        }
        switch (d >> 24) {
        case 4:
            bcopy(src, dst, elemSize * count);
            break;
        case 0:
            Desc_ScaleFields(100, elemSize, count, src, dst);
            break;
        case 1:
            Desc_ScaleFields(1000, elemSize, count, src, dst);
            break;
        case 2:
            Desc_AddFieldRepeated(elemSize, count, src, dst);
            break;
        case 3:
            Desc_AddField(elemSize, src, dst);
            break;
        }
        desc++;
    }
}

void Desc_AddField(int elemSize, void *src, void *dst)
{
    switch (elemSize) {
    case 1:
        *(unsigned char *)dst += *(unsigned char *)src;
        break;
    case 2:
        *(unsigned short *)dst += *(unsigned short *)src;
        break;
    case 4:
        *(unsigned int *)dst += *(unsigned int *)src;
        break;
    }
}

void Desc_AddFieldRepeated(int elemSize, int count, void *src, void *dst)
{
    for (; count != 0; count = count + -1) {
        Desc_AddField(elemSize, src, dst);
        dst = (void *)((int)dst + elemSize);
    }
}

// The game divides into the register of the first load ($v1) and has one nop less.
void Desc_ScaleFields(int divisor, int elemSize, int count, void *src, void *dst)
{
    for (; count != 0; count = count + -1) {
        switch (elemSize) {
        case 1:
            *(signed char *)dst = *(signed char *)dst * *(signed char *)src / divisor;
            break;
        case 2:
            *(unsigned short *)dst = *(unsigned short *)dst * *(unsigned short *)src / divisor;
            break;
        case 4:
            *(int *)dst = *(int *)dst * *(int *)src / divisor;
            break;
        }
        src = (void *)((int)src + elemSize);
        dst = (void *)((int)dst + elemSize);
    }
}

// The game decrements value and steps in their own registers; this allocates new ones for them.
int Setting_Interpolate(int min, int max, int steps, int value)
{
    value--;
    if (steps < 2) {
        return min;
    }
    steps--;
    return min + (max - min) * value / steps;
}

void CarParams_ApplyBrakeBalance(CarSpec *spec, void *params)
{
    unsigned char *b = (unsigned char *)spec->brakeBalance;
    unsigned int rear = spec->t01_50[1];

    *(char *)((int)params + 0x2e) = Setting_Interpolate(b[0xa], b[0xb], b[9], spec->t01_50[0]);
    *(char *)((int)params + 0x2f) = Setting_Interpolate(b[0xe], b[0xf], b[0xd], rear);
}

void CarParams_CopyT11(CarSpec *spec, void *params)
{
    int i;

    for (i = 0; i < 8; i++) {
        ((short *)((int)params + 0x18))[i] = spec->t11_values[i];
    }
    *(short *)((int)params + 0x28) = spec->t11_4c;
    *(unsigned char *)((int)params + 0x1ab) = spec->t11_4e;
}

void CarParams_CopyT05(CarSpec *spec, void *params)
{
    *(unsigned char *)((int)params + 0x88) = spec->t05_52;
    *(unsigned char *)((int)params + 0x89) = spec->t05_53;
}

void CarParams_ApplyDesc8e08(CarSpec *spec, void *params)
{
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092E08, spec, params);
}

void CarParams_ApplySuspension(CarSpec *spec, void *params)
{
    unsigned char *s;
    unsigned char s0;
    unsigned char s1;
    unsigned char s2;
    unsigned char s3;
    unsigned char s4;
    unsigned char s5;
    unsigned char s6;
    unsigned char s7;
    int steps;

    CarSetup_ApplyDescriptors((unsigned int *)&D_80092E24, spec, (CarSetup *)params);
    s = (unsigned char *)spec->suspension;
    s0 = spec->settings[0];
    s1 = spec->settings[1];
    s2 = spec->settings[2];
    s3 = spec->settings[3];
    s4 = spec->settings[4];
    s5 = spec->settings[5];
    s6 = spec->settings[6];
    s7 = spec->settings[7];
    steps = s[0x27];
    *(char *)((int)params + 0x73) = Setting_Interpolate(s[0x28], s[0x29], steps, s0);
    *(char *)((int)params + 0x75) = Setting_Interpolate(s[0x2b], s[0x2c], steps, s1);
    steps = s[0x2e];
    *(char *)((int)params + 0x77) = Setting_Interpolate(s[0x2f], s[0x30], steps, s2);
    *(char *)((int)params + 0x79) = Setting_Interpolate(s[0x32], s[0x33], steps, s3);
    steps = s[0x35];
    *(char *)((int)params + 0x7f) = Setting_Interpolate(s[0x36], s[0x37], steps, s4);
    *(char *)((int)params + 0x81) = Setting_Interpolate(s[0x39], s[0x3a], steps, s5);
    steps = s[0x3c];
    *(char *)((int)params + 0x83) = Setting_Interpolate(s[0x3d], s[0x3e], steps, s6);
    *(char *)((int)params + 0x85) = Setting_Interpolate(s[0x40], s[0x41], steps, s7);
    *(char *)((int)params + 0x6f) = Setting_Interpolate(s[0x44], s[0x45], s[0x43], spec->settings[8]);
    *(char *)((int)params + 0x7b) = Setting_Interpolate(s[0x48], s[0x49], s[0x47], spec->settings[9]);
}

void CarParams_Nop(CarSpec *spec, void *params)
{
}

void CarParams_ApplySettingB4(CarSpec *spec, void *params)
{
    int iVar1;
    void *pvVar2;

    pvVar2 = spec->asc;
    iVar1 = Setting_Interpolate((unsigned int)*(unsigned char *)((int)pvVar2 + 0xc), (unsigned int)*(unsigned char *)((int)pvVar2 + 0xd),
                                                            (unsigned int)*(unsigned char *)((int)pvVar2 + 9), (unsigned int)spec->settingB4);
    *(char *)((int)params + 0x1a6) = (char)iVar1;
}

void CarParams_ApplySettingB5(CarSpec *spec, void *params)
{
    int iVar1;
    void *pvVar2;

    pvVar2 = spec->tcs;
    iVar1 = Setting_Interpolate((unsigned int)*(unsigned char *)((int)pvVar2 + 0xd), (unsigned int)*(unsigned char *)((int)pvVar2 + 0xe),
                                                            (unsigned int)*(unsigned char *)((int)pvVar2 + 10), (unsigned int)spec->settingB5);
    *(char *)((int)params + 0x1a2) = (char)iVar1;
}

void CarParams_ApplyDesc8e50(CarSpec *spec, void *params)
{
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092E50, spec, params);
}
}
