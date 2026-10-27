// Resident code 8006b548-8006bcb8 (menudraw). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {

unsigned int Rgb_Lerp(unsigned char *from, unsigned char *to, int t, int range)
{
    if (range < t) {
        t = range;
    }
    if (t < 0) {
        t = 0;
    }
    return (unsigned int)from[3] << 0x18 |
                  ((unsigned int)*from + (int)(((unsigned int)*to - (unsigned int)*from) * t) / range |
                    ((unsigned int)from[1] + (int)(((unsigned int)to[1] - (unsigned int)from[1]) * t) / range) * 0x100 |
                  ((unsigned int)from[2] + (int)(((unsigned int)to[2] - (unsigned int)from[2]) * t) / range) * 0x10000) & 0xffffff;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/menudraw", Menu_AddGreyBand);

INCLUDE_ASM("asm/sim/exe/nonmatchings/menudraw", Menu_AddGouraudQuad);

INCLUDE_ASM("asm/sim/exe/nonmatchings/menudraw", Menu_AddGouraudQuadOpaque);

INCLUDE_ASM("asm/sim/exe/nonmatchings/menudraw", Menu_DrawCursorBox);

INCLUDE_ASM("asm/sim/exe/nonmatchings/menudraw", Menu_DrawScrollArrow);

INCLUDE_ASM("asm/sim/exe/nonmatchings/menudraw", Menu_DrawSideArrow);

INCLUDE_ASM("asm/sim/exe/nonmatchings/menudraw", Menu_DrawColorSwatch);
}
