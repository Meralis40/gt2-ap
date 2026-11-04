// OVL2 80013844-80014708 (arcademenu): ArcadeMenu, the arcade mode screen loop,
// with its page drawing and the course preview movie. Its vtable and type name
// (.rodata, after Update's jump table), its __tf function and typeinfo (.bss, which
// build.py's CXX_NAMES places) are emitted here; BeginFrame is in menupages.cpp.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "copies.h"
#include "common.h"

extern "C" {
extern ArcCoursePreviewMovie g_ArcCoursePreviewMovie;
extern short g_CarLogoArcState;
extern short g_ArcCardSlotStatus[];
extern unsigned char g_ArcadeMenuExit; // what ArcadeMode_Run does next: 0 title, 1 race, 2 and 3 movies
extern KanjiFont *g_ArcKanjiFont;
extern GameState g_GameState;
extern void *g_EventDataBuf;
extern short g_ArcadeBgFileByLang[];    // background TIMs, per language
extern short g_ArcadeBg2FileByLang[];
extern unsigned char g_OVL2_PacketBuf[];
extern unsigned char g_OVL2_OrderingTable[];
extern unsigned char g_OVL2_FileBuf[];  // the TIMs, then the logo archive, the course map info and the font info
extern CarInfoA g_OVL2_storageCarInfoA;
extern unsigned char g_ArcadeT002nModel[];
extern unsigned char g_ArcadeT002nTex[];
extern TextCtx g_ArcadeText;
extern unsigned char g_OVL2_CarGzBuf[];
// The car views and their buffers: models, textures, compressed textures and
// wheels, and the inflate state they share.
extern ArcCarView D_800EF4A8[2];
extern unsigned char D_800274D4[];
extern unsigned char D_800A8D68[];
extern unsigned char D_800CF4A8[];
extern unsigned char D_800334D4[];
extern unsigned char D_800DF4A8[];
extern unsigned short D_800454D4[];     // [1] clut y and [4] texture page of the car views
extern unsigned char D_800E34A8[];      // logo TIM buffers
extern unsigned char D_800C74A8[];      // course map buffer
extern unsigned char D_800F8800[];      // arcade game data
extern unsigned char D_80103800[];      // preview movie buffer
extern unsigned char D_800455DC[];      // kanji font
// The fonts of fontinfo, by their digit width.
extern Font D_80129830;                 // 12, the page titles
extern Font D_80129820;                 // 7
extern Font D_80123810;                 // 5
extern Font D_80129840;                 // 3
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu_DrawPage);

// Starts the course preview movie movie.
void ArcadeMenu_PlayMovie(ArcadeMenu *menu, int movie)
{
    CoursePreviewMovie_Play(&g_ArcCoursePreviewMovie, menu, movie);
}

// Stops the course preview movie.
void ArcadeMenu_StopMovie(ArcadeMenu *menu)
{
    CoursePreviewMovie_Stop(&g_ArcCoursePreviewMovie, menu);
}

// Draws the course preview movie's frame at x, y.
void ArcadeMenu_DrawMovie(ArcadeMenu *menu, void *ot, int x, int y, int unused)
{
    CoursePreviewMovie_Draw(&g_ArcCoursePreviewMovie, ot, x, y, unused);
}

ArcadeMenu::ArcadeMenu()
{
}

ArcadeMenu::~ArcadeMenu()
{
}

// pageStack[0] = page, pageDepth = 0.
void ArcadeMenu_SetRootPage(ArcadeMenu *this_, MenuPage *page)
{
    this_->pageDepth = '\0';
    this_->pageStack[0] = page;
}

// Sets up the view (6 layers), loads the backgrounds (per language), the maker
// logos, font and other TIMs to VRAM, the logo archive, the course map and font
// info, the arcade game data, CarInfoA, and starts reading the t002n model. Then
// the two car views and logos, the course map, the fonts, the music, both pads, the
// card slots, the kanji font, and enters the current page.
#ifdef NON_MATCHING
// Everything up to the car view loop is the game's code. In the loop, the game
// reloads the address of D_800454D4 each time, where GCC keeps it on the stack (a
// 0x70-byte frame for the game's 0x68), and its pointer increments and the font
// setup's stores come in another order. Index expressions instead of the pointer
// locals, and a struct for D_800454D4, are further off.
void ArcadeMenu::Enter()
{
    int lang = g_GameState.language;
    int size;
    int *mapInfo;
    int *fontInfo;
    CarAsset *asset;
    void *buffers[6];
    ArcCarView *view;
    unsigned char *model;
    unsigned char *tex;
    unsigned char *modelGz;
    unsigned char *texGz;
    unsigned char *wheel;
    int i;
    void (*enter)(ArcadeMenu *, int);

    psxViewLoop::Enter();
    SetVideoMode(100);
    PacketBuffer_Init(&packets, g_OVL2_PacketBuf, 0x20000);
    ViewDrawContext_Init(&draw, g_OVL2_OrderingTable, 0x1260);
    layers[1].priority = 1;
    layers[1].otSize = 0xc;
    layers[2].otSize = 0xc;
    layers[4].priority = 3;
    layers[5].priority = 4;
    layers[0].priority = 0;
    layers[0].otSize = 2;
    layers[2].priority = 2;
    layers[4].otSize = 0x118;
    layers[5].otSize = 0x118;
    layers[3].priority = 5;
    layers[3].otSize = 2;
    ViewDrawContext_Reset(&draw);
    ViewDrawContext_AddLayer(&draw, &layers[0]);
    ViewDrawContext_AddLayer(&draw, &layers[1]);
    ViewDrawContext_AddLayer(&draw, &layers[2]);
    ViewDrawContext_AddLayer(&draw, &layers[4]);
    ViewDrawContext_AddLayer(&draw, &layers[5]);
    ViewDrawContext_AddLayer(&draw, &layers[3]);
    Tim_LoadFile(g_OVL2_FileBuf, g_ArcadeBgFileByLang[lang]);
    Tim_UploadToPage_OVL2(g_OVL2_FileBuf, 6);
    Tim_LoadFile(g_OVL2_FileBuf, VOLID_ARCADE_ARC_MAKER_TIM);
    Tim_UploadToPage_OVL2(g_OVL2_FileBuf, 0xb);
    Tim_LoadFile(g_OVL2_FileBuf, VOLID_ARCADE_ARC_FONT_TIM);
    Tim_UploadToPage_OVL2(g_OVL2_FileBuf, 0x1e);
    Tim_LoadFile(g_OVL2_FileBuf, VOLID_ARCADE_ARC_OTHER_TIM);
    Tim_UploadToPage_OVL2(g_OVL2_FileBuf, 0xf);
    Tim_LoadFile(g_OVL2_FileBuf, g_ArcadeBg2FileByLang[lang]);
    Tim_UploadToPage_OVL2(g_OVL2_FileBuf, 0x1c);
    size = (VolId_GetSize(VOLID_ARCADE_ARC_CARLOGO) + 3) & ~3;
    mapInfo = (int *)(g_OVL2_FileBuf + size);
    size = (size + Tim_LoadFile(mapInfo, VOLID_ARCADE_COURSE_MAPINFO) + 3) & ~3;
    fontInfo = (int *)(g_OVL2_FileBuf + size);
    Tim_LoadFile(fontInfo, VOLID_ARCADE_ARC_FONTINFO);
    g_EventDataBuf = GameData_LoadArcade(D_800F8800, 0xb000);
    LoadCarInfoA(&g_OVL2_storageCarInfoA, 0x10000);
    asset = CarAsset_Find(CarCode_Encode((signed char *)"t002n"));
    Vol_ReadFileAsync(asset->file, g_ArcadeT002nModel);
    Vol_ReadFileAsync(asset->file + 1, g_ArcadeT002nTex);
    hookActive = 0;
    AsyncRead_Init_OVL2(&cdRead);
    cdBusy = 0;
    CoursePreviewMovie_Init(&g_ArcCoursePreviewMovie, D_80103800);
    view = D_800EF4A8;
    model = D_800274D4;
    tex = D_800A8D68;
    modelGz = g_OVL2_CarGzBuf;
    texGz = D_800CF4A8;
    wheel = D_800334D4;
    for (i = 0; i < 2; i++) {
        buffers[0] = model;
        buffers[1] = tex;
        buffers[2] = modelGz;
        buffers[3] = texGz;
        buffers[4] = wheel;
        buffers[5] = D_800DF4A8;
        carViews[i] = view;
        ArcCarView_Init(view, buffers, i + 0x18, D_800454D4[1], D_800454D4[4]);
        view++;
        model += 0x6000;
        tex += 0xb3a0;
        modelGz += 0x8000;
        texGz += 0x8000;
        wheel += 0x1000;
    }

    for (i = 0; i < 2; i++) {
        ArcLogo_Init(&logos[i], i + 0x16, g_OVL2_FileBuf, D_800E34A8 + i * 0x6000);
    }
    CourseMap_Init(&courseMap, 0x16, (ArcCourseMapEntry *)mapInfo, g_OVL2_CarGzBuf, D_800C74A8);
    text = &g_ArcadeText;
    Text_Init(&g_ArcadeText, 0x1e);
    text->unk1c = 1;
    D_80129830.glyphs = (FontGlyphTable *)((char *)fontInfo + fontInfo[1]);
    D_80129830.digitWidth = 12;
    D_80129830.spacing = 0;
    D_80129830.kerning = (FontKerning *)((char *)fontInfo + fontInfo[2]);
    D_80129820.glyphs = (FontGlyphTable *)((char *)fontInfo + fontInfo[3]);
    D_80129820.digitWidth = 7;
    D_80129820.spacing = 0;
    D_80129820.kerning = (FontKerning *)((char *)fontInfo + fontInfo[4]);
    D_80123810.glyphs = (FontGlyphTable *)((char *)fontInfo + fontInfo[5]);
    D_80123810.digitWidth = 5;
    D_80123810.spacing = 0;
    D_80123810.kerning = (FontKerning *)((char *)fontInfo + fontInfo[6]);
    D_80129840.glyphs = (FontGlyphTable *)((char *)fontInfo + fontInfo[7]);
    D_80129840.digitWidth = 3;
    D_80129840.spacing = 0;
    D_80129840.kerning = (FontKerning *)((char *)fontInfo + fontInfo[8]);
    ArcadeBgm_Load();
    PadDevice_Open(&pads[0], 0, g_PadHandlers);
    PadDevice_Open(&pads[1], 1, g_PadHandlers);
    PadDevice_FlushInput(&pads[0]);
    PadDevice_FlushInput(&pads[1]);
    CardSlots_Init(g_ArcCardSlotStatus);
    g_ArcKanjiFont = (KanjiFont *)D_800455DC;
    KanjiFont_Init(D_800455DC, 0x160, 0, 0x80, 0x1e0, 0x160, 0x1e1);
    logoArcReady = 0;
    CarLogoArc_Init(&g_CarLogoArcState);
    leavingPage = 0;
    transitionTimer = 0;
    activePage = pageStack[pageDepth];
    enter = pageStack[pageDepth]->enter;
    if (enter != 0) {
        enter(this, 0);
    }
    pageFrames = 0;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademenu", ArcadeMenu__Enter);
#endif

// Reads the pads, updates the card slots and the logo archive, runs the hook, then
// during a transition the leaving page, and the active page: 1 enters the page it
// pushed, 2 pops back to the previous one (16-frame transitions), 3 to 6 leave the
// menu with g_ArcadeMenuExit 1 (race), 0 (title), 2 or 3 (movies). Then the logos,
// car views, course map and preview movie. Returns 0 to leave.
int ArcadeMenu::Update()
{
    int result = 1;
    int (*update)(ArcadeMenu *, int);
    void (*enter)(ArcadeMenu *, int);
    int r;
    int i;

    psxViewLoop::Update();
    frameCount++;
    PadButtons_Read(&pads[0].buttons, &padState[0]);
    PadButtons_Read(&pads[1].buttons, &padState[1]);
    CardSlots_Update(g_ArcCardSlotStatus);
    logoArcReady = CarLogoArc_Update(&g_CarLogoArcState, this);
    pageFrames++;
    if (hookActive != 0) {
        hookActive = hook(hookArg, 1, 1);
    }
    if (transitionTimer > 0 && --transitionTimer > 0) {
        MenuPage *page = leavingPage;
        activePage = page;
        if (page != 0 && page->update != 0) {
            page->update(this, 0);
        }
    }
    activePage = pageStack[pageDepth];
    update = pageStack[pageDepth]->update;
    r = 0;
    if (update != 0) {
        r = update(this, 1);
    }
    switch (r) {
    case 0:
        break;
    case 1:
        activePage = pageStack[pageDepth];
        enter = pageStack[pageDepth]->enter;
        if (enter != 0) {
            enter(this, 0);
        }
        transitionTimer = 16;
        transitionBack = 0;
        break;
    case 2:
        if (ArcadeMenu_PopPage(this) < 0) {
            break;
        }
        activePage = pageStack[pageDepth];
        enter = pageStack[pageDepth]->enter;
        if (enter != 0) {
            enter(this, 1);
        }
        transitionTimer = 16;
        transitionBack = 1;
        break;
    case 4:
        g_ArcadeMenuExit = 0;
        result = 0;
        break;
    case 3:
        g_ArcadeMenuExit = 1;
        result = 0;
        break;
    case 5:
        g_ArcadeMenuExit = 2;
        result = 0;
        break;
    case 6:
        g_ArcadeMenuExit = 3;
        result = 0;
        break;
    }
    for (i = 0; i < 2; i++) {
        ArcLogo_Update(&logos[i], this);
    }
    for (i = 0; i < 2; i++) {
        ArcCarView_Update(carViews[i], this);
    }
    CourseMap_Update(&courseMap, this);
    CoursePreviewMovie_Update(&g_ArcCoursePreviewMovie, this);
    return result;
}

// The active and leaving pages' rectangles during a transition: they slide up
// (down when going back) as transitionTimer counts down from 16.
void ArcadeMenu_GetTransitionRects(ArcadeMenu *menu, RECT *active, RECT *leaving)
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

// The active page and, during a transition, the leaving page, each clipped to its
// sliding rectangle and faded by the transition; the black background, and the
// preview movie's frame.
void ArcadeMenu::Draw()
{
    RECT active;
    RECT leaving;
    RECT *r;
    int fade;

    psxViewLoop::Draw();
    ArcadeMenu_GetTransitionRects(this, &active, &leaving);
    fade = (transitionTimer << 7) / 16;
    if (transitionTimer > 0) {
        ArcadeMenu_DrawPage(this, leavingPage, &layers[2], fade);
        DrawLayer_SetArea(&layers[2], &leaving);
    }
    ArcadeMenu_DrawPage(this, pageStack[pageDepth], &layers[1], 0x80 - fade);
    DrawLayer_SetArea(&layers[1], &active);
    r = Prim_AddTile(layers[3].ot, 0);
    r->w = 0x160;
    r->x = 0;
    r->y = 0;
    r->h = 0x1e0;
    CoursePreviewMovie_Decode(&g_ArcCoursePreviewMovie, this);
    drawStamp = g_GfxIrq.gpuDmaDoneTime;
}

// Closes both pads and waits for the menu's CD read to stop.
void ArcadeMenu::Leave()
{
    psxViewLoopDoubleBuffer::Leave();
    PadDevice_Close(&pads[0]);
    PadDevice_Close(&pads[1]);
    while (AsyncRead_Stop(&cdRead) == 0) {
    }
}

// Empty. Not referenced.
void OVL2_Nop(void)
{
}
