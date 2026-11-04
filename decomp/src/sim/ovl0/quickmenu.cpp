// OVL0 80047024-80048418 (quickmenu): QuickMenu, the race's menu loop.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

// The packet of Gpu_MoveImage.
struct MoveImage {
    unsigned int tag;
    short srcX, srcY;
    short dstX, dstY;
    short w, h;
};

extern "C" {
extern GameState g_GameState;
extern QuickMenu *g_QuickMenu;
extern unsigned char g_HudTexFile[];
extern unsigned char g_QuickMenu_OtBuf[];
extern unsigned char g_PreQuickMenu_PacketBuf[];
extern unsigned char g_OVL0_FontInfo[];
extern void *g_OVL0_EndFontInfo;
extern unsigned char D_8019EC00[];
extern void *g_OVL0_KanjiFont;
extern short g_OVL0_CardSlotStatus[];
extern unsigned int g_VolLba;
extern PadHandler g_PadHandlers[];
extern Font g_OVL0_FontA;
extern Font g_OVL0_FontB;
extern Font g_OVL0_FontC;
extern Font g_OVL0_FontD;
extern char QuickMenu__vtable[];
extern char PreQuickMenuIO__vtable[];
extern void *QuickMenu__typeinfo;
extern void *PreQuickMenuIO__typeinfo;
extern char psxViewLoopDoubleBuffer__typeinfo[];
extern char D_8005A840[];
extern char D_8005A854[];
void psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer(void *self);
void psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(void *self, int flags);
void psxViewLoopDoubleBuffer__SetVideoMode(void *self, unsigned int mode);
void psxViewLoopDoubleBuffer__Leave(void *self);
void *psxViewLoopDoubleBuffer____tf(void);
void psxViewLoop__Enter(void *self);
int psxViewLoop__Update(void *self);
void psxViewLoop__Draw(void *self);
void psxViewLoop__BeginFrame(void *self);
void __rtti_si(void *info, char *name, void *base);
void AsyncRead_Init(AsyncRead *r);
int AsyncRead_Poll(AsyncRead *r);
void Tim_UploadImageToSlot(void *tim, int slot);
int QuickMenu_PopPage(QuickMenu *menu);
int RaceType_UsesSettingTim(void);
int RaceType_IsLicenseTest(void);

static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline void Text_SetRgb(TextCtx *ctx, unsigned int rgb)
{
    ctx->rgb = rgb;
}

// Makes the top of the page stack active and enters it.
static inline void QuickMenu_EnterTop(QuickMenu *menu, int back)
{
    void (*enter)(int);

    menu->activePage = menu->pageStack[menu->pageDepth];
    enter = (void (*)(int))menu->pageStack[menu->pageDepth]->page.enter;
    if (enter != 0) {
        enter(back);
    }
}

#ifdef NON_MATCHING
// Draws page: its title bar, with fade (0-128) dimming it, then the page itself. The
// title of the page being left keeps its spacing; the one coming in closes up.
void QuickMenu_DrawPage(QuickMenu *menu, QuickMenuPage *page, DrawLayer *layer, int fade, int active)
{
    uint *ot = (uint *)((char *)layer->ot + 0x10);
    int r;
    int g;
    int b;
    int size;
    unsigned int color;
    PrimG4 *p;

    if (page == 0) {
        return;
    }
    if (page->page.title != 0) {
        TextCtx text;
        TextCtx *ctx;
        int spacing;
        int x;
        RECT *rect;

        Text_Init(&text, 6);
        Text_SetFont(&text, &g_OVL0_FontC);
        ctx = &text;
        ctx->ot = ot;
        spacing = 2;
        if (active != 0) {
            spacing = 0x22 - (fade >> spacing);
        }
        size = Text_MeasureString(ctx, (signed char *)page->page.title, spacing);
        x = (0x160 - size) >> 1;
        rect = Prim_AddTile(ot, fade * 0xff >> 7);
        g = fade * 0x66 >> 7;
        rect->x = x + 1;
        rect->y = 0x4e;
        rect->w = size;
        rect->h = 2;
        ctx->rgb = g | g << 8 | g << 16;
        Text_DrawString(ctx, (signed char *)page->page.title, x, 0x4c, spacing);
        Text_SetRgb(ctx, 0);
        Text_DrawString(ctx, (signed char *)page->page.title, x + 2, 0x4f, spacing);
    }
    color = page->page.titleColor;
    r = (int)(color & 0xff) * fade >> 7;
    g = (int)(color >> 8 & 0xff) * fade >> 7;
    b = (int)(color >> 16 & 0xff) * fade >> 7;
    size = fade * 0x2d >> 7;
    p = Prim_AddPolyG4(ot);
    color = r | g << 8 | b << 16 | 0x3a000000;
    p->c1 = color;
    p->c0 = color;
    p->x1 = p->x3 = 0x160;
    p->y0 = p->y1 = 0x2d - size;
    p->c2 = p->c3 = 0;
    p->x0 = p->x2 = 0;
    p->y2 = p->y3 = size + 0x2d;
    Prim_AddDrawMode(ot, 0x220);
    menu->activePage = page;
    if (page->page.draw != 0) {
        ((void (*)(DrawLayer *))page->page.draw)(layer);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu_DrawPage);
#endif

void *QuickMenu__QuickMenu(void *self)
{
    psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer(self);
    *(char **)self = QuickMenu__vtable;
    return self;
}

void QuickMenu__QuickMenu_dtor(void *self, int flags)
{
    *(char **)self = QuickMenu__vtable;
    psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(self, flags);
}

void QuickMenu_SetRootPage(QuickMenu *menu, MenuPage *page)
{
    menu->pageDepth = 0;
    menu->pageStack[0] = (QuickMenuPage *)page;
}

void QuickMenu__Enter(QuickMenu *menu)
{
    TextCtx *text;
    int two = 2;

    psxViewLoop__Enter(menu);
    g_QuickMenu = menu;
    menu->unk7c = two;
    psxViewLoopDoubleBuffer__SetVideoMode(menu, 100);
    PacketBuffer_Init(&menu->base.packets, g_HudTexFile, 0x10000);
    ViewDrawContext_Init(&menu->base.draw, g_QuickMenu_OtBuf, 0x13b0);
    menu->layers[0].priority = 0;
    menu->layers[0].otSize = two;
    menu->layers[1].priority = 1;
    menu->layers[1].otSize = 0x20;
    menu->layers[2].priority = two;
    menu->layers[2].otSize = 0x20;
    menu->layers[4].priority = 3;
    menu->layers[4].otSize = 0x118;
    menu->layers[5].priority = 4;
    menu->layers[5].otSize = 0x118;
    menu->layers[3].priority = 5;
    menu->layers[3].otSize = 4;
    ViewDrawContext_Reset(&menu->base.draw);
    ViewDrawContext_AddLayer(&menu->base.draw, &menu->layers[0]);
    ViewDrawContext_AddLayer(&menu->base.draw, &menu->layers[1]);
    ViewDrawContext_AddLayer(&menu->base.draw, &menu->layers[2]);
    ViewDrawContext_AddLayer(&menu->base.draw, &menu->layers[4]);
    ViewDrawContext_AddLayer(&menu->base.draw, &menu->layers[5]);
    ViewDrawContext_AddLayer(&menu->base.draw, &menu->layers[3]);
    menu->hookActive = 0;
    PadDevice_Open(&menu->pads[0], 0, g_PadHandlers);
    PadDevice_Open(&menu->pads[1], 1, g_PadHandlers);
    PadDevice_FlushInput(&menu->pads[0]);
    PadDevice_FlushInput(&menu->pads[1]);
    CardSlots_Init(g_OVL0_CardSlotStatus);
    text = &menu->text;
    Text_Init(text, 6);
    text->unk1c = 1;
    g_OVL0_KanjiFont = D_8019EC00;
    KanjiFont_Init(D_8019EC00, 0x160, 0, 0x80, 0x1e0, 0x160, 0x1e1);
    menu->leavingPage = 0;
    menu->transitionTimer = 0;
    QuickMenu_EnterTop(menu, 0);
    menu->updateCount = 0;
    menu->drawStamp = 0;
}

// Reads the pads and updates the card slots, runs the hook, then during a transition
// the leaving page, and the active page: 1 enters the page it pushed, 2 pops back to the
// previous one, 3 and 4 leave the menu, 5 and 6 enter a page the sideways way.
int QuickMenu__Update(QuickMenu *menu)
{
    int result = 1;
    int r;
    int (*update)(int);

    psxViewLoop__Update(menu);
    menu->frameCount += result;
    PadButtons_Read(&menu->pads[0].buttons, &menu->padState[0]);
    PadButtons_Read(&menu->pads[1].buttons, &menu->padState[1]);
    CardSlots_Update(g_OVL0_CardSlotStatus);
    menu->updateCount += result;
    if (menu->hookActive != 0) {
        menu->hookActive = menu->hook(menu->hookArg, result, result);
    }
    if (menu->transitionTimer > 0 && --menu->transitionTimer > 0) {
        QuickMenuPage *page = menu->leavingPage;

        menu->activePage = page;
        if (page != 0 && page->page.update != 0) {
            ((int (*)(int))page->page.update)(0);
        }
    }
    menu->activePage = menu->pageStack[menu->pageDepth];
    update = (int (*)(int))menu->pageStack[menu->pageDepth]->page.update;
    r = 0;
    if (update != 0) {
        r = update(1);
    }
    switch (r) {
    case 0:
        break;
    case 6:
        QuickMenu_EnterTop(menu, 0);
        menu->transitionTimer = 0x10;
        menu->transitionKind = 2;
        break;
    case 5:
        QuickMenu_EnterTop(menu, 0);
        menu->transitionTimer = 0x10;
        menu->transitionKind = 3;
        break;
    case 1:
        QuickMenu_EnterTop(menu, 0);
        menu->transitionTimer = 0x10;
        menu->transitionKind = 0;
        break;
    case 2:
        if (QuickMenu_PopPage(menu) < 0) {
            return result;
        }
        QuickMenu_EnterTop(menu, 1);
        menu->transitionTimer = 0x10;
        menu->transitionKind = 1;
        break;
    case 3:
    case 4:
        result = 0;
        break;
    }
    return result;
}

// The active and leaving pages' rectangles during a transition: 0 and 1 slide up and
// down, 2 and 3 sideways, as transitionTimer counts down from 16.
void QuickMenu_GetTransitionRects(QuickMenu *menu, RECT *active, RECT *leaving)
{
    leaving->w = 0x160;
    active->w = 0x160;
    leaving->h = 0x1e0;
    active->h = 0x1e0;
    switch (menu->transitionKind) {
    case 2:
        leaving->y = 0;
        active->y = 0;
        active->x = menu->transitionTimer * 200 / 16;
        leaving->x = (menu->transitionTimer - 16) * 100 / 16;
        return;
    case 3:
        leaving->y = 0;
        active->y = 0;
        leaving->x = (16 - menu->transitionTimer) * 100 / 16;
        active->x = -menu->transitionTimer * 200 / 16;
        return;
    case 0:
        leaving->x = 0;
        active->x = 0;
        active->y = menu->transitionTimer * 200 / 16;
        leaving->y = (menu->transitionTimer - 16) * 5;
        return;
    case 1:
    default:
        leaving->x = 0;
        active->x = 0;
        leaving->y = (16 - menu->transitionTimer) * 5;
        active->y = -menu->transitionTimer * 200 / 16;
        return;
    }
}

#ifdef NON_MATCHING
void QuickMenu__Draw(QuickMenu *menu)
{
    RECT active;
    RECT leaving;
    RECT *r;
    int fade;
    int stamp;

    psxViewLoop__Draw(menu);
    QuickMenu_GetTransitionRects(menu, &active, &leaving);
    fade = (menu->transitionTimer << 7) / 16;
    if (menu->transitionTimer > 0) {
        QuickMenu_DrawPage(menu, menu->leavingPage, &menu->layers[2], fade, 0);
        DrawLayer_SetArea(&menu->layers[2], &leaving);
    }
    QuickMenu_DrawPage(menu, menu->pageStack[menu->pageDepth], &menu->layers[1], 0x80 - fade, 1);
    DrawLayer_SetArea(&menu->layers[1], &active);
    r = Prim_AddTile(menu->layers[3].ot, 0);
    r->w = 0x160;
    stamp = g_GfxIrq.gpuDmaDoneTime;
    r->x = 0;
    r->y = 0;
    r->h = 0x1e0;
    menu->drawStamp = stamp;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/quickmenu", QuickMenu__Draw);
#endif

void QuickMenu__Leave(QuickMenu *menu)
{
    psxViewLoopDoubleBuffer__Leave(menu);
    PadDevice_Close(&menu->pads[0]);
    PadDevice_Close(&menu->pads[1]);
}

void *PreQuickMenuIO__PreQuickMenuIO(void *self)
{
    psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer(self);
    *(char **)self = PreQuickMenuIO__vtable;
    return self;
}

void PreQuickMenuIO__PreQuickMenuIO_dtor(void *self, int flags)
{
    *(char **)self = PreQuickMenuIO__vtable;
    psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(self, flags);
}

void PreQuickMenuIO__Enter(PreQuickMenuIO *io)
{
    unsigned char *tim;

    psxViewLoop__Enter(io);
    VolId_GetSize((VolBootId)0xd);
    tim = g_HudTexFile;
    VolId_GetSize((VolBootId)0x39);
    psxViewLoopDoubleBuffer__SetVideoMode(io, 1);
    PacketBuffer_Init(&io->base.packets, g_PreQuickMenu_PacketBuf, 0x1002);
    ViewDrawContext_Init(&io->base.draw, g_QuickMenu_OtBuf, 0x13b0);
    io->layer.priority = 0;
    io->layer.otSize = 2;
    ViewDrawContext_Reset(&io->base.draw);
    ViewDrawContext_AddLayer(&io->base.draw, &io->layer);
    if (io->mode == 0) {
        AsyncRead *read = (AsyncRead *)&io->loader;
        int sector;

        AsyncRead_Init(read);
        sector = g_VolLba + VolId_GetSector((VolBootId)0xd);
        AsyncRead_Start(read, tim, sector, VolId_GetSize((VolBootId)0xd));
    }
    io->step = 0;
    io->fontTimUploaded = 0;
    io->menuTimUploaded = 0;
    io->frames = 0;
}

int RaceType_UsesSettingTim(void)
{
    int type = RaceSetup_Get()->raceType;
    int uses = 0;

    switch (type) {
    case 1:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        uses = 1;
        break;
    }
    return uses;
}

int RaceType_IsLicenseTest(void)
{
    return RaceSetup_Get()->raceType == 3;
}

void PreQuickMenuIO__BeginFrame(PreQuickMenuIO *io)
{
    MoveImage move;
    RECT rect;

    if (io->mode != 0) {
        if (io->mode == 1) {
            RaceFont_Load(6);
            move.srcX = 0x200;
            move.srcY = 0;
            move.dstX = 0x140;
            move.dstY = 0x100;
            move.w = 0x40;
            move.h = 0x100;
            Gpu_MoveImage((unsigned int *)&move);
            Gpu_DrawSync();
        }
        return;
    }
    if (io->frames == 0) {
        move.srcX = 0;
        move.srcY = 0;
        move.dstX = 0;
        move.dstY = 0xf0;
        move.w = 0x140;
        move.h = 0xf0;
        Gpu_MoveImage((unsigned int *)&move);
        Gpu_DrawSync();
        move.srcX = 0x140;
        move.srcY = 0x100;
        move.dstX = 0x200;
        move.dstY = 0;
        move.w = 0x40;
        move.h = 0x100;
        Gpu_MoveImage((unsigned int *)&move);
        Gpu_DrawSync();
    }
    if ((io->fontTimUploaded == 0) & (io->step > 0)) {
        int *file = (int *)g_HudTexFile;
        int *block = file + 2;

        if (((unsigned int)file[1] >> 3) & 1) {
            block = (int *)((char *)block + file[2]);
        }
        rect.x = 0x180;
        rect.y = 0;
        rect.w = ((short *)block)[4];
        rect.h = ((short *)block)[5];
        Gpu_LoadImageBlock(block, &rect);
        io->fontTimUploaded = 1;
    }
    if (RaceType_UsesSettingTim()) {
        if ((io->menuTimUploaded == 0) & (io->step >= 4)) {
            Tim_UploadImageToSlot(g_HudTexFile, 0x16);
            io->menuTimUploaded = 1;
        }
    }
    if (RaceType_IsLicenseTest()) {
        if ((io->menuTimUploaded == 0) & (io->step >= 4)) {
            Tim_UploadImageToSlot(g_HudTexFile, 0x16);
            io->menuTimUploaded = 1;
        }
    }
}

// Going to the menus: reads the font info and the textures in steps, and keeps going
// for at least 0x42 frames.
int PreQuickMenuIO__Update(PreQuickMenuIO *io)
{
    int result = 1;
    int *info = (int *)g_OVL0_FontInfo;

    psxViewLoop__Update(io);
    io->frames += result;
    if (io->mode != 0) {
        if (io->mode == result) {
            return 0;
        }
    } else {
        if (AsyncRead_Poll((AsyncRead *)&io->loader) == 0) {
            switch (io->step) {
            case 0: {
                int sector = g_VolLba + VolId_GetSector((VolBootId)0xe);

                AsyncRead_Start((AsyncRead *)&io->loader, g_OVL0_FontInfo, sector,
                                VolId_GetSize((VolBootId)0xe));
                io->step++;
                break;
            }
            case 1:
                g_OVL0_FontC.digitWidth = 12;
                g_OVL0_FontC.spacing = 0;
                g_OVL0_FontD.digitWidth = 7;
                g_OVL0_FontD.spacing = 0;
                g_OVL0_FontB.digitWidth = 5;
                g_OVL0_FontB.spacing = 0;
                g_OVL0_FontA.digitWidth = 4;
                g_OVL0_FontA.spacing = 0;
                g_OVL0_FontC.glyphs = (FontGlyphTable *)((char *)info + info[1]);
                g_OVL0_FontC.kerning = (FontKerning *)((char *)info + info[2]);
                g_OVL0_FontD.glyphs = (FontGlyphTable *)((char *)info + info[3]);
                g_OVL0_FontD.kerning = (FontKerning *)((char *)info + info[4]);
                g_OVL0_FontB.glyphs = (FontGlyphTable *)((char *)info + info[5]);
                g_OVL0_FontB.kerning = (FontKerning *)((char *)info + info[6]);
                g_OVL0_FontA.glyphs = (FontGlyphTable *)((char *)info + info[7]);
                g_OVL0_FontA.kerning = (FontKerning *)((char *)info + info[8]);
                g_OVL0_EndFontInfo =
                    (void *)(((int)g_OVL0_FontInfo + VolId_GetSize((VolBootId)0xe) + 3) & ~3);
                io->step++;
                break;
            case 2:
                if (io->fontTimUploaded != 0) {
                    if (RaceType_UsesSettingTim()) {
                        int sector = g_VolLba + VolId_GetSector((VolBootId)0x39);

                        AsyncRead_Start((AsyncRead *)&io->loader, g_HudTexFile, sector,
                                        VolId_GetSize((VolBootId)0x39));
                    } else if (RaceType_IsLicenseTest()) {
                        int sector = g_VolLba + VolId_GetSector((VolBootId)0x38);

                        AsyncRead_Start((AsyncRead *)&io->loader, g_HudTexFile, sector,
                                        VolId_GetSize((VolBootId)0x38));
                    }
                    io->step++;
                }
                break;
            case 4:
                if (!RaceType_UsesSettingTim()) {
                    if (!RaceType_IsLicenseTest()) {
                        goto next;
                    }
                }
                if (io->menuTimUploaded == 0) {
                    break;
                }
            case 3:
            next:
                io->step++;
                break;
            }
        }
        if (io->frames > 0x42) {
            io->frames = 0x42;
            if (io->step == 5) {
                result = 0;
            }
        }
    }
    return result;
}

void PreQuickMenuIO__Draw(PreQuickMenuIO *io)
{
    psxViewLoop__Draw(io);
    if (io->mode == 0) {
        RECT *r = Prim_AddTile(io->layer.ot, 0x2080810);

        r->w = 0x140;
        r->x = 0;
        r->y = 0;
        r->h = 0xf0;
        Prim_AddDrawMode(io->layer.ot, 0x40);
    }
}

void PreQuickMenuIO__Leave(PreQuickMenuIO *io)
{
    psxViewLoopDoubleBuffer__Leave(io);
}

void QuickMenu_PlayMusic(int track)
{
    Music_PlayTrack(track, 1);
}

// Music_Stop.
void QuickMenu_StopMusic(void)
{
    Music_Stop();
}

void *PreQuickMenuIO____tf(void)
{
    if (PreQuickMenuIO__typeinfo == 0) {
        psxViewLoopDoubleBuffer____tf();
        __rtti_si(&PreQuickMenuIO__typeinfo, D_8005A840, psxViewLoopDoubleBuffer__typeinfo);
    }
    return &PreQuickMenuIO__typeinfo;
}

void *QuickMenu____tf(void)
{
    if (QuickMenu__typeinfo == 0) {
        psxViewLoopDoubleBuffer____tf();
        __rtti_si(&QuickMenu__typeinfo, D_8005A854, psxViewLoopDoubleBuffer__typeinfo);
    }
    return &QuickMenu__typeinfo;
}

void QuickMenu__BeginFrame(QuickMenu *menu)
{
    psxViewLoop__BeginFrame(menu);
    if (menu->hookActive != 0) {
        menu->hookActive = menu->hook(menu->hookArg, 1, 0);
    }
    KanjiFont_Upload(g_OVL0_KanjiFont);
}

int QuickMenu_SetHook(QuickMenu *menu, unsigned char (*hook)(void *, int, int), void *arg)
{
    if (menu->hookActive != 0) {
        return 0;
    }
    menu->hookActive = 1;
    menu->hook = hook;
    menu->hookArg = arg;
    return 1;
}

void FUN_OVL0__80048330(QuickMenu *menu)
{
    if (menu->hookActive != 0) {
        menu->hook(menu->hookArg, 0, 2);
        menu->hookActive = 0;
    }
}

void QuickMenu_ReplacePage(QuickMenu *menu, QuickMenuPage *page)
{
    menu->leavingPage = menu->pageStack[menu->pageDepth];
    menu->pageStack[menu->pageDepth] = page;
}

void QuickMenu_PushPage(QuickMenu *menu, QuickMenuPage *page)
{
    menu->leavingPage = menu->pageStack[menu->pageDepth];
    menu->pageDepth++;
    menu->pageStack[menu->pageDepth] = page;
}

int QuickMenu_PopPage(QuickMenu *menu)
{
    if (menu->pageDepth <= 0) {
        return -1;
    }
    menu->leavingPage = menu->pageStack[menu->pageDepth];
    return --menu->pageDepth;
}
}
