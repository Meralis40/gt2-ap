// Resident code 8007f01c-8007f5a8 (kanjifont). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int Gpu_TransferImage(RECT *rect, unsigned int *data, int store);

INCLUDE_ASM("asm/sim/exe/nonmatchings/kanjifont", KanjiFont_AddGlyphQuad);

void KanjiFont_SetBuffers(void *font, void *pixels, void *nodes, unsigned char wideMode)
{
    *(void **)font = pixels;
    *(void **)((int)font + 0x10) = nodes;
    *(unsigned char *)((int)font + 0x23) = wideMode;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/kanjifont", KanjiFont_SetVram);

INCLUDE_ASM("asm/sim/exe/nonmatchings/kanjifont", KanjiFont_UploadClut);

void KanjiFont_UploadTexture(void *font)
{
    Gpu_TransferImage((RECT *)((int)font + 4), *(unsigned int **)font, 0);
}

void KanjiFont_Reset(void *font)
{
    *(unsigned int *)((int)font + 0x14) = 0;
    *(unsigned short *)((int)font + 0x1e) = 0;
    *(unsigned short *)((int)font + 0x1c) = 0;
    *(unsigned int *)((int)font + 0x18) = *(unsigned int *)((int)font + 0x10);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/kanjifont", KanjiFont_FindGlyph);

INCLUDE_ASM("asm/sim/exe/nonmatchings/kanjifont", KanjiFont_GetPixelPtr);

INCLUDE_ASM("asm/sim/exe/nonmatchings/kanjifont", KanjiFont_RenderGlyph);
}
