// OVL2 800228c0-8002344c (courseselect): Course selection page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern short g_Arc_MovieId;
extern short g_Arc_MovieDelay;

// Starts the course preview movie after 12 frames.
void CourseSelect_QueueMovie(short movie)
{
    g_Arc_MovieId = movie;
    g_Arc_MovieDelay = 0xc;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/courseselect", ArcCourseList_GetCurrent);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/courseselect", ArcCourseList_Pick);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/courseselect", CourseSelect_DrawItem);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/courseselect", CourseSelect_SetCourse);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/courseselect", CourseSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/courseselect", CourseSelect_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/courseselect", CourseSelect_Draw);
}
