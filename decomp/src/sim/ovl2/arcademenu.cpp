// OVL2 80013844-80014708 (arcademenu): Arcade menu: page drawing, movies and the
// ArcadeMenu class.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu_DrawPage);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu_PlayMovie);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu_StopMovie);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu_DrawMovie);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu__ArcadeMenu_dtor);

// pageStack[0] = page, pageDepth = 0.
void ArcadeMenu_SetRootPage(ArcadeMenu *this_, MenuPage *page)
{
    this_->pageDepth = '\0';
    this_->pageStack[0] = page;
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu__Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu__Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu_GetTransitionRects);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu__Draw);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu__Leave);

// Empty. Not referenced.
void OVL2_Nop(void)
{
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu____tf);
}
