// OVL4 8001d090-8001da24 (menuview): Menu view: car camera, pages and hotspots.
#include "ovl4.h"

extern "C" {
void CarPreview_Load(CarPreview *preview, unsigned int carId, int colorIdx, int garageSlot, int hasWheel, unsigned int wheel, int dirt);
void CarPreview_Init(CarPreview *preview, void *config);
void CarPreview_ApplyColor(CarPreview *preview);
void CarPreview_ShowCurrentCar(CarPreview *preview);
void FUN_OVL4__8001ab64(CarPreview *preview);
int MenuData_Load(void *buf, int size);
void MenuData_LoadIcons(void *buf);
void MenuData_SetBuffers(void *pageBuf, void *scratch);
void MenuPage_LoadById(int page, void *buf);
void *MenuPage_BuildBackground(void *unused, void *buf);
MenuElement *MenuPage_GetElement(int index);
int MenuPage_FindCarPage(unsigned int carId);
void OT_InsertChain(void *ot, void *head, void *tail);
void MenuBgm_Request(int track);
int MenuElement_IsActive(MenuElement *e);
int MenuElement_IsLicenseOk(MenuElement *e);
void Txn_InitWheelShop(ShopTxn *txn);
void Txn_PreviewPart(ShopTxn *txn, int part);
void Txn_SetBuyPart(ShopTxn *txn, int part);
void GarageList_Open(MenuCarList *list, MenuElement *element);
void UsedCarList_Init(MenuCarList *list, int unused);
void UsedCarList_Build(MenuCarList *list, MenuElement *element, int makerId);
int Math_ISqrt(int lo, int hi); // of a 64-bit value; callers pass 0 as the high word
void MenuPage_Enter(GTMenuView *view, int page, int keepList);
extern FlowMenuState g_Flow_IntroShown;
extern int g_MenuElementCount;
extern int g_MenuPageBackPage;
extern unsigned int g_MenuPageAttr;
extern unsigned char D_800A8D7A; // g_MenuPageAttr's third byte: the page's music track
extern ShopTxn g_ShopTxn;
extern MenuCarList g_GarageList;
extern MenuCarList g_UsedCarList;
extern DirVector g_MapCursor_DirVectors[];

// The element count through a pointer: the loop pass then hoists its address out of
// loops it would otherwise leave it in (MenuPage_Enter, MenuView_HotspotAt).
static inline int *MenuPage_Count()
{
    return &g_MenuElementCount;
}

// Default turntable camera: distance 0xa0, yaw 0x1500, ambient 0x525252.
void CarCamera_Init(CarCamera *camera)
{
    camera->fieldD0 = '\x01';
    camera->distance = 0xa0;
    camera->yaw = 0x1500;
    camera->fieldD1 = '\0';
    camera->ambient = 0x525252;
    camera->fieldAC = 0;
    camera->fieldB0 = 0;
    camera->fieldB4 = 0;
    camera->fieldB8 = 0;
    camera->fieldA0 = 0;
    camera->field9C = 0;
    camera->fieldA4 = 0x94ccc;
    camera->fieldCC = 400;
}

// Sets the distance to 0x6e and adds delta to the yaw (14 bits).
void CarCamera_Rotate(CarCamera *camera, int delta)
{
    camera->distance = 0x6e;
    camera->yaw = camera->yaw + (short)delta & 0x3fff;
}

// Starts the 64-frame car wash spin.
void MenuView_StartCarWash(GTMenuView *view)
{
    view->carWash = '\x01';
    view->carWashTimer = 0x40;
}

// Takes the three buffers, loads the menu data into them and enters the page (and
// music) the flow state says to come back to.
void MenuView_Init(GTMenuView *view, void **assets, void *previewConfig)
{
    *(ViewAssets *)&view->assets0 = *(ViewAssets *)assets;
    view->backPage = -1;
    view->prevBackPage = -1;
    view->loaded = 0;
    view->page = g_Flow_IntroShown.page;
    view->bgmTrack = g_Flow_IntroShown.bgmTrack;
    view->carBaseId = 0;
    view->carId = 0;
    view->carWash = 0;
    view->carWashTimer = 0;
    CarPreview_Init(&view->preview, previewConfig);
    CarCamera_Init(&view->camera);
    MenuData_Load(assets[1], 0x58c0);
    MenuData_LoadIcons(assets[0]);
    MenuData_SetBuffers(assets[2], assets[0]);
    MenuPage_Enter(view, view->page, 0);
    if (view->bgmTrack >= 0)
        MenuBgm_Request(view->bgmTrack);
    view->backPage = g_MenuPageBackPage;
}

// Links the background display list into the OT and applies a pending colour change.
void MenuView_BeginFrame(GTMenuView *view, void **ot)
{
    CarPreview *preview;

    OT_InsertChain(ot + 1, view->assets2, view->bgListTail);
    preview = &view->preview;
    if (preview->colorDirty)
        CarPreview_ApplyColor(preview);
}

// Turns the car; during the car wash it spins faster and the dirt goes when it ends.
void MenuView_Update(GTMenuView *view)
{
    CarCamera_Rotate(&view->camera, 0xc);
    if (view->carWash) {
        if (--view->carWashTimer <= 0) {
            view->carWash = 0;
            view->preview.dirt = 0;
        }
        CarCamera_Rotate(&view->camera, 0x71);
    }
}

// Message pages have the top bit set. Both tests in MenuPage_Enter share the
// 0x80000000: the first becomes a bgez, the second keeps its and.
static inline int MenuPage_IsMessage(int page)
{
    return page & 0x80000000;
}

// Loads a page and its background, keeps the back pages, opens the page's widgets
// (wheel shop, parts, garage and used car lists) and starts its music. A sub page
// (attribute bit 8) entered from a main page, or a message page entered from a
// normal one, goes back to the page before; other pages take the back page from
// their footer. fromMain and hasTrack as values give the game's xori; the element
// copy gives its second register for the element.
void MenuPage_Enter(GTMenuView *view, int page, int keepList)
{
    int oldPage = view->page;
    unsigned int oldAttr = g_MenuPageAttr;
    int fromMain;
    int hasTrack;
    int i;
    int track;

    view->page = page;
    MenuPage_LoadById(page, view->assets0);
    view->bgListTail = MenuPage_BuildBackground(view->assets0, view->assets2);
    view->loaded = 1;
    if (g_MenuPageAttr >> 8 & 1) {
        fromMain = oldAttr >> 8 & 1;
        fromMain ^= 1;
        if (fromMain) {
            view->prevBackPage = view->backPage;
            view->backPage = oldPage;
        }
    } else if (MenuPage_IsMessage(page)) {
        if (!MenuPage_IsMessage(oldPage)) {
            view->prevBackPage = view->backPage;
            view->backPage = oldPage;
        }
    } else {
        view->prevBackPage = view->backPage;
        view->backPage = g_MenuPageBackPage;
    }
    for (i = 0; i < *MenuPage_Count(); i++) {
        MenuElement *c = MenuPage_GetElement(i);
        MenuElement *e = c;
        int kind = e->flags & 0xffff;

        if (e->flags >> 28 & 1) {
            CarPreview_ShowCurrentCar(&view->preview);
            Txn_InitWheelShop(&g_ShopTxn);
        }
        if ((unsigned int)(kind - 20) < 51) {
            kind -= 20;
            Txn_PreviewPart(&g_ShopTxn, kind);
            Txn_SetBuyPart(&g_ShopTxn, kind);
        }
        if (kind == 9)
            GarageList_Open(&g_GarageList, e);
        if (kind == 0x50) {
            if (!keepList)
                UsedCarList_Init(&g_UsedCarList, 0);
            UsedCarList_Build(&g_UsedCarList, e, (unsigned char)g_MenuPageAttr);
        }
    }
    track = D_800A8D7A;
    hasTrack = track != 0xff;
    if (hasTrack && track != view->bgmTrack) {
        MenuBgm_Request(track);
        view->bgmTrack = track;
    }
}

// The page's garage list element (kind 9), or 0.
MenuElement *MenuView_FindGarageList(void)
{
    int i;
    MenuElement *e;

    i = 0;
    while (1) {
        if (i >= g_MenuElementCount)
            return 0;
        e = MenuPage_GetElement(i);
        i++;
        if ((unsigned short)e->flags == 9)
            return e;
    }
}

// The page's used car list element (kind 0x50), or 0.
MenuElement *MenuView_FindUsedCarList(void)
{
    int i;
    MenuElement *e;

    i = 0;
    while (1) {
        if (i >= g_MenuElementCount)
            return 0;
        e = MenuPage_GetElement(i);
        i++;
        if ((unsigned short)e->flags == 0x50)
            return e;
    }
}

void FUN_OVL4__8001d5c0(void)
{
}

// Shows a car on the turntable: stores the ids, resets the camera and loads it into
// the preview (CarPreview_Load).
void MenuView_ShowCar(GTMenuView *view, int baseCarId, int carId, int colorIdx, int garageSlot, int hasWheel, int wheel, int dirt)
{
    view->carBaseId = baseCarId;
    view->carId = carId;
    CarCamera_Init(&view->camera);
    CarPreview_Load(&view->preview, baseCarId, colorIdx, garageSlot, hasWheel, wheel, dirt);
}

void FUN_OVL4__8001d660(GTMenuView *view)
{
    FUN_OVL4__8001ab64(&view->preview);
}

// prevBackPage.
int MenuView_GetPrevBackPage(GTMenuView *view)
{
    return view->prevBackPage;
}

// backPage.
int MenuView_GetBackPage(GTMenuView *view)
{
    return view->backPage;
}

// The car info page of the car shown, or -1 before a page is loaded.
int MenuView_GetCarInfoPage(GTMenuView *view)
{
    if (!view->loaded)
        return -1;
    return MenuPage_FindCarPage(view->carId);
}

// The element the cursor starts on (flag bit 25), or 0.
MenuElement *MenuView_FindDefaultElement(GTMenuView *view)
{
    int i;

    if (!view->loaded)
        return 0;
    for (i = 0; i < g_MenuElementCount; i++) {
        MenuElement *c = MenuPage_GetElement(i);
        MenuElement *e = c;
        if (e->flags >> 25 & 1)
            return e;
    }
    return 0;
}

// The hotspot that best lines up with direction dir from (x, y): the score is the
// squared cosine over the distance. from is skipped.
MenuElement *MenuView_FindHotspotInDir(GTMenuView *view, int x, int y, int dir, MenuElement *from, int arg)
{
    int best;
    MenuElement *bestE;
    int i;
    int dirX;
    int dirY;

    if (!view->loaded)
        return 0;
    best = 0;
    bestE = 0;
    {
        DirVector &v = g_MapCursor_DirVectors[dir];
        dirX = v.x;
        dirY = v.y;
    }
    for (i = 0; i < g_MenuElementCount; i++) {
        // c is the copy that dies at MenuElement_IsActive (kept in $a0), e the one
        // kept for the rest; the skip and bad values give the game's sltiu and xori.
        MenuElement *c = MenuPage_GetElement(i);
        MenuElement *e = c;
        int dx;
        int dy;
        int len;
        int dot;
        int inv;
        long long score;

        if (c == from)
            continue;
        {
            int skip = !(c->flags >> 24 & 1) && !MenuElement_IsActive(c);
            if (skip)
                continue;
        }
        {
            int bad = MenuElement_IsLicenseOk(e) != 1;
            if (bad)
                continue;
        }
        dx = ((e->x0 + e->x1) >> 1) - x;
        dy = ((e->y0 + e->y1) >> 1) - y;
        len = Math_ISqrt(dx * dx + dy * dy, 0);
        if (len == 0)
            continue;
        dot = (dirX * ((dx << 12) / len) >> 12) + (dirY * ((dy << 12) / len) >> 12);
        inv = 0x1000000 / len;
        if (dot < 0)
            continue;
        dot = dot * dot >> 12;
        score = (long long)inv * dot;
        if (best < (int)(score >> 12)) {
            best = score >> 12;
            bestE = e;
        }
    }
    return bestE;
}

// The active hotspot whose rectangle holds (x, y), or 0.
MenuElement *MenuView_HotspotAt(GTMenuView *view, int x, int y)
{
    int i;

    if (!view->loaded)
        return 0;
    for (i = 0; i < *MenuPage_Count(); i++) {
        MenuElement *c = MenuPage_GetElement(i);
        MenuElement *e = c;
        if (MenuElement_IsActive(e)) {
            if ((e->x0 < x) & (x < e->x1)) {
                if ((e->y0 < y) & (y < e->y1))
                    return e;
            }
        }
    }
    return 0;
}
}
