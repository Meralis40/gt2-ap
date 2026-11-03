// OVL0 8004a718-8004c728 (timetrialmenu): Time trial menu pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern QuickMenu *g_QuickMenu;
extern void *g_OVL0_EndFontInfo;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrial_SetFreshResult);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrial_IsGhostLoaded);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialRoot_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialRoot_Update);

// Empty.
void TimeTrialRoot_Draw(DrawLayer *layer)
{
}

// Same as QuitPage_Enter, for the time trial menu.
void TimeTrialQuit_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x14;
    QuickMenu_StopMusic();
}

// Returns 4 when the timer runs out.
int TimeTrialQuit_Update(int active)
{
    unsigned int uVar1;

    uVar1 = (unsigned short)g_QuickMenu->activePage->timer - 1;
    g_QuickMenu->activePage->timer = (short)uVar1;
    return uVar1 >> 0xd & 4;
}

// Empty.
void TimeTrialQuit_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialName_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialName_Update);

// TextEntry_Draw.
void TimeTrialName_Draw(DrawLayer *layer)
{
    TextEntry_Draw((TextEntry *)((int)g_OVL0_EndFontInfo + 0x44c), layer->ot);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", SessionResults_ItemCallback);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", Records_FindBestSectors);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", RecordRow_SetColors);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", SessionResults_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", SessionResults_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", SessionResults_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialMenu_ItemCallback);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", GhostTypeList_ItemCallback);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialMenu_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialMenu_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrialmenu", TimeTrialMenu_Draw);
}
