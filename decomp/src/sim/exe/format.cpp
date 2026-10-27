// Resident code 80068734-80068e2c (format). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int sprintf(char *buf, char *fmt, ...);
extern char D_8008FA68[];

INCLUDE_ASM("asm/sim/exe/nonmatchings/format", Time_FormatMs);

INCLUDE_ASM("asm/sim/exe/nonmatchings/format", Time_FormatDiff);

void Speed_Format(char *buf, unsigned int v)
{
    if (99999 < v) {
        v = 99999;
    }
    sprintf(buf, D_8008FA68, v / 100, v % 100);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/format", Credits_FormatCommas);
}
