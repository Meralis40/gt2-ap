// OVL2 800265d4-8002678c (strplayer): Course preview stream player.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "copies.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/strplayer", Movie_InitRing_OVL2);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/strplayer", StrPlayer_Poll);

// Stops and shuts down OVL2's copy of the movie player.
void StrPlayer_Close(ArcStrPlayer *p)
{
    Movie_Stop(p);
    Movie_Shutdown(p);
}
}
