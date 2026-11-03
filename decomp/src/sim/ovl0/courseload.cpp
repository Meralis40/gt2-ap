// OVL0 80026bb4-800279e8 (courseload): Course data relocation and loading.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned short g_VolBootIds[248];
extern RECT g_CourseMapClutRect;
extern RECT g_CourseMapImageRect;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseSection_LoadTransform);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseMesh_Relocate);

// Empty.
void CourseWalls_Relocate(int *walls, int base)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseGrid_Relocate);

// Empty.
void CourseVisList_Relocate(unsigned short *list, int base)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseSection_Relocate);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseSections_Relocate);

// Empty.
void CourseObjGroup_Relocate(unsigned int *group, int base)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", LodTable_Relocate);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", ModelLib_Relocate);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseObjModels_Relocate);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", PtrTable_Relocate);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", Course_Relocate);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", TimList_Upload);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", Race_SetCourseIndex);

// Loads sky skyId: the TIM list file (g_VolBootIds[0x46] + skyId * 2 + 1) through the
// load buffer at race +0xe5700 to VRAM, then the sky mesh file to race +0x5d34.
void Race_LoadSky(void *race, int skyId)
{
    int file;

    file = (unsigned int)g_VolBootIds[0x46] + skyId *2;
    Vol_ReadFileAsync(file + 1, (int *)((int)race + 0xe5700));
    TimList_Upload((int *)((int)race + 0xe5700));
    Vol_ReadFileAsync(file, (void *)((int)race + 0x5d34));
}

// Loads the course map image (CourseMap_GetFileId of the course's map id, Course_Get
// +4) to the load buffer and uploads it to g_CourseMapClutRect / g_CourseMapImageRect.
void Race_LoadCourseMap(void *race, int course)
{
    CourseEntry *pCVar1;
    int file;

    pCVar1 = Course_Get(course);
    file = CourseMap_GetFileId(pCVar1->hash);
    Vol_ReadFileAsync(file, (void *)((int)race + 0xe5700));
    Tim_LoadClutAndImage((void *)((int)race + 0xe5700), &g_CourseMapClutRect, &g_CourseMapImageRect);
}

// Loads a course: the map image, the course textures (g_VolBootIds[0x6c] + course * 2
// + 1, uploaded), the course file (same id) to race +0xb534 (0x800b4a34),
// Course_Relocate, then the sky of the course (Course_Get +0xa).
void Race_LoadCourse(void *race, int course)
{
    CourseEntry *pCVar1;
    int file;
    CourseData *course_00;

    Race_LoadCourseMap(race, course);
    pCVar1 = Course_Get(course);
    course_00 = (CourseData *)((int)race + 0xb534);
    file = (unsigned int)g_VolBootIds[0x6c] + course *2;
    Vol_ReadFileAsync(file + 1, course_00);
    TimList_Upload((int *)course_00);
    Vol_ReadFileAsync(file, course_00);
    Course_Relocate(course_00);
    Race_LoadSky(race, (unsigned int)*(unsigned short *)pCVar1->unkA);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", Race_LoadCourseTims);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", Race_LoadBanners);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", Race_LoadCourseOnce);
}
