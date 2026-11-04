// OVL2 80015ab4-80016ad4 (arccarview): Arcade 3D car preview.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" unsigned int g_VolLba;

static inline void Translate(MATRIX *m, int x, int y, int z)
{
    VECTOR v;

    v.vx = x;
    v.vy = y;
    v.vz = z;
    Matrix_Translate(m, &v);
}

static inline void Camera_SetViewport(Camera *cam, int x, int y, int w, int h)
{
    cam->vpX = x;
    cam->vpY = y;
    cam->width = w;
    cam->height = h;
}

static inline Car *Garage_Car(Garage *garage, int slot)
{
    return garage->cars + slot;
}

extern "C" {
extern int g_ArcCarView_ModelStep;
extern int g_ArcCarView_TexStep;
extern GameState g_GameState;
extern short g_WheelOffsetTable[];
void GroundDisc_Init_OVL2(GroundDisc *disc, int unused);
void GroundDisc_Draw_OVL2(GroundDisc *disc, void *ot, Camera *camera, int blend, unsigned int rgb);
void ArcCarView_SetColor(ArcCarView *view, int color);
int ArcCarView_GetColorCount(ArcCarView *view);

#ifdef NON_MATCHING
// The menu hook that inflates what ArcCarView_Update reads: active is 0 when the hook
// is cancelled. Each stage waits for enough bytes to arrive.
int ArcCarView_DecodeHook(ArcCarView *view, int active, int phase)
{
    WheelDims front;
    WheelDims rear;
    RECT rect;
    int slot = view->vramSlot;
    ArcCarBuffers *b = &view->buf;
    unsigned int need;
    unsigned int size;

    if (active == 0) {
        view->hookActive = 0;
        return 0;
    }
    switch (view->decodeState) {
    case 0:
        if (phase != 0) {
            need = 0x800;
            size = view->modelSize;
            if (size < need) {
                need = size;
            }
            if (need <= view->modelBytes) {
                view->decodeState = 1;
                GzipStream_Begin(b->modelGz, b->model, g_ArcCarView_ModelStep, b->gzip);
                view->decodePos = 0;
            }
        }
        break;
    case 1:
        if (phase != 0) {
            need = view->decodePos + g_ArcCarView_ModelStep;
            size = view->modelSize;
            if (size < need) {
                need = size;
            }
            if (view->modelBytes >= need) {
                if (GzipStream_Continue() != 0) {
                    view->decodeState = 2;
                }
                view->decodePos = need;
            }
        }
        break;
    case 2:
        if (phase != 0) {
            int offset;

            CarModel_Init((CarModelHeader *)b->model, slot);
            offset = g_WheelOffsetTable[view->wheelColor];
            CarModel_GetWheelDims((CarModelHeader *)b->model, &front, &rear);
            rear.offset = offset;
            front.offset = offset;
            CarModel_BuildWheels((WheelGeom *)((char *)b->model + 0x40), &front, &rear);
            view->decodeState = 3;
        }
        break;
    case 3:
        if (phase != 0) {
            need = 0x800;
            size = view->texSize;
            if (size < need) {
                need = size;
            }
            if (view->texBytes >= need) {
                GzipStream_Begin(b->texGz, b->tex, g_ArcCarView_TexStep, b->gzip);
                view->decodeState = 4;
                view->decodePos = 0;
            }
        }
        break;
    case 4:
        if (phase == 0) {
            return 1;
        }
        need = view->decodePos + g_ArcCarView_TexStep;
        size = view->texSize;
        if (size < need) {
            need = size;
        }
        if (view->texBytes < need) {
            return 1;
        }
        if (GzipStream_Continue() != 0) {
            view->decodeState = 5;
        }
        view->decodePos = need;
        break;
    case 5:
        if (phase != 1) {
            rect.x = (slot & 0xf) << 6;
            rect.y = (slot << 4) & 0x100;
            rect.w = 0x40;
            rect.h = 0xe0;
            Gpu_TransferImage(&rect, (unsigned int *)((char *)b->tex + 0x43a0), 0);
            if (view->hasWheel != 0) {
                view->decodeState = 6;
            } else {
                ArcCarView_SetColor(view, -1);
                view->decoded = 1;
                view->hookActive = 0;
                return 0;
            }
        }
        break;
    case 6:
        if (phase != 0 && view->wheelRead != 0) {
            CarModel_SetColor((CarModelHeader *)b->model, 0, *(unsigned short *)((char *)b->wheelBuf + 0x14));
            ArcCarView_SetColor(view, -1);
            view->decoded = 1;
            view->hookActive = 0;
            return 0;
        }
        break;
    }
    return 1;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_DecodeHook);
#endif

// Uploads colour color's palette, the current one if negative.
void ArcCarView_SetColor(ArcCarView *view, int color)
{
    unsigned short clut[0x100];
    int index = color;
    int offset;

    if (index < 0) {
        index = view->colorIdx;
    }
    offset = index * 0x240 + 0x20;
    view->colorIdx = index;
    Clut_Blend(clut, (unsigned short *)((char *)view->buf.tex + offset), 0, 0);
    Clut_UploadToSlot(clut, view->vramSlot, 0);
    if (view->hasWheel != 0) {
        Tim_LoadToSlot((char *)view->buf.wheelBuf, view->vramSlot, 1);
    }
}

void ArcCarView_ApplyColor(ArcCarView *view)
{
    ArcCarView_SetColor(view, view->colorIdx);
    view->colorDirty = 0;
}

// How many colours the loaded car has, -1 if none is loaded.
int ArcCarView_GetColorCount(ArcCarView *view)
{
    return view->loaded != 0 ? *(unsigned short *)view->buf.tex : -1;
}

// Moves step colours on, wrapping; the colour is applied on the next BeginFrame.
void ArcCarView_CycleColor(ArcCarView *view, int step)
{
    int count = ArcCarView_GetColorCount(view);

    if (view->loaded != 0) {
        step += view->colorIdx;
        while (step < 0) {
            step += count;
        }
        while (step >= count) {
            step -= count;
        }
        view->colorIdx = step;
        view->colorDirty = 1;
    }
}

// Draws the car on layer, on its ground disc if the stage has one.
void ArcCarView_Draw(ArcCarView *view, DrawLayer *layer, DrawLayer *mirror, ResultCarView *stage)
{
    MATRIX m;
    Camera *cam;

    if (view->loaded == 0) {
        return;
    }
    Matrix_Identity(&m);
    Matrix_RotY(&m, -stage->rotY);
    Matrix_RotX(&m, -stage->rotX);
    Translate(&m, stage->pos[0], stage->pos[1], stage->pos[2]);
    cam = &stage->cam;
    Camera_SetViewport(cam, stage->viewport[0], stage->viewport[1], stage->viewport[2],
                       stage->viewport[3]);
    Camera_SetFrustum(cam, stage->frustum[0], stage->frustum[1], stage->frustum[2], stage->frustum[3],
                      stage->frustum[4], stage->frustum[5]);
    Camera_Update(cam, &m, layer->ot, 0x10);
    {
        CarModelInst inst;

        inst.lod = 1;
        inst.detail = 0;
        inst.unk02 = 0;
        inst.drawShadow = 1;
        inst.model = (CarModelHeader *)view->buf.model;
        inst.unk06 = view->clutY;
        inst.tpage = view->tpage;
        inst.brightness = 0x40;
        Matrix_Identity(&inst.world);
        Translate(&inst.world, 0, CarModel_GetRideHeight(inst.model), 0);
        CarInst_InitWheels(&inst);
        Matrix_Identity(&inst.shadow);
        CarModel_Draw(cam, &inst, 0);
        if (stage->drawDisc != 0) {
            Matrix_Identity(&m);
            Matrix_RotX(&m, -stage->rotX);
            Translate(&m, stage->pos[0], stage->pos[1], stage->pos[2]);
            Camera_Update(cam, &m, layer->ot, 0x10);
            GroundDisc_Draw_OVL2(&view->disc, (char *)layer->ot + 0xa0, cam, stage->discSemi != 0,
                                 stage->discRgb);
        }
        if (mirror != 0) {
            inst.drawShadow = 0;
            inst.lod = 2;
            inst.detail = 0;
            Matrix_Identity(&m);
            Matrix_RotY(&m, -stage->rotY);
            Matrix_RotX(&m, stage->rotX);
            Translate(&m, stage->pos[0], stage->pos[1], stage->pos[2]);
            m.m[0][1] = -m.m[0][1];
            m.m[1][1] = -m.m[1][1];
            m.m[2][1] = -m.m[2][1];
            Camera_Update(cam, &m, mirror->ot, 0x10);
            Matrix_Identity(&inst.world);
            Translate(&inst.world, 0, CarModel_GetRideHeight(inst.model), 0);
        }
    }
}

void ArcCarView_Init(ArcCarView *view, ArcCarBuffers *buffers, int vramSlot, unsigned short clutY,
                     unsigned short tpage)
{
    view->clutY = clutY;
    view->vramSlot = vramSlot;
    view->tpage = tpage;
    view->buf = *buffers;
    view->loaded = 0;
    view->perfDone = 0;
    view->loadState = 0;
    view->hookActive = 0;
    view->hookPending = 0;
    view->cdBusy = 0;
    view->cancel = 0;
    view->firstFile = 0;
    view->colorDirty = 0;
    view->colorIdx = 0;
    GroundDisc_Init_OVL2(&view->disc, 0xa0000);
}

// Starts loading car carId.
void ArcCarView_Begin(ArcCarView *view, ArcadeMenu *menu, unsigned int carId)
{
    CarAsset *asset = CarAsset_Find(carId);
    unsigned short file;

    ArcCarView_Unload(view, menu);
    file = asset->file;
    view->carId = carId;
    view->modelRead = 0;
    view->texRead = 0;
    view->wheelRead = 0;
    view->modelBytes = 0;
    view->texBytes = 0;
    view->wheelBytes = 0;
    view->firstFile = file;
    view->modelSize = Vol_GetSize(view->firstFile);
    view->texSize = Vol_GetSize(view->firstFile + 1);
    view->wheelSize = 0;
    view->loadState = 1;
    view->hasWheel = 0;
}

#ifdef NON_MATCHING
// Loads the car in slot of garage, its stats from the garage rather than computed.
void ArcCarView_LoadGarageCar(ArcCarView *view, ArcadeMenu *menu, int garage, int slot)
{
    GameState *state = &g_GameState;
    Garage *g = (Garage *)((char *)state + (garage * sizeof(Garage) + 0x3c74));
    unsigned int carId = Garage_Car(g, slot)->baseCarId;
    int color = Garage_Car(g, slot)->color;
    unsigned int wheel = Garage_Car(g, slot)->setup.wheel;
    unsigned short *rgb;
    char *codes;
    int count;
    int i;

    ArcCarView_Begin(view, menu, carId);
    view->colorIdx = 0;
    count = CarInfoA_GetColors(carId, &rgb, &codes);
    for (i = 0; i < count; i++) {
        if ((signed char)codes[i] == color) {
            view->colorIdx = i;
        }
    }
    view->setup = Garage_Car(g, slot)->setup;
    view->wheelColor = 2;
    view->params.drivetrain = Garage_Car(g, slot)->stat94 >> 13;
    view->perf.maxPowerRpm = 0;
    view->params.weight = Garage_Car(g, slot)->stat94 & 0x1fff;
    view->perf.maxTorqueRpm = 0;
    view->perf.maxPower = Garage_Car(g, slot)->flags98 & 0x3fff;
    view->hasPerf = 0;
    view->perf.maxTorque = Garage_Car(g, slot)->stat96;
    if ((wheel & ~0x1f00) != 0) {
        view->hasWheel = 1;
        view->wheelFile = Wheel_GetFileId(wheel & ~0x1f00);
        view->wheelSize = Vol_GetSize(view->wheelFile);
        view->wheelColor = (wheel & 0x1f00) >> 8;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_LoadGarageCar);
#endif

// Loads the car of car code code (CarCode_Encode) with colour color modulo its colour
// count, default setup and params (perf computed after the load).
void ArcCarView_LoadCarCode(ArcCarView *view, ArcadeMenu *menu, char *code, int color)
{
    unsigned int carId;
    int iVar1;
    unsigned short *puStack_18;
    char *pcStack_14;

    carId = CarCode_Encode(code);
    ArcCarView_Begin(view, menu, carId);
    iVar1 = CarInfoA_GetColors(carId, &puStack_18, &pcStack_14);
    view->colorIdx = (char)(color % iVar1);
    Car_GetDefaultSetup(carId, &view->setup);
    CarParams_BuildFromSetup(&view->setup, &view->params);
    view->wheelColor = 2;
    view->hasPerf = 1;
}

// Stops drawing the car and whatever is loading it.
void ArcCarView_Unload(ArcCarView *view, ArcadeMenu *menu)
{
    view->loaded = 0;
    view->perfDone = 0;
    if (view->hookActive != 0) {
        ArcadeMenu_CancelHook(menu);
    }
    view->hookPending = 0;
    if (view->cdBusy != 0) {
        view->cancel = 1;
    }
    view->loadState = 0;
}

#ifdef NON_MATCHING
// Runs the load: the model, textures and wheel come off the CD one after another,
// while the decode hook inflates them.
void ArcCarView_Update(ArcCarView *view, ArcadeMenu *menu)
{
    AsyncRead *read = &menu->cdRead;
    int status;

    if (view->cdBusy & view->cancel) {
        menu->cdRead.busy = 0;
        read->state = 0;
        view->cancel = 0;
        menu->cdBusy = 0;
        view->cdBusy = 0;
        return;
    }
    if (view->hookPending != 0) {
        if (ArcadeMenu_SetHook(menu, (unsigned char (*)(void *, int, int))ArcCarView_DecodeHook, view) != 0) {
            view->hookActive = 1;
            view->hookPending = 0;
            view->decodePos = 0;
        }
    }
    switch ((signed char)(view->loadState - 1)) {
    case 0:
        if (view->perfDone == 0 && view->hasPerf != 0) {
            CarParams_GetPerformance(&view->params, &view->perf);
        }
        view->perfDone = 1;
        if (view->cancel == 0 && menu->cdBusy == 0) {
            menu->cdBusy = 1;
            view->cdBusy = 1;
            view->hookPending = 1;
            view->decoded = 0;
            view->decodeState = 0;
            view->loadState++;
        }
        break;
    case 1:
    {
        int sector = g_VolLba + Vol_GetSector(view->firstFile);

        if (AsyncRead_Start_OVL2(read, view->buf.modelGz, sector, Vol_GetSize(view->firstFile)) == 0) {
            break;
        }
        view->loadState++;
    }
    case 2:
    {
        int size = Vol_GetSize(view->firstFile);
        int result = AsyncRead_Poll_OVL2(read, &status);

        if (status < 0) {
            view->modelBytes = 0;
        } else {
            view->modelBytes = size - status;
        }
        switch (result) {
        case 0:
            view->loadState = 2;
            break;
        case 2:
            view->modelRead = 1;
            view->loadState++;
            break;
        }
        break;
    }
    case 3:
    {
        int sector = g_VolLba + Vol_GetSector(view->firstFile + 1);

        if (AsyncRead_Start_OVL2(read, view->buf.texGz, sector, Vol_GetSize(view->firstFile + 1)) == 0) {
            break;
        }
        view->loadState++;
    }
    case 4:
    {
        int size = Vol_GetSize(view->firstFile + 1);
        int result = AsyncRead_Poll_OVL2(read, &status);

        if (status < 0) {
            view->texBytes = 0;
        } else {
            view->texBytes = size - status;
        }
        switch (result) {
        case 0:
            view->loadState = 4;
            break;
        case 2:
            view->texRead = 1;
            view->loadState++;
            if (view->hasWheel == 0) {
                menu->cdBusy = 0;
                view->cdBusy = 0;
                view->loadState = 8;
            }
            break;
        }
        break;
    }
    case 5:
    {
        int sector = g_VolLba + Vol_GetSector(view->wheelFile);

        if (AsyncRead_Start_OVL2(read, view->buf.wheelBuf, sector, Vol_GetSize(view->wheelFile)) == 0) {
            break;
        }
        view->loadState++;
    }
    case 6:
    {
        int size = Vol_GetSize(view->wheelFile);
        int result = AsyncRead_Poll_OVL2(read, &status);

        if (status < 0) {
            view->wheelBytes = 0;
        } else {
            view->wheelBytes = size - status;
        }
        switch (result) {
        case 0:
            view->loadState = 6;
            break;
        case 2:
            menu->cdBusy = 0;
            view->cdBusy = 0;
            view->wheelRead = 1;
            view->loadState++;
            break;
        }
        break;
    }
    case 7:
        if (view->decoded == 0) {
            break;
        }
        view->loaded = 1;
    default:
        view->loadState = 0;
        break;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arccarview", ArcCarView_Update);
#endif

// Inflate steps 0x17d0 (model) / 9000 (textures) bytes per frame.
void ArcCarView_SetFastDecode(ArcCarView *view)
{
    g_ArcCarView_ModelStep = 0x17d0;
    g_ArcCarView_TexStep = 9000;
}

// Inflate steps 3000 / 5000 bytes per frame.
void ArcCarView_SetSlowDecode(ArcCarView *view)
{
    g_ArcCarView_ModelStep = 3000;
    g_ArcCarView_TexStep = 5000;
}

int ArcCarView_GetDrivetrainIcon(ArcCarView *view)
{
    int icon = 0;

    switch (view->params.drivetrain) {
    case 0:
        icon = 1;
        break;
    case 3:
        icon = 3;
        break;
    case 2:
        icon = 2;
        break;
    case 4:
        icon = 4;
        break;
    }
    return icon;
}

// out: perf words 0-3 (max power, its rpm, max torque, its rpm) and the weight (params
// +0x5a).
void ArcCarView_GetStats(ArcCarView *view, unsigned short *out)
{
    out[4] = (view->params).weight;
    out[1] = (view->perf).maxPowerRpm;
    *out = (view->perf).maxPower;
    out[3] = (view->perf).maxTorqueRpm;
    out[2] = (view->perf).maxTorque;
}
}
