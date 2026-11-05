// OVL2 8001d13c-8001d568 (arclists): Arcade course and car lists.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
int License_IsEarned_OVL2(int license);
extern GameState g_GameState;
extern ArcCourse g_ArcCourses_Race[];
extern ArcCourse g_ArcCourses_RaceRev[];
extern ArcCourse g_ArcCourses_TimeTrial[];
extern ArcCourse g_ArcCourses_TimeTrialRev[];
extern ArcCourse g_ArcCourses_Rally[];
extern ArcCourse g_ArcCourses_2P[];
extern ArcCourse g_ArcCourses_2PRally[];
extern short g_Arc_CourseCount;
extern short D_800F3958;
extern short D_800F395A;
extern short D_800F395C;
extern short D_800F395E;
extern short D_800F3960;
extern short D_800F3962;
extern short g_ArcCarList_Count[7];
extern signed char g_ArcCarList_UnlockS[];
extern signed char g_ArcCarList_UnlockRally[];
extern unsigned char D_80051BAC[]; // class S: available
extern unsigned char D_80051F20[]; // rally class: available
extern unsigned char D_800507A8;
extern unsigned char D_8005076C;
extern short *g_ArcCarList_Colors[7];
extern char **g_ArcCarList_Codes[7];
extern short *g_ArcCarList_Logos[7];
extern int g_Arc_ClassSUnlocked;

// Hashes the names of a course list and sets which courses are unlocked: by their
// license, and with reverse by their stat byte too. Returns how many are.
int ArcCourseList_UpdateUnlocks(ArcCourse *list, int reverse)
{
    GameState *state = &g_GameState;
    unsigned char *stats = (unsigned char *)&state->stats;
    int count = 0;

    for (; list->label != 0; list++) {
        list->hash = Str_Hash(list->name);
        list->unlocked = 0;
        if (License_IsEarned_OVL2(list->license) != 0) {
            list->unlocked = 1;
            if (list->stat != -1 && (reverse & 1)) {
                list->unlocked = (stats[list->stat] >> 2) & 1;
            }
            if (list->unlocked != 0) {
                count++;
            }
        }
    }
    return count;
}

// Updates all the course lists and their counts.
void ArcCourseList_InitAll(void)
{
    D_800507A8 = 0x7f;
    D_8005076C = 0x7f;
    g_Arc_CourseCount = ArcCourseList_UpdateUnlocks(g_ArcCourses_Race, 0);
    D_800F3958 = ArcCourseList_UpdateUnlocks(g_ArcCourses_RaceRev, 1);
    D_800F395A = ArcCourseList_UpdateUnlocks(g_ArcCourses_TimeTrial, 0);
    D_800F395C = ArcCourseList_UpdateUnlocks(g_ArcCourses_TimeTrialRev, 1);
    D_800F395E = ArcCourseList_UpdateUnlocks(g_ArcCourses_Rally, 0);
    D_800F3960 = ArcCourseList_UpdateUnlocks(g_ArcCourses_2P, 0);
    D_800F3962 = ArcCourseList_UpdateUnlocks(g_ArcCourses_2PRally, 0);
}

// g_ArcCarList_Colors[cls][car]; passed with the car code to the car preview (garage
// cars pass -1), probably the default colour.
int ArcCarList_GetColor(int cls, int car)
{
    return (int)g_ArcCarList_Colors[cls][car];
}

// Car code string of an arcade car (g_ArcCarList_Codes[cls][car]).
char *ArcCarList_GetCode(int cls, int car)
{
    return g_ArcCarList_Codes[cls][car];
}

// Maker logo sprite index (table of 0xc-byte sprites at 0x8004f7c4).
int ArcCarList_GetLogo(int cls, int car)
{
    return (int)g_ArcCarList_Logos[cls][car];
}

// The first available car of class cls, for classes 0 and 6; else 0.
int ArcCarList_FirstAvailable(int cls)
{
    int result = 0;
    int i;
    int count;

    switch (cls) {
    case 0:
        count = g_ArcCarList_Count[0];
        for (i = 0; i < count; i++) {
            if (((signed char *)D_80051BAC)[i] != 0) {
                result = i;
                break;
            }
        }
        break;
    case 6:
        count = g_ArcCarList_Count[6];
        for (i = 0; i < count; i++) {
            if (((signed char *)D_80051F20)[i] != 0) {
                result = i;
                break;
            }
        }
        break;
    }
    return result;
}

// Sets which cars of class S and of the rally class are available: those without a
// stat byte, or with bit 1 or 2 set in it.
void ArcCarList_UpdateUnlocks(void)
{
    GameState *state = &g_GameState;
    unsigned char *stats = (unsigned char *)&state->stats;
    int i;

    g_Arc_ClassSUnlocked = 0;
    for (i = 0; i < g_ArcCarList_Count[0]; i++) {
        signed char stat = g_ArcCarList_UnlockS[i];

        if (stat >= 0) {
            ArcBonusFlags *flags = (ArcBonusFlags *)&stats[stat];

            D_80051BAC[i] = 0;
            if ((flags->bit1 | flags->bit2) != 0) {
                D_80051BAC[i] = 1;
                g_Arc_ClassSUnlocked = 1;
            }
        } else {
            D_80051BAC[i] = 1;
        }
    }
    for (i = 0; i < g_ArcCarList_Count[6]; i++) {
        signed char stat = g_ArcCarList_UnlockRally[i];

        if (stat >= 0) {
            ArcBonusFlags *flags = (ArcBonusFlags *)&stats[stat];

            D_80051F20[i] = 0;
            if ((flags->bit1 | flags->bit2) == 0) {
                continue;
            }
        }
        D_80051F20[i] = 1;
    }
}

// g_Arc_ClassSUnlocked: the class menus show class S (6 items instead of 5).
int ArcCarList_IsClassSUnlocked(void)
{
    return g_Arc_ClassSUnlocked;
}
}
