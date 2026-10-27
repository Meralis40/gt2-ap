// Resident code 8005db90-8005e7b0 (records). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
unsigned int Course_FindByHash(unsigned int hash);
void *Course_Get(int index);
void RaceSetup_SetCourse(RaceSetup *setup, unsigned int hash);
unsigned int Str_Hash(char *s);
void *memset(void *__s, int __c, size_t __n);
char *strcpy(char *__dest, char *__src);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", PlayerStats_GetEventResult);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", PlayerStats_SetEventResult);

void PlayerStats_AddRaceResult(PlayerStats *stats, int place)
{
    stats->finishPlaceSum = stats->finishPlaceSum + place;
    stats->finishCount = stats->finishCount + 1;
    if (place == 1) {
        stats->racesWon = stats->racesWon + 1;
    }
}

void Stats_AddWinnings(void *stats, int amount)
{
    unsigned int uVar1;
    int iVar2;

    uVar1 = *(int *)((int)stats + 0x5c) + amount;
    *(unsigned int *)((int)stats + 0x5c) = uVar1;
    if (100000000 < uVar1) {
        iVar2 = *(int *)((int)stats + 0x58) + 1;
        *(int *)((int)stats + 0x58) = iVar2;
        if (iVar2 < 0) {
            *(unsigned int *)((int)stats + 0x58) = 0x7fffffff;
        }
        *(int *)((int)stats + 0x5c) = *(int *)((int)stats + 0x5c) + -100000000;
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", PlayerStats_SetArcadeCourseFlags);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", TimeRecord_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", Records_GetSectorTime);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", LicenseTest_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", LicenseTest_FindRank);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", LicenseTest_InsertTime);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", MachineTestRecords_SetName);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", MachineTestRecords_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", MachineTest_InsertRecord);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", RaceRecord_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", Records_GetLapTime);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", RaceRecord_AddLap);

void RaceSetup_SetEventCode(RaceSetup *setup, char *code)
{
    memset(setup->eventCode, 0, 0x10);
    strcpy(setup->eventCode, code);
}

void RaceSetup_SetCourse(RaceSetup *setup, unsigned int hash)
{
    unsigned int index;
    unsigned int *puVar1;

    setup->courseHash = hash;
    index = Course_FindByHash(hash);
    puVar1 = Course_Get(index);
    memset(setup->courseName, 0, 0x20);
    strcpy(setup->courseName, (char *)*puVar1);
}

void RaceSetup_SetCourseByName(RaceSetup *setup, char *name)
{
    unsigned int hash;

    hash = Str_Hash(name);
    RaceSetup_SetCourse(setup, hash);
}

void RaceResult_Init(void *res)
{
    memset(res, 0, 0xc);
    *(unsigned int *)((int)res + 4) = 0xffffffff;
}

void ChampRaceList_Clear(ChampRaceList *list)
{
    memset(list, 0, 0x94);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", ChampRaceList_AddPoints);

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", ChampRaceList_SortStandings);

void FUN_8005e73c(void *param_1)
{
    memset(param_1, 0, 0x15a5c);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/records", CourseRecord_GetCurrent);
}
