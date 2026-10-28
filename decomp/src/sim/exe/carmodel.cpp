// Resident code 80061010-800680a0 (carmodel): car model setup (LOD meshes, wheels, VRAM
// slots, colours) and drawing. The drawing inner loops (CarBody_Draw*, WheelMesh_Draw,
// CarModel_DrawWheel, ShadowMesh_Draw, Mesh_NormalsToEnvUV, Clut_Blend) are handwritten
// GTE assembly; the functions that don't match yet are the original assembly too.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"
#include "gte.h"

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
    // A copy of the Camera from view to ot (CameraView), the view composed with the
    // car's world matrix.
    MATRIX view;
    int negPos[3];
    short clipX[9];
    short pad3e;
    short clipY[9];
    short pad52;
    int ofx;
    int ofy;
    short projH;
    short screenW;
    short screenH;
    short nearZ;                // 0x62
    unsigned int *ot;           // 0x64
    char *packetPtr;            // 0x68
    char *packetLimit;
    char *verts;                // 0x70; ShadowMesh_Draw's transformed vertices
    char *normals;
    char unk78[0x20];
    short otShift;              // 0x98: depth << otShift >> 13 is the OT index
    short pad9a;
    MATRIX wheelMat;            // 0x9c: CarModel_DrawWheel's wheel rotation and position
    char unkbc[0x2dc];
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

// GPU primitives the handwritten code builds.
struct PolyF4 {
    unsigned int tag;
    unsigned int rgbc;
    int xy0;
    int xy1;
    int xy2;
    int xy3;
};

struct PolyFT4 {
    unsigned int tag;
    unsigned int rgbc;
    int xy0;
    unsigned short uv0;
    unsigned short clut;
    int xy1;
    unsigned short uv1;
    unsigned short tpage;
    int xy2;
    unsigned short uv2;
    unsigned short pad1e;
    int xy3;
    unsigned short uv3;
    unsigned short pad26;
};

struct PolyG3 {
    unsigned int tag;
    unsigned int rgbc0;
    int xy0;
    unsigned int rgb1;
    int xy1;
    unsigned int rgb2;
    int xy2;
};

struct PolyG4 {
    unsigned int tag;
    unsigned int rgbc0;
    int xy0;
    unsigned int rgb1;
    int xy1;
    unsigned int rgb2;
    int xy2;
    unsigned int rgb3;
    int xy3;
};

struct DrawMode {
    unsigned int tag;
    unsigned int code;
};

// Links a primitive into an OT entry (the handwritten code's lwl/swl pairs, which keep
// both top bytes).
static inline void Ot_AddPrim(unsigned int *ot, void *prim)
{
    unsigned int *tag = (unsigned int *)prim;

    *tag = (*tag & 0xff000000) | (*ot & 0xffffff);
    *ot = (*ot & 0xff000000) | ((unsigned int)prim & 0xffffff);
}

// The OT index of a depth, as the handwritten code computes it.
static inline int Ot_Index(int z, int shift)
{
    int i = (z << shift) >> 13;

    if (i >= 0x1000) {
        i = 0xfff;
    }
    return i;
}

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

// Turns each normal of a body mesh into environment map texture coordinates: the normal
// (10 bits a component) rotated by m, plus 64, gives u and v. The coordinates go from
// out + 2 (the scratchpad's normals), after a zero pair.
#ifdef NON_MATCHING
// Handwritten: the original is a software-pipelined loop (it stores each result in the
// next pass), which is where the zero pair at out comes from.
void Mesh_NormalsToEnvUV(CarMesh *mesh, MATRIX *m, unsigned char *out)
{
    CarScratch *sp = (CarScratch *)0x1f800000;
    unsigned int *n = (unsigned int *)mesh->normals;
    unsigned int *end = n + mesh->normalCount;

    gte_SetCtrlReg(GTE_TRX, 0x40);
    gte_SetCtrlReg(GTE_TRY, 0x40);
    gte_SetRotMatrixWords(m);
    sp->normals = (char *)(out + 2);
    if (n == end) {
        return;
    }
    out[0] = 0;
    out[1] = 0;
    out += 2;
    for (; n != end; n++) {
        int w = *n;

        gte_SetDataReg(GTE_IR1, (w << 20) >> 22);
        gte_SetDataReg(GTE_IR2, (w << 10) >> 22);
        gte_SetDataReg(GTE_IR3, w >> 22);
        gte_mvmva(1, GTE_MX_RT, GTE_V_IR, GTE_CV_TR, 0);
        out[0] = gte_GetDataReg(GTE_IR1);
        out[1] = gte_GetDataReg(GTE_IR2);
        out += 2;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Mesh_NormalsToEnvUV);
#endif

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarBody_DrawUnclipped);

INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarBody_DrawClipped);

// Draws a wheel mesh (from CarModel_DrawWheel, with the wheel's transform loaded): strips
// of vertex pairs, each pair of pairs a flat quad in the wheel colour when it faces the
// camera (black otherwise), sorted on its nearest or farthest corner.
#ifdef NON_MATCHING
// One vertex pair of a wheel strip: the first vertex's x and y, both z, the second's x and y.
struct WheelPair {
    int xyA;
    unsigned short zA;
    unsigned short zB;
    int xyB;
};

// Handwritten: the original projects one pair ahead (its first pass builds a quad from
// stale registers, which its flags test throws away). mesh is a list of strips, each a
// pair count and the pairs, ended by a zero count.
void WheelMesh_Draw(CarScratch *sp, int *mesh)
{
    char *pkt = sp->packetPtr;
    int shift = (unsigned short)sp->otShift;
    unsigned int *ot = sp->ot + 1;
    int mirror = sp->mirror;
    unsigned int color = sp->wheelColor;
    int n = *mesh++;

    do {
        WheelPair *pair = (WheelPair *)mesh;
        unsigned int prevFlags;
        int prevA;
        int i;

        gte_SetDataReg(GTE_VXY0, pair[0].xyA);
        gte_SetDataReg(GTE_VZ0, pair[0].zA);
        gte_rtps();
        prevFlags = gte_GetCtrlReg(GTE_FLAG);
        prevA = gte_GetDataReg(GTE_SXY2);
        gte_SetDataReg(GTE_VXY0, pair[0].xyB);
        gte_SetDataReg(GTE_VZ0, pair[0].zB);
        gte_rtps();
        prevFlags |= gte_GetCtrlReg(GTE_FLAG);
        for (i = 1; i < n; i++) {
            PolyF4 *poly = (PolyF4 *)pkt;
            unsigned int flags;
            int n1;
            int n2;
            int front;
            int sz[4];
            int z;
            int k;

            // Each nclip takes the last three points: (A, B of the last pair, this A),
            // then (B of the last pair, this A, this B).
            gte_SetDataReg(GTE_VXY0, pair[i].xyA);
            gte_SetDataReg(GTE_VZ0, pair[i].zA);
            gte_rtps();
            flags = gte_GetCtrlReg(GTE_FLAG);
            gte_nclip();
            n1 = gte_GetDataReg(GTE_MAC0);
            gte_SetDataReg(GTE_VXY0, pair[i].xyB);
            gte_SetDataReg(GTE_VZ0, pair[i].zB);
            gte_rtps();
            flags |= gte_GetCtrlReg(GTE_FLAG);
            gte_nclip();
            n2 = gte_GetDataReg(GTE_MAC0);

            poly->xy0 = prevA;
            poly->xy1 = gte_GetDataReg(GTE_SXY0);
            poly->xy2 = gte_GetDataReg(GTE_SXY1);
            poly->xy3 = gte_GetDataReg(GTE_SXY2);
            prevA = poly->xy2;
            sz[0] = gte_GetDataReg(GTE_SZ0);
            sz[1] = gte_GetDataReg(GTE_SZ1);
            sz[2] = gte_GetDataReg(GTE_SZ2);
            sz[3] = gte_GetDataReg(GTE_SZ3);

            // Facing: the first triangle's nclip not negative and the second's negative
            // (the other way round in a mirror).
            if (mirror >= 0) {
                front = ~n1 & n2;
            } else {
                front = ~n1 | n2;
            }
            front = (front ^ mirror) < 0;
            z = sz[0];
            for (k = 1; k < 4; k++) {
                if (front ? sz[k] > z : sz[k] < z) {
                    z = sz[k];
                }
            }
            poly->rgbc = (front ? color & 0xffffff : 0) | 0x28000000;
            ((unsigned char *)poly)[3] = 5;
            if ((int)(prevFlags | flags) >= 0) {
                Ot_AddPrim(ot + Ot_Index(z, shift), poly);
                pkt += sizeof(PolyF4);
            }
            prevFlags = flags;
        }
        mesh = (int *)(pair + n);
        sp->packetPtr = pkt;
        n = *mesh++;
    } while (n != 0);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", WheelMesh_Draw);
#endif

// Draws wheel index (0-3; bit 0 the right side, bit 1 the rear) of a car: the wheel mesh
// of LOD lod rotated by the wheel's angles (mirrored on the right) and, over it, the hub
// as two textured quads around its centre.
#ifdef NON_MATCHING
void Matrix_SetEuler(MATRIX *m, int x, int y, int z);
void WheelMesh_Draw(CarScratch *sp, int *mesh);

// Handwritten: the original is close to compiler output, with the hub's packets written
// in the GTE's latency slots.
void CarModel_DrawWheel(Camera *cam, MATRIX *world, WheelInst *wheel, CarModelHeader *model, int lod, int index)
{
    CarScratch *sp = (CarScratch *)0x1f800000;
    WheelGeom *geom = (WheelGeom *)&model->wheelTexture;
    int rear = (index >> 1) & 1;
    int right = index & 1;
    int *mesh;
    int spin;
    signed char mirror;
    unsigned int *quad;
    int z;
    unsigned int tex;
    PolyFT4 *p;
    PolyFT4 *q;
    unsigned int flags;
    unsigned int *ot;

    if (sp->packetLimit < sp->packetPtr) {
        return;
    }
    mesh = rear ? geom->rearLod0 : geom->frontLod0;
    if (lod == 1) {
        mesh = rear ? geom->rearLod1 : geom->frontLod1;
    }
    if (lod == 2) {
        mesh = rear ? geom->rearLod2 : geom->frontLod2;
    }
    spin = wheel->spin;
    if (right) {
        spin += 0x800;
    }
    Matrix_SetEuler(&sp->wheelMat, spin, wheel->angleA, wheel->angleB);
    sp->wheelMat.t[0] = wheel->x << 4;
    sp->wheelMat.t[1] = wheel->y << 4;
    sp->wheelMat.t[2] = wheel->z << 4;
    *(CameraView *)sp = *(CameraView *)&cam->view;
    Matrix_Comp(&sp->view, world);
    Matrix_Comp(&sp->view, &sp->wheelMat);
    if (right) {
        sp->view.m[0][2] = -sp->view.m[0][2];
        sp->view.m[2][2] = -sp->view.m[2][2];
        sp->view.m[1][2] = -sp->view.m[1][2];
    }
    Gte_LoadTransform(sp, 0x1000, 0x10, 0xc);
    mirror = sp->mirror;
    if (right) {
        sp->mirror = mirror ? 0 : -1;
    }
    WheelMesh_Draw(sp, mesh);
    sp->mirror = mirror;

    // The hub: corners q0-q3 and the centre, at the hub's z.
    quad = rear ? geom->rearQuad : geom->frontQuad;
    z = (unsigned short)(rear ? geom->rearHalfWidth : geom->frontHalfWidth);
    p = (PolyFT4 *)sp->packetPtr;
    q = p + 1;
    gte_SetDataReg(GTE_VZ0, z);
    gte_SetDataReg(GTE_VZ1, z);
    gte_SetDataReg(GTE_VZ2, z);
    gte_SetDataReg(GTE_VXY0, quad[0]);
    gte_SetDataReg(GTE_VXY1, quad[1]);
    gte_SetDataReg(GTE_VXY2, quad[2]);
    gte_rtpt();
    tex = model->wheelTexture + sp->detail;
    p->tag = 0x09000000;
    p->rgbc = 0x2c808080;
    p->uv0 = 0x2f;
    p->clut = tex >> 16;
    p->uv1 = 0;
    p->tpage = tex;
    p->uv2 = 0x1717;
    p->uv3 = 0x2f00;
    flags = gte_GetCtrlReg(GTE_FLAG);
    gte_StoreDataReg(GTE_SXY0, &p->xy1);
    gte_StoreDataReg(GTE_SXY1, &p->xy0);
    gte_StoreDataReg(GTE_SXY1, &q->xy0);
    gte_StoreDataReg(GTE_SXY2, &q->xy2);
    // q3 and the centre; the third point is q2 again.
    gte_SetDataReg(GTE_VXY0, quad[3]);
    gte_SetDataReg(GTE_VXY1, 0);
    gte_rtpt();
    q->tag = 0x09000000;
    q->rgbc = 0x2c808080;
    q->uv0 = 0x2f;
    q->clut = tex >> 16;
    q->uv1 = 0x2f2f;
    q->tpage = tex;
    q->uv2 = 0x1717;
    q->uv3 = 0x2f00;
    gte_StoreDataReg(GTE_SXY0, &p->xy3);
    gte_StoreDataReg(GTE_SXY0, &q->xy3);
    gte_StoreDataReg(GTE_SXY1, &p->xy2);
    gte_StoreDataReg(GTE_SXY1, &q->xy1);
    flags |= gte_GetCtrlReg(GTE_FLAG);
    if ((int)flags < 0) {
        return;
    }
    // Sorted on the centre's depth, one entry before Ot_Index's.
    ot = sp->ot + Ot_Index(gte_GetDataReg(GTE_SZ2), sp->otShift) - 1;
    Ot_AddPrim(ot, p);
    Ot_AddPrim(ot, q);
    sp->packetPtr = (char *)(q + 1);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", CarModel_DrawWheel);
#endif

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

// Blends a 256-colour CLUT towards rgb (24-bit) by t/4096 with the GTE's intpl, leaving
// the colours whose bit is set in the 16 row masks after it. A colour that blends to 0
// (transparent) becomes 0x8000; the blend of 0 stays as it is.
#ifdef NON_MATCHING
// Handwritten: written with the GTE loads in the latency slots.
void Clut_Blend(unsigned short *dst, unsigned short *src, unsigned int rgb, int t)
{
    unsigned short *masks = src + 0x100;
    int row;
    int col;

    gte_SetDataReg(GTE_RGBC, 0);
    gte_SetCtrlReg(GTE_RFC, (rgb << 4) & 0xff0);
    gte_SetCtrlReg(GTE_GFC, (rgb >> 4) & 0xff0);
    gte_SetCtrlReg(GTE_BFC, (rgb >> 12) & 0xff0);
    for (row = 0; row < 16; row++) {
        unsigned int keep = masks[row];

        for (col = 0; col < 16; col++) {
            unsigned int c = *src++;
            unsigned int out = c;

            if (!(keep & 1)) {
                gte_SetDataReg(GTE_IR0, t);
                gte_SetDataReg(GTE_IRGB, c);
                gte_intpl();
                out = gte_GetDataReg(GTE_ORGB);
                if (c != 0) {
                    if (out == 0) {
                        out = 0x8000;
                    } else {
                        out |= c & 0x8000;
                    }
                }
            }
            *dst++ = out;
            keep >>= 1;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", Clut_Blend);
#endif

// Draws a car's shadow mesh (with CarModel_DrawShadow's transform loaded) as
// subtractive Gouraud triangles and quads: transforms the vertices (up to 64) into a
// stack table with their clip codes, then draws each face that isn't wholly outside one
// plane, through Poly_Clip when it crosses one.
#ifdef NON_MATCHING
// A shadow mesh: the counts, then the vertices (x and z, a word each), the triangles and
// the quads (6-bit vertex indices packed in a word, bit 31 darkening the first two).
struct ShadowMesh {
    unsigned short vertCount;
    unsigned short triCount;
    unsigned short quadCount;
    char unk6[0x12];
    unsigned short unk18;
    unsigned short unk1a;
    unsigned int data[1];
};

// A transformed shadow vertex: view-space x, y, z, the clip code (bit 0 nearer than half
// the near plane, bits 1-4 outside the planes in the light and light colour matrices,
// bit 5 a GTE error), the screen position and the or of |x|, |y| and |z|.
struct ShadowVert {
    short x;
    short y;
    short clip;
    short z;
    int sxy;
    unsigned int extent;
};

// Poly_Clip's input, in the scratchpad at 0x1f80009c.
struct ShadowClipVert {
    int xy;
    short z;
    short extent;
    int sxy;
    unsigned int rgbc;          // the colour, with the clip code in the top byte
};

struct ShadowClip {
    unsigned int *ot;
    unsigned int code;
    int pad;
    ShadowClipVert v[4];
    ShadowVert *src[4];
};

void Poly_Clip(void *scratch, int count);

// The clip code word (clip, then z in the top half) of a vertex.
static inline int ShadowVert_ClipZ(ShadowVert *v)
{
    return *(int *)&v->clip;
}

static inline void ShadowClip_SetVert(ShadowClipVert *c, ShadowVert *v, unsigned int rgb)
{
    c->xy = *(int *)&v->x;
    c->z = v->z;
    c->extent = v->extent;
    c->sxy = v->sxy;
    c->rgbc = (rgb & 0xffffff) | (v->clip & 0xff) << 24;
}

static inline unsigned int *ShadowFace_Ot(CarScratch *sp, unsigned int *ot, int zMax)
{
    if (zMax < 0) {
        zMax = 0;
    }
    return ot + Ot_Index(zMax >> 16, sp->otShift);
}

// Adds the subtractive draw mode packet that goes before each face.
static inline char *Shadow_AddDrawMode(char *pkt, unsigned int *ot)
{
    DrawMode *m = (DrawMode *)pkt;

    m->tag = 0x01000000;
    m->code = 0xe1000240;
    Ot_AddPrim(ot, m);
    return pkt + sizeof(DrawMode);
}

// Handwritten: the original's vertex loop is software-pipelined (its first pass writes a
// record before the table from stale registers). Around Poly_Clip it also keeps its face
// pointers in the scratchpad's mirror and detail words, which this version doesn't.
void ShadowMesh_Draw(void *meshData)
{
    ShadowMesh *mesh = (ShadowMesh *)meshData;
    CarScratch *sp = (CarScratch *)0x1f800000;
    ShadowClip *clip = (ShadowClip *)0x1f80009c;
    ShadowVert verts[64];
    int nearHalf;
    unsigned int *face;
    unsigned int *end;
    unsigned int *ot;
    char *pkt;
    int i;

    if (mesh->vertCount == 0) {
        return;
    }
    nearHalf = (unsigned short)sp->nearZ >> 1;
    sp->verts = (char *)verts;
    for (i = 0; i < mesh->vertCount; i++) {
        ShadowVert *v = &verts[i];
        unsigned int w = mesh->data[i];
        int x;
        int y;
        int z;
        int flags;
        int code;
        int l1;
        int l2;
        int c1;
        int c2;

        gte_SetDataReg(GTE_VXY0, w & 0xffff);
        gte_SetDataReg(GTE_VZ0, w >> 16);
        gte_rtps();
        x = gte_GetDataReg(GTE_IR1);
        y = gte_GetDataReg(GTE_IR2);
        z = gte_GetDataReg(GTE_IR3);
        flags = gte_GetCtrlReg(GTE_FLAG);
        v->sxy = gte_GetDataReg(GTE_SXY2);
        // The view-space point against the planes held in the light matrix, then in the
        // light colour matrix.
        gte_mvmva(1, GTE_MX_LL, GTE_V_IR, GTE_CV_NONE, 0);
        gte_SetDataReg(GTE_VXY1, (x & 0xffff) | y << 16);
        gte_SetDataReg(GTE_VZ1, z);
        l1 = gte_GetDataReg(GTE_IR1);
        l2 = gte_GetDataReg(GTE_IR2);
        gte_mvmva(1, GTE_MX_LC, GTE_V_V1, GTE_CV_NONE, 0);
        c1 = gte_GetDataReg(GTE_IR1);
        c2 = gte_GetDataReg(GTE_IR2);
        code = z < nearHalf;
        if (l1 < 0) {
            code |= 2;
        }
        if (l2 < 0) {
            code |= 4;
        }
        if (c1 < 0) {
            code |= 8;
        }
        if (c2 < 0) {
            code |= 0x10;
        }
        if (flags < 0) {
            code |= 0x20;
        }
        v->x = x;
        v->y = y;
        v->z = z;
        v->clip = code;
        v->extent = (x < 0 ? -x : x) | (y < 0 ? -y : y) | (z < 0 ? -z : z);
    }

    // Poly_Clip projects the clipped points itself, with no rotation or translation.
    gte_SetCtrlReg(GTE_TRX, 0);
    gte_SetCtrlReg(GTE_TRY, 0);
    gte_SetCtrlReg(GTE_TRZ, 0);
    gte_SetCtrlReg(GTE_R11R12, 0x1000);
    gte_SetCtrlReg(GTE_R13R21, 0);
    gte_SetCtrlReg(GTE_R22R23, 0x1000);
    gte_SetCtrlReg(GTE_R31R32, 0);
    gte_SetCtrlReg(GTE_R33, 0x1000);

    ot = sp->ot + 4;
    face = mesh->data + mesh->vertCount;
    end = face + mesh->triCount;
    pkt = sp->packetPtr;
    for (; face != end; face++) {
        unsigned int f = *face;
        ShadowVert *a = &verts[f & 0x3f];
        ShadowVert *b = &verts[(f >> 6) & 0x3f];
        ShadowVert *c = &verts[(f >> 12) & 0x3f];
        int ca = ShadowVert_ClipZ(a);
        int cb = ShadowVert_ClipZ(b);
        int cc = ShadowVert_ClipZ(c);
        int zMax;
        unsigned int rgb;
        unsigned int *entry;

        if (ca & cb & cc & 0x1f) {
            continue;
        }
        zMax = ca > cb ? ca : cb;
        if (cc > zMax) {
            zMax = cc;
        }
        entry = ShadowFace_Ot(sp, ot, zMax);
        rgb = (int)f >> 31 & 0xffffff;
        if ((ca | cb | cc) & 0x3f) {
            sp->packetPtr = pkt;
            clip->ot = entry;
            clip->code = 0x32000000;
            ShadowClip_SetVert(&clip->v[0], a, rgb);
            ShadowClip_SetVert(&clip->v[1], b, rgb);
            ShadowClip_SetVert(&clip->v[2], c, 0xffffffff);
            clip->src[0] = a;
            clip->src[1] = b;
            clip->src[2] = c;
            Poly_Clip(sp, 3);
            pkt = sp->packetPtr;
        } else {
            PolyG3 *g = (PolyG3 *)pkt;

            g->tag = 0x06000000;
            g->rgbc0 = 0x32000000 | rgb;
            g->xy0 = a->sxy;
            g->rgb1 = rgb;
            g->xy1 = b->sxy;
            g->rgb2 = 0xffffffff;
            g->xy2 = c->sxy;
            Ot_AddPrim(entry, g);
            pkt += sizeof(PolyG3);
        }
        pkt = Shadow_AddDrawMode(pkt, entry);
    }
    sp->packetPtr = pkt;

    end = face + mesh->quadCount;
    for (; face != end; face++) {
        unsigned int f = *face;
        ShadowVert *a = &verts[f & 0x3f];
        ShadowVert *b = &verts[(f >> 6) & 0x3f];
        ShadowVert *c = &verts[(f >> 12) & 0x3f];
        ShadowVert *d = &verts[(f >> 18) & 0x3f];
        int ca = ShadowVert_ClipZ(a);
        int cb = ShadowVert_ClipZ(b);
        int cc = ShadowVert_ClipZ(c);
        int cd = ShadowVert_ClipZ(d);
        int zMax;
        unsigned int rgb;
        unsigned int *entry;

        if (ca & cb & cc & cd & 0x1f) {
            continue;
        }
        zMax = ca > cb ? ca : cb;
        if (cc > zMax) {
            zMax = cc;
        }
        if (cd > zMax) {
            zMax = cd;
        }
        entry = ShadowFace_Ot(sp, ot, zMax);
        rgb = (int)f >> 31 & 0xffffff;
        if ((ca | cb | cc | cd) & 0x3f) {
            sp->packetPtr = pkt;
            clip->ot = entry;
            clip->code = 0x3a000000;
            ShadowClip_SetVert(&clip->v[0], a, rgb);
            ShadowClip_SetVert(&clip->v[1], b, rgb);
            ShadowClip_SetVert(&clip->v[2], c, 0xffffffff);
            ShadowClip_SetVert(&clip->v[3], d, 0xffffffff);
            clip->src[0] = a;
            clip->src[1] = b;
            clip->src[2] = c;
            clip->src[3] = d;
            Poly_Clip(sp, 4);
            pkt = sp->packetPtr;
        } else {
            // The GPU's quad order: the third point is d, the fourth c.
            PolyG4 *g = (PolyG4 *)pkt;

            g->tag = 0x08000000;
            g->rgbc0 = 0x3a000000 | rgb;
            g->xy0 = a->sxy;
            g->rgb1 = rgb;
            g->xy1 = b->sxy;
            g->rgb2 = 0xffffffff;
            g->xy2 = d->sxy;
            g->rgb3 = 0xffffffff;
            g->xy3 = c->sxy;
            Ot_AddPrim(entry, g);
            pkt += sizeof(PolyG4);
        }
        pkt = Shadow_AddDrawMode(pkt, entry);
    }
    sp->packetPtr = pkt;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/carmodel", ShadowMesh_Draw);
#endif

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
