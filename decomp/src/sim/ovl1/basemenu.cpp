// OVL1 8001191c-800124a4 (basemenu): BaseMenu, the main menu screen loop, and the menu
// music. Its vtable and type name (.rodata, after Update's jump table) and its __tf
// function are emitted here; build.py's CXX_NAMES places its typeinfo.
// The functions that don't match yet are the original assembly, with their C++
// under NON_MATCHING.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "copies.h"
#include "common.h"

extern "C" {
extern TextCtx g_OVL1_TitleText;
extern Font g_OVL1_FontTitle;
extern Font g_OVL1_FontLarge;
extern Font g_OVL1_FontMedium;
extern Font g_OVL1_FontSmall;
extern unsigned char D_801A95C0[];
extern unsigned char D_8003A6C4[];
extern int g_OVL1_FontInfo[];
extern unsigned int g_OVL1_EndFontInfo;
extern BaseMenu *g_CurrentMenu;
extern short g_CardSlotStatus[];
extern unsigned int D_801F0698;
extern KanjiFont *g_OVL1_KanjiFont;
extern unsigned char g_OVL1_KanjiFontData[];
int VolId_ReadFileAndGetSize2(void *dst, int file);
void BaseMenu__DrawPage(BaseMenu *menu, BaseMenuPage *page, DrawLayer *layer, int fade);
void BaseMenu__GetTransitionRects(BaseMenu *menu, RECT *active, RECT *leaving);
int BaseMenu__PopPage(BaseMenu *menu);

#ifdef NON_MATCHING
// Draws a page: its title with a shadow, the bar under it and the band behind it
// in the page's colour, then the page's draw callback.
void BaseMenu__DrawPage(BaseMenu *menu, BaseMenuPage *page, DrawLayer *layer, int fade)
{
    TextCtx *text = &g_OVL1_TitleText;
    uint *ot = (uint *)((char *)layer->ot + 0x10);
    int spacing;
    int width;
    int x;
    int b;
    int r;
    int g;
    int band;
    unsigned int rgb;
    unsigned int color;
    RECT *tile;
    PrimG4 *poly;

    if (page == 0) {
        return;
    }
    if (page->title != 0) {
        Text_SetFont(text, &g_OVL1_FontTitle);
        spacing = 0x22 - (fade >> 2);
        text->ot = ot;
        width = Text_MeasureString(text, (signed char *)page->title, spacing);
        x = (0x160 - width) >> 1;
        tile = Prim_AddTile(ot, (fade * 0xf2) >> 7);
        b = (fade * 0x66) >> 7;
        tile->x = x + 1;
        tile->y = 0x5c;
        tile->w = width;
        tile->h = 2;
        text->rgb = b | b << 8 | b << 16;
        Text_DrawString(text, (signed char *)page->title, x, 0x5a, spacing);
        text->rgb = 0;
        Text_DrawString(text, (signed char *)page->title, x + 3, 0x5d, spacing);
        color = page->titleColor;
        r = ((int)(color & 0xff) * fade) >> 7;
        g = ((int)((color >> 8) & 0xff) * fade) >> 7;
        b = ((int)((color >> 16) & 0xff) * fade) >> 7;
        band = (fade * 0x30) >> 7;
        poly = Prim_AddPolyG4(ot);
        rgb = r | g << 8 | b << 16 | 0x3a000000;
        poly->y1 = poly->y0 = 0x30 - band;
        poly->c1 = poly->c0 = rgb;
        poly->c3 = poly->c2 = 0;
        poly->x2 = poly->x0 = 0;
        poly->x3 = poly->x1 = 0x160;
        poly->y3 = poly->y2 = 0x30 + band;
        Prim_AddDrawMode(ot, 0x220);
    }
    menu->activePage = page;
    if (page->draw != 0) {
        page->draw(layer);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__DrawPage);
#endif
}

BaseMenu::BaseMenu()
{
}

BaseMenu::~BaseMenu()
{
}

extern "C" {
// pageStack[0] = page, pageDepth = 0.
void BaseMenu__SetRootPage(BaseMenu *this_, BaseMenuPage *page)
{
    this_->pageDepth = 0;
    this_->pageStack[0] = page;
}
}

#ifdef NON_MATCHING
// Sets up the view (6 layers), the fonts, both pads, the card slots and the kanji
// font, and enters the current page.
void BaseMenu::Enter()
{
    int *fontInfo;
    int size;
    void (*enter)(int);

    psxViewLoop::Enter();
    result = 0;
    SetVideoMode(100);
    PacketBuffer_Init(&packets, D_801A95C0, 0x10000);
    ViewDrawContext_Init(&draw, D_8003A6C4, 0x10a40);
    layers[3].priority = 2;
    layers[0].priority = 0;
    layers[0].otSize = 4;
    layers[2].priority = 1;
    layers[2].otSize = 0x20;
    layers[3].otSize = 0x20;
    layers[4].priority = 3;
    layers[4].otSize = 0x1060;
    layers[5].priority = 4;
    layers[5].otSize = 0x1060;
    layers[1].priority = 5;
    layers[1].otSize = 4;
    ViewDrawContext_Reset(&draw);
    ViewDrawContext_AddLayer(&draw, &layers[0]);
    ViewDrawContext_AddLayer(&draw, &layers[2]);
    ViewDrawContext_AddLayer(&draw, &layers[3]);
    ViewDrawContext_AddLayer(&draw, &layers[4]);
    ViewDrawContext_AddLayer(&draw, &layers[5]);
    ViewDrawContext_AddLayer(&draw, &layers[1]);
    fontInfo = g_OVL1_FontInfo;
    hookActive = 0;
    size = VolId_ReadFileAndGetSize2(fontInfo, 0xe);
    g_OVL1_EndFontInfo = ((size + 3) & ~3) + (unsigned int)fontInfo;
    g_CurrentMenu = this;
    Text_Init(&g_OVL1_TitleText, 0x1e);
    g_OVL1_TitleText.unk1c = 1;
    g_OVL1_FontTitle.digitWidth = 12;
    g_OVL1_FontTitle.spacing = 0;
    g_OVL1_FontLarge.digitWidth = 7;
    g_OVL1_FontLarge.spacing = 0;
    g_OVL1_FontMedium.digitWidth = 5;
    g_OVL1_FontMedium.spacing = 0;
    g_OVL1_FontSmall.digitWidth = 3;
    g_OVL1_FontSmall.spacing = 0;
    g_OVL1_FontTitle.glyphs = (FontGlyphTable *)((char *)fontInfo + fontInfo[1]);
    g_OVL1_FontTitle.kerning = (FontKerning *)((char *)fontInfo + fontInfo[2]);
    g_OVL1_FontLarge.glyphs = (FontGlyphTable *)((char *)fontInfo + fontInfo[3]);
    g_OVL1_FontLarge.kerning = (FontKerning *)((char *)fontInfo + fontInfo[4]);
    g_OVL1_FontMedium.glyphs = (FontGlyphTable *)((char *)fontInfo + fontInfo[5]);
    g_OVL1_FontMedium.kerning = (FontKerning *)((char *)fontInfo + fontInfo[6]);
    g_OVL1_FontSmall.glyphs = (FontGlyphTable *)((char *)fontInfo + fontInfo[7]);
    g_OVL1_FontSmall.kerning = (FontKerning *)((char *)fontInfo + fontInfo[8]);
    PadDevice_Open(&pads[0], 0, g_PadHandlers);
    PadDevice_Open(&pads[1], 1, g_PadHandlers);
    PadDevice_FlushInput(&pads[0]);
    PadDevice_FlushInput(&pads[1]);
    CardSlots_Init(g_CardSlotStatus);
    g_OVL1_KanjiFont = (KanjiFont *)g_OVL1_KanjiFontData;
    KanjiFont_Init(g_OVL1_KanjiFontData, 0x160, 0, 0x80, 0x1e0, 0x160, 0x1e1);
    leavingPage = 0;
    transitionTimer = 0;
    activePage = pageStack[pageDepth];
    enter = pageStack[pageDepth]->enter;
    if (enter != 0) {
        enter(0);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/basemenu", BaseMenu__Enter);
#endif

// Reads the pads, updates the card slots, runs the hook, then during a transition
// the leaving page, and the active page: 1 enters the page it pushed, 2 pops back to
// the previous one (16-frame transitions), 3 and 4 leave the menu. Returns 0 to leave.
int BaseMenu::Update()
{
    int status = 1;
    int (*update)(int);
    void (*enter)(int);
    int r;

    psxViewLoop::Update();
    frameCount++;
    PadButtons_Read(&pads[0].buttons, &padState[0]);
    PadButtons_Read(&pads[1].buttons, &padState[1]);
    CardSlots_Update(g_CardSlotStatus);
    if (hookActive != 0) {
        hookActive = hook(hookArg, 1, 1);
    }
    if (transitionTimer > 0 && --transitionTimer > 0) {
        BaseMenuPage *page = leavingPage;
        activePage = page;
        if (page != 0 && page->update != 0) {
            page->update(0);
        }
    }
    activePage = pageStack[pageDepth];
    update = pageStack[pageDepth]->update;
    r = 0;
    if (update != 0) {
        r = update(1);
    }
    switch (r) {
    case 0:
        break;
    case 1:
        activePage = pageStack[pageDepth];
        enter = pageStack[pageDepth]->enter;
        if (enter != 0) {
            enter(0);
        }
        transitionTimer = 16;
        transitionBack = 0;
        break;
    case 2:
        if (BaseMenu__PopPage(this) < 0) {
            break;
        }
        activePage = pageStack[pageDepth];
        enter = pageStack[pageDepth]->enter;
        if (enter != 0) {
            enter(1);
        }
        transitionTimer = 16;
        transitionBack = 1;
        break;
    case 3:
    case 4:
        status = 0;
        break;
    }
    return status;
}

extern "C" {
// The active and leaving pages' rectangles during a transition: they slide up
// (down when going back) as transitionTimer counts down from 16.
void BaseMenu__GetTransitionRects(BaseMenu *menu, RECT *active, RECT *leaving)
{
    leaving->w = 0x160;
    active->w = 0x160;
    leaving->h = 0x1e0;
    active->h = 0x1e0;
    if (menu->transitionBack == 0) {
        leaving->x = 0;
        active->x = 0;
        active->y = menu->transitionTimer * 200 / 16;
        leaving->y = (menu->transitionTimer - 16) * 5;
        return;
    }
    leaving->x = 0;
    active->x = 0;
    leaving->y = (16 - menu->transitionTimer) * 5;
    active->y = -menu->transitionTimer * 200 / 16;
}
}

// The active page and, during a transition, the leaving page, each clipped to its
// sliding rectangle and faded by the transition, and the black background.
void BaseMenu::Draw()
{
    RECT active;
    RECT leaving;
    RECT *r;
    int fade;

    psxViewLoop::Draw();
    BaseMenu__GetTransitionRects(this, &active, &leaving);
    fade = (transitionTimer << 7) / 16;
    if (transitionTimer > 0) {
        BaseMenu__DrawPage(this, leavingPage, &layers[3], fade);
        DrawLayer_SetArea(&layers[3], &leaving);
    }
    BaseMenu__DrawPage(this, pageStack[pageDepth], &layers[2], 0x80 - fade);
    DrawLayer_SetArea(&layers[2], &active);
    r = Prim_AddTile(layers[1].ot, 0);
    r->w = 0x160;
    r->x = 0;
    r->y = 0;
    r->h = 0x1e0;
    drawStamp = D_801F0698;
}

extern "C" {
// Pushes page; the current one becomes the leaving page.
void PushPage(BaseMenu *menu, BaseMenuPage *page)
{
    menu->leavingPage = menu->pageStack[menu->pageDepth];
    menu->pageDepth++;
    menu->pageStack[menu->pageDepth] = page;
}

// Pops back to the previous page, which it returns the depth of; -1 at the root.
int BaseMenu__PopPage(BaseMenu *menu)
{
    if (menu->pageDepth <= 0) {
        return -1;
    }
    menu->leavingPage = menu->pageStack[menu->pageDepth];
    return --menu->pageDepth;
}
}

// Closes both pads.
void BaseMenu::Leave()
{
    psxViewLoopDoubleBuffer::Leave();
    PadDevice_Close(&pads[0]);
    PadDevice_Close(&pads[1]);
}

// Runs the hook (step 0) and uploads the kanji glyphs.
void BaseMenu::BeginFrame()
{
    psxViewLoop::BeginFrame();
    if (hookActive != 0) {
        hookActive = hook(hookArg, 1, 0);
    }
    KanjiFont_Upload(g_OVL1_KanjiFont);
}

extern "C" {
// OVL2's ArcadeMenu_SetHook: installs the per-frame hook if none is active. Returns
// 1 if installed.
int FUN_OVL1__800123a4(BaseMenu *menu, unsigned char (*hook)(void *, int, int), void *arg)
{
    if (menu->hookActive != 0) {
        return 0;
    }
    menu->hookActive = 1;
    menu->hook = hook;
    menu->hookArg = arg;
    return 1;
}

// OVL2's ArcadeMenu_CancelHook: calls the active hook with (arg, 0, 2) and removes it.
void FUN_OVL1__800123d0(BaseMenu *menu)
{
    if (menu->hookActive != 0) {
        menu->hook(menu->hookArg, 0, 2);
        menu->hookActive = 0;
    }
}

// Music_PlayTrack(track, loop).
void MainMenu_PlayMusic(int track)
{
    Music_PlayTrack(track, 1);
}

// Music_Stop.
void MainMenu_StopMusic(void)
{
    Music_Stop();
}
}
