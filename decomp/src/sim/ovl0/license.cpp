// OVL0 8004c728-8004f8b4 (license): License tests: pages, medals and prize cars.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char g_LicenseResult_Fresh;
extern QuickMenu *g_QuickMenu;
extern void *g_OVL0_EndFontInfo;
extern unsigned char g_License_RuleRgbFrom;
extern unsigned char g_License_RuleRgbTo;

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_IdFromCode);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_SetupTest);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_LoadTestInfo);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawTestNumber);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_LoadDescription);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawTitleKanji);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawTitleText);

// One kanji glyph quad, 15x15, semi-transparent.
void Kanji_AddGlyph15(void *ot, unsigned int rgb, short x, short y, void *glyph)
{
    KanjiQuad *pKVar1;

    pKVar1 = KanjiFont_AddGlyphQuad(glyph, ot, rgb);
    pKVar1->y0 = y;
    pKVar1->y1 = y;
    pKVar1->x0 = x;
    pKVar1->x1 = x + 0xf;
    pKVar1->x2 = x;
    pKVar1->y2 = y + 0xf;
    pKVar1->x3 = x + 0xf;
    pKVar1->y3 = y + 0xf;
    pKVar1->tpage = pKVar1->tpage & 0xff9f | 0x20;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawDescKanji);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawDescText);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseMenu_ItemCallback);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawMedalTime);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawTestInfo);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawMedalRow);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_IsEarned_OVL0);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_IsAllGold);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_AwardMedal);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_GivePrizeCar);

// Sets g_LicenseResult_Fresh: the test was just driven (medals are only awarded then).
void LicenseResult_SetFresh(unsigned char fresh)
{
    g_LicenseResult_Fresh = fresh;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_LoadPrizeCar);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseRoot_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseRoot_Update);

// Empty.
void LicenseRoot_Draw(DrawLayer *layer)
{
}

// Leave page of the license menu: 20-frame timer and music stop.
void LicenseRetry_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x14;
    QuickMenu_StopMusic();
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseRetry_Update);

// Empty.
void LicenseRetry_Draw(DrawLayer *layer)
{
}

// VOL file of the demonstration replay of a test (License_DemoFileId).
int License_GetDemoFileId(int license, int test)
{
    int iVar1;

    iVar1 = License_DemoFileId(license, test);
    return iVar1;
}

// "Demonstration" from the license menu: 20-frame timer and music stop.
void LicenseDemo_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x14;
    QuickMenu_StopMusic();
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseDemo_Update);

// Empty.
void LicenseDemo_Draw(DrawLayer *layer)
{
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseName_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseName_Update);

// TextEntry_Draw.
void LicenseName_Draw(DrawLayer *layer)
{
    TextEntry_Draw((TextEntry *)((int)g_OVL0_EndFontInfo + 0x180), layer->ot);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", PrizeCarView_ZoomIn);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseAcquired_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseAcquired_Update);

// Dialog, both texts and the prize car (body only).
void LicenseAcquired_Draw(DrawLayer *layer)
{
    QuickMenu *pQVar1;
    TextCtx *ctx;
    unsigned int *ot;

    pQVar1 = g_QuickMenu;
    ot = layer->ot;
    ctx = &g_QuickMenu->text;
    CardDialog_Draw((CardDialog *)((int)g_OVL0_EndFontInfo + 0x58), ot, ctx);
    TextItem_Draw((TextItem *)((int)g_OVL0_EndFontInfo + 4), ot, 0xb0, 200, ctx);
    TextItem_Draw((TextItem *)((int)g_OVL0_EndFontInfo + 0x30), ot, 0xb0, 300, ctx);
    if ((*(int *)((int)g_OVL0_EndFontInfo + 0x688) != 0) &&
          (0x17 < *(short *)((int)g_OVL0_EndFontInfo + 0x68c))) {
        ResultCar_Draw((ResultCar *)((int)g_OVL0_EndFontInfo + 0x690), pQVar1->layers + 4, pQVar1->layers + 5,
                       (ResultCarView *)((int)g_OVL0_EndFontInfo + 0x7d4), 1);
    }
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseMenu_Enter);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseMenu_Update);

// Horizontal separator line at y.
void License_DrawRule(void *ot, short y, int fade)
{
    unsigned int rgb;
    RECT *pRVar1;

    rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &g_License_RuleRgbTo, fade, 0x80);
    pRVar1 = Prim_AddTile(ot, rgb);
    pRVar1->x = 0x7e;
    pRVar1->w = 0x160;
    pRVar1->y = y;
    pRVar1->h = 1;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseMenu_Draw);
}
