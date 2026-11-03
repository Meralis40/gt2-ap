// OVL2 8001f238-80020224 (garageselect): Garage car selection page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern MenuList g_ArcList_Garage;
extern ArcGridPicker g_Arc_CarCarousel;
extern unsigned char g_Arc_CarGrowBar;
extern ArcDialog g_Arc_TransPicker;
extern ResultCarView g_Arc_CarView;

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garageselect", GarageList_DrawRow);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garageselect", GarageList_Callback);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garageselect", GarageList_HomeCallback);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garageselect", GarageList_GuestCallback);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garageselect", GarageSelect_DrawItem);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garageselect", GarageSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/garageselect", GarageSelect_Update);

// Car list, carousel, grow bar, picker and the 3D car view.
void GarageSelect_Draw(void *menu, DrawLayer *layer)
{
    unsigned int *ot;
    void *ot_00;
    TextCtx *ctx;
    int iVar1;
    RECT local_20;

    ot_00 = layer->ot;
    iVar1 = *(int *)((int)menu + 0x1c8);
    ctx = *(TextCtx **)((int)menu + 0x1c4);
    ot = (unsigned int *)((int)ot_00 + 8);
    MenuList_Draw(&g_ArcList_Garage, ot);
    GridPicker_Draw(&g_Arc_CarCarousel, ot);
    GrowBar_Draw((GrowBar *)&g_Arc_CarGrowBar, ot);
    Dialog_Draw_OVL2(&g_Arc_TransPicker, ot_00, ctx);
    if (*(int *)(iVar1 + 0x14) == 0) {
        ArcCarView_Draw(*(ArcCarView **)((int)menu + 0x228), (DrawLayer *)((int)menu + 0xbc),
                                        (DrawLayer *)((int)menu + 0xcc), (unsigned char *)&g_Arc_CarView);
    }
    local_20.x = 0x30;
    local_20.y = 0xb4;
    local_20.w = 0x100;
    local_20.h = 200;
    DrawLayer_SetArea((DrawLayer *)((int)menu + 0xbc), &local_20);
    DrawLayer_SetArea((DrawLayer *)((int)menu + 0xcc), &local_20);
}

// Camera of the 2P (and garage) car views: distance 0xb0000, smaller viewport, ground
// disc off (colour 0x404040).
void ArcCarView_InitSmall(ResultCarView *view)
{
    view->drawDisc = '\x01';
    view->rotX = 0x5e;
    view->rotY = 0x1500;
    view->pos[2] = 0xb0000;
    view->viewport[2] = 0xd2;
    view->viewport[3] = 0x90;
    view->frustum[0] = -0x69;
    view->frustum[1] = 0x69;
    view->frustum[2] = 0x43;
    view->frustum[3] = -0x11;
    view->frustum[4] = 400;
    view->discSemi = '\0';
    view->discRgb = 0x404040;
    view->rotZ = 0;
    view->unk_b4 = 0;
    view->unk_b8 = 0;
    view->unk_bc = 0;
    view->pos[1] = 0;
    view->pos[0] = 0;
    view->viewport[0] = 0;
    view->viewport[1] = 0;
    view->frustum[5] = 0x7fff;
    view->unk00 = 0;
}
}
