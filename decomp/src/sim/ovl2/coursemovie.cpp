// OVL2 8001802c-8001824c (coursemovie): Course preview movie.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


// Course preview movie (g_ArcCoursePreviewMovie at 0x80129850): sets the decode
// buffer, idle, no movie.
void CoursePreviewMovie_Init(ArcCoursePreviewMovie *m, void *buffer)
{
    m->buffer = buffer;
    m->cdClaimed = '\0';
    m->stopRequested = '\0';
    m->started = '\0';
    m->movie = -1;
    m->state = '\0';
}

// Requests movie index movie (state 1).
void CoursePreviewMovie_Play(ArcCoursePreviewMovie *m, void *menu, int movie)
{
    m->movie = movie;
    m->state = '\x01';
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemovie", CoursePreviewMovie_Stop);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemovie", CoursePreviewMovie_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemovie", CoursePreviewMovie_Decode);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/coursemovie", CoursePreviewMovie_Draw);
}
