// Resident code 80061010-800680a0 (carmodel): car model setup (LOD meshes, wheels, VRAM
// slots, colours) and drawing. The drawing inner loops (CarBody_Draw*, WheelMesh_Draw,
// CarModel_DrawWheel, ShadowMesh_Draw, Mesh_NormalsToEnvUV, Clut_Blend) are handwritten
// GTE assembly; the functions that don't match yet are the original assembly too.
#include "common.h"
#include "gte.h"

extern "C" {
extern unsigned char D_801C946C;

extern int g_WheelMeshLod0[];
extern int g_WheelMeshLod1[];
extern int g_WheelMeshLod2[];

static inline LodEntry *LodAt(LodTable *table, int i)
{
    return &table->lods[i];
}

extern unsigned char g_ModelPrimSizes[];
extern unsigned char g_ModelPrimUvOffset[];

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

void CarModel_InitStub(CarModelHeader *model, int arg)
{
    int unused[4];
}

// Prepares a car model loaded in RAM for drawing from VRAM slot slot: the wheel texture page
// and CLUT, each LOD mesh's pointers, the texture position and CLUT of its textured
// primitives (types 4 to 7), the squared LOD distances and the shadow mesh after the last LOD.
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

// Copies a wheel mesh template (groups of a vertex count and pairs of vertices) scaled by the
// wheel's radius and width; the first group's second vertices are also scaled by dims->scale.
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
    FUN_800608e8(param_1, (int)&D_801C946C);
}

// Turns each normal of a body mesh into environment map texture coordinates: the normal
// (10 bits a component) rotated by m, plus 64, gives u and v. The coordinates go from
// out + 2 (the scratchpad's normals), after a zero pair.
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

// Draws a car body mesh that is wholly on screen, from its 8 primitive lists: flat,
// Gouraud, textured and Gouraud-textured triangles and quads (types 0-7). light is the
// light matrix CarModel_Draw built, which turns the normals into environment map
// coordinates. A face is drawn when the GTE projects it without error and it faces the
// camera; it is sorted on its nearest or farthest corner (flags bit 0) plus a bias
// (bits 1-4), and when the car is lit and flags bit 31 is set an environment map layer
// goes over it. Large textured faces are subdivided (Body_SubdivTri, Body_SubdivQuad).
// The words of a body primitive: the vertex indices (a byte each), the flags, the normal
// indices of the vertices after the first (bits 1-9, 10-18 and 19-27; the first's are
// flags bits 5-13),
// then the type's colours and texture words.
enum {
    BODY_VERTS,
    BODY_FLAGS,
    BODY_NORMALS,
    BODY_DATA
};

// The original keeps up to 7 subdivision frames in the scratchpad (0x1f8000e8-0x1f800387);
// a part that is still too large at the last one is dropped.
#define SUBDIV_MAX_DEPTH 6

static inline int Body_NClip(int a, int b, int c)
{
    gte_SetDataReg(GTE_SXY0, a);
    gte_SetDataReg(GTE_SXY1, b);
    gte_SetDataReg(GTE_SXY2, c);
    gte_nclip();
    return gte_GetDataReg(GTE_MAC0);
}

static inline unsigned int *Body_OtEntry(CarScratch *sp, unsigned int flags, unsigned int z)
{
    unsigned int i = (z << sp->otShift) >> 13;

    if (i >= 0x1000) {
        i = 0xfff;
    }
    return sp->ot + i + ((flags >> 1) & 0xf) - 8;
}

// The sort depth: the nearest corner, or the farthest when flags bit 0 is set.
static inline unsigned int Body_SortZ(unsigned int flags, int *sz, int n)
{
    int z = sz[0];
    int k;

    for (k = 1; k < n; k++) {
        if ((flags & 1) ? sz[k] > z : sz[k] < z) {
            z = sz[k];
        }
    }
    return z;
}

// Links a packet of len words into an OT entry.
static inline void Ot_Link(unsigned int *ot, void *prim, int len)
{
    *(unsigned int *)prim = len << 24 | (*ot & 0xffffff);
    *ot = (*ot & 0xff000000) | ((unsigned int)prim & 0xffffff);
}

// A vertex's entry in the table: its normal's field is the byte offset of the entry.
static inline unsigned short Body_EnvUV(CarScratch *sp, unsigned int field)
{
    return *(unsigned short *)(sp->normals + (field & 0x3fe));
}

// Projects a triangle. The original loads the vertices as V0, V2, V1 (so the nclip's
// sign is that of the first, third and second); xy gets them in index order.
static int Body_ProjectTri(CarScratch *sp, unsigned int *w, BodyVert **v, int *xy, int *sz, int *area)
{
    BodyVert *verts = (BodyVert *)sp->verts;
    int flags;

    v[0] = &verts[w[BODY_VERTS] & 0xff];
    v[1] = &verts[(w[BODY_VERTS] >> 8) & 0xff];
    v[2] = &verts[(w[BODY_VERTS] >> 16) & 0xff];
    gte_SetDataReg(GTE_VXY0, v[0]->xy);
    gte_SetDataReg(GTE_VZ0, v[0]->z);
    gte_SetDataReg(GTE_VXY2, v[1]->xy);
    gte_SetDataReg(GTE_VZ2, v[1]->z);
    gte_SetDataReg(GTE_VXY1, v[2]->xy);
    gte_SetDataReg(GTE_VZ1, v[2]->z);
    gte_rtpt();
    flags = gte_GetCtrlReg(GTE_FLAG);
    sz[0] = gte_GetDataReg(GTE_SZ1);
    sz[1] = gte_GetDataReg(GTE_SZ2);
    sz[2] = gte_GetDataReg(GTE_SZ3);
    gte_nclip();
    *area = gte_GetDataReg(GTE_MAC0);
    xy[0] = gte_GetDataReg(GTE_SXY0);
    xy[1] = gte_GetDataReg(GTE_SXY2);
    xy[2] = gte_GetDataReg(GTE_SXY1);
    return flags;
}

// Projects a quad (its vertices in order round the face): the second, third and first
// vertices, then the fourth, with an nclip after each projection.
static int Body_ProjectQuad(CarScratch *sp, unsigned int *w, BodyVert **v, int *xy, int *sz, int *n1, int *n2)
{
    BodyVert *verts = (BodyVert *)sp->verts;
    int flags;

    v[0] = &verts[w[BODY_VERTS] & 0xff];
    v[1] = &verts[(w[BODY_VERTS] >> 8) & 0xff];
    v[2] = &verts[(w[BODY_VERTS] >> 16) & 0xff];
    v[3] = &verts[w[BODY_VERTS] >> 24];
    gte_SetDataReg(GTE_VXY2, v[0]->xy);
    gte_SetDataReg(GTE_VZ2, v[0]->z);
    gte_SetDataReg(GTE_VXY0, v[1]->xy);
    gte_SetDataReg(GTE_VZ0, v[1]->z);
    gte_SetDataReg(GTE_VXY1, v[2]->xy);
    gte_SetDataReg(GTE_VZ1, v[2]->z);
    gte_rtpt();
    flags = gte_GetCtrlReg(GTE_FLAG);
    xy[1] = gte_GetDataReg(GTE_SXY0);
    sz[0] = gte_GetDataReg(GTE_SZ1);
    sz[1] = gte_GetDataReg(GTE_SZ2);
    sz[2] = gte_GetDataReg(GTE_SZ3);
    gte_nclip();
    *n1 = gte_GetDataReg(GTE_MAC0);
    gte_SetDataReg(GTE_VXY0, v[3]->xy);
    gte_SetDataReg(GTE_VZ0, v[3]->z);
    gte_rtps();
    flags |= gte_GetCtrlReg(GTE_FLAG);
    sz[3] = gte_GetDataReg(GTE_SZ3);
    gte_nclip();
    *n2 = gte_GetDataReg(GTE_MAC0);
    xy[2] = gte_GetDataReg(GTE_SXY0);
    xy[0] = gte_GetDataReg(GTE_SXY1);
    xy[3] = gte_GetDataReg(GTE_SXY2);
    return flags;
}

// A triangle is drawn when its nclip (negated in a mirror) is positive.
static inline int Body_TriVisible(CarScratch *sp, int flags, int area)
{
    if (flags < 0) {
        return 0;
    }
    if (sp->mirror < 0) {
        area = -area;
    }
    return area > 0;
}

// A quad is drawn when either half faces the camera (in a mirror, when either doesn't),
// and not when both nclips are 0.
static inline int Body_QuadVisible(CarScratch *sp, int flags, int n1, int n2)
{
    int a = -n1;

    if (flags < 0 || (a | n2) == 0) {
        return 0;
    }
    if (sp->mirror >= 0) {
        return a > 0 || n2 > 0;
    }
    return a <= 0 || n2 <= 0;
}

// The environment map layer over a triangle or quad.
static void Body_EnvTri(CarScratch *sp, unsigned int *ot, unsigned int *w, int *xy)
{
    PolyFT3 *p = (PolyFT3 *)sp->packetPtr;

    p->rgbc = sp->envFT3;
    p->xy0 = xy[0];
    p->uv0 = Body_EnvUV(sp, w[BODY_FLAGS] >> 4);
    p->clut = sp->envClut;
    p->xy1 = xy[1];
    p->uv1 = Body_EnvUV(sp, w[BODY_NORMALS]);
    p->tpage = sp->envTpage;
    p->xy2 = xy[2];
    p->uv2 = Body_EnvUV(sp, w[BODY_NORMALS] >> 9);
    Ot_Link(ot, p, 7);
    sp->packetPtr = (char *)(p + 1);
}

static void Body_EnvQuad(CarScratch *sp, unsigned int *ot, unsigned int *w, int *xy)
{
    PolyFT4 *p = (PolyFT4 *)sp->packetPtr;

    p->rgbc = sp->envFT4;
    p->xy0 = xy[0];
    p->uv0 = Body_EnvUV(sp, w[BODY_FLAGS] >> 4);
    p->clut = sp->envClut;
    p->xy1 = xy[1];
    p->uv1 = Body_EnvUV(sp, w[BODY_NORMALS]);
    p->tpage = sp->envTpage;
    p->xy2 = xy[3];
    p->uv2 = Body_EnvUV(sp, w[BODY_NORMALS] >> 18);
    p->xy3 = xy[2];
    p->uv3 = Body_EnvUV(sp, w[BODY_NORMALS] >> 9);
    Ot_Link(ot, p, 9);
    sp->packetPtr = (char *)(p + 1);
}

// A textured face's CLUT word: its own, with flags >> 14 masked by clutSelect and the
// instance's detail added.
static inline unsigned int Body_ClutWord(CarScratch *sp, unsigned int word, unsigned int flags)
{
    return word + (((int)flags >> 14) & sp->clutSelect) + sp->detail;
}

static inline void SubVert_Set(SubVert *s, BodyVert *v, int sxy, unsigned int uv, unsigned int rgb)
{
    *(int *)&s->x = v->xy;
    s->z = v->z;
    *(unsigned short *)&s->u = uv;
    s->sxy = sxy;
    s->rgb = rgb;
}

static inline unsigned int SubVert_UV(SubVert *s)
{
    return *(unsigned short *)&s->u;
}

static inline unsigned int Sub_Mid(unsigned int a, unsigned int b)
{
    return ((a & 0xfefefe) + (b & 0xfefefe)) >> 1;
}

// m = the midpoint of a and b (position, texture coordinates and colour).
static void SubVert_Mid(SubVert *m, SubVert *a, SubVert *b)
{
    m->x = (a->x + b->x) >> 1;
    m->y = (a->y + b->y) >> 1;
    m->z = (a->z + b->z) >> 1;
    m->u = (a->u + b->u) >> 1;
    m->v = (a->v + b->v) >> 1;
    m->rgb = Sub_Mid(a->rgb, b->rgb);
}

static void SubVert_Project(SubVert *s)
{
    gte_SetDataReg(GTE_VXY0, *(int *)&s->x);
    gte_SetDataReg(GTE_VZ0, s->z);
    gte_rtps();
    s->sxy = gte_GetDataReg(GTE_SXY2);
}

// Adds a packet to the face's chain: each one's tag points to the next address, and the
// chain goes into the OT when the face is done.
static void Sub_Chain(CarScratch *sp, SubFace *f, void *p, int len, int size)
{
    *(unsigned int *)p = len << 24 | (((unsigned int)p + size) & 0xffffff);
    f->last = (char *)p;
    sp->packetPtr = (char *)p + size;
}

// A textured (FT3) or Gouraud-textured (GT3) triangle of a subdivided face; code is the
// command word (its top byte for GT3).
static void Sub_EmitTri(CarScratch *sp, SubFace *f, unsigned int code, SubVert *a, SubVert *b, SubVert *c)
{
    if (!f->gouraud) {
        PolyFT3 *p = (PolyFT3 *)sp->packetPtr;

        p->rgbc = code;
        p->xy0 = a->sxy;
        p->uv0 = SubVert_UV(a);
        p->clut = f->texWord;
        p->xy1 = b->sxy;
        p->uv1 = SubVert_UV(b);
        p->tpage = f->texWord >> 16;
        p->xy2 = c->sxy;
        p->uv2 = SubVert_UV(c);
        Sub_Chain(sp, f, p, 7, sizeof(PolyFT3));
    } else {
        PolyGT3 *p = (PolyGT3 *)sp->packetPtr;

        p->rgbc0 = (a->rgb & 0xffffff) | (code & 0xff000000);
        p->xy0 = a->sxy;
        p->uv0 = SubVert_UV(a);
        p->clut = f->texWord;
        p->rgb1 = b->rgb;
        p->xy1 = b->sxy;
        p->uv1 = SubVert_UV(b);
        p->tpage = f->texWord >> 16;
        p->rgb2 = c->rgb;
        p->xy2 = c->sxy;
        p->uv2 = SubVert_UV(c);
        Sub_Chain(sp, f, p, 9, sizeof(PolyGT3));
    }
}

// A quad (corner a, then b and c next to it, d opposite) of a subdivided face. The
// Gouraud version gives the third and fourth points each other's colour, as the original.
static void Sub_EmitQuad(CarScratch *sp, SubFace *f, SubVert *a, SubVert *b, SubVert *c, SubVert *d)
{
    if (!f->gouraud) {
        PolyFT4 *p = (PolyFT4 *)sp->packetPtr;

        p->rgbc = f->code;
        p->xy0 = a->sxy;
        p->uv0 = SubVert_UV(a);
        p->clut = f->texWord;
        p->xy1 = b->sxy;
        p->uv1 = SubVert_UV(b);
        p->tpage = f->texWord >> 16;
        p->xy2 = c->sxy;
        p->uv2 = SubVert_UV(c);
        p->xy3 = d->sxy;
        p->uv3 = SubVert_UV(d);
        Sub_Chain(sp, f, p, 9, sizeof(PolyFT4));
    } else {
        PolyGT4 *p = (PolyGT4 *)sp->packetPtr;

        p->rgbc0 = (a->rgb & 0xffffff) | (f->code & 0xff000000);
        p->xy0 = a->sxy;
        p->uv0 = SubVert_UV(a);
        p->clut = f->texWord;
        p->rgb1 = b->rgb;
        p->xy1 = b->sxy;
        p->uv1 = SubVert_UV(b);
        p->tpage = f->texWord >> 16;
        p->rgb2 = d->rgb;
        p->xy2 = c->sxy;
        p->uv2 = SubVert_UV(c);
        p->rgb3 = c->rgb;
        p->xy3 = d->sxy;
        p->uv3 = SubVert_UV(d);
        Sub_Chain(sp, f, p, 12, sizeof(PolyGT4));
    }
}

// Splits triangle c into four at its edges' midpoints (m[k] between c[k] and c[k + 1]).
// First come the three slivers between each edge and its midpoint, which fill the cracks
// the projected midpoints leave; then the corner triangles and the middle one, each
// subdivided again while its area is above the threshold. Returns 0 when the packets ran
// out (the original then drops the whole face).
static int Body_SubdivTri(CarScratch *sp, SubFace *f, SubVert **c, int depth)
{
    SubVert m[3];
    int a;

    SubVert_Mid(&m[0], c[0], c[1]);
    SubVert_Mid(&m[1], c[1], c[2]);
    SubVert_Mid(&m[2], c[2], c[0]);
    SubVert_Project(&m[0]);
    SubVert_Project(&m[1]);
    SubVert_Project(&m[2]);
    for (a = 2; a >= 0; a--) {
        int prev = a == 0 ? 2 : a - 1;

        if (Body_NClip(c[a]->sxy, m[prev].sxy, c[prev]->sxy) != 0) {
            Sub_EmitTri(sp, f, f->code, c[a], &m[prev], c[prev]);
        }
    }
    if (sp->packetLimit < sp->packetPtr) {
        return 0;
    }
    for (a = 3; a >= 0; a--) {
        SubVert *t[3];
        int area;

        if (a == 3) {
            t[0] = &m[0];
            t[1] = &m[1];
            t[2] = &m[2];
        } else {
            t[0] = c[a];
            t[1] = &m[a];
            t[2] = &m[a == 0 ? 2 : a - 1];
        }
        area = Body_NClip(t[0]->sxy, t[1]->sxy, t[2]->sxy);
        if (area < 0) {
            area = -area;
        }
        if ((unsigned int)area > (unsigned int)f->threshold) {
            if (depth < SUBDIV_MAX_DEPTH && !Body_SubdivTri(sp, f, t, depth + 1)) {
                return 0;
            }
        } else {
            Sub_EmitTri(sp, f, f->code, t[0], t[1], t[2]);
        }
    }
    return 1;
}

// Splits quad c (corners in order round it) into four at its centre and edge midpoints,
// with the same crack slivers (as triangles), alternating where each part's corner list
// starts. The Gouraud version calls Gpu_DrawSync before each projection after the
// centre's, and starts the list at the part's corner only for the last part (it tests
// the byte it keeps the part index in rather than its low bit).
static int Body_SubdivQuad(CarScratch *sp, SubFace *f, SubVert **c, int depth)
{
    SubVert ctr;
    SubVert m[4];
    unsigned int slivers = f->code - 0x08000000;     // the triangle version of the code
    int a;

    ctr.x = (c[0]->x + c[1]->x + (c[2]->x + c[3]->x)) >> 2;
    ctr.y = (c[0]->y + c[1]->y + (c[2]->y + c[3]->y)) >> 2;
    ctr.z = (c[0]->z + c[1]->z + (c[2]->z + c[3]->z)) >> 2;
    ctr.u = (c[0]->u + c[1]->u + (c[2]->u + c[3]->u)) >> 2;
    ctr.v = (c[0]->v + c[1]->v + (c[2]->v + c[3]->v)) >> 2;
    for (a = 0; a < 4; a++) {
        SubVert_Mid(&m[a], c[a], c[(a + 1) & 3]);
    }
    ctr.rgb = Sub_Mid(m[0].rgb, m[2].rgb);
    SubVert_Project(&ctr);
    for (a = 0; a < 4; a++) {
        if (f->gouraud) {
            Gpu_DrawSync();
        }
        SubVert_Project(&m[a]);
    }
    if (f->gouraud) {
        Gpu_DrawSync();
    }
    for (a = 3; a >= 0; a--) {
        int prev = (a - 1) & 3;

        if (Body_NClip(c[a]->sxy, m[prev].sxy, c[prev]->sxy) != 0) {
            Sub_EmitTri(sp, f, slivers, c[a], &m[prev], c[prev]);
        }
    }
    if (sp->packetLimit < sp->packetPtr) {
        return 0;
    }
    for (a = 3; a >= 0; a--) {
        SubVert *corner = c[a];
        SubVert *next = &m[a];
        SubVert *prev = &m[(a - 1) & 3];
        int area = Body_NClip(next->sxy, corner->sxy, ctr.sxy) - Body_NClip(prev->sxy, corner->sxy, ctr.sxy);

        if (area < 0) {
            area = -area;
        }
        if ((unsigned int)area > (unsigned int)f->threshold) {
            SubVert *t[4];
            int odd = f->gouraud ? a != 0 : a & 1;

            if (depth >= SUBDIV_MAX_DEPTH) {
                continue;
            }
            if (odd) {
                t[0] = prev;
                t[1] = corner;
                t[2] = next;
                t[3] = &ctr;
            } else {
                t[0] = corner;
                t[1] = next;
                t[2] = &ctr;
                t[3] = prev;
            }
            if (!Body_SubdivQuad(sp, f, t, depth + 1)) {
                return 0;
            }
        } else {
            Sub_EmitQuad(sp, f, corner, next, prev, &ctr);
        }
    }
    return 1;
}

// Starts and finishes a subdivided face: the packets go into the face's OT entry as one
// chain, or all go when the packet area ran out.
static void SubFace_Begin(CarScratch *sp, SubFace *f, unsigned int *ot, unsigned int code, unsigned int tex, int threshold, int gouraud)
{
    f->ot = ot;
    f->code = code;
    f->texWord = tex;
    f->threshold = threshold;
    f->gouraud = gouraud;
    f->first = sp->packetPtr;
    f->last = sp->packetPtr;
}

static void SubFace_End(CarScratch *sp, SubFace *f, int ok)
{
    if (!ok || sp->packetLimit < sp->packetPtr) {
        sp->packetPtr = f->first;
        return;
    }
    if (sp->packetPtr != f->first) {
        unsigned int head = *f->ot;

        *f->ot = (head & 0xff000000) | ((unsigned int)f->first & 0xffffff);
        *(unsigned int *)f->last = (*(unsigned int *)f->last & 0xff000000) | (head & 0xffffff);
    }
}

// Handwritten: each type has its own loop in the original, with the packet writes in the
// GTE's latency slots; the subdivision keeps its frames in the scratchpad and is not
// recursive there.
void CarBody_DrawUnclipped(CarMesh *mesh, MATRIX *light)
{
    CarScratch *sp = (CarScratch *)0x1f800000;
    unsigned short env[0x204];
    unsigned int *w;
    unsigned int *end;
    BodyVert *v[4];
    int xy[4];
    int sz[4];
    int n1;
    int n2;
    unsigned int *ot;
    SubFace face;
    SubVert rec[4];
    SubVert *c[4];

    sp->verts = mesh->verts;
    Mesh_NormalsToEnvUV(mesh, light, (unsigned char *)env);
    gte_SetCtrlReg(GTE_TRX, sp->savedTrans[0]);
    gte_SetCtrlReg(GTE_TRY, sp->savedTrans[1]);
    gte_SetCtrlReg(GTE_TRZ, sp->savedTrans[2]);
    gte_SetRotMatrixWords(sp->savedRot);
    c[0] = &rec[0];
    c[1] = &rec[1];
    c[2] = &rec[2];
    c[3] = &rec[3];

    // Type 0: flat triangles.
    w = (unsigned int *)mesh->prims[0];
    end = w + mesh->primCount[0] * 4;
    for (; w != end; w += 4) {
        PolyF3 *p;

        if (!Body_TriVisible(sp, Body_ProjectTri(sp, w, v, xy, sz, &n1), n1)) {
            continue;
        }
        ot = Body_OtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], sz, 3));
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvTri(sp, ot, w, xy);
        }
        p = (PolyF3 *)sp->packetPtr;
        p->rgbc = w[BODY_DATA];
        p->xy0 = xy[0];
        p->xy1 = xy[1];
        p->xy2 = xy[2];
        Ot_Link(ot, p, 4);
        sp->packetPtr = (char *)(p + 1);
        if (sp->packetLimit < sp->packetPtr) {
            break;
        }
    }

    // Type 1: flat quads.
    w = (unsigned int *)mesh->prims[1];
    end = w + mesh->primCount[1] * 4;
    for (; w != end; w += 4) {
        PolyF4 *p;
        int flags = Body_ProjectQuad(sp, w, v, xy, sz, &n1, &n2);

        if (!Body_QuadVisible(sp, flags, n1, n2)) {
            continue;
        }
        ot = Body_OtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], sz, 4));
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvQuad(sp, ot, w, xy);
        }
        p = (PolyF4 *)sp->packetPtr;
        p->rgbc = w[BODY_DATA];
        p->xy0 = xy[0];
        p->xy1 = xy[1];
        p->xy2 = xy[3];
        p->xy3 = xy[2];
        Ot_Link(ot, p, 5);
        sp->packetPtr = (char *)(p + 1);
        if (sp->packetLimit < sp->packetPtr) {
            break;
        }
    }

    // Type 2: Gouraud triangles.
    w = (unsigned int *)mesh->prims[2];
    end = w + mesh->primCount[2] * 6;
    for (; w != end; w += 6) {
        PolyG3 *p;

        if (!Body_TriVisible(sp, Body_ProjectTri(sp, w, v, xy, sz, &n1), n1)) {
            continue;
        }
        ot = Body_OtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], sz, 3));
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvTri(sp, ot, w, xy);
        }
        p = (PolyG3 *)sp->packetPtr;
        p->rgbc0 = w[BODY_DATA];
        p->xy0 = xy[0];
        p->rgb1 = w[BODY_DATA + 1];
        p->xy1 = xy[1];
        p->rgb2 = w[BODY_DATA + 2];
        p->xy2 = xy[2];
        Ot_Link(ot, p, 6);
        sp->packetPtr = (char *)(p + 1);
        if (sp->packetLimit < sp->packetPtr) {
            break;
        }
    }

    // Type 3: Gouraud quads (the colours in vertex order).
    w = (unsigned int *)mesh->prims[3];
    end = w + mesh->primCount[3] * 7;
    for (; w != end; w += 7) {
        PolyG4 *p;
        int flags = Body_ProjectQuad(sp, w, v, xy, sz, &n1, &n2);

        if (!Body_QuadVisible(sp, flags, n1, n2)) {
            continue;
        }
        ot = Body_OtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], sz, 4));
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvQuad(sp, ot, w, xy);
        }
        p = (PolyG4 *)sp->packetPtr;
        p->rgbc0 = w[BODY_DATA];
        p->xy0 = xy[0];
        p->rgb1 = w[BODY_DATA + 1];
        p->xy1 = xy[1];
        p->rgb2 = w[BODY_DATA + 3];
        p->xy2 = xy[3];
        p->rgb3 = w[BODY_DATA + 2];
        p->xy3 = xy[2];
        Ot_Link(ot, p, 8);
        sp->packetPtr = (char *)(p + 1);
        if (sp->packetLimit < sp->packetPtr) {
            break;
        }
    }

    // Type 4: textured triangles: code, uv0 and CLUT, uv1 and texture page, uv2.
    w = (unsigned int *)mesh->prims[4];
    end = w + mesh->primCount[4] * 7;
    for (; w != end; w += 7) {
        unsigned int clut;

        if (!Body_TriVisible(sp, Body_ProjectTri(sp, w, v, xy, sz, &n1), n1)) {
            continue;
        }
        if (sp->mirror < 0) {
            n1 = -n1;
        }
        ot = Body_OtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], sz, 3));
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvTri(sp, ot, w, xy);
        }
        clut = Body_ClutWord(sp, w[BODY_DATA + 1], w[BODY_FLAGS]);
        if (n1 > 0x480) {
            SubFace_Begin(sp, &face, ot, w[BODY_DATA], (w[BODY_DATA + 2] & 0xffff0000) | clut >> 16, 0x480, 0);
            SubVert_Set(&rec[0], v[0], xy[0], clut, 0);
            SubVert_Set(&rec[1], v[1], xy[1], w[BODY_DATA + 2], 0);
            SubVert_Set(&rec[2], v[2], xy[2], w[BODY_DATA + 3], 0);
            SubFace_End(sp, &face, Body_SubdivTri(sp, &face, c, 0));
            if (!(sp->packetPtr < sp->packetLimit)) {
                break;
            }
        } else {
            PolyFT3 *p = (PolyFT3 *)sp->packetPtr;

            p->rgbc = w[BODY_DATA];
            p->xy0 = xy[0];
            *(unsigned int *)&p->uv0 = clut;
            p->xy1 = xy[1];
            *(unsigned int *)&p->uv1 = w[BODY_DATA + 2];
            p->xy2 = xy[2];
            *(unsigned int *)&p->uv2 = w[BODY_DATA + 3];
            Ot_Link(ot, p, 7);
            sp->packetPtr = (char *)(p + 1);
            if (sp->packetLimit < sp->packetPtr) {
                break;
            }
        }
    }

    // Type 5: textured quads: code, uv0 and CLUT, uv1 and texture page, uv2 | uv3 << 16
    // (the third and fourth vertices').
    w = (unsigned int *)mesh->prims[5];
    end = w + mesh->primCount[5] * 7;
    for (; w != end; w += 7) {
        unsigned int clut;
        int area;
        int flags = Body_ProjectQuad(sp, w, v, xy, sz, &n1, &n2);

        if (!Body_QuadVisible(sp, flags, n1, n2)) {
            continue;
        }
        area = n2 - n1;
        ot = Body_OtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], sz, 4));
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvQuad(sp, ot, w, xy);
        }
        clut = Body_ClutWord(sp, w[BODY_DATA + 1], w[BODY_FLAGS]);
        if (area < 0) {
            area = -area;
        }
        if (area > 0x800) {
            SubFace_Begin(sp, &face, ot, w[BODY_DATA], (w[BODY_DATA + 2] & 0xffff0000) | clut >> 16, 0x800, 0);
            SubVert_Set(&rec[0], v[0], xy[0], clut, 0);
            SubVert_Set(&rec[1], v[1], xy[1], w[BODY_DATA + 2], 0);
            SubVert_Set(&rec[2], v[2], xy[2], w[BODY_DATA + 3], 0);
            SubVert_Set(&rec[3], v[3], xy[3], w[BODY_DATA + 3] >> 16, 0);
            SubFace_End(sp, &face, Body_SubdivQuad(sp, &face, c, 0));
            if (!(sp->packetPtr < sp->packetLimit)) {
                break;
            }
        } else {
            PolyFT4 *p = (PolyFT4 *)sp->packetPtr;

            p->rgbc = w[BODY_DATA];
            p->xy0 = xy[0];
            *(unsigned int *)&p->uv0 = clut;
            p->xy1 = xy[1];
            *(unsigned int *)&p->uv1 = w[BODY_DATA + 2];
            p->xy2 = xy[3];
            p->uv2 = w[BODY_DATA + 3] >> 16;
            p->xy3 = xy[2];
            *(unsigned int *)&p->uv3 = w[BODY_DATA + 3];
            Ot_Link(ot, p, 9);
            sp->packetPtr = (char *)(p + 1);
            if (sp->packetLimit < sp->packetPtr) {
                break;
            }
        }
    }

    // Type 6: Gouraud-textured triangles: three colours (the first with the code), uv0
    // and CLUT, uv1 and texture page, uv2.
    w = (unsigned int *)mesh->prims[6];
    end = w + mesh->primCount[6] * 9;
    for (; w != end; w += 9) {
        unsigned int clut;

        if (!Body_TriVisible(sp, Body_ProjectTri(sp, w, v, xy, sz, &n1), n1)) {
            continue;
        }
        if (sp->mirror < 0) {
            n1 = -n1;
        }
        ot = Body_OtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], sz, 3));
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvTri(sp, ot, w, xy);
        }
        clut = Body_ClutWord(sp, w[BODY_DATA + 3], w[BODY_FLAGS]);
        if (n1 > 0x480) {
            SubFace_Begin(sp, &face, ot, w[BODY_DATA], (w[BODY_DATA + 4] & 0xffff0000) | clut >> 16, 0x480, 1);
            SubVert_Set(&rec[0], v[0], xy[0], clut, w[BODY_DATA]);
            SubVert_Set(&rec[1], v[1], xy[1], w[BODY_DATA + 4], w[BODY_DATA + 1]);
            SubVert_Set(&rec[2], v[2], xy[2], w[BODY_DATA + 5], w[BODY_DATA + 2]);
            SubFace_End(sp, &face, Body_SubdivTri(sp, &face, c, 0));
            if (!(sp->packetPtr < sp->packetLimit)) {
                break;
            }
        } else {
            PolyGT3 *p = (PolyGT3 *)sp->packetPtr;

            p->rgbc0 = w[BODY_DATA];
            p->xy0 = xy[0];
            *(unsigned int *)&p->uv0 = clut;
            p->rgb1 = w[BODY_DATA + 1];
            p->xy1 = xy[1];
            *(unsigned int *)&p->uv1 = w[BODY_DATA + 4];
            p->rgb2 = w[BODY_DATA + 2];
            p->xy2 = xy[2];
            *(unsigned int *)&p->uv2 = w[BODY_DATA + 5];
            Ot_Link(ot, p, 9);
            sp->packetPtr = (char *)(p + 1);
            if (sp->packetLimit < sp->packetPtr) {
                break;
            }
        }
    }

    // Type 7: Gouraud-textured quads: four colours, uv0 and CLUT, uv1 and texture page,
    // uv2 | uv3 << 16.
    w = (unsigned int *)mesh->prims[7];
    end = w + mesh->primCount[7] * 10;
    for (; w != end; w += 10) {
        unsigned int clut;
        int area;
        int flags = Body_ProjectQuad(sp, w, v, xy, sz, &n1, &n2);

        if (!Body_QuadVisible(sp, flags, n1, n2)) {
            continue;
        }
        area = n2 - n1;
        ot = Body_OtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], sz, 4));
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvQuad(sp, ot, w, xy);
        }
        clut = Body_ClutWord(sp, w[BODY_DATA + 4], w[BODY_FLAGS]);
        if (area < 0) {
            area = -area;
        }
        if (area > 0x800) {
            // The original gives the last three corners the GTE's RGB0-RGB2 instead of the
            // face's colours (it stores both, the FIFO last).
            SubFace_Begin(sp, &face, ot, w[BODY_DATA], (w[BODY_DATA + 5] & 0xffff0000) | clut >> 16, 0x800, 1);
            SubVert_Set(&rec[0], v[0], xy[0], clut, w[BODY_DATA]);
            SubVert_Set(&rec[1], v[1], xy[1], w[BODY_DATA + 5], gte_GetDataReg(GTE_RGB0));
            SubVert_Set(&rec[2], v[2], xy[2], w[BODY_DATA + 6], gte_GetDataReg(GTE_RGB1));
            SubVert_Set(&rec[3], v[3], xy[3], w[BODY_DATA + 6] >> 16, gte_GetDataReg(GTE_RGB2));
            SubFace_End(sp, &face, Body_SubdivQuad(sp, &face, c, 0));
            if (!(sp->packetPtr < sp->packetLimit)) {
                break;
            }
        } else {
            PolyGT4 *p = (PolyGT4 *)sp->packetPtr;

            p->rgbc0 = w[BODY_DATA];
            p->xy0 = xy[0];
            *(unsigned int *)&p->uv0 = clut;
            p->rgb1 = w[BODY_DATA + 1];
            p->xy1 = xy[1];
            *(unsigned int *)&p->uv1 = w[BODY_DATA + 5];
            p->rgb2 = w[BODY_DATA + 3];
            p->xy2 = xy[3];
            p->uv2 = w[BODY_DATA + 6] >> 16;
            p->rgb3 = w[BODY_DATA + 2];
            p->xy3 = xy[2];
            *(unsigned int *)&p->uv3 = w[BODY_DATA + 6];
            Ot_Link(ot, p, 12);
            sp->packetPtr = (char *)(p + 1);
            if (sp->packetLimit < sp->packetPtr) {
                break;
            }
        }
    }
}

// Draws a car body mesh that crosses the screen's edges or comes near the camera. It first
// transforms every vertex (up to 256) into a table with its view-space position, screen
// position and clip code, as ShadowMesh_Draw does, then draws the 8 primitive lists as
// CarBody_DrawUnclipped does. A face wholly outside one plane is skipped. One with a
// vertex the GTE couldn't project is dropped when untextured; a textured one is clipped
// by subdividing it in view space until each part projects (Body_ClipSubdivTri,
// Body_ClipSubdivQuad), and gets no environment map layer.
// The clip code word (clip, then z in the top half) of a vertex.
static inline int ShadowVert_ClipZ(ShadowVert *v)
{
    return *(int *)&v->clip;
}

// The clip code of a subdivision vertex is the top byte of its colour word.
static inline unsigned int SubVert_Clip(SubVert *s)
{
    return s->rgb >> 24;
}

static inline void SubVert_SetClip(SubVert *s, unsigned int code)
{
    s->rgb = (s->rgb & 0xffffff) | code << 24;
}

// Projects a midpoint with the identity transform (the positions are in view space) and
// codes it against the near plane and the screen's edges: bit 0 nearer than nearZ, bits
// 1-4 left of, right of, above and below the screen, bit 5 a GTE error.
static void SubVert_ProjectClip(CarScratch *sp, SubVert *s)
{
    int flags;
    int z;
    int x;
    int y;
    unsigned int code;

    gte_SetDataReg(GTE_VXY0, *(int *)&s->x);
    gte_SetDataReg(GTE_VZ0, s->z);
    gte_rtps();
    flags = gte_GetCtrlReg(GTE_FLAG);
    z = gte_GetDataReg(GTE_IR3);
    s->sxy = gte_GetDataReg(GTE_SXY2);
    x = (short)s->sxy;
    y = s->sxy >> 16;
    code = z < (unsigned short)sp->nearZ;
    if (x < 0) {
        code |= 2;
    }
    if (x > (unsigned short)sp->screenW - 1) {
        code |= 4;
    }
    if (y < 0) {
        code |= 8;
    }
    if (y > (unsigned short)sp->screenH - 1) {
        code |= 0x10;
    }
    if (flags < 0) {
        code |= 0x20;
    }
    SubVert_SetClip(s, code);
}

// The corner records of a clipped face, from the vertex table. The original's records
// keep their colour words' low three bytes from earlier use of the scratchpad
// (0x1f8000b4 on, every 16 bytes): it never sets them, so a subdivided Gouraud-textured
// face gets those stale colours. A vertex that didn't project is projected again with the
// identity transform, and loses its error bit when that works.
static void SubVert_SetClipped(SubVert *s, ShadowVert *v, unsigned int uv, int k)
{
    *(int *)&s->x = *(int *)&v->x;
    s->z = v->z;
    *(unsigned short *)&s->u = uv;
    s->sxy = v->sxy;
    s->rgb = (((unsigned int *)0x1f8000b4)[k * 4] & 0xffffff) | (v->clip & 0xff) << 24;
    if (SubVert_Clip(s) & 0x20) {
        gte_SetDataReg(GTE_VXY0, *(int *)&s->x);
        gte_SetDataReg(GTE_VZ0, s->z);
        gte_rtps();
        s->sxy = gte_GetDataReg(GTE_SXY2);
        if (gte_GetCtrlReg(GTE_FLAG) >= 0) {
            SubVert_SetClip(s, SubVert_Clip(s) & 0x1f);
        }
    }
}

// Body_SubdivTri's clipping version: slivers aren't drawn when a point didn't project;
// a part wholly outside one plane is skipped, and one with a point that didn't project is
// subdivided again whatever its size.
static int Body_ClipSubdivTri(CarScratch *sp, SubFace *f, SubVert **c, int depth)
{
    SubVert m[3];
    int a;

    SubVert_Mid(&m[0], c[0], c[1]);
    SubVert_Mid(&m[1], c[1], c[2]);
    SubVert_Mid(&m[2], c[2], c[0]);
    SubVert_ProjectClip(sp, &m[0]);
    SubVert_ProjectClip(sp, &m[1]);
    SubVert_ProjectClip(sp, &m[2]);
    for (a = 2; a >= 0; a--) {
        int prev = a == 0 ? 2 : a - 1;

        if ((SubVert_Clip(c[a]) | SubVert_Clip(&m[prev]) | SubVert_Clip(c[prev])) & 0x20) {
            continue;
        }
        if (Body_NClip(c[a]->sxy, m[prev].sxy, c[prev]->sxy) != 0) {
            Sub_EmitTri(sp, f, f->code, c[a], &m[prev], c[prev]);
        }
    }
    if (sp->packetLimit < sp->packetPtr) {
        return 0;
    }
    for (a = 3; a >= 0; a--) {
        SubVert *t[3];
        unsigned int all;
        unsigned int any;
        int area;

        if (a == 3) {
            t[0] = &m[0];
            t[1] = &m[1];
            t[2] = &m[2];
        } else {
            t[0] = c[a];
            t[1] = &m[a];
            t[2] = &m[a == 0 ? 2 : a - 1];
        }
        all = SubVert_Clip(t[0]) & SubVert_Clip(t[1]) & SubVert_Clip(t[2]);
        any = SubVert_Clip(t[0]) | SubVert_Clip(t[1]) | SubVert_Clip(t[2]);
        if (all & 0x1f) {
            continue;
        }
        if (!(any & 0x20)) {
            area = Body_NClip(t[0]->sxy, t[1]->sxy, t[2]->sxy);
            if (area < 0) {
                area = -area;
            }
            if ((unsigned int)area <= (unsigned int)f->threshold) {
                Sub_EmitTri(sp, f, f->code, t[0], t[1], t[2]);
                continue;
            }
        }
        if (depth < SUBDIV_MAX_DEPTH && !Body_ClipSubdivTri(sp, f, t, depth + 1)) {
            return 0;
        }
    }
    return 1;
}

// Body_SubdivQuad's clipping version, with the same rules as Body_ClipSubdivTri. The
// Gouraud version doesn't call Gpu_DrawSync and gives each point its own colour, but
// starts the corner list as Body_SubdivQuad's does.
static int Body_ClipSubdivQuad(CarScratch *sp, SubFace *f, SubVert **c, int depth)
{
    SubVert ctr;
    SubVert m[4];
    unsigned int slivers = f->code - 0x08000000;     // the triangle version of the code
    int a;

    ctr.x = (c[0]->x + c[1]->x + (c[2]->x + c[3]->x)) >> 2;
    ctr.y = (c[0]->y + c[1]->y + (c[2]->y + c[3]->y)) >> 2;
    ctr.z = (c[0]->z + c[1]->z + (c[2]->z + c[3]->z)) >> 2;
    ctr.u = (c[0]->u + c[1]->u + (c[2]->u + c[3]->u)) >> 2;
    ctr.v = (c[0]->v + c[1]->v + (c[2]->v + c[3]->v)) >> 2;
    for (a = 0; a < 4; a++) {
        SubVert_Mid(&m[a], c[a], c[(a + 1) & 3]);
    }
    ctr.rgb = Sub_Mid(m[0].rgb, m[2].rgb);
    SubVert_ProjectClip(sp, &ctr);
    for (a = 0; a < 4; a++) {
        SubVert_ProjectClip(sp, &m[a]);
    }
    for (a = 3; a >= 0; a--) {
        int prev = (a - 1) & 3;

        if ((SubVert_Clip(c[a]) | SubVert_Clip(&m[prev]) | SubVert_Clip(c[prev])) & 0x20) {
            continue;
        }
        if (Body_NClip(c[a]->sxy, m[prev].sxy, c[prev]->sxy) != 0) {
            Sub_EmitTri(sp, f, slivers, c[a], &m[prev], c[prev]);
        }
    }
    if (sp->packetLimit < sp->packetPtr) {
        return 0;
    }
    for (a = 3; a >= 0; a--) {
        SubVert *corner = c[a];
        SubVert *next = &m[a];
        SubVert *prev = &m[(a - 1) & 3];
        unsigned int all = SubVert_Clip(corner) & SubVert_Clip(next) & SubVert_Clip(&ctr) & SubVert_Clip(prev);
        unsigned int any = SubVert_Clip(corner) | SubVert_Clip(next) | SubVert_Clip(&ctr) | SubVert_Clip(prev);
        SubVert *t[4];
        int odd;

        if (all & 0x1f) {
            continue;
        }
        if (!(any & 0x20)) {
            int area = Body_NClip(next->sxy, corner->sxy, ctr.sxy) - Body_NClip(prev->sxy, corner->sxy, ctr.sxy);

            if (area < 0) {
                area = -area;
            }
            if ((unsigned int)area <= (unsigned int)f->threshold) {
                if (f->gouraud) {
                    // The colours go with their own points here.
                    PolyGT4 *p = (PolyGT4 *)sp->packetPtr;

                    p->rgbc0 = (corner->rgb & 0xffffff) | (f->code & 0xff000000);
                    p->xy0 = corner->sxy;
                    p->uv0 = SubVert_UV(corner);
                    p->clut = f->texWord;
                    p->rgb1 = next->rgb;
                    p->xy1 = next->sxy;
                    p->uv1 = SubVert_UV(next);
                    p->tpage = f->texWord >> 16;
                    p->rgb2 = prev->rgb;
                    p->xy2 = prev->sxy;
                    p->uv2 = SubVert_UV(prev);
                    p->rgb3 = ctr.rgb;
                    p->xy3 = ctr.sxy;
                    p->uv3 = SubVert_UV(&ctr);
                    Sub_Chain(sp, f, p, 12, sizeof(PolyGT4));
                } else {
                    Sub_EmitQuad(sp, f, corner, next, prev, &ctr);
                }
                continue;
            }
        }
        if (depth >= SUBDIV_MAX_DEPTH) {
            continue;
        }
        odd = f->gouraud ? a != 0 : a & 1;
        if (odd) {
            t[0] = prev;
            t[1] = corner;
            t[2] = next;
            t[3] = &ctr;
        } else {
            t[0] = corner;
            t[1] = next;
            t[2] = &ctr;
            t[3] = prev;
        }
        if (!Body_ClipSubdivQuad(sp, f, t, depth + 1)) {
            return 0;
        }
    }
    return 1;
}

// A face's vertices in the table (a byte index each, 16 bytes a record).
static inline ShadowVert *Body_ClipVert(CarScratch *sp, unsigned int w, int shift)
{
    return &((ShadowVert *)sp->verts)[(w >> shift) & 0xff];
}

// The OT entry of a clipped face: its sort word (clip code, z on top) shifted down.
static inline unsigned int *Body_ClipOtEntry(CarScratch *sp, unsigned int flags, int word)
{
    int z = (word >> 16) << sp->otShift;
    unsigned int i;

    if (z < 0) {
        z = 0;
    }
    i = (unsigned int)z >> 13;
    if (i >= 0x1000) {
        i = 0xfff;
    }
    return sp->ot + i + ((flags >> 1) & 0xf) - 8;
}

// The visibility tests with clip codes: a face with a point that didn't project passes
// (its nclip can't be trusted); a mirrored triangle passes up to an nclip of 1.
static inline int Body_ClipTriVisible(CarScratch *sp, int area, unsigned int any)
{
    int err = any & 0x20;
    int v = area - 1;

    if ((area | err) == 0) {
        return 0;
    }
    if (sp->mirror < 0) {
        v = -v;
    }
    return (v & (err - 0x20)) >= 0;
}

static inline int Body_ClipQuadVisible(CarScratch *sp, int n1, int n2, unsigned int any)
{
    int err = any & 0x20;
    int a = -n1 - 1;
    int b = n2 - 1;
    int v;

    if ((-n1 | n2 | err) == 0) {
        return 0;
    }
    v = sp->mirror >= 0 ? a & b : ~(a | b);
    return (v & (err - 0x20)) >= 0;
}

// Handwritten: like CarBody_DrawUnclipped, with the vertex loop software-pipelined (its
// first pass writes a record before the table from stale registers).
void CarBody_DrawClipped(CarMesh *mesh, MATRIX *light)
{
    CarScratch *sp = (CarScratch *)0x1f800000;
    ShadowVert table[256];
    unsigned short env[0x204];
    BodyVert *bv;
    unsigned int *w;
    unsigned int *end;
    ShadowVert *v[4];
    int xy[4];
    int cz[4];
    int n1;
    int n2;
    int nearHalf;
    unsigned int all;
    unsigned int any;
    unsigned int *ot;
    SubFace face;
    SubVert rec[4];
    SubVert *c[4];
    int i;

    if (mesh->vertCount == 0) {
        return;
    }
    nearHalf = (unsigned short)sp->nearZ >> 1;
    sp->verts = (char *)table;
    bv = (BodyVert *)mesh->verts;
    for (i = 0; i < mesh->vertCount; i++) {
        ShadowVert *t = &table[i];
        int x;
        int y;
        int z;
        int flags;
        int code;
        int l1;
        int l2;
        int c1;
        int c2;

        gte_SetDataReg(GTE_VXY0, bv[i].xy);
        gte_SetDataReg(GTE_VZ0, bv[i].z);
        gte_rtps();
        x = gte_GetDataReg(GTE_IR1);
        y = gte_GetDataReg(GTE_IR2);
        z = gte_GetDataReg(GTE_IR3);
        flags = gte_GetCtrlReg(GTE_FLAG);
        t->sxy = gte_GetDataReg(GTE_SXY2);
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
        t->x = x;
        t->y = y;
        t->z = z;
        t->clip = code;
        t->extent = (x < 0 ? -x : x) | (y < 0 ? -y : y) | (z < 0 ? -z : z);
    }
    Mesh_NormalsToEnvUV(mesh, light, (unsigned char *)env);
    // The subdivision projects view-space points.
    gte_SetCtrlReg(GTE_TRX, 0);
    gte_SetCtrlReg(GTE_TRY, 0);
    gte_SetCtrlReg(GTE_TRZ, 0);
    gte_SetCtrlReg(GTE_R11R12, 0x1000);
    gte_SetCtrlReg(GTE_R13R21, 0);
    gte_SetCtrlReg(GTE_R22R23, 0x1000);
    gte_SetCtrlReg(GTE_R31R32, 0);
    gte_SetCtrlReg(GTE_R33, 0x1000);
    c[0] = &rec[0];
    c[1] = &rec[1];
    c[2] = &rec[2];
    c[3] = &rec[3];

#define CLIP_TRI_SETUP()                                                        \
    v[0] = Body_ClipVert(sp, w[BODY_VERTS], 0);                                 \
    v[1] = Body_ClipVert(sp, w[BODY_VERTS], 8);                                 \
    v[2] = Body_ClipVert(sp, w[BODY_VERTS], 16);                                \
    cz[0] = ShadowVert_ClipZ(v[0]);                                             \
    cz[1] = ShadowVert_ClipZ(v[1]);                                             \
    cz[2] = ShadowVert_ClipZ(v[2]);                                             \
    all = cz[0] & cz[1] & cz[2];                                                \
    any = cz[0] | cz[1] | cz[2];                                                \
    if (all & 0x1f) {                                                           \
        continue;                                                               \
    }                                                                           \
    xy[0] = v[0]->sxy;                                                          \
    xy[1] = v[1]->sxy;                                                          \
    xy[2] = v[2]->sxy;                                                          \
    n1 = Body_NClip(xy[0], xy[2], xy[1]);                                       \
    ot = Body_ClipOtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], cz, 3)); \
    if (!Body_ClipTriVisible(sp, n1, any)) {                                    \
        continue;                                                               \
    }

#define CLIP_QUAD_SETUP()                                                       \
    v[0] = Body_ClipVert(sp, w[BODY_VERTS], 0);                                 \
    v[1] = Body_ClipVert(sp, w[BODY_VERTS], 8);                                 \
    v[2] = Body_ClipVert(sp, w[BODY_VERTS], 16);                                \
    v[3] = Body_ClipVert(sp, w[BODY_VERTS], 24);                                \
    cz[0] = ShadowVert_ClipZ(v[0]);                                             \
    cz[1] = ShadowVert_ClipZ(v[1]);                                             \
    cz[2] = ShadowVert_ClipZ(v[2]);                                             \
    cz[3] = ShadowVert_ClipZ(v[3]);                                             \
    all = cz[0] & cz[1] & cz[2] & cz[3];                                        \
    any = cz[0] | cz[1] | cz[2] | cz[3];                                        \
    if (all & 0x1f) {                                                           \
        continue;                                                               \
    }                                                                           \
    xy[0] = v[0]->sxy;                                                          \
    xy[1] = v[1]->sxy;                                                          \
    xy[2] = v[2]->sxy;                                                          \
    xy[3] = v[3]->sxy;                                                          \
    n1 = Body_NClip(xy[1], xy[2], xy[0]);                                       \
    n2 = Body_NClip(xy[2], xy[0], xy[3]);                                       \
    ot = Body_ClipOtEntry(sp, w[BODY_FLAGS], Body_SortZ(w[BODY_FLAGS], cz, 4)); \
    if (!Body_ClipQuadVisible(sp, n1, n2, any)) {                               \
        continue;                                                               \
    }

    // Type 0: flat triangles (dropped when a point didn't project).
    w = (unsigned int *)mesh->prims[0];
    end = w + mesh->primCount[0] * 4;
    for (; w != end; w += 4) {
        PolyF3 *p;

        CLIP_TRI_SETUP();
        if (any & 0x20) {
            continue;
        }
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvTri(sp, ot, w, xy);
        }
        p = (PolyF3 *)sp->packetPtr;
        p->rgbc = w[BODY_DATA];
        p->xy0 = xy[0];
        p->xy1 = xy[1];
        p->xy2 = xy[2];
        Ot_Link(ot, p, 4);
        sp->packetPtr = (char *)(p + 1);
        if (sp->packetLimit < sp->packetPtr) {
            break;
        }
    }

    // Type 1: flat quads.
    w = (unsigned int *)mesh->prims[1];
    end = w + mesh->primCount[1] * 4;
    for (; w != end; w += 4) {
        PolyF4 *p;

        CLIP_QUAD_SETUP();
        if (any & 0x20) {
            continue;
        }
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvQuad(sp, ot, w, xy);
        }
        p = (PolyF4 *)sp->packetPtr;
        p->rgbc = w[BODY_DATA];
        p->xy0 = xy[0];
        p->xy1 = xy[1];
        p->xy2 = xy[3];
        p->xy3 = xy[2];
        Ot_Link(ot, p, 5);
        sp->packetPtr = (char *)(p + 1);
        if (sp->packetLimit < sp->packetPtr) {
            break;
        }
    }

    // Type 2: Gouraud triangles.
    w = (unsigned int *)mesh->prims[2];
    end = w + mesh->primCount[2] * 6;
    for (; w != end; w += 6) {
        PolyG3 *p;

        CLIP_TRI_SETUP();
        if (any & 0x20) {
            continue;
        }
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvTri(sp, ot, w, xy);
        }
        p = (PolyG3 *)sp->packetPtr;
        p->rgbc0 = w[BODY_DATA];
        p->xy0 = xy[0];
        p->rgb1 = w[BODY_DATA + 1];
        p->xy1 = xy[1];
        p->rgb2 = w[BODY_DATA + 2];
        p->xy2 = xy[2];
        Ot_Link(ot, p, 6);
        sp->packetPtr = (char *)(p + 1);
        if (sp->packetLimit < sp->packetPtr) {
            break;
        }
    }

    // Type 3: Gouraud quads.
    w = (unsigned int *)mesh->prims[3];
    end = w + mesh->primCount[3] * 7;
    for (; w != end; w += 7) {
        PolyG4 *p;

        CLIP_QUAD_SETUP();
        if (any & 0x20) {
            continue;
        }
        if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
            Body_EnvQuad(sp, ot, w, xy);
        }
        p = (PolyG4 *)sp->packetPtr;
        p->rgbc0 = w[BODY_DATA];
        p->xy0 = xy[0];
        p->rgb1 = w[BODY_DATA + 1];
        p->xy1 = xy[1];
        p->rgb2 = w[BODY_DATA + 3];
        p->xy2 = xy[3];
        p->rgb3 = w[BODY_DATA + 2];
        p->xy3 = xy[2];
        Ot_Link(ot, p, 8);
        sp->packetPtr = (char *)(p + 1);
        if (sp->packetLimit < sp->packetPtr) {
            break;
        }
    }

    // Type 4: textured triangles.
    w = (unsigned int *)mesh->prims[4];
    end = w + mesh->primCount[4] * 7;
    for (; w != end; w += 7) {
        unsigned int clut;

        CLIP_TRI_SETUP();
        if (n1 < 0) {
            n1 = -n1;
        }
        if (!(any & 0x20)) {
            if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
                Body_EnvTri(sp, ot, w, xy);
            }
            if (n1 <= 0x480) {
                PolyFT3 *p = (PolyFT3 *)sp->packetPtr;

                p->rgbc = w[BODY_DATA];
                p->xy0 = xy[0];
                *(unsigned int *)&p->uv0 = Body_ClutWord(sp, w[BODY_DATA + 1], w[BODY_FLAGS]);
                p->xy1 = xy[1];
                *(unsigned int *)&p->uv1 = w[BODY_DATA + 2];
                p->xy2 = xy[2];
                *(unsigned int *)&p->uv2 = w[BODY_DATA + 3];
                Ot_Link(ot, p, 7);
                sp->packetPtr = (char *)(p + 1);
                if (sp->packetLimit < sp->packetPtr) {
                    break;
                }
                continue;
            }
        }
        clut = Body_ClutWord(sp, w[BODY_DATA + 1], w[BODY_FLAGS]);
        SubFace_Begin(sp, &face, ot, w[BODY_DATA], (w[BODY_DATA + 2] & 0xffff0000) | clut >> 16, 0x480, 0);
        SubVert_SetClipped(&rec[0], v[0], w[BODY_DATA + 1], 0);
        SubVert_SetClipped(&rec[1], v[1], w[BODY_DATA + 2], 1);
        SubVert_SetClipped(&rec[2], v[2], w[BODY_DATA + 3], 2);
        SubFace_End(sp, &face, Body_ClipSubdivTri(sp, &face, c, 0));
        if (!(sp->packetPtr < sp->packetLimit)) {
            break;
        }
    }

    // Type 5: textured quads.
    w = (unsigned int *)mesh->prims[5];
    end = w + mesh->primCount[5] * 7;
    for (; w != end; w += 7) {
        unsigned int clut;
        int area;

        CLIP_QUAD_SETUP();
        area = n2 - n1;
        if (area < 0) {
            area = -area;
        }
        if (!(any & 0x20)) {
            if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
                Body_EnvQuad(sp, ot, w, xy);
            }
            if (area <= 0x800) {
                PolyFT4 *p = (PolyFT4 *)sp->packetPtr;

                p->rgbc = w[BODY_DATA];
                p->xy0 = xy[0];
                *(unsigned int *)&p->uv0 = Body_ClutWord(sp, w[BODY_DATA + 1], w[BODY_FLAGS]);
                p->xy1 = xy[1];
                *(unsigned int *)&p->uv1 = w[BODY_DATA + 2];
                p->xy2 = xy[3];
                p->uv2 = w[BODY_DATA + 3] >> 16;
                p->xy3 = xy[2];
                *(unsigned int *)&p->uv3 = w[BODY_DATA + 3];
                Ot_Link(ot, p, 9);
                sp->packetPtr = (char *)(p + 1);
                if (sp->packetLimit < sp->packetPtr) {
                    break;
                }
                continue;
            }
        }
        clut = Body_ClutWord(sp, w[BODY_DATA + 1], w[BODY_FLAGS]);
        SubFace_Begin(sp, &face, ot, w[BODY_DATA], (w[BODY_DATA + 2] & 0xffff0000) | clut >> 16, 0x800, 0);
        SubVert_SetClipped(&rec[0], v[0], clut, 0);
        SubVert_SetClipped(&rec[1], v[1], w[BODY_DATA + 2], 1);
        SubVert_SetClipped(&rec[2], v[2], w[BODY_DATA + 3], 2);
        SubVert_SetClipped(&rec[3], v[3], w[BODY_DATA + 3] >> 16, 3);
        SubFace_End(sp, &face, Body_ClipSubdivQuad(sp, &face, c, 0));
        if (!(sp->packetPtr < sp->packetLimit)) {
            break;
        }
    }

    // Type 6: Gouraud-textured triangles.
    w = (unsigned int *)mesh->prims[6];
    end = w + mesh->primCount[6] * 9;
    for (; w != end; w += 9) {
        unsigned int clut;

        CLIP_TRI_SETUP();
        if (n1 < 0) {
            n1 = -n1;
        }
        if (!(any & 0x20)) {
            if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
                Body_EnvTri(sp, ot, w, xy);
            }
            if (n1 <= 0x480) {
                PolyGT3 *p = (PolyGT3 *)sp->packetPtr;

                p->rgbc0 = w[BODY_DATA];
                p->xy0 = xy[0];
                *(unsigned int *)&p->uv0 = Body_ClutWord(sp, w[BODY_DATA + 3], w[BODY_FLAGS]);
                p->rgb1 = w[BODY_DATA + 1];
                p->xy1 = xy[1];
                *(unsigned int *)&p->uv1 = w[BODY_DATA + 4];
                p->rgb2 = w[BODY_DATA + 2];
                p->xy2 = xy[2];
                *(unsigned int *)&p->uv2 = w[BODY_DATA + 5];
                Ot_Link(ot, p, 9);
                sp->packetPtr = (char *)(p + 1);
                if (sp->packetLimit < sp->packetPtr) {
                    break;
                }
                continue;
            }
        }
        clut = Body_ClutWord(sp, w[BODY_DATA + 3], w[BODY_FLAGS]);
        SubFace_Begin(sp, &face, ot, w[BODY_DATA], (w[BODY_DATA + 4] & 0xffff0000) | clut >> 16, 0x480, 1);
        SubVert_SetClipped(&rec[0], v[0], w[BODY_DATA + 3], 0);
        SubVert_SetClipped(&rec[1], v[1], w[BODY_DATA + 4], 1);
        SubVert_SetClipped(&rec[2], v[2], w[BODY_DATA + 5], 2);
        SubFace_End(sp, &face, Body_ClipSubdivTri(sp, &face, c, 0));
        if (!(sp->packetPtr < sp->packetLimit)) {
            break;
        }
    }

    // Type 7: Gouraud-textured quads.
    w = (unsigned int *)mesh->prims[7];
    end = w + mesh->primCount[7] * 10;
    for (; w != end; w += 10) {
        unsigned int clut;
        int area;

        CLIP_QUAD_SETUP();
        area = n2 - n1;
        if (area < 0) {
            area = -area;
        }
        if (!(any & 0x20)) {
            if ((int)(w[BODY_FLAGS] & sp->lit) < 0) {
                Body_EnvQuad(sp, ot, w, xy);
            }
            if (area <= 0x800) {
                PolyGT4 *p = (PolyGT4 *)sp->packetPtr;

                p->rgbc0 = w[BODY_DATA];
                p->xy0 = xy[0];
                *(unsigned int *)&p->uv0 = Body_ClutWord(sp, w[BODY_DATA + 4], w[BODY_FLAGS]);
                p->rgb1 = w[BODY_DATA + 1];
                p->xy1 = xy[1];
                *(unsigned int *)&p->uv1 = w[BODY_DATA + 5];
                p->rgb2 = w[BODY_DATA + 3];
                p->xy2 = xy[3];
                p->uv2 = w[BODY_DATA + 6] >> 16;
                p->rgb3 = w[BODY_DATA + 2];
                p->xy3 = xy[2];
                *(unsigned int *)&p->uv3 = w[BODY_DATA + 6];
                Ot_Link(ot, p, 12);
                sp->packetPtr = (char *)(p + 1);
                if (sp->packetLimit < sp->packetPtr) {
                    break;
                }
                continue;
            }
        }
        clut = Body_ClutWord(sp, w[BODY_DATA + 4], w[BODY_FLAGS]);
        SubFace_Begin(sp, &face, ot, w[BODY_DATA], (w[BODY_DATA + 5] & 0xffff0000) | clut >> 16, 0x800, 1);
        SubVert_SetClipped(&rec[0], v[0], clut, 0);
        SubVert_SetClipped(&rec[1], v[1], w[BODY_DATA + 5], 1);
        SubVert_SetClipped(&rec[2], v[2], w[BODY_DATA + 6], 2);
        SubVert_SetClipped(&rec[3], v[3], w[BODY_DATA + 6] >> 16, 3);
        SubFace_End(sp, &face, Body_ClipSubdivQuad(sp, &face, c, 0));
        if (!(sp->packetPtr < sp->packetLimit)) {
            break;
        }
    }
#undef CLIP_TRI_SETUP
#undef CLIP_QUAD_SETUP
}

// Draws a wheel mesh (from CarModel_DrawWheel, with the wheel's transform loaded): strips
// of vertex pairs, each pair of pairs a flat quad in the wheel colour when it faces the
// camera (black otherwise), sorted on its nearest or farthest corner.
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

// Draws wheel index (0-3; bit 0 the right side, bit 1 the rear) of a car: the wheel mesh
// of LOD lod rotated by the wheel's angles (mirrored on the right) and, over it, the hub
// as two textured quads around its centre.

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
    Gte_LoadTransform((ViewScratch *)sp, 0x1000, 0x10, 0xc);
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
    gte_StoreDataReg(GTE_SXY2, &q->xy1);
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
    gte_StoreDataReg(GTE_SXY1, &q->xy2);
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

// Draws a car: copies the camera to the scratchpad, composes the world matrix, picks the LOD
// (inst->lod - 1, or by distance; -1 when too far), culls the bounding box, then draws the
// body, the wheels when near enough and the shadow. Returns the LOD used.
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
    if (lod == 0) {
        distSq = Lod_DistanceSq(table, (int *)sp->view.t, cam->depthScale);
        lod = Lod_Select(table, distSq);
        if (lod == -1) {
            return -1;
        }
    } else {
        lod--;
    }
    Matrix_Mul(&light, (MATRIX *)cam->invRot, world);
    mesh = LodAt(table, lod)->mesh;
    Gte_LoadTransform((ViewScratch *)sp, mesh->unk4e, mesh->unk4c, 0xc);
    sp->verts = mesh->verts;
    sp->normals = mesh->normals;
    clip = Clip_TestBox((ViewScratch *)sp, mesh->bboxMin, mesh->bboxMax);
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

// Uploads CLUT row of VRAM car slot slot.
void Clut_UploadToSlot(void *clut, unsigned int slot, int row)
{
    RECT rect;

    rect.x = (slot & 0xf) << 6;
    rect.y = ((slot << 4) & 0x100) + row * 4 + 0xe0;
    rect.w = 0x40;
    rect.h = 4;
    Gpu_TransferImage(&rect, (unsigned int *)clut, 0);
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
    Gpu_TransferImage(&rect, (unsigned int *)image, 0);
    Gpu_DrawSync();
}

// Loads a TIM's image (and its CLUT when withClut) into VRAM car slot slot.
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

// Blends a 256-colour CLUT towards rgb (24-bit) by t/4096 with the GTE's intpl, leaving
// the colours whose bit is set in the 16 row masks after it. A colour that blends to 0
// (transparent) becomes 0x8000; the blend of 0 stays as it is.
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

// Draws a car's shadow mesh (with CarModel_DrawShadow's transform loaded) as
// subtractive Gouraud triangles and quads: transforms the vertices (up to 64) into a
// stack table with their clip codes, then draws each face that isn't wholly outside one
// plane, through Poly_Clip when it crosses one.

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
            Poly_Clip((ViewScratch *)sp, 3);
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
            Poly_Clip((ViewScratch *)sp, 4);
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

void CarModel_DrawShadow(Camera *cam, CarModelInst *inst, CarModelHeader *model)
{
    void *mesh = model->shadowMesh;
    MATRIX *s = (MATRIX *)0x1f800000;

    *(CameraView *)s = *(CameraView *)&cam->view;
    Matrix_Comp(s, &inst->shadow);
    Gte_LoadTransform((ViewScratch *)s, ((unsigned short *)mesh)[0xd], ((unsigned short *)mesh)[0xc], 0xc);
    ShadowMesh_Draw(mesh);
}
}
