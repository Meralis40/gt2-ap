// OVL0 800279e8-80028cc0 (ground): Ground and course sections.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", Ground_FindTriangle);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSect_IsInFront);

// Ground_FindTriangle in the section's grid for pos (relative to the section's 4 MB
// block).
unsigned int *CourseSection_FindGround(CourseSection *sect, int *pos)
{
    unsigned int *puVar1;

    puVar1 = Ground_FindTriangle(sect->groundGrid, (sect->mesh).verts,
                                 (int)(*pos - (sect->center[0] & 0xffc00000U)) >> 10,
                                 (int)(pos[2] - (sect->center[2] & 0xffc00000U)) >> 10);
    return puVar1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", Ground_GetHeight);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSection_CollideWalls);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSect_FindNearest);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSect_Locate);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSect_GetSurface);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSect_GetDistance);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", World_LocateSection);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", World_GetGroundInfo);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", World_GetGroundHeight);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", World_CollideWalls);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", World_GetCourseDistance);
}
