// Resident code 8007da44-8007e0b0 (font). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
unsigned char FUN_8007da9c(...);
int Printf_Format(void *putc, char *fmt, void *args);
extern unsigned char D_80093148;
extern unsigned int D_8009314C;

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", Prim_AddDrawMode);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", Text_SetFont);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", FUN_8007da9c);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", FUN_8007daac);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", FUN_8007dae4);

void FUN_8007db30(unsigned int param_1, char *param_2, void *param_3)
{
    unsigned int uVar1;

    uVar1 = D_8009314C;
    D_8009314C = param_1;
    Printf_Format(FUN_8007da9c, param_2, param_3);
    D_8009314C = uVar1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", FUN_8007db70);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", FUN_8007db9c);

void Text_SetTexturePage(TextCtx *ctx, int tpageId)
{
    unsigned short uVar1;

    if (tpageId < 0) {
        tpageId = (int)D_80093148;
    }
    uVar1 = (unsigned short)tpageId;
    ctx->clut = (uVar1 & 0xf) * 4 + (uVar1 & 0x10) * 0x400;
    ctx->tpage = uVar1;
    *(unsigned char *)((int)&ctx->rgb + 3) = 0;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", Text_GetAdvance);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", Text_DrawGlyph);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", RaceFont_OnSector);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", RaceFont_InitUpload);

INCLUDE_ASM("asm/sim/exe/nonmatchings/font", RaceFont_Load);
}
