// Resident code 8006ac68-8006b548 (text). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Text_SetTexturePage(TextCtx *ctx, int tpageId);

void Text_Init(TextCtx *ctx, int tpageId)
{
    Text_SetTexturePage(ctx, tpageId);
    ctx->unk1c = '\0';
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawString);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_MeasureString);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawCentered);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawStringRight);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_TruncateToWidth);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawStringMonoDigits);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_MeasureStringMonoDigits);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawCenteredMonoDigits);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawStringMonoDigitsRight);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawDigits);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_MeasureDigits);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawDigitsRight);

INCLUDE_ASM("asm/sim/exe/nonmatchings/text", Text_DrawDigitsCentered);
}
