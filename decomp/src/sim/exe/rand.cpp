// Resident code 80083ae0-80083b0c (rand): the game's random number generator.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

// seed = seed * 17 + 17; the result is the seed xored with itself rotated by 16 bits.
unsigned int Rand_Next(unsigned int *seed)
{
    unsigned int v = *seed * 17 + 17;
    unsigned int rot = v << 16 | v >> 16;

    *seed = v;
    return v ^ rot;
}

}
