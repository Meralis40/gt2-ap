// OVL2 8001e53c-8001f238 (carselect): Car selection page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern ArcGridPicker g_Arc_CarCarousel;
extern unsigned char g_Arc_CarGrowBar;
extern ArcDialog g_Arc_TransPicker;
extern ArcDialog g_Arc_SettingPicker;
extern ResultCarView g_Arc_CarView;

// Same as ResultCarView_Init (OVL0) with the 1P car selection camera: distance
// 0x94ccc, pitch 0x5e, yaw 0x1500, ground disc on (colour 0xa2a2a2).
void ArcCarView_InitLarge(ResultCarView *view)
{
    view->drawDisc = '\x01';
    view->discSemi = '\x01';
    view->rotX = 0x5e;
    view->rotY = 0x1500;
    view->viewport[2] = 0x100;
    view->viewport[3] = 0xf0;
    view->frustum[0] = -0x80;
    view->frustum[1] = 0x80;
    view->frustum[2] = 0x5a;
    view->frustum[3] = -0x32;
    view->frustum[4] = 400;
    view->discRgb = 0xa2a2a2;
    view->rotZ = 0;
    view->unk_b4 = 0;
    view->unk_b8 = 0;
    view->unk_bc = 0;
    view->pos[1] = 0;
    view->pos[0] = 0;
    view->pos[2] = 0x94ccc;
    view->viewport[0] = 0;
    view->viewport[1] = 0;
    view->frustum[5] = 0x7fff;
    view->unk00 = 0;
}

// Copy of ResultCarView_Spin: turns the car 0x10 per frame; after frame 0xfb90 also
// pulls the camera back.
void ResultCarView_Spin_OVL2(ResultCarView *view, PadState *pad, int frame)
{
    view->rotY = view->rotY + 0x10U & 0x3fff;
    if (0xfb90 < frame) {
        view->pos[2] = view->pos[2] + 0x8000;
    }
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carselect", CarSelect_DrawItem);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carselect", CarSelect_Enter);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carselect", CarSelect_Update);

// Carousel, grow bar, pickers and the 3D car view.
void CarSelect_Draw(void *menu, DrawLayer *layer)
{
    void *ot;
    TextCtx *ctx;
    int iVar1;
    RECT local_20;

    ot = layer->ot;
    iVar1 = *(int *)((int)menu + 0x1c8);
    ctx = *(TextCtx **)((int)menu + 0x1c4);
    GridPicker_Draw(&g_Arc_CarCarousel, (unsigned int *)((int)ot + 8));
    GrowBar_Draw((GrowBar *)&g_Arc_CarGrowBar, (unsigned int *)((int)ot + 8));
    Dialog_Draw_OVL2(&g_Arc_TransPicker, ot, ctx);
    Dialog_Draw_OVL2(&g_Arc_SettingPicker, ot, ctx);
    if (*(int *)(iVar1 + 0x24) == 0) {
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
}
