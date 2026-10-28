// Resident code 80061010-800680a0 (carmodel): car model setup (LOD meshes, wheels, VRAM
// slots, colours) and drawing. The drawing inner loops (CarBody_Draw*, WheelMesh_Draw,
// CarModel_DrawWheel, ShadowMesh_Draw, Mesh_NormalsToEnvUV, Clut_Blend) are handwritten
// GTE assembly; the functions that don't match yet are the original assembly too.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CarModel_Init(CarModelHeader *model, unsigned int slot);
unsigned char FUN_800608e8(...);
void Vol_ReadFileAsync(int file, void *dst);
extern unsigned char D_801C946C;
int Wheel_GetFileId(unsigned int code);
void Vol_ReadFile(int file, void *dst);
int Gpu_TransferImage(RECT *rect, void *data, int store);
void Gpu_DrawSync(void);
void Tim_LoadImage(void *tim, RECT *rect);
void Tim_LoadClut(void *tim, RECT *rect);
void Matrix_Comp(MATRIX *m, MATRIX *n);
void Gte_LoadTransform(void *scratch, int a, int b, int c);
void ShadowMesh_Draw(void *mesh);

// A car instance's wheel: position, and the spin direction (+-0x400) and angles.
struct WheelInst {
    short x;
    short y;
    short z;
    short pad6;
    short spin;
    short angleA;
    short angleB;
    short padE;
};

struct CarModelInst {
    unsigned char lod;          // 0: by distance, else LOD + 1
    unsigned char detail;
    unsigned char unk02;
    unsigned char drawShadow;
    unsigned char brightness;
    unsigned char pad05;
    short unk06;
    unsigned short tpage;
    short pad0a;
    CarModelHeader *model;
    MATRIX world;
    WheelInst wheels[4];
    MATRIX shadow;
};

// The colour codes of a car texture: count, then the letters.
struct CarTex {
    unsigned short count;
    char codes[1];
};

// The wheels of a car model (CarModelHeader from wheelTexture): the hub half widths, the hub
// sprite corners and the front and rear wheel meshes of the 3 LODs.
struct WheelGeom {
    unsigned int texture;
    short frontHalfWidth;
    short rearHalfWidth;
    unsigned int frontQuad[4];
    int pad18;
    unsigned int rearQuad[4];
    int pad2c;
    int frontLod0[0x82];
    int rearLod0[0x82];
    int frontLod1[0x46];
    int rearLod1[0x46];
    int frontLod2[0x37];
    int rearLod2[0x37];
};

extern int g_WheelMeshLod0[];
extern int g_WheelMeshLod1[];
extern int g_WheelMeshLod2[];

// A car body mesh (one LOD): the counts, then pointers set by CarModel_Init to the vertices,
// normals and each primitive type's list, which follow the 0x50-byte header.
struct CarMesh {
    unsigned short vertCount;
    unsigned short normalCount;
    unsigned short primCount[8];
    char *verts;
    char *normals;
    char *prims[8];
    short bboxMin[4];
    short bboxMax[4];
    unsigned short unk4c;
    unsigned short unk4e;
};

struct LodEntry {
    int dist;
    CarMesh *mesh;
};

// CarModelHeader.lodCount and the LOD entries after it; the meshes follow.
struct LodTable {
    int count;
    LodEntry lods[1];
};

static inline LodEntry *LodAt(LodTable *table, int i)
{
    return &table->lods[i];
}

extern unsigned char g_ModelPrimSizes[];
extern unsigned char g_ModelPrimUvOffset[];
void LodTable_SquareDistances(LodTable *table);

// The camera fields CarModel_Draw and CarModel_DrawShadow copy to the scratchpad.
struct CameraView {
    int data[0x1a];
};

// What CarModel_Draw sets up in the scratchpad (0x1f800000) for the body and wheel drawing.
struct CarScratch {
    MATRIX view;                // the camera's, composed with the car's world matrix
    int camRest[0x12];
    char *packetPtr;            // 0x68
    char *packetLimit;
    char *verts;                // 0x70
    char *normals;
    char unk78[0x320];
    signed char mirror;         // 0x398
    signed char lit;
    char pad39a[2];
    unsigned int detail;        // 0x39c
    unsigned int unk3a0;
    unsigned int rgbF3;         // 0x3a4: the brightness, with POLY_FT3's code
    unsigned int rgbG3;         // 0x3a8
    short unk3ac;
    unsigned short tpage;
    unsigned int wheelColor;    // 0x3b0
};

extern char *g_PacketPtr;
extern char *g_PacketEnd;
int Lod_DistanceSq(LodTable *table, long *pos, int depthScale);
int Lod_Select(LodTable *table, unsigned int distSq);
void Matrix_Mul(MATRIX *out, short *rot, MATRIX *m);
unsigned int Clip_TestBox(void *scratch, short *min, short *max);
void CarBody_DrawUnclipped(CarMesh *mesh, MATRIX *light);
void CarBody_DrawClipped(CarMesh *mesh, MATRIX *light);
void CarModel_DrawWheel(Camera *cam, MATRIX *world, WheelInst *wheel, CarModelHeader *model, int lod, int index);
void CarModel_DrawShadow(Camera *cam, CarModelInst *inst, CarModelHeader *model);

void CarModel_InitStub(CarModelHeader *model, int arg)
{
    int unused[4];
}

// Prepares a car model loaded in RAM for drawing from VRAM slot slot: the wheel texture page
// and CLUT, each LOD mesh's pointers, the texture position and CLUT of its textured
// primitives (types 4 to 7), the squared LOD distances and the shadow mesh after the last LOD.
#ifdef NON_MATCHING
// The game allocates registers differently (the LOD count and table stay in $a0/$a1,
// a product in $s7) and spills nothing; the code shape matches.
void CarModel_Init(CarModelHeader *model, unsigned int slot)
{
    int tx = (slot & 0xf) * 4;
    int ty = ((slot & 0x10) * 16 + 0xe0) * 64;
    LodTable *table = (LodTable *)&model->lodCount;
    int count = table->count;
    CarMesh *mesh;
    int i;

    model->wheelTexture = (tx + ty) << 16 | slot;
    mesh = (CarMesh *)((int *)table + count * 2 + 1);
    for (i = 0; i < count; i++) {
        char *p;
        int k;

        table->lods[i].mesh = mesh;
        mesh->verts = (char *)(mesh + 1);
        mesh->normals = mesh->verts + mesh->vertCount * 8;
        p = mesh->normals + mesh->normalCount * 4;
        k = 0;
        do {
            mesh->prims[k] = p;
            p += g_ModelPrimSizes[k] * mesh->primCount[k];
            k++;
        } while (k < 8);
        for (k = 4; k < 8; k++) {
            char *q = mesh->prims[k] + g_ModelPrimUvOffset[k];
            int n = mesh->primCount[k];

            while (--n != -1) {
                unsigned int w = *(unsigned int *)(q + 4);

                *(short *)(q + 2) += tx + ty;
                *(unsigned int *)(q + 4) = (w & 0xffe0ffff) | (slot & 0x1f) << 16;
                q += g_ModelPrimSizes[k];
            }
        }
        mesh = (CarMesh *)p;
    }
    model->shadowMesh = mesh;
    LodTable_SquareDistances(table);
    CarModel_InitStub(model, 0);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_Init);
#endif

// Copies a wheel mesh template (groups of a vertex count and pairs of vertices) scaled by the
// wheel's radius and width; the first group's second vertices are also scaled by dims->scale.
#ifdef NON_MATCHING
// The game walks each vertex pair with two pointers a word apart and computes the
// second vertex's x last; no loop or pointer shape tried gives that.
void WheelMesh_Scale(int *dst, int *src, WheelDims *dims)
{
    int scale = dims->scale;
    int radius = dims->radius;
    int r2 = radius * scale >> 12;
    int width = dims->width;
    int n;
    short *s;
    short *d;

    n = *src++;
    *dst++ = n;
    while (n != 0) {
        s = (short *)src;
        d = (short *)dst;
        do {
            d[0] = s[0] * radius >> 12;
            d[1] = s[1] * radius >> 12;
            d[2] = s[2] * width >> 12;
            d[3] = s[3] * width >> 12;
            d[4] = s[4] * r2 >> 12;
            d[5] = s[5] * r2 >> 12;
            s += 6;
            d += 6;
        } while (--n != 0);
        src = (int *)s;
        dst = (int *)d;
        r2 = radius;
        n = *src++;
        *dst++ = n;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", WheelMesh_Scale);
#endif

#ifdef NON_MATCHING
// The game issues the rear multiply after the front quad's stores; the order of the
// loads and the multiplies differs.
void CarModel_BuildWheels(WheelGeom *geom, WheelDims *front, WheelDims *rear)
{
    int frontRadius = front->radius;
    int frontScale = front->scale;
    int rearRadius = rear->radius;
    int rearScale = rear->scale;
    int v;
    unsigned int lo;
    unsigned int nlo;

    geom->frontHalfWidth = (front->width >> 1) - front->offset;
    geom->rearHalfWidth = (rear->width >> 1) - rear->offset;
    v = frontRadius * frontScale >> 12;
    lo = v & 0xffff;
    nlo = -v & 0xffff;
    geom->frontQuad[1] = lo | lo << 16;
    geom->frontQuad[0] = nlo | lo << 16;
    geom->frontQuad[2] = lo | nlo << 16;
    geom->frontQuad[3] = nlo | nlo << 16;
    v = rearRadius * rearScale >> 12;
    lo = v & 0xffff;
    nlo = -v & 0xffff;
    geom->rearQuad[1] = lo | lo << 16;
    geom->rearQuad[0] = nlo | lo << 16;
    geom->rearQuad[2] = lo | nlo << 16;
    geom->rearQuad[3] = nlo | nlo << 16;
    WheelMesh_Scale(geom->frontLod0, g_WheelMeshLod0, front);
    WheelMesh_Scale(geom->frontLod1, g_WheelMeshLod1, front);
    WheelMesh_Scale(geom->frontLod2, g_WheelMeshLod2, front);
    WheelMesh_Scale(geom->rearLod0, g_WheelMeshLod0, rear);
    WheelMesh_Scale(geom->rearLod1, g_WheelMeshLod1, rear);
    WheelMesh_Scale(geom->rearLod2, g_WheelMeshLod2, rear);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_BuildWheels);
#endif

void CarInst_InitWheels(CarModelInst *inst)
{
    CarModelHeader *model = inst->model;
    int i;

    for (i = 0; i < 4; i++) {
        inst->wheels[i].x = model->wheelPos[i].x;
        inst->wheels[i].y = model->wheelPos[i].y;
        inst->wheels[i].z = model->wheelPos[i].z;
        if ((i & 1) == 0) {
            inst->wheels[i].spin = -0x400;
        } else {
            inst->wheels[i].spin = 0x400;
        }
        inst->wheels[i].angleA = 0;
        inst->wheels[i].angleB = 0;
    }
}

void CarModel_GetWheelDims(CarModelHeader *model, WheelDims *front, WheelDims *rear)
{
    short sVar1;

    front->radius = model->frontWheel[0];
    sVar1 = model->frontWheel[1];
    front->scale = 0xc00;
    front->offset = 0x7a;
    front->width = sVar1;
    rear->radius = model->rearWheel[0];
    sVar1 = model->rearWheel[1];
    rear->scale = 0xc00;
    rear->offset = 0x7a;
    rear->width = sVar1;
}

int CarModel_GetRideHeight(CarModelHeader *model)
{
    return ((int)model->frontWheel[0] - (int)model->wheelPos[0].y) * 0x10;
}

// Sets body colour index from a 15-bit colour, as a 24-bit one.
void CarModel_SetColor(CarModelHeader *model, int index, unsigned int rgb)
{
    unsigned int r = (rgb & 0x1f) << 3;
    unsigned int g = rgb & 0x3e0;
    unsigned int b = rgb & 0x7c00;

    model->colors[index] = r | g << 6 | b << 9;
}

void CarModel_Load(int fileId, CarModelHeader *dst, unsigned int slot)
{
    Vol_ReadFileAsync(fileId, dst);
    CarModel_Init(dst, slot);
}

void CarModel_ReadTim(int fileId, void *dst)
{
    Vol_ReadFileAsync(fileId + 1, dst);
}

// Reads the wheel texture of a wheel code; 0 when the code has no wheel.
int Wheel_LoadTim(unsigned int code, void *dst)
{
    code &= ~0x1f00;
    if (code == 0) {
        return 0;
    }
    Vol_ReadFile(Wheel_GetFileId(code), dst);
    return 1;
}

// The index of colour letter ch (case-insensitive); ch modulo the count when not found.
int CarTex_FindColorIndex(CarTex *tex, unsigned int ch)
{
    int i;
    unsigned int c;

    if (ch - 'A' < 26) {
        ch += 0x20;
    }
    i = 0;
    int n = tex->count;
loop:
    if (i < n) {
        c = (signed char)tex->codes[i];
        if (c - 'A' < 26) {
            c += 0x20;
        }
        if (c == ch) {
            return i;
        }
        i++;
        goto loop;
    }
    return ch % tex->count;
}

void FUN_800616a0(unsigned int param_1)
{
    FUN_800608e8(param_1, &D_801C946C);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Mesh_NormalsToEnvUV);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarBody_DrawUnclipped);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarBody_DrawClipped);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", WheelMesh_Draw);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_DrawWheel);

// Draws a car: copies the camera to the scratchpad, composes the world matrix, picks the LOD
// (inst->lod - 1, or by distance; -1 when too far), culls the bounding box, then draws the
// body, the wheels when near enough and the shadow. Returns the LOD used.
#ifdef NON_MATCHING
// Only register assignment differs: the game gives inst, the LOD and the model
// $s2-$s4 and cam and the world matrix $s6/$s7 (ours permutes them).
int CarModel_Draw(Camera *cam, CarModelInst *inst, int mirror)
{
    CarScratch *sp = (CarScratch *)0x1f800000;
    MATRIX light;
    MATRIX *world;
    unsigned int distSq;
    CarModelHeader *model;
    LodTable *table;
    int detail;
    unsigned int brightness;
    unsigned int rgb;
    int lod;
    int lodIn;
    CarMesh *mesh;
    unsigned int clip;
    unsigned int unk02;
    short unk06;
    unsigned short tpage;

    *(CameraView *)sp = *(CameraView *)&cam->view;
    world = &inst->world;
    Matrix_Comp(&sp->view, world);
    distSq = 0;
    model = inst->model;
    lod = inst->lod;
    detail = inst->detail;
    unk02 = inst->unk02;
    brightness = inst->brightness;
    unk06 = inst->unk06;
    tpage = inst->tpage;
    table = (LodTable *)&model->lodCount;
    if (mirror) {
        sp->mirror = -1;
    } else {
        sp->mirror = 0;
    }
    sp->detail = detail << 24;
    sp->unk3a0 = unk02 << 16;
    rgb = brightness | brightness << 8 | brightness << 16;
    sp->rgbF3 = rgb | 0x26000000;
    sp->rgbG3 = rgb | 0x2e000000;
    sp->unk3ac = unk06;
    sp->tpage = tpage | 0x20;
    if (brightness) {
        sp->lit = -1;
    } else {
        sp->lit = 0;
    }
    if (lod == 0) {
        distSq = Lod_DistanceSq(table, sp->view.t, cam->depthScale);
        lod = Lod_Select(table, distSq);
        if (lod == -1) {
            return -1;
        }
    } else {
        lod--;
    }
    Matrix_Mul(&light, cam->invRot, world);
    mesh = LodAt(table, lod)->mesh;
    Gte_LoadTransform(sp, mesh->unk4e, mesh->unk4c, 0xc);
    sp->verts = mesh->verts;
    sp->normals = mesh->normals;
    clip = Clip_TestBox(sp, mesh->bboxMin, mesh->bboxMax);
    light.t[1] = (long)mesh;
    if ((clip & 0x1f) == 0) {
        sp->packetPtr = g_PacketPtr;
        sp->packetLimit = g_PacketEnd - 0x2000;
        light.m[0][0] >>= 3;
        light.m[0][1] >>= 3;
        light.m[0][2] >>= 3;
        light.m[1][0] = -(light.m[1][0] >> 3);
        light.m[1][1] = -(light.m[1][1] >> 3);
        light.m[1][2] = -(light.m[1][2] >> 3);
        light.m[2][0] >>= 3;
        light.m[2][1] >>= 3;
        light.m[2][2] >>= 3;
        if ((clip >> 16 & 0x3f) == 0) {
            CarBody_DrawUnclipped((CarMesh *)light.t[1], &light);
        } else {
            CarBody_DrawClipped((CarMesh *)light.t[1], &light);
        }
        if (distSq < 0x2400 && detail < 3) {
            int wheelLod = 2;

            if (distSq < 0x900) {
                wheelLod = 1;
            }
            if (distSq < 0x240) {
                wheelLod = 0;
            }
            sp->wheelColor = model->colors[detail];
            CarModel_DrawWheel(cam, world, &inst->wheels[0], model, wheelLod, 0);
            CarModel_DrawWheel(cam, world, &inst->wheels[1], model, wheelLod, 1);
            CarModel_DrawWheel(cam, world, &inst->wheels[2], model, wheelLod, 2);
            CarModel_DrawWheel(cam, world, &inst->wheels[3], model, wheelLod, 3);
        }
        if (inst->drawShadow && (lod < 2 || detail == 3)) {
            CarModel_DrawShadow(cam, inst, model);
        }
        g_PacketPtr = sp->packetPtr;
    }
    return lod;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_Draw);
#endif

// Uploads CLUT row of VRAM car slot slot.
void Clut_UploadToSlot(void *clut, unsigned int slot, int row)
{
    RECT rect;

    rect.x = (slot & 0xf) << 6;
    rect.y = ((slot << 4) & 0x100) + row * 4 + 0xe0;
    rect.w = 0x40;
    rect.h = 4;
    Gpu_TransferImage(&rect, clut, 0);
    Gpu_DrawSync();
}

// Uploads the texture of VRAM car slot slot.
void Vram_UploadCarSlot(void *image, unsigned int slot)
{
    RECT rect;

    rect.x = (slot & 0xf) << 6;
    rect.y = (slot << 4) & 0x100;
    rect.w = 0x40;
    rect.h = 0xe0;
    Gpu_TransferImage(&rect, image, 0);
    Gpu_DrawSync();
}

// Loads a TIM's image (and its CLUT when withClut) into VRAM car slot slot.
#ifdef NON_MATCHING
// The TIM pointer and the slot x are in $a2 and $a3 the other way round.
void Tim_LoadToSlot(char *tim, unsigned int slot, int withClut)
{
    short x = (slot & 0xf) << 6;
    short y = (slot << 4) & 0x100;
    char *p = tim + 8;

    ((short *)p)[2] = x;
    ((short *)p)[3] = y + 0xe0;
    p += *(int *)p;
    ((short *)p)[2] = x;
    ((short *)p)[3] = y;
    Tim_LoadImage(tim, 0);
    if (withClut) {
        Tim_LoadClut(tim, 0);
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Tim_LoadToSlot);
#endif

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Clut_Blend);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", ShadowMesh_Draw);

void CarModel_DrawShadow(Camera *cam, CarModelInst *inst, CarModelHeader *model)
{
    void *mesh = model->shadowMesh;
    MATRIX *s = (MATRIX *)0x1f800000;

    *(CameraView *)s = *(CameraView *)&cam->view;
    Matrix_Comp(s, &inst->shadow);
    Gte_LoadTransform(s, ((unsigned short *)mesh)[0xd], ((unsigned short *)mesh)[0xc], 0xc);
    ShadowMesh_Draw(mesh);
}
}
