// OVL2 8001d13c-8001d568 (arclists): Arcade course and car lists.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern short *g_ArcCarList_Colors[7];
extern char **g_ArcCarList_Codes[7];
extern short *g_ArcCarList_Logos[7];
extern int g_Arc_ClassSUnlocked;

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclists", ArcCourseList_UpdateUnlocks);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclists", ArcCourseList_InitAll);

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

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclists", ArcCarList_FirstAvailable);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclists", ArcCarList_UpdateUnlocks);

// g_Arc_ClassSUnlocked: the class menus show class S (6 items instead of 5).
int ArcCarList_IsClassSUnlocked(void)
{
    return g_Arc_ClassSUnlocked;
}
}
