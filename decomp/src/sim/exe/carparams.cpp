// Resident code 80075fac-80077d5c (carparams). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CarSetup_ApplyDescriptors(unsigned int *desc, void *entry, CarSetup *setup);
void CarSetup_Clear(CarSetup *setup);
void CarSetup_FromCarEntry(void *carEntry, CarSetup *setup);
void Desc_AddField(int elemSize, void *src, void *dst);
int GameData_FindPart(int table, char stage, int stageOffset, unsigned int carCode);
void *GameData_GetEntry(int table, int index);
void *GameData_GetEntryIn(void *data, int table, int index);
int GameData_Relocate(void *data, unsigned int *entrySizes);
int Setting_Interpolate(int min, int max, int steps, int value);
short *UniStrDb_Get(void *db, int index);
void VolId_ReadFileAsync(VolBootId id, void *dst);
void *memset(void *__s, int __c, size_t __n);
extern unsigned char D_80092C24;
extern unsigned char D_80092E08;
extern unsigned char D_80092E50;
extern unsigned int D_801C94E4;
extern unsigned char g_ArcadeDataEntrySizes;
extern unsigned char g_CarSpecFromSetupDesc;
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

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarParams_GetTyreRadius);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarParams_GetDriveLayout);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarSetup_ApplyPartEntry);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarSpec_ApplyPartEntry);

void CarSpec_CopySetupFields(CarSpec *spec, CarSetup *setup)
{
    CarSetup_ApplyDescriptors((unsigned int *)&g_CarSpecFromSetupDesc, setup, (CarSetup *)spec);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarSpec_Resolve);

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

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", Part_Lookup);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", GameData_FindPart);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", GameData_FindFirstForCar);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", GameData_GetCarEntry);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", Car_GetDefaultSetup);

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

int UniStrDb_Load(void *buf)
{
    g_DataLanguage = (unsigned int)g_GameState.language;
    VolId_ReadFileAsync((unsigned int)g_LangCarParamFiles[g_DataLanguage].unistrdb, buf);
    g_UniStrDb = buf;
    return *(int *)buf;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", GameData_Relocate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", GameData_RelocateTable);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", StrPool_Get);

short *UniStr_Get(int index)
{
    short *psVar1;

    psVar1 = UniStrDb_Get(g_UniStrDb, index);
    return psVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", UniStrDb_Get);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", StrPool_Find);

void *GameData_LoadGtModeRace(void *buffer)
{
    g_DataLanguage = (unsigned int)g_GameState.language;
    VolId_ReadFileAsync((unsigned int)g_LangCarParamFiles[g_DataLanguage].gtmodeRace, buffer);
    g_GtModeRaceData = buffer;
    GameData_Relocate(buffer, (unsigned int *)&g_GtModeRaceEntrySizes);
    D_801C94E4 = (unsigned int)*(unsigned short *)((int)buffer + 4);
    return buffer;
}

void *GameData_LoadGtMode(void *buffer)
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

void *GameData_LoadArcade(void *buffer)
{
    g_GameDataMode = 1;
    g_DataLanguage = (unsigned int)g_GameState.language;
    VolId_ReadFileAsync((unsigned int)g_LangCarParamFiles[g_DataLanguage].arcadeData, buffer);
    g_RaceGameData = buffer;
    g_GameDataBase = buffer;
    GameData_Relocate(buffer, (unsigned int *)&g_ArcadeDataEntrySizes);
    return buffer;
}

void *GameData_LoadLicense(void *buffer)
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

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarSetup_FromCarEntry);

void CarSetup_CopySoundFromSpec(CarSetup *setup, CarSpec *spec)
{
    setup->engineSound = spec->outB6;
    setup->engineSoundVariant = spec->outB8;
    setup->flags_7a = spec->flags;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarParams_BuildFromSetup);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarParams_Build);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarSetup_ApplyDescriptors);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", Desc_AddField);

void Desc_AddFieldRepeated(int elemSize, int count, void *src, void *dst)
{
    for (; count != 0; count = count + -1) {
        Desc_AddField(elemSize, src, dst);
        dst = (void *)((int)dst + elemSize);
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", Desc_ScaleFields);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", Setting_Interpolate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarParams_ApplyBrakeBalance);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarParams_CopyT11);

void CarParams_CopyT05(CarSpec *spec, void *params)
{
    *(unsigned char *)((int)params + 0x88) = spec->t05_52;
    *(unsigned char *)((int)params + 0x89) = spec->t05_53;
}

void CarParams_ApplyDesc8e08(CarSpec *spec, void *params)
{
    CarSetup_ApplyDescriptors((unsigned int *)&D_80092E08, spec, params);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carparams", CarParams_ApplySuspension);

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
