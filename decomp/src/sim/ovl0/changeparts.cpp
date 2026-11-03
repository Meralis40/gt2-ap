// OVL0 80052838-80057628 (changeparts): Parts change and settings pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern void *g_Settings_Work;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_SetHelpTitle);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_SetHelpText);

// CarWork_SetPartSlot(work, category, variant).
void ChangeParts_Install(int category, int variant, CarWork *work)
{
    CarWork_SetPartSlot(work, category, variant);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsGroup_CountItems);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsGroup_CountVariants);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_DrawItems);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_GetGroup);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_CountGroups);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_VariantCallback);

// Selects a group: keeps the previous colour for the fade and updates the help title.
void ChangeParts_SelectGroup(void *menu, int group)
{
    void *pvVar1;
    unsigned int uVar2;

    pvVar1 = ChangeParts_GetGroup(group);
    *(unsigned int *)((int)menu + 0x30) = *(unsigned int *)((int)menu + 0x34);
    uVar2 = *(unsigned int *)((int)pvVar1 + 8);
    *(char *)((int)menu + 5) = (char)group;
    *(unsigned int *)((int)menu + 0x34) = uVar2;
    ChangeParts_SetHelpTitle(*(char **)((int)pvVar1 + 4));
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_SelectItem);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_Init);

// Starts the open animation.
void ChangeParts_Open(void *menu)
{
    *(unsigned short *)((int)menu + 10) = 0;
    *(unsigned short *)((int)menu + 0x2c) = 0;
    *(unsigned short *)((int)menu + 0xe) = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_Close);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangeParts_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_SetHelpTitle);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_SetHelpText);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", Slider_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", Setting_Format);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", SettingRow_Hide);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", SettingRow_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", SettingRow_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", SettingRow_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", Setting_GetCurrent);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", SettingItem_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", SettingsGroup_CountItems);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_DrawItems);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_BuildGroups);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_ValueCallback);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_Init);

// Starts the open animation.
void PartsSetting_Open(void *menu)
{
    *(unsigned short *)((int)menu + 0xc) = 0;
    *(unsigned short *)((int)menu + 0x23c) = 0;
    *(unsigned short *)((int)menu + 10) = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_Close);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_SelectGroup);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_SelectItem);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSetting_Draw);

// Sets g_Settings_Work (Change Parts menu; the Parts Setting menu is at +0x2a7c) and
// returns work + 0x2cc4 (free space after it).
void *Settings_SetWork(void *work)
{
    g_Settings_Work = work;
    return (void *)((int)work + 0x2cc4);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", Settings_Apply);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangePartsPage_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", ChangePartsPage_Update);

// ChangeParts_Draw.
void ChangePartsPage_Draw(DrawLayer *layer)
{
    ChangeParts_Draw(g_Settings_Work, (unsigned int *)((int)layer->ot + 0x10));
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSettingPage_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/changeparts", PartsSettingPage_Update);

// PartsSetting_Draw.
void PartsSettingPage_Draw(DrawLayer *layer)
{
    PartsSetting_Draw((void *)((int)g_Settings_Work + 0x2a7c), (unsigned int *)((int)layer->ot + 0x10));
}
}
