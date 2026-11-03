// OVL0 800299d8-80029d58 (racemusic): Race music.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racemusic", RaceMusic_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racemusic", RaceMusic_Update);

void RaceMusic_Request(void *view, int track, int loop)
{
    *(char *)((int)view + 0x2ee) = (char)track;
    *(char *)((int)view + 0x2ed) = (char)loop;
}

// Requests the random race track (+0x2ef), looped. Called at the start signal.
void RaceMusic_PlayRaceTrack(void *view)
{
    RaceMusic_Request(view, (unsigned int)*(unsigned char *)((int)view + 0x2ef), 1);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racemusic", RaceMusic_PlayFinish);
}
