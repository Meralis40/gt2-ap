// OVL4 80013bd4-8001508c (gtmenu): GTMenu, the GT mode screen loop, and
// GTLoadingMenu, the screen shown after it. The vtables and type names of both
// classes (.rodata, after SimMap_Update's jump table), their __tf functions and
// typeinfo (.bss, which build.py's CXX_NAMES places) are emitted here.
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

// A garage's cars, credits and current car, from D_801CD554.
extern unsigned char D_801CD554[];
extern short D_800A8D62;            // the car highlighted in the garage list
extern unsigned char g_MenuPageAttr;
extern char g_GtMenu_SelectedEvent[];
extern FlowMenuState g_Flow_IntroShown;

#define GARAGE_CAR(slot) ((Car *)(D_801CD554 + 4 + (slot) * 0xa4))
#define GARAGE_CREDITS (*(int *)(D_801CD554 + 0x4014))
#define GARAGE_CURRENT (*(short *)(D_801CD554 + 0x4018))
#define WHEEL_MASK 0xffffe0ff       // Car.setup's first word without its wheel colour

static void SimMap_GoToPage(GTMenu *menu, int page, int arg)
{
    menu->pendingPageDelay = 3;
    menu->pendingPage = page;
    menu->pendingPageArg = arg;
    menu->inputDelay = 8;
}

static void SimMap_QueueGarageCar(GTMenu *menu, Car *car, int slot)
{
    unsigned int wheel = *(unsigned int *)&car->setup;
    int color = CarColor_FindIndex(car->baseCarId, car->color);

    GTMenu_QueueCarPreview(menu, 1, car->baseCarId, car->carId, color, slot, (wheel & WHEEL_MASK) != 0, wheel,
                           car->dirt);
}

// Remembers the view for when the screen comes back.
static void SimMap_LeaveMenu(GTMenu *menu, int exit)
{
    FlowMenuState *flow = &g_Flow_IntroShown;

    flow->gtMenuExit = exit;
    flow->page = menu->view.page;
    flow->bgmTrack = *(short *)((char *)&menu->view + 0x550);
    menu->quit = 1;
}

// A confirmed element of the map or of a shop page. Returns 1 when the caller may go on,
// 0 when the element was a part or a sound has been played. Written by hand from the
// assembly: this function was never in C++ before the port.
int SimMap_Update(GTMenu *menu, MenuElement *element, int arg)
{
    int result = 1;
    int silent = 1;     // no sound has been played
    int page = 0;
    int current = GARAGE_CURRENT;
    int action = element->flags & 0xffff;
    int msg;

    if (element->flags & 0x1000000) {
        Car *car;
        int slot;
        unsigned int id;

        switch (action) {
        case 0x50: // used car lot: opens the list, then picks a car
            if (menu->listMode == 0) {
                if (*(short *)&g_UsedCarList > 0) {
                    menu->listMode = 2;
                    Sfx_PlayMenu(1);
                } else {
                    Sfx_PlayMenu(0);
                }
                goto events;
            }
            if (menu->listMode == 2) {
                int price;

                menu->listMode = 0;
                page = element->target;
                id = UsedCarList_GetSelectedCarId();
                GTMenu_QueueCarPreview(menu, 1, id, id, CarColor_FindIndex(id, UsedCarList_GetSelectedColor()), -1, 0,
                                       0, -1);
                price = UsedCarList_GetSelectedPrice();
                Txn_SetBuyNewCar(&g_ShopTxn, id, price);
            }
            break;
        case 9: // garage: opens the list, then picks a car
            if (menu->listMode == 0) {
                if (*(short *)&g_GarageList > 0) {
                    Sfx_PlayMenu(1);
                    menu->listMode = 1;
                } else {
                    Sfx_PlayMenu(0);
                }
                goto events;
            }
            if (menu->listMode == 1) {
                menu->listMode = 0;
                slot = D_800A8D62;
                page = element->target;
                SimMap_QueueGarageCar(menu, GARAGE_CAR(slot), slot);
                ListCarWork_Load(slot);
            }
            break;
        case 0x9c: // racing modification
            if (current < 0) {
                page = 0x80000002;
            } else if (Car_HasPart(GARAGE_CAR(current), 0x22)) {
                page = 0x8000001a;
            } else if (!RacingMod_HasVariants()) {
                page = 0x80000019;
            } else {
                page = element->target;
                id = RacingMod_FirstVariant();
                GTMenu_QueueCarPreview(menu, 1, id, id, 0, -1, 0, 0, -1);
            }
            break;
        case 0x98: // a tuning shop: only for its manufacturer's cars
            if (current < 0) {
                Txn_Clear(&g_ShopTxn);
                page = 0x80000002;
            } else if (GARAGE_CAR(current)->setup.manufacturerId == g_MenuPageAttr) {
                page = element->target;
            } else {
                Txn_Clear(&g_ShopTxn);
                page = 0x8000001d;
            }
            break;
        case 0xad:
            if (current < 0) {
                Txn_Clear(&g_ShopTxn);
                page = 0x80000002;
            } else {
                SimMap_QueueGarageCar(menu, GARAGE_CAR(current), current);
                page = element->target;
            }
            break;
        case 0xae:
            if (current < 0) {
                Txn_Clear(&g_ShopTxn);
                page = 0x80000002;
            } else if (Car_IsNotExcluded(GARAGE_CAR(current)->carId)) {
                page = element->target;
            } else {
                Txn_Clear(&g_ShopTxn);
                page = 0x8000001e;
            }
            break;
        case 0xbb:
            goto events;
        default:
            page = element->target;
            break;
        }
        Sfx_PlayMenu(1);
        silent = 0;
        SimMap_GoToPage(menu, page, 0);
    }

events:
    if ((element->flags & (1 << 27)) && !(element->flags & 0xf00000)) {
        char *code = (char *)&element->target;
        int exit = 2;

        if (element->flags & (1 << 30)) {
            exit = 3;
        } else {
            int entry = Event_IsLicense(code) ? License_CheckEntry(code, &msg) : Race_CheckEntry(code, &msg);

            if (entry != 1) {
                Sfx_PlayMenu(0);
                silent = 0;
                SimMap_GoToPage(menu, msg, 0);
                goto wheels;
            }
        }
        Sfx_PlayMenu(1);
        strcpy(g_GtMenu_SelectedEvent, code);
        silent = 0;
        SimMap_LeaveMenu(menu, exit);
    }

wheels:
    if (element->flags & (1 << 28)) {
        Car *car = GARAGE_CAR(current);
        unsigned int code = Wheel_ParseCode((signed char *)&element->target);
        int variant = Wheel_GetVariant(code, car->carId != car->baseCarId);

        silent = 0;
        Sfx_PlayMenu(1);
        Txn_SetBuyWheels(&g_ShopTxn, code);
        menu->previewDelay = 3;
        menu->inputDelay = 8;
        menu->previewWheel = code;
        menu->previewWheelColor = variant;
    }

    switch (action) {
    case 2: // yes
        g_ShopTxn.color = CarPreview_GetColorCode(&menu->view.preview);
        page = Shop_ExecuteTransaction(&g_ShopTxn);
        if (page == 0) {
            menu->canGoBack = 1;
            page = MenuView_GetBackPage(&menu->view);
        }
        SimMap_GoToPage(menu, page, 0);
        break;
    case 3: // no
        Txn_Decline(&g_ShopTxn);
        menu->canGoBack = 1;
        SimMap_GoToPage(menu, MenuView_GetBackPage(&menu->view), 0);
        break;
    case 4:
        page = Shop_CheckTransaction(&g_ShopTxn);
        if (page == 0) {
            Sfx_PlayMenu(0);
            return result;
        }
        menu->canGoBack = 0;
        SimMap_GoToPage(menu, page, 0);
        break;
    case 5:
        page = MenuView_GetCarInfoPage(&menu->view);
        if (page == 0) {
            Sfx_PlayMenu(0);
            return result;
        }
        SimMap_GoToPage(menu, page, 0);
        break;
    case 6: { // a dealer's car tile
        unsigned int id = element->arg10;

        Txn_SetBuyNewCar(&g_ShopTxn, id, 0);
        GTMenu_QueueCarPreview(menu, 1, id, id, 0, -1, 0, 0, -1);
        return result;
    }
    case 9:
    case 0x50:
        return result;
    case 11:
        SimMap_LeaveMenu(menu, 0);
        return result;
    case 13: // car wash, 50 credits
        if (!CarWash_CanAfford() || GARAGE_CAR(current)->dirt == 0) {
            Sfx_PlayMenu(0);
            return result;
        }
        GARAGE_CREDITS -= 50;
        menu->listMode = 3;
        MenuView_StartCarWash(&menu->view);
        break;
    case 16:
        menu->canGoBack = 1;
        if (Txn_Back(&g_ShopTxn)) {
            SimMap_GoToPage(menu, MenuView_GetPrevBackPage(&menu->view), 1);
        } else {
            SimMap_GoToPage(menu, MenuView_GetBackPage(&menu->view), 0);
        }
        break;
    case 18:
        SimMap_GoToPage(menu, MenuView_GetBackPage(&menu->view), 1);
        break;
    case 19:
        SimMap_GoToPage(menu, 0x8000002e, 0);
        break;
    case 0x95:
    case 0x96:
        CarPreview_CycleColor(&menu->view.preview, arg != 0 ? 1 : -1);
        break;
    case 0x9d: {
        unsigned int id = RacingMod_NextVariant();

        GTMenu_QueueCarPreview(menu, 3, id, id, 0, -1, 0, 0, -1);
        menu->inputDelay = 4;
        break;
    }
    case 0xaa:
        Txn_SetSellCar(&g_ShopTxn, D_800A8D62);
        menu->canGoBack = 0;
        SimMap_GoToPage(menu, Shop_CheckTransaction(&g_ShopTxn), 0);
        break;
    case 0xab:
        Txn_SetSelectCar(&g_ShopTxn, D_800A8D62);
        menu->canGoBack = 0;
        SimMap_GoToPage(menu, Shop_CheckTransaction(&g_ShopTxn), 0);
        break;
    default:
        if ((unsigned int)(action - 0x14) < 0x33) {
            Txn_SetBuyPart(&g_ShopTxn, action - 0x14);
            menu->canGoBack = 0;
            SimMap_GoToPage(menu, Shop_CheckTransaction(&g_ShopTxn), 0);
            break;
        }
        if (silent) {
            Sfx_PlayMenu(0);
        }
        return 0;
    }
    Sfx_PlayMenu(1);
    return result;
}
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
