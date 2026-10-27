// Resident code 8006a3fc-8006ac68 (replaydraw). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void *Prim_AddDrawMode(void *ot, unsigned int tpage);
int *Prim_AddTile(void *ot, unsigned int rgb);
unsigned int Rgb_Lerp(unsigned char *from, unsigned char *to, int t, int range);
int Text_DrawStringMonoDigits(TextCtx *ctx, char *str, int x, int y, int spacing, int digitShift, int digitExtra);
int Text_TruncateToWidth(TextCtx *ctx, char *str, int spacing, int maxWidth);
char *strcat(char *dst, char *src);
extern unsigned char D_8008FA74;
extern unsigned int D_80091E7C;

void ReplayEntry_DrawField(void *ctx, char *str, int maxWidth, int x, int y)
{
    int iVar1;
    unsigned int uVar2;
    int *piVar3;

    iVar1 = Text_TruncateToWidth
                                        (*(TextCtx **)((int)ctx + 4), str, (int)*(short *)((int)ctx + 8), maxWidth);
    if (iVar1 != 0) {
        strcat(str, &D_8008FA74);
    }
    uVar2 = Rgb_Lerp((unsigned char *)((int)ctx + 0xc), (unsigned char *)&D_80091E7C, (int)*(short *)((int)ctx + 10),
                                      0x80);
    *(unsigned int *)(*(int *)((int)ctx + 4) + 0x14) = uVar2;
    Text_DrawStringMonoDigits
                        (*(TextCtx **)((int)ctx + 4), str, x + 5, y, (int)*(short *)((int)ctx + 8), -2, 0);
    piVar3 = Prim_AddTile(*(void **)ctx, *(unsigned int *)((int)ctx + 0x10));
    *(unsigned short *)(piVar3 + 1) = 4;
    *(unsigned short *)((int)piVar3 + 6) = 6;
    *(short *)piVar3 = (short)x;
    *(short *)((int)piVar3 + 2) = (short)y + -0xc;
    Prim_AddDrawMode(*(void **)ctx, 0x220);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydraw", ReplayEntry_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/replaydraw", ReplayDir_DrawUsageBar);
}
