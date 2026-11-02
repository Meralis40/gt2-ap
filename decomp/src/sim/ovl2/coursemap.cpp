// OVL2 80017b7c-8001802c (coursemap): Course map.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemap", CourseMap_Init);

// Cancels any load (CourseMap_Cancel), then requests the map whose name hash is
// nameHash (+0x14) and sets state 1.
void CourseMap_Start(void *cm, void *menu, unsigned int nameHash)
{
    CourseMap_Cancel(cm, menu);
    *(unsigned int *)((int)cm + 0x14) = nameHash;
    *(unsigned short *)((int)cm + 0x10) = 1;
    *(unsigned char *)((int)cm + 0x19) = 0;
    *(unsigned char *)((int)cm + 0x18) = 0;
}

// Thunk to CourseMap_Start.
void CourseMap_Request(void *cm, void *menu, unsigned int nameHash)
{
    CourseMap_Start(cm, menu, nameHash);
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemap", CourseMap_Cancel);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemap", CourseMap_FindIndex);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemap", CourseMap_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemap", CourseMap_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemap", FUN_OVL2__80017fc4);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemap", FUN_OVL2__80017ff8);
}
