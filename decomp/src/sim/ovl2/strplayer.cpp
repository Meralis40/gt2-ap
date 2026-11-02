// OVL2 8002667c-8002678c (strplayer): Course preview stream player.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/strplayer", StrPlayer_Poll);

// Movie_Stop_OVL2 and Movie_Shutdown_OVL2.
void StrPlayer_Close(ArcStrPlayer *p)
{
    Movie_Stop_OVL2(p);
    Movie_Shutdown_OVL2(p);
}
}
