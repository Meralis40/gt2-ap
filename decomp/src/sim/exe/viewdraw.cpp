// Resident code 80080088-800804f8 (viewdraw). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Gpu_DrawOTag(void *list);
void ViewDrawContext_Reset(ViewDrawContext *ctx);

void ViewDrawContext_Init(ViewDrawContext *ctx, void *ot, unsigned int bytes)
{
    ctx->otBase = ot;
    ctx->otSize = (unsigned short)(bytes >> 2);
    ctx->field_18 = 0;
    ViewDrawContext_Reset(ctx);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", ViewDrawContext_AddLayer);

void ViewDrawContext_Reset(ViewDrawContext *ctx)
{
    ctx->layers = 0;
    ctx->otHead = 0;
    ctx->otHalf = 0;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", ViewDrawContext_AssignOT);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", ViewDrawContext_Begin);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", ViewDrawContext_End);

void ViewDrawContext_Submit(ViewDrawContext *ctx)
{
    if (ctx->otHead != 0) {
        Gpu_DrawOTag(ctx->otHead);
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", ViewDrawContext_SwapOT);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", ViewDrawContext_ClearOT);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", DrawLayer_SetArea);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", Prim_AddTexQuad);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", PacketBuffer_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", PacketBuffer_Swap);
}
