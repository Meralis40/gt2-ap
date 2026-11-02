// OVL4 80013bd4-8001508c (gtmenu): GTMenu, the GT mode screen loop, and
// GTLoadingMenu, the screen shown after it. The vtables and type names of both
// classes (.rodata, after SimMap_Update's jump table), their __tf functions and
// typeinfo (.bss, which build.py's CXX_NAMES places) are emitted here.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
// Pointers into g_OVL4_SharedBuffer.
extern void *g_OVL4_WorkBuffer;     // TIM loads and the screen loops' packets (0x10000 bytes)
extern void *g_OVL4_PreviewBuffer;  // car preview
extern void *g_OVL4_UniStrBuffer;   // unistrdb strings
extern void *g_OVL4_IconBuffer;     // menu icons (GTMenuView.assets0)
extern void *g_OVL4_MenuDataBuffer; // menu data (GTMenuView.assets1)
extern unsigned char g_OVL4_OrderingTable[];
extern unsigned char g_Battle_PacketBuf[];
extern unsigned char D_800A9040[];
extern GTMenuPreviewParams D_80050918;
extern GameState g_GameState;
extern ShopTxn g_ShopTxn;
extern MenuCarList g_GarageList;
extern MenuCarList g_UsedCarList;
}

GTMenu::GTMenu()
{
    SetVideoMode(0x26);
    PacketBuffer_Init(&packets, g_OVL4_WorkBuffer, 0x10000);
    ViewDrawContext_Init(&draw, g_OVL4_OrderingTable, 0x2340);
    layers[0].priority = 1;
    layers[0].otSize = 0x10;
    layers[2].priority = 3;
    layers[2].otSize = 0x440;
    layers[1].priority = 4;
    layers[1].otSize = 4;
    ViewDrawContext_Reset(&draw);
    ViewDrawContext_AddLayer(&draw, &layers[0]);
    ViewDrawContext_AddLayer(&draw, &layers[2]);
    ViewDrawContext_AddLayer(&draw, &layers[1]);
    ViewDrawContext_ClearOT(&draw);
    ViewDrawContext_AssignOT(&draw);
    Dma_WaitOtc();
    frameParity = 0;
    g_GTMenu = this;
    noDrawSync = 1;
}

GTMenu::~GTMenu()
{
}

// Loads the cursor, item and font TIMs into VRAM pages 9, 0x18 and 10, builds the
// GT font index, inits the cursor, the shop transaction, the garage and used car
// lists, loads the current car into g_CarWork, loads the menu music bank, inits
// the view (MenuView_Init enters the saved page), opens both pads and puts the
// cursor on the default element.
void GTMenu::Enter()
{
    void *assets[3];
    GTMenuPreviewConfig preview;

    psxViewLoop::Enter();
    hookActive = 0;
    blankFrames = 2;
    Tim_LoadFile(g_OVL4_WorkBuffer, VOLID_ARCADE_GT_CURSOR_TIM);
    Tim_UploadToPage(g_OVL4_WorkBuffer, 9);
    Tim_LoadFile(g_OVL4_WorkBuffer, VOLID_ARCADE_GT_ITEMS_TIM);
    Tim_UploadToPage(g_OVL4_WorkBuffer, 0x18);
    Tim_LoadFile(g_OVL4_WorkBuffer, VOLID_ARCADE_GTMODE_FONT_TIM);
    Tim_UploadToPage(g_OVL4_WorkBuffer, 10);
    GTFont_BuildIndex();
    UniStrDb_Load(g_OVL4_UniStrBuffer, 0x6000);
    MapCursor_Init(&cursor);
    canGoBack = 1;
    listMode = 0;
    Txn_Init(&g_ShopTxn);
    GarageList_Init(&g_GarageList, 0);
    UsedCarList_Init(&g_UsedCarList, 0);
    quit = 0;
    previewDelay = 0;
    pendingPageDelay = 0;
    pendingCarDelay = 0;
    inputDelay = 0;
    g_GTMenu = this;
    if (g_GameState.garages[0].currentCar >= 0) {
        Garage_LoadCar(g_GameState.garages[0].currentCar, 0);
    }
    MenuBgm_Init();
    assets[0] = g_OVL4_IconBuffer;
    assets[1] = g_OVL4_MenuDataBuffer;
    assets[2] = g_Battle_PacketBuf;
    preview.buf = g_OVL4_PreviewBuffer;
    preview.work = D_800A9040;
    preview.buf2 = (unsigned char *)g_OVL4_PreviewBuffer + 0x6000;
    preview.unk0c = 8;
    preview.unk0e = D_80050918.unk0 >> 16;
    preview.unk10 = D_80050918.unk8;
    MenuView_Init(&view, assets, &preview);
    PadDevice_Open(&pads[0], 0, g_PadHandlers);
    PadDevice_Open(&pads[1], 1, g_PadHandlers);
    PadDevice_FlushInput(&pads[0]);
    PadDevice_FlushInput(&pads[1]);
    MapCursor_ResetToDefault(&cursor);
    inputDelay = 8;
}

// Reads the pads, runs the hook, counts down the pending page and wheel preview
// delays (input is ignored while one is pending), updates the view (car
// turntable, car wash), then by listMode: 0 map cursor (MapCursor_Update, which
// calls SimMap_Update on confirm), 1 garage list, 2 used car list (a list confirm
// calls SimMap_Update with the list element), 3 car wash (when the spin ends,
// clears the current car's dirt). Runs the menu music (stopped when quitting).
// Returns 0 once quit is set.
int GTMenu::Update()
{
    int done = 1;
    int input = 1;
    PadState *pad = padState;
    GTMenuView *v;
    MenuCarList *list;
    int result;

    psxViewLoop::Update();
    if (blankFrames > 0) {
        blankFrames--;
    }
    frameParity = (frameParity + 1) & 1;
    PadButtons_Read(&pads[0].buttons, pad);
    PadButtons_Read(&pads[1].buttons, &padState[1]);
    if (hookActive != 0) {
        hookActive = hook(hookArg, 1, 1);
    }
    if (pendingPageDelay > 0) {
        pendingPageDelay--;
    }
    if (previewDelay > 0) {
        previewDelay--;
    }
    // Not short-circuited: the original tests all three with sltiu and and.
    if ((pendingPageDelay == 0) & (pendingCarDelay == 0) & (previewDelay == 0)) {
        v = &view;
        MenuView_Update(v);
        switch (listMode) {
        case 0:
            if (inputDelay > 0) {
                input = 0;
            }
            MapCursor_Update(&cursor, this, input);
            GarageList_Update(&g_GarageList, 0, 0);
            UsedCarList_Update(&g_UsedCarList, 0, 0);
            break;
        case 3:
            MapCursor_Update(&cursor, this, 0);
            if (v->carWash == 0) {
                GameState *state = &g_GameState;
                Garage *garage = &state->garages[0];

                garage->cars[garage->currentCar].dirt = 0;
                listMode = 0;
                Sfx_PlayMenu(3);
            }
            break;
        case 1:
            MapCursor_Update(&cursor, this, 0);
            if (inputDelay > 0) {
                pad = 0;
            }
            list = &g_GarageList;
            result = GarageList_Update(list, pad, 1);
            switch (result) {
            case -1:
                break;
            case -2:
                listMode = 0;
                Sfx_PlayMenu(2);
                break;
            default:
                SimMap_Update(this, list->element, 0);
                break;
            }
            break;
        case 2:
            MapCursor_Update(&cursor, this, 0);
            if (inputDelay > 0) {
                pad = 0;
            }
            list = &g_UsedCarList;
            result = UsedCarList_Update(list, pad, 1);
            switch (result) {
            case -1:
                break;
            case -2:
                listMode = 0;
                Sfx_PlayMenu(2);
                break;
            default:
                SimMap_Update(this, list->element, 0);
                break;
            }
            break;
        }
        if (inputDelay > 0) {
            inputDelay--;
        }
    }
    if (quit != 0) {
        MenuBgm_StopIfPlaying();
        done = 0;
    }
    if (done) {
        MenuBgm_Update();
    }
    drawStartTime = Timer_SinceFrameStart();
    return done;
}

// The map cursor (list mode 0 only), then the page (MenuPage_DrawElements with
// the three layers).
#ifdef NON_MATCHING
// The game fills the listMode test's delay slot with the target's &layers[0] and
// sets MenuPage_DrawElements's arguments in order (a0, a1, a3, a2, the stack one
// last); GCC leaves the slot empty and sets a2 and the stack argument first.
// Locals for ot, the view, the parity or the model layer, casts and other forms
// of the test (!listMode, a switch, an else, a goto) didn't change either.
void GTMenu::Draw()
{
    RECT area;
    void *ot;

    psxViewLoop::Draw();
    area.w = 0x200;
    area.x = 0;
    area.y = 0;
    area.h = 0x1f8;
    ot = layers[0].ot;
    if (listMode == 0) {
        MapCursor_Draw(&cursor, ot);
    }
    DrawLayer_SetArea(&layers[0], &area);
    MenuPage_DrawElements(&view, layers[1].ot, frameParity, layers[0].ot, &layers[2]);
    DrawLayer_SetArea(&layers[1], &area);
    drawEndTime = Timer_SinceFrameStart();
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmenu", GTMenu__Draw);
#endif

// Closes both pads.
void GTMenu::Leave()
{
    psxViewLoopDoubleBuffer::Leave();
    PadDevice_Close(&pads[0]);
    PadDevice_Close(&pads[1]);
}

extern "C" {
void FUN_OVL4__800142c4(void)
{
}

// Square/triangle on the map: if canGoBack and the page has a back page, queues
// it (pendingPageArg 1 = restore list selection, delay 3); otherwise the error
// sound.
#ifdef NON_MATCHING
// Only registers differ: the game tests the page in $v0 (the copy to $a1 goes in
// the branch's delay slot) and keeps the flag in $a0 and canGoBack in $v1; GCC
// copies the page first, then loads canGoBack into $v0. A canGoBack local,
// other statement orders, a ternary, && and Sfx_PlayMenu(ok) didn't help.
void GTMenu_GoBack(GTMenu *menu)
{
    int page;
    unsigned int ok;

    page = MenuView_GetBackPage(&menu->view);
    ok = 0;
    if (page >= 0) {
        ok = menu->canGoBack & 1;
    }
    if (ok) {
        menu->pendingPageArg = 1;
        menu->pendingPage = page;
        menu->pendingPageDelay = 3;
        menu->inputDelay = 8;
        Sfx_PlayMenu(2);
        menu->inputDelay = 8;
    } else {
        Sfx_PlayMenu(0);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmenu", GTMenu_GoBack);
#endif

// Queues a MenuView_ShowCar call, run by GTMenu::BeginFrame after delay frames
// (or right after a pending page change). garageSlot -1 shows the car with its
// default setup.
void GTMenu_QueueCarPreview(GTMenu *menu, int delay, int baseCarId, int carId, int colorIdx, int garageSlot,
                            int hasWheel, int wheel, int dirt)
{
    menu->pendingCarDelay = (char)delay;
    menu->pendingCarBaseId = baseCarId;
    menu->pendingCarId = carId;
    menu->pendingCarColor = colorIdx;
    menu->pendingCarSlot = garageSlot;
    menu->pendingCarHasWheel = (signed char)hasWheel;
    menu->pendingCarWheel = wheel;
    menu->pendingCarDirt = dirt;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmenu", SimMap_Update);
}

GTLoadingMenu::GTLoadingMenu()
{
    noDrawSync = 1;
}

GTLoadingMenu::~GTLoadingMenu()
{
}

// Packets in the shared buffer, one layer, a 12-frame counter.
void GTLoadingMenu::Enter()
{
    psxViewLoop::Enter();
    SetVideoMode(0x26);
    PacketBuffer_Init(&packets, g_OVL4_WorkBuffer, 0x10000);
    ViewDrawContext_Init(&draw, g_OVL4_OrderingTable, 0x2340);
    layer.priority = 0;
    layer.otSize = 0x20;
    ViewDrawContext_Reset(&draw);
    ViewDrawContext_AddLayer(&draw, &layer);
    frames = 12;
}

// Counts down; returns 0 (leave) when the counter goes below 0.
int GTLoadingMenu::Update()
{
    psxViewLoop::Update();
    return --frames >= 0;
}

// Full-screen tile 0x202020.
void GTLoadingMenu::Draw()
{
    RECT *r;

    psxViewLoop::Draw();
    r = Prim_AddTile(layer.ot, 0x2202020);
    r->w = 0x200;
    r->x = 0;
    r->y = 0;
    r->h = 0x1e0;
    Prim_AddDrawMode(layer.ot, 0x40);
}
