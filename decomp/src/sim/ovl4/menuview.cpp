// OVL4 8001d090-8001da24 (menuview): Menu view: car camera, pages and hotspots.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void CarPreview_Load(CarPreview *preview, unsigned int carId, int colorIdx, int garageSlot, int hasWheel, unsigned int wheel, int dirt);

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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_Init);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_BeginFrame);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_Update);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuPage_Enter);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_FindGarageList);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_FindUsedCarList);

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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", FUN_OVL4__8001d660);

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

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_GetCarInfoPage);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_FindDefaultElement);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_FindHotspotInDir);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menuview", MenuView_HotspotAt);
}
