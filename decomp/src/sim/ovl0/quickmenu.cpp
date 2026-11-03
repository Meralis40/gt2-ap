// OVL0 80047024-80048418 (quickmenu): QuickMenu, the race's menu loop.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_DrawPage);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu__QuickMenu);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu__QuickMenu_dtor);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_SetRootPage);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu__Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu__Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_GetTransitionRects);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu__Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu__Leave);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", PreQuickMenuIO__PreQuickMenuIO);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", PreQuickMenuIO__PreQuickMenuIO_dtor);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", PreQuickMenuIO__Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", RaceType_UsesSettingTim);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", RaceType_IsLicenseTest);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", PreQuickMenuIO__BeginFrame);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", PreQuickMenuIO__Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", PreQuickMenuIO__Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", PreQuickMenuIO__Leave);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_PlayMusic);

// Music_Stop.
void QuickMenu_StopMusic(void)
{
    Music_Stop();
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", PreQuickMenuIO____tf);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu____tf);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu__BeginFrame);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_SetHook);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", FUN_OVL0__80048330);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_ReplacePage);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_PushPage);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_PopPage);
}
