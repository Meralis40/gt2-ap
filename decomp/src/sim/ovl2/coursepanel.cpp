// OVL2 800173d4-80017b7c (coursepanel): Slide animation, course info and record panels.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", SlideAnim_Clear);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", SlideAnim_Open);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", SlideAnim_Close);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", CourseInfoPanel_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", FUN_OVL2__80017460);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", CourseInfoPanel_Draw);

// Hides the panel (active = 0, anim = 0).
void CourseRecordPanel_Init(ArcRecordPanel *p)
{
    p->active = '\0';
    p->anim = 0;
}

// Shows the best-time record of a course (0x24-byte entry of the course record table
// at 0x801c9af8) and starts the open animation.
void CourseRecordPanel_Open(ArcRecordPanel *p, void *record)
{
    p->anim = 1;
    p->active = '\x01';
    p->data = record;
}

// Starts the close animation (anim -1, then Update counts down to -12).
void CourseRecordPanel_Close(ArcRecordPanel *p)
{
    p->anim = -1;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", CourseRecordPanel_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", FUN_OVL2__80017690);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", CourseRecordPanel_Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursepanel", FUN_OVL2__80017a14);
}
