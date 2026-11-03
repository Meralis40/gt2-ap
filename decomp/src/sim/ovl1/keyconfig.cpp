// OVL1 80019218-8001aad4 (keyconfig): Controller key configuration.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_ToNeGconCode);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_FromNeGconCode);

// Analog stick codes 0xa0..0xc3 to icon index.
int KeyConfig_AnalogCodeToIcon(int code)
{
    if (code == 0xc3) {
        return 3;
    }
    if (code == 0xa3) {
        return 2;
    }
    if (code == 0xc2) {
        return 1;
    }
    if (code == 0xa2) {
        return 0;
    }
    if (code == 0xc1) {
        return 7;
    }
    if (code == 0xa1) {
        return 6;
    }
    if (code != 0xc0) {
        return (unsigned int)(code == 0xa0) << 2;
    }
    return 5;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_GetNeGconIcon);

// padType = 3 (no controller).
void KeyConfig_Clear(KeyConfig *cfg)
{
    cfg->padType = 3;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_Load);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_Store);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_FindButton);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_Assign);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_ChangeLayout);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_ChangePreset);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_SetDefaults);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_HandleInput);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_DrawCell);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_DrawTable);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfigPage_Init);

// Both cursors on Exit (row 10), 4-frame input delay.
void KeyConfigPage_Open(KeyConfigPage *page)
{
    page->row[1] = 10;
    page->row[0] = 10;
    page->delay = 4;
}

void KeyConfigPage_Close(KeyConfigPage *page)
{
    page->row[1] = -1;
    page->row[0] = -1;
    page->delay = -1;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfigPage_Update);

// KeyConfig_DrawTable.
void KeyConfigPage_Draw(KeyConfigPage *page, unsigned int *ot, int x, int y, int brightness)
{
    KeyConfig_DrawTable(ot, x, y, brightness, (int)page->row[0], (int)page->row[1], (int)page->blink);
}
}
