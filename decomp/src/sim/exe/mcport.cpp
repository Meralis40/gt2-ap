// Resident code 8008264c-80082ba8 (mcport). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Text_SetFont(TextCtx *ctx, Font *font);
void Text_SetTexturePage(TextCtx *ctx, int tpageId);
void *memset(void *__s, int __c, size_t __n);
extern unsigned char D_801F0D00;
extern unsigned int D_801F0D14;

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McPort_MapSector);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McPort_RemapBadSector);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McPort_RebuildBlockUse);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McDir_FindFile);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McDir_GetBlockChain);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McPort_ResetDir);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McPort_AllocBlocks);

void McDirEntry_Clear(McDirEntry *e)
{
    memset(e, 0, 0x20);
    e->state = 0xa0;
    e->next = -1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McDirEntry_FromFrame);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McFrame_FromDirEntry);

void Text_SetDefaultTexturePage(int tpage)
{
    Text_SetTexturePage((TextCtx *)&D_801F0D00, tpage);
}

void FUN_80082abc(Font *param_1)
{
    Text_SetFont((TextCtx *)&D_801F0D00, param_1);
}

void FUN_80082ae4(void)
{
}

void FUN_80082aec(unsigned int param_1)
{
    D_801F0D14 = param_1;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", FUN_80082af8);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", FUN_80082b40);

INCLUDE_ASM("asm/sim/exe/nonmatchings/mcport", McFrame_Checksum);
}
