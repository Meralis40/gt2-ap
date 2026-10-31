// OVL4 8001f210-8001fcdc (gtfont): GT mode font and number drawing.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
int GTText_WidthW(unsigned short *str);
int GTFont_DrawString(void *ot, unsigned short *text, int x, int y, unsigned int rgb);
int GTText_Draw(void *ot, char *str, int x, int y, unsigned int rgb);
int GTText_Width(char *str);
int GTNum_DrawW(void *ot, unsigned short *str, int x, int y, unsigned int rgb);
int GTNum_WidthW(unsigned short *str);
int GTNum_Draw(void *ot, char *str, int x, int y, unsigned int rgb);
int GTNum_Width(char *str);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTFont_BuildIndex);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTFont_FindGlyph);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTFont_DrawString);

// GTText_DrawW with the right edge at x.
int GTText_DrawWRight(void *ot, unsigned short *str, int x, int y, unsigned int rgb)
{
    int iVar1;

    iVar1 = GTText_WidthW(str);
    iVar1 = GTFont_DrawString(ot, str, x - iVar1, y, rgb);
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTText_Draw);

// GTText_Draw with the right edge at x.
int GTText_DrawRight(void *ot, char *str, int x, int y, unsigned int rgb)
{
    int iVar1;

    iVar1 = GTText_Width(str);
    iVar1 = GTText_Draw(ot, str, x - iVar1, y, rgb);
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTText_WidthW);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTText_Width);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTNum_DrawW);

// GTNum_DrawW with the right edge at x.
int GTNum_DrawWRight(void *ot, unsigned short *str, int x, int y, unsigned int rgb)
{
    int iVar1;

    iVar1 = GTNum_WidthW(str);
    iVar1 = GTNum_DrawW(ot, str, x - iVar1, y, rgb);
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTNum_Draw);

// GTNum_Draw with the right edge at x.
int GTNum_DrawRight(void *ot, char *str, int x, int y, unsigned int rgb)
{
    int iVar1;

    iVar1 = GTNum_Width(str);
    iVar1 = GTNum_Draw(ot, str, x - iVar1, y, rgb);
    return iVar1;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTNum_WidthW);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTNum_Width);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTFont_DrawW);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTFont_DrawWRight);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTFont_Draw);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtfont", GTFont_DrawRight);
}
