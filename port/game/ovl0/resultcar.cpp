// OVL0 80048528-800489d8 (resultcar): Result car drawing.
// The functions that don't match yet are the original assembly.
#include "common.h"

extern "C" {
extern char *g_PacketPtr;
extern char *g_PacketEnd;

// Cut-down CarModel_Draw: LOD 0 body only (always the clipped path), no wheels and no
// shadow, lit by the car's rotation scaled down by 8. Used for the prize car shown on
// the license and championship prize pages.
void CarModel_DrawBodyOnly(Camera *cam, CarModelInst *inst)
{
    CarScratch *sp = (CarScratch *)0x1f800000;
    short light[12];            // the rotation part of a MATRIX: Matrix_Mul writes no translation
    MATRIX *world;
    CarModelHeader *model;
    LodTable *table;
    int detail;
    unsigned int brightness;
    unsigned int rgb;
    CarMesh *mesh;
    unsigned int unk02;
    short unk06;
    unsigned short tpage;

    *(CameraView *)sp = *(CameraView *)&cam->view;
    model = inst->model;
    detail = inst->detail;
    unk02 = inst->unk02;
    brightness = inst->brightness;
    unk06 = inst->unk06;
    tpage = inst->tpage;
    table = (LodTable *)&model->lodCount;
    sp->mirror = 0;
    sp->detail = detail << 24;
    sp->clutSelect = unk02 << 16;
    rgb = brightness | brightness << 8 | brightness << 16;
    sp->envFT3 = rgb | 0x26000000;
    sp->envFT4 = rgb | 0x2e000000;
    sp->envClut = unk06;
    sp->envTpage = tpage | 0x20;
    if (brightness) {
        sp->lit = -1;
    } else {
        sp->lit = 0;
    }
    world = &inst->world;
    Matrix_Comp(&sp->view, world);
    Matrix_Mul((MATRIX *)light, (MATRIX *)cam->invRot, world);
    mesh = table->lods[0].mesh;
    Gte_LoadTransform((ViewScratch *)sp, mesh->unk4e, mesh->unk4c, 0xc);
    sp->verts = mesh->verts;
    sp->normals = mesh->normals;
    sp->packetPtr = g_PacketPtr;
    sp->packetLimit = g_PacketEnd - 0x2000;
    light[0] = light[0] >> 3;
    light[1] = light[1] >> 3;
    light[2] = light[2] >> 3;
    light[3] = -(light[3] >> 3);
    light[4] = -(light[4] >> 3);
    light[5] = -(light[5] >> 3);
    light[6] = light[6] >> 3;
    light[7] = light[7] >> 3;
    light[8] = light[8] >> 3;
    if ((Clip_TestBox((ViewScratch *)sp, mesh->bboxMin, mesh->bboxMax) & 0x1f) == 0) {
        CarBody_DrawClipped(mesh, (MATRIX *)light);
    }
    g_PacketPtr = sp->packetPtr;
}

// Matrix_Translate by (x, y, z).
static inline void Matrix_TranslateXyz(MATRIX *m, int x, int y, int z)
{
    VECTOR v;

    v.vx = x;
    v.vy = y;
    v.vz = z;
    Matrix_Translate(m, &v);
}

// Draws the car shown on the post-race pages: builds the view matrix from view (rotY,
// rotX, rotZ, pos), sets the viewport and frustum, draws the model at its ride height
// (CarModel_Draw, or CarModel_DrawBodyOnly when bodyOnly), then the ground disc if
// view->drawDisc.
void ResultCar_Draw(ResultCar *car, DrawLayer *layer, DrawLayer *shadowLayer, ResultCarView *view, int bodyOnly)
{
    MATRIX m;
    Camera *cam;
    int vpX, vpY, width, height;

    Matrix_Identity(&m);
    Matrix_RotY(&m, -view->rotY);
    Matrix_RotX(&m, -view->rotX);
    Matrix_RotZ(&m, -view->rotZ);
    Matrix_TranslateXyz(&m, view->pos[0], view->pos[1], view->pos[2]);
    vpX = view->viewport[0];
    vpY = view->viewport[1];
    width = view->viewport[2];
    height = view->viewport[3];
    cam = &view->cam;
    cam->vpX = vpX;
    cam->vpY = vpY;
    cam->width = width;
    cam->height = height;
    Camera_SetFrustum(cam, view->frustum[0], view->frustum[1], view->frustum[2], view->frustum[3],
                      view->frustum[4], view->frustum[5]);
    Camera_Update(cam, &m, layer->ot, 0x10);
    {
        CarModelInst inst;

        inst.lod = 1;
        inst.detail = 0;
        inst.unk02 = 0;
        inst.drawShadow = 1;
        inst.model = (CarModelHeader *)car->model;
        inst.unk06 = car->unk00;
        inst.tpage = car->tpage;
        inst.brightness = car->brightness;
        Matrix_Identity(&inst.world);
        Matrix_TranslateXyz(&inst.world, 0, CarModel_GetRideHeight(inst.model), 0);
        CarInst_InitWheels(&inst);
        Matrix_Identity(&inst.shadow);
        if (bodyOnly) {
            CarModel_DrawBodyOnly(cam, &inst);
        } else {
            CarModel_Draw(cam, &inst, 0);
        }
    }
    if (*(signed char *)((char *)view + 0xd4)) {
        Matrix_Identity(&m);
        Matrix_RotX(&m, -view->rotX);
        Matrix_TranslateXyz(&m, view->pos[0], view->pos[1], view->pos[2]);
        Camera_Update(&view->cam, &m, layer->ot, 0x10);
        GroundDisc_Draw(&car->disc, (uint *)layer->ot + 0x28, (uchar *)&view->cam,
                        *(signed char *)((char *)view + 0xd5) != 0, view->discRgb);
    }
}

// Counts the short at +6 up from 0 to 12 (-1 = off). Not referenced.
void FadeIn12_Update(void *item)
{
    short *t = (short *)((char *)item + 6);

    if (*t != -1) {
        if (++*t > 11) {
            *t = 12;
        }
    }
}
}
