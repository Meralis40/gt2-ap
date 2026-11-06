// OVL4 8001a454-8001b3ac (carpreview): The car preview of the GT mode menus: the
// car's model on its ground disc, its maker logo, its colours (chips, names, the
// texture CLUT) and the spec sheet of a dealer or garage car.
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern CarSpecStrings g_CarSpecStrings;
extern short g_WheelOffsetTable[];
extern unsigned short g_CarPreview_NoText[];

}

inline void Preview_SetVector(VECTOR *v, int x, int y, int z)
{
    v->vx = x;
    v->vy = y;
    v->vz = z;
}

// The colour list is read through a cursor that the game keeps in memory.
inline unsigned short Preview_ReadHalf(unsigned short **cursor)
{
    unsigned short v = **cursor;

    (*cursor)++;
    return v;
}

extern "C" {
// The colour code (letter) of the preview's colour, '-' when nothing is loaded.
int CarPreview_GetColorCode(CarPreview *preview)
{
    int code = '-';

    if (preview->loaded) {
        unsigned short *rgb;
        signed char *codes;

        CarInfoA_GetColors(preview->carId, &rgb, (char **)&codes);
        code = codes[preview->colorIdx];
    }
    return code;
}

// The name of the preview's colour, empty when nothing is loaded.
void CarPreview_GetColorNameW(CarPreview *car, unsigned short *out)
{
    *out = 0;
    if (car->loaded) {
        CarColor_GetNameW(out, car->carId, CarPreview_GetColorCode(car));
    }
}

// The car texture's colour count, -1 when nothing is loaded.
int CarPreview_GetColorCount(CarPreview *preview)
{
    if (!preview->loaded) {
        return -1;
    }
    return *(unsigned short *)preview->texBuf;
}

// Moves the colour by delta, wrapping around; CarPreview_ApplyColor shows it.
void CarPreview_CycleColor(CarPreview *preview, int delta)
{
    int count = CarPreview_GetColorCount(preview);

    if (preview->loaded) {
        delta += preview->colorIdx;
        while (delta < 0) {
            delta += count;
        }
        while (delta >= count) {
            delta -= count;
        }
        preview->colorIdx = delta;
        preview->colorDirty = 1;
    }
}

// Shows colour colorIdx: uploads the texture's CLUT for it to the preview's slot.
void CarPreview_SetColor(CarPreview *preview, int colorIdx)
{
    if (preview->loaded) {
        unsigned short clut[256];

        preview->colorIdx = colorIdx;
        Clut_Blend(clut, (unsigned short *)((char *)preview->texBuf + (colorIdx * 0x240 + 0x20)), 0, 0);
        Clut_UploadToSlot(clut, (signed char)preview->vramPage, 0);
    }
}

void CarPreview_ApplyColor(CarPreview *preview)
{
    CarPreview_SetColor(preview, preview->colorIdx);
    preview->colorDirty = 0;
}

// The maker logo centred on (cx, cy).
void CarPreview_DrawLogo(CarPreview *car, void *ot, short cx, short cy)
{
    int w = car->logoW;
    int h = car->logoH;
    PrimSprite *s = Prim_AddSprite(ot, 0x808080);

    s->x = cx - (w >> 1);
    s->y = cy - (h >> 1);
    s->clut = 0x2924;
    s->u = 0;
    s->v = 0xa5;
    s->w = w;
    s->h = h;
    Prim_AddDrawMode(ot, 9);
}

// One chip per colour from right leftwards, the current one outlined brighter.
// Registers: the game keeps ot in $s3 and y + 8 in $s4, GCC the other way round,
// and reloads car after the tile's stores. Tried: the top and bottom y as locals
// (int or short, declared either way round) or computed in the loop, short or int
// parameters, the colour read through an inline cursor or *rgb++.
void CarPreview_DrawColorChips(CarPreview *car, void *ot, int right, int y)
{
    if (car->loaded) {
        unsigned short *rgb;
        signed char *codes;
        int count;
        int i = 0;
        int top = y - 1;
        int bottom = y + 8;

        count = CarInfoA_GetColors(car->carId, &rgb, (char **)&codes);
        while (1) {
            GouraudQuad q;
            unsigned int c;
            int k;
            RECT *r;
            PrimLineF4 *l;

            right -= 12;
            if (i >= count) {
                break;
            }
            c = *rgb++;
            q.rect.x = right;
            q.rect.y = y;
            q.rect.w = 10;
            q.rect.h = 8;
            q.top = 0;
            q.bottom = (c & 0x1f) << 3 | (c & 0x3e0) << 6 | (c & 0xf800) << 9;
            Menu_AddGouraudQuad((uint *)ot, &q);
            r = Prim_AddTile(ot, 0);
            r->x = right;
            r->y = y;
            r->w = 10;
            r->h = 8;
            k = 0x46;
            if (i == car->colorIdx) {
                k = 0xb4;
            }
            l = Prim_AddLineF4(ot, k | k << 8 | k << 16);
            i++;
            l->x0 = l->x3 = l->x4 = right - 1;
            l->x1 = l->x2 = right + 10;
            l->y0 = l->y1 = l->y4 = top;
            l->y2 = l->y3 = bottom;
        }
        Prim_AddDrawMode(ot, 0x220);
    }
}

// The car seen from cam, darkened by its dirt, then the ground disc.
// Registers: the game keeps &view in $s0 for each translate and Camera_Update, and
// rebuilds &v each time; GCC keeps &v in $s2 (one more saved register). The
// camera position matches only when its fields are passed through an inline
// setter (else GCC interleaves each load with its store), and the screen and
// frustum copies only through int locals (else lhu). Tried: one VECTOR per block,
// an inline that builds the translation, a 12-byte struct copy.
void CarPreview_DrawModel(CarPreview *car, DrawLayer *layer, CarCamera *cam)
{
    if (car->loaded) {
        MATRIX view;
        VECTOR v;
        CarModelInst inst;
        int dirt;

        Matrix_Identity(&view);
        Matrix_RotY(&view, -(short)cam->yaw);
        Matrix_RotX(&view, -cam->distance);
        Preview_SetVector(&v, cam->field9C, cam->fieldA0, cam->fieldA4);
        Matrix_Translate(&view, &v);
        {
            int s0 = cam->screen[0];
            int s1 = cam->screen[1];
            int s2 = cam->screen[2];
            int s3 = cam->screen[3];
            int l = cam->frustum[0];
            int r = cam->frustum[1];
            int t = cam->frustum[2];
            int b = cam->frustum[3];

            ((short *)cam->pad00)[0] = s0;
            ((short *)cam->pad00)[1] = s1;
            ((short *)cam->pad00)[2] = s2;
            ((short *)cam->pad00)[3] = s3;
            Camera_SetFrustum((Camera *)cam, l, r, t, b, cam->fieldCC, cam->fieldCE);
        }
        Camera_Update((Camera *)cam, &view, layer->ot, 0x10);
        inst.lod = 1;
        inst.detail = 0;
        inst.unk02 = 0;
        inst.drawShadow = 1;
        inst.model = (CarModelHeader *)car->modelBuf;
        inst.unk06 = car->x;
        inst.tpage = car->y;
        inst.brightness = 0x40;
        dirt = car->dirt;
        if (dirt >= 0) {
            if (dirt > 0x1000) {
                dirt = 0x1000;
            }
            inst.brightness = (0x1000 - dirt) / 64;
        }
        Matrix_Identity(&inst.world);
        v.vy = CarModel_GetRideHeight(inst.model);
        v.vx = 0;
        v.vz = 0;
        Matrix_Translate(&inst.world, &v);
        CarInst_InitWheels(&inst);
        Matrix_Identity(&inst.shadow);
        CarModel_Draw((Camera *)cam, &inst, 0);
        if (cam->fieldD0) {
            Matrix_Identity(&view);
            Matrix_RotX(&view, -cam->distance);
            Preview_SetVector(&v, cam->field9C, cam->fieldA0, cam->fieldA4);
            Matrix_Translate(&view, &v);
            Camera_Update((Camera *)cam, &view, layer->ot, 0x10);
            GroundDisc_Draw((GroundDisc *)car->model2c, (uint *)((char *)layer->ot + 0xa0), (uchar *)cam, cam->fieldD1 != 0, cam->ambient);
        }
    }
}

// Nothing loaded yet, at the position and with the buffers of config.
void CarPreview_Init(CarPreview *preview, CarPreviewConfig *config)
{
    preview->isCurrentCar = 0;
    preview->x = config->x;
    preview->y = config->y;
    preview->vramPage = config->vramPage;
    *(PreviewBuffers *)&preview->modelBuf = config->bufs; // one copy: three loads, then three stores
    preview->loaded = 0;
    preview->colorIdx = 0;
    preview->colorDirty = 0;
    GroundDisc_Init((GroundDisc *)preview->model2c, 0x60000);
}

void FUN_OVL4__8001ab64(int param_1)
{
    *(unsigned char *)(param_1 + 0x1c) = 0;
}

// Shows the garage's current car (with its colour, wheels and dirt) unless it is
// shown already.
// Scheduling: the game loads the wheel before the dirt for CarPreview_Load's stack
// arguments, GCC the other way round. Tried: the colour, wheel and dirt as locals
// in either order, the dirt and wheel parameters as other types.
void CarPreview_ShowCurrentCar(CarPreview *preview)
{
    GameState *state = &g_GameState;
    Garage *garage = &state->garages[0];
    int slot = garage->currentCar;

    if (slot >= 0 && !preview->isCurrentCar) {
        Car *car = garage->cars + slot;
        unsigned int carId = car->baseCarId;

        CarPreview_Load(preview, carId, CarColor_FindIndex(carId, car->color), slot, 1, car->setup.wheel, car->dirt);
    }
}

// Loads carId's logo, model and texture, shows colour colorIdx and the wheels (when
// hasWheel and wheel is not the stock one), and fills the spec sheet: from the
// default setup for a dealer car (garageSlot < 0), else from the garage car.
// Scheduling and registers: the game stores colorIdx and loads the VRAM page
// after the three buffer loads and the first file's store, where GCC moves them
// above; in the garage branch it swaps $s2 and $s3 (the garage and the perf
// pointer). The VRAM page matches as vramPage << 24 with slot = page >> 24 and
// (page >> 20) & 0x100. Tried: the buffers read before or after the first file,
// several orders of the first stores, the first file and the logo through locals.
void CarPreview_Load(CarPreview *preview, unsigned int carId, int colorIdx, int garageSlot, int hasWheel,
                     unsigned int wheel, int dirt)
{
    CarAsset *asset = CarAsset_Find(carId);
    void *model;
    void *tex;
    void *wheelBuf;
    WheelDims front;
    WheelDims rear;
    RECT r;
    TimBlock *b;
    int page;
    int slot;

    preview->firstFile = asset->file;
    model = preview->modelBuf;
    tex = preview->texBuf;
    wheelBuf = preview->wheelBuf;
    preview->colorIdx = colorIdx;
    page = preview->vramPage << 24;
    slot = page >> 24;
    preview->isCurrentCar = 0;
    preview->dirt = dirt;
    preview->logoFile = asset->logoFile;
    preview->carId = carId;
    Vol_ReadFile(preview->logoFile, model);
    b = (TimBlock *)((char *)model + 8);
    r.x = 0x240;
    r.y = 0xa4;
    r.w = b->w;
    r.h = b->h;
    Tim_LoadClut(model, &r);
    b = (TimBlock *)((char *)b + ((TimBlock *)((char *)model + 8))->len);
    r.x = 0x240;
    r.y = 0xa5;
    r.w = b->w;
    r.h = b->h;
    Tim_LoadImage(model, &r);
    preview->logoW = b->w << 2;
    preview->logoH = b->h;
    Vol_ReadFileAsync(preview->firstFile, model);
    Vol_ReadFileAsync(preview->firstFile + 1, tex);
    CarModel_Init((CarModelHeader *)model, slot);
    CarModel_GetWheelDims((CarModelHeader *)model, &front, &rear);
    CarModel_BuildWheels((WheelGeom *)((char *)model + 0x40), &front, &rear);
    r.x = (slot & 0xf) << 6;
    r.y = (page >> 20) & 0x100;
    r.w = 0x40;
    r.h = 0xe0;
    Gpu_TransferImage(&r, (unsigned int *)((char *)tex + 0x43a0), 0);
    preview->loaded = 1;
    CarPreview_SetColor(preview, preview->colorIdx);
    if (hasWheel && (wheel & ~0x1f00)) {
        Wheel_LoadTim(wheel, wheelBuf);
        Tim_LoadToSlot((char *)wheelBuf, slot, 1);
        CarModel_SetColor((CarModelHeader *)model, 0, *(unsigned short *)((char *)wheelBuf + 0x14));
    }
    if (garageSlot < 0) {
        CarSetup setup;

        Car_GetDefaultSetup(carId, &setup);
        CarParams_BuildFromSetup(&setup, preview->carParams);
        CarParams_GetPerformance((CarParams *)preview->carParams, (EnginePerf *)preview->perf);
        CarPreview_SpecFromDefault(&preview->spec, carId);
    } else {
        GameState *state = &g_GameState;
        Garage *garage = &state->garages[0];
        Car *car = garage->cars + garageSlot;
        CarSetup *setup = &car->setup;

        CarParams_BuildFromSetup(setup, preview->carParams);
        CarParams_GetPerformance((CarParams *)preview->carParams, (EnginePerf *)preview->perf);
        CarPreview_SpecFromCar(&preview->spec, car->carId, setup, preview->carParams, preview->perf);
        if (garageSlot == garage->currentCar) {
            preview->isCurrentCar = 1;
        }
    }
}

// Puts wheel on the preview car, with the offset of wheelColor.
void CarPreview_SetWheel(CarPreview *preview, unsigned int wheel, int wheelColor)
{
    void *wheelBuf = preview->wheelBuf;
    void *model = preview->modelBuf;
    WheelDims front;
    WheelDims rear;
    int offset;

    Wheel_LoadTim(wheel, wheelBuf);
    Tim_LoadToSlot((char *)wheelBuf, (signed char)preview->vramPage, 1);
    CarModel_SetColor((CarModelHeader *)model, 0, *(unsigned short *)((char *)wheelBuf + 0x14));
    offset = g_WheelOffsetTable[wheelColor];
    CarModel_GetWheelDims((CarModelHeader *)model, &front, &rear);
    rear.offset = offset;
    front.offset = offset;
    CarModel_BuildWheels((WheelGeom *)((char *)model + 0x40), &front, &rear);
}

// Fills the spec sheet from the car's default setup (GameData tables 3, 5, 6, 0xd and
// the type 0x1e car entry).
void CarPreview_SpecFromDefault(CarSpecSheet *spec, int carId)
{
    void *pvVar1;
    unsigned int *puVar2;
    void *pvVar3;
    void *pvVar4;
    void *pvVar5;
    CarSetup CStack_a0;

    pvVar1 = GameData_GetGtMode();
    puVar2 = (unsigned int *)GameData_FindEntryById((GameData *)pvVar1, 0x1e, carId);
    Car_GetDefaultSetup(carId, &CStack_a0);
    pvVar1 = GameData_GetEntry(3, (unsigned int)(unsigned short)CStack_a0.idx_0a);
    pvVar3 = GameData_GetEntry(5, (unsigned int)(unsigned short)CStack_a0.idx_t05);
    pvVar4 = GameData_GetEntry(6, (unsigned int)(unsigned short)CStack_a0.idx_0c);
    pvVar5 = GameData_GetEntry(0xd, (unsigned int)(unsigned short)CStack_a0.idx_t0d);
    // Most racing modification variants have no table 0x1e entry: the PlayStation reads
    // addresses 0x41 and 0x44 there. A year of 0 shows as "----".
    spec->modelYear = puVar2 != 0 ? (unsigned short)*(unsigned char *)((int)puVar2 + 0x41) : 0;
    spec->length = (unsigned int)*(unsigned short *)((int)pvVar1 + 8);
    spec->width = (unsigned int)*(unsigned short *)((int)pvVar1 + 10);
    spec->height = (unsigned int)*(unsigned short *)((int)pvVar3 + 0x1a);
    spec->weight = (unsigned int)*(unsigned short *)((int)pvVar1 + 0xe);
    spec->displacement = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x2c);
    spec->drivetrain = (unsigned int)*(unsigned char *)((int)pvVar5 + 8);
    spec->engineStrA = (unsigned int)*(unsigned short *)((int)pvVar4 + 4);
    spec->engineStrB = (unsigned int)*(unsigned short *)((int)pvVar4 + 8);
    spec->engineStrC = (unsigned int)*(unsigned short *)((int)pvVar4 + 6);
    spec->power = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x2e);
    spec->powerRpm = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x30) * 10;
    spec->torque = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x32);
    spec->torqueRpmStr = (unsigned int)*(unsigned short *)((int)pvVar4 + 0x34);
    spec->price = puVar2 != 0 ? puVar2[0x11] : 0;
}

// Fills the spec sheet of a garage car from its setup, its parameters (the weight)
// and its performance (power and its rpm, torque, and the max rpm as text).
void CarPreview_SpecFromCar(CarSpecSheet *spec, int carId, CarSetup *setup, void *carParams, unsigned short *perf)
{
    unsigned int *entry = (unsigned int *)GameData_FindEntryById((GameData *)GameData_GetGtMode(), 0x1e, carId);
    unsigned char *body = (unsigned char *)GameData_GetEntry(3, (unsigned short)setup->idx_0a);
    unsigned char *t05 = (unsigned char *)GameData_GetEntry(5, (unsigned short)setup->idx_t05);
    unsigned char *engine = (unsigned char *)GameData_GetEntry(6, (unsigned short)setup->idx_0c);
    unsigned char *t0d = (unsigned char *)GameData_GetEntry(0xd, (unsigned short)setup->idx_t0d);
    char text[64];
    int i;

    spec->modelYear = ((unsigned char *)entry)[0x41];
    spec->length = *(unsigned short *)(body + 8);
    spec->width = *(unsigned short *)(body + 10);
    spec->height = *(unsigned short *)(t05 + 0x1a);
    spec->displacement = *(unsigned short *)(engine + 0x2c);
    spec->drivetrain = t0d[8];
    spec->engineStrA = *(unsigned short *)(engine + 4);
    spec->engineStrB = *(unsigned short *)(engine + 8);
    spec->engineStrC = *(unsigned short *)(engine + 6);
    spec->weight = ((CarParams *)carParams)->weight;
    spec->power = perf[0];
    spec->powerRpm = perf[1];
    spec->torque = perf[2];
    spec->torqueRpmStr = -1;
    sprintf(text, g_CarSpecStrings.rpm, perf[3]);
    for (i = 0; i < (int)strlen(text); i++) {
        spec->rpmText[i] = (signed char)text[i];
    }
    spec->rpmText[i] = 0;
    spec->price = entry[0x11];
}

unsigned short *CarPreview_GetEngineTextA(CarPreview *car)
{
    if (!car->loaded) {
        return g_CarPreview_NoText;
    }
    return (unsigned short *)UniStr_Get(car->spec.engineStrA);
}

unsigned short *FUN_OVL4__8001b2f8(CarPreview *car)
{
    if (!car->loaded) {
        return g_CarPreview_NoText;
    }
    return (unsigned short *)UniStr_Get(car->spec.engineStrB);
}

unsigned short *CarPreview_GetEngineTextC(CarPreview *car)
{
    if (!car->loaded) {
        return g_CarPreview_NoText;
    }
    return (unsigned short *)UniStr_Get(car->spec.engineStrC);
}

// The torque rpm text: a game string, or the one CarPreview_SpecFromCar built.
unsigned short *CarPreview_GetTorqueRpmText(CarPreview *car)
{
    int index = car->spec.torqueRpmStr;

    if (index >= 0) {
        return (unsigned short *)UniStr_Get(index);
    }
    return car->spec.rpmText;
}
}
