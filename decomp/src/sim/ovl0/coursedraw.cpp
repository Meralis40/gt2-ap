// OVL0 80018d1c-80026bb4 (coursedraw): Course, sky and course object drawing.
// The functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gte.h"
#include "gamestate.h"
#include "prim.h"

#define SCRATCH ((ViewScratch *)0x1f800000)

// A sky mesh: its vertices follow the header, then the eight primitive lists.
struct SkyMesh {
    int unk00;
    unsigned int color0;
    unsigned int color1;
    int vertCount;
    unsigned short counts[8];
    int verts[1];
};

// The flat quad Prim_AddPolyF4 returns.
struct PrimF4 {
    unsigned int rgb;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
};

extern "C" int g_MeshFrameParity;
extern "C" RaceWorld g_RaceWorld;

void Gte_LoadRotationOnly(ViewScratch *scr);
extern "C" void Sky_DrawHorizonFill(unsigned int *ot, Camera *cam, SkyMesh *sky);

#define SKY_F3(rgb, ax, ay, bx, by, cx, cy) \
    do { \
        PrimF3 *_p = Prim_AddPolyF3(ot, rgb); \
        \
        _p->x0 = ax; \
        _p->y0 = ay; \
        _p->x1 = bx; \
        _p->y1 = by; \
        _p->x2 = cx; \
        _p->y2 = cy; \
    } while (0)

#define SKY_F4(rgb, ax, ay, bx, by, cx, cy, dx, dy) \
    do { \
        PrimF4 *_p = (PrimF4 *)Prim_AddPolyF4(ot, rgb); \
        \
        _p->x0 = ax; \
        _p->y0 = ay; \
        _p->x1 = bx; \
        _p->y1 = by; \
        _p->x2 = cx; \
        _p->y2 = cy; \
        _p->x3 = dx; \
        _p->y3 = dy; \
    } while (0)

#ifdef NON_MATCHING
// A corner of a sprite, as the GTE loads it.
struct SpriteCorner {
    short x;
    short y;
    int z;
};

// The scratchpad while the course and its objects draw.
struct MeshScratch {
    ViewScratch view;
    SpriteCorner corners[4];    // 0x9c
    char unkbc[0x2dc];
    int subdivTri;              // 0x398
    int subdivQuad;
    char *spriteTex;
    char unk3a4[0x4c];
    short yawSin;               // 0x3f0: half the sine of the camera's heading
    short yawCos;
};

#define MESH_SCRATCH ((MeshScratch *)0x1f800000)
#define COURSE ((CourseData *)((char *)&g_Race_IsDemo + 0xb534))

// A textured quad as whole words.
struct PrimFT4Words {
    unsigned int tag;
    unsigned int rgb;
    int xy0;
    unsigned int uv0;
    int xy1;
    unsigned int uv1;
    int xy2;
    unsigned int uv2;
    int xy3;
    unsigned int uv3;
};

// A course sprite: it stands on the ground, turned towards the camera's heading.
struct CourseSprite {
    int xy;                     // x, y
    int ztex;                   // z, then the entry of the sprite texture table
    int size;                   // half width, height
    unsigned int rgb;
};

// An object's sprite.
struct ObjSprite {
    int xy;
    int z;
    int size;
    unsigned int rgb;
    unsigned int uv[3];
};

// A flare: a glow of four quads around a point, sized by depth.
struct MeshFlare {
    int xy;
    int zsize;                  // z, then the size
    int unk8[2];
    unsigned int rgb;
};

// A Gouraud textured quad.
struct PrimGT4 {
    unsigned int tag;
    unsigned int c0;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    unsigned int c1;
    short x1, y1;
    unsigned char u1, v1;
    unsigned short tpage;
    unsigned int c2;
    short x2, y2;
    unsigned char u2, v2;
    unsigned short pad;
    unsigned int c3;
    short x3, y3;
    unsigned char u3, v3;
    unsigned short pad2;
};

extern "C" {
extern char *g_PacketPtr;
extern char *g_PacketEnd;
extern short g_SinTable[];
extern unsigned char g_Race_IsDemo;

void CourseObj_DrawMeshUnclipped(CourseMesh *mesh);
void CourseObj_DrawMeshClipped(CourseMesh *mesh);
void CourseMesh_DrawUnclipped(CourseMesh *mesh);
void CourseMesh_DrawClipped(CourseMesh *mesh);
void CourseObjects_Draw(Camera *cam, unsigned int *group);
void CourseSection_LoadTransform(CourseSection *section, ViewScratch *view);
void Matrix_SwapYZ(MatrixRot *m);
void Gte_LoadRotationNoFog(ViewScratch *scr);
int CourseSection_TestBox(ViewScratch *scr, int *box);
unsigned int Vec_ApproxDist(ViewScratch *scr, int *pos);
}

// Loads the light matrix with the sprites' sideways axis.
static inline void Sprite_LoadAxis(int cos, int sinHigh)
{
    gte_SetCtrlReg(GTE_L11L12, cos);
    gte_SetCtrlReg(GTE_L13L21, sinHigh);
    gte_SetCtrlReg(GTE_L22L23, 0);
    gte_SetCtrlReg(GTE_L31L32, 0);
    gte_SetCtrlReg(GTE_L33, 0);
}

static inline void Sprite_RestoreAxis(MeshScratch *scr)
{
    int *light = (int *)scr->view.clipX;

    gte_SetCtrlReg(GTE_L11L12, light[0]);
    gte_SetCtrlReg(GTE_L13L21, light[1]);
    gte_SetCtrlReg(GTE_L22L23, light[2]);
    gte_SetCtrlReg(GTE_L31L32, light[3]);
    gte_SetCtrlReg(GTE_L33, light[4]);
}

// Turns half a width (in IR1) into the sprite's sideways offset.
static inline void Sprite_Offset(int halfWidth, int *dx, int *dy)
{
    gte_SetDataReg(GTE_IR1, halfWidth);
    gte_mvmva(1, 1, 3, 3, 0);
    *dx = gte_GetDataReg(GTE_IR1);
    *dy = gte_GetDataReg(GTE_IR2);
}

static inline void Sprite_Link(MeshScratch *scr, PrimFT4Words *prim, int z)
{
    if (z >= 0x1000) {
        z = 0xfff;
    }
    ot_AddPrim(&scr->view.ot[z], prim);
}

// Draws a section's sprites: two corners on the ground and two on top, at their z.
static inline void Course_DrawSprites(MeshScratch *scr, CourseMesh *mesh)
{
    int count = mesh->spriteCount;
    CourseSprite *sprite = (CourseSprite *)mesh->sprites;
    CourseSprite *end = sprite + count;
    PrimFT4Words *prim;

    if (count == 0) {
        return;
    }
    prim = (PrimFT4Words *)scr->view.packetPtr;
    Sprite_LoadAxis((unsigned short)scr->yawCos, (unsigned short)scr->yawSin << 16);
    for (; sprite != end; sprite++) {
        int dx, dy;
        int z = sprite->ztex;
        int top = z + ((unsigned int)sprite->size >> 16);
        int y = sprite->xy >> 16;
        unsigned int *tex;
        int flag;
        int sz1, sz2, sz3;

        Sprite_Offset(sprite->size & 0xffff, &dx, &dy);
        gte_SetDataReg(GTE_VZ0, z);
        gte_SetDataReg(GTE_VZ1, top);
        gte_SetDataReg(GTE_VZ2, top);
        scr->corners[0].x = sprite->xy - dx;
        scr->corners[0].y = y + dy;
        scr->corners[1].x = sprite->xy + dx;
        scr->corners[1].y = y - dy;
        gte_LoadDataReg(GTE_VXY0, &scr->corners[0]);
        gte_LoadDataReg(GTE_VXY1, &scr->corners[0]);
        gte_LoadDataReg(GTE_VXY2, &scr->corners[1]);
        gte_rtpt();
        tex = (unsigned int *)(scr->spriteTex + (((unsigned int)z >> 16) << 5));
        prim->uv0 = tex[0];
        prim->uv1 = tex[1];
        prim->uv2 = tex[2];
        *(unsigned short *)&prim->uv3 = tex[2] >> 16;
        flag = gte_GetCtrlReg(GTE_FLAG);
        sz1 = gte_GetDataReg(GTE_SZ1);
        sz2 = gte_GetDataReg(GTE_SZ2);
        sz3 = gte_GetDataReg(GTE_SZ3);
        gte_LoadDataReg(GTE_VXY0, &scr->corners[1]);
        gte_StoreDataReg(GTE_SXY0, &prim->xy2);
        gte_StoreDataReg(GTE_SXY1, &prim->xy0);
        gte_rtps();
        prim->tag = 0x9000000;
        prim->rgb = sprite->rgb;
        flag |= gte_GetCtrlReg(GTE_FLAG);
        gte_StoreDataReg(GTE_SXY1, &prim->xy1);
        gte_StoreDataReg(GTE_SXY2, &prim->xy3);
        if (flag >= 0) {
            int sum = sz1 + sz2 + sz3 + gte_GetDataReg(GTE_SZ3);

            Sprite_Link(scr, prim, sum >> (scr->view.rotShift + 5));
            prim++;
        }
    }
    scr->view.packetPtr = (char *)prim;
    Sprite_RestoreAxis(scr);
}

// The same for an object: the size lies across the ground, and the depth uses the
// object's shift.
static inline void Obj_DrawSprites(MeshScratch *scr, CourseMesh *mesh)
{
    int count = mesh->spriteCount;
    ObjSprite *sprite = (ObjSprite *)mesh->sprites;
    ObjSprite *end = sprite + count;
    PrimFT4Words *prim;

    if (count == 0) {
        return;
    }
    prim = (PrimFT4Words *)scr->view.packetPtr;
    Sprite_LoadAxis((unsigned short)scr->yawCos, -((unsigned short)scr->yawSin << 16));
    for (; sprite != end; sprite++) {
        int dx, dy;
        int y = sprite->xy >> 16;
        int top = y + ((unsigned int)sprite->size >> 16);
        int left, right;
        int near;
        int flag;
        int sz1, sz2, sz3;

        Sprite_Offset(sprite->size & 0xffff, &dx, &dy);
        scr->corners[0].y = top;
        scr->corners[1].y = top;
        scr->corners[2].y = y;
        scr->corners[3].y = y;
        left = sprite->xy - dx;
        right = sprite->xy + dx;
        near = sprite->z + dy;
        scr->corners[0].x = left;
        scr->corners[1].x = right;
        scr->corners[2].x = left;
        scr->corners[3].x = right;
        gte_SetDataReg(GTE_VZ0, sprite->z - dy);
        gte_SetDataReg(GTE_VZ1, near);
        gte_SetDataReg(GTE_VZ2, near);
        gte_LoadDataReg(GTE_VXY0, &scr->corners[1]);
        gte_LoadDataReg(GTE_VXY1, &scr->corners[0]);
        gte_LoadDataReg(GTE_VXY2, &scr->corners[2]);
        gte_rtpt();
        prim->uv0 = sprite->uv[0];
        prim->uv1 = sprite->uv[1];
        prim->uv2 = sprite->uv[2];
        *(unsigned short *)&prim->uv3 = sprite->uv[2] >> 16;
        flag = gte_GetCtrlReg(GTE_FLAG);
        sz1 = gte_GetDataReg(GTE_SZ1);
        sz2 = gte_GetDataReg(GTE_SZ2);
        sz3 = gte_GetDataReg(GTE_SZ3);
        gte_LoadDataReg(GTE_VXY0, &scr->corners[3]);
        gte_StoreDataReg(GTE_SXY0, &prim->xy1);
        gte_StoreDataReg(GTE_SXY1, &prim->xy0);
        gte_rtps();
        prim->tag = 0x9000000;
        prim->rgb = sprite->rgb;
        flag |= gte_GetCtrlReg(GTE_FLAG);
        gte_StoreDataReg(GTE_SXY1, &prim->xy2);
        gte_StoreDataReg(GTE_SXY2, &prim->xy3);
        if (flag >= 0) {
            int sum = sz1 + sz2 + sz3 + gte_GetDataReg(GTE_SZ3);

            Sprite_Link(scr, prim, ((sum >> 2) << scr->view.otShift) >> 13);
            prim++;
        }
    }
    scr->view.packetPtr = (char *)prim;
    Sprite_RestoreAxis(scr);
}

// Writes one of a flare's four quads; they meet at the flare's centre (the third corner).
static inline void Flare_Quad(PrimGT4 *prim, unsigned int *slot, unsigned int edge, unsigned int faint,
                              unsigned int centre, int x0, int y0, int x1, int y1, int cx, int cy, int x3, int y3,
                              int u0, int v0, int u1)
{
    prim->tag = 0xc000000;
    prim->c0 = edge;
    prim->c1 = faint;
    prim->c2 = centre;
    prim->c3 = edge;
    prim->x0 = x0;
    prim->y0 = y0;
    prim->x1 = x1;
    prim->y1 = y1;
    prim->x2 = cx;
    prim->y2 = cy;
    prim->x3 = x3;
    prim->y3 = y3;
    prim->clut = 0x7f57;
    prim->tpage = 0x29;
    prim->u0 = u0;
    prim->v0 = v0;
    prim->u1 = u1;
    prim->v1 = v0;
    prim->u2 = 0xa0;
    prim->v2 = 0x60;
    prim->u3 = u1;
    prim->v3 = 0x60;
    ot_AddPrim(slot, prim);
}

// Draws a flare at the screen point xy, on OT entry z, with its radius in fixed point.
static inline PrimGT4 *Flare_Draw(PrimGT4 *prim, unsigned int *slot, int xy, int radius, unsigned int rgb)
{
    int x = (short)xy;
    int y = xy >> 16;
    int a = g_SinTable[0x100] * radius;
    int b = g_SinTable[0x500] * radius;
    int fullA = a >> 12;
    int halfA = a >> 13;
    int fullB = b >> 12;
    int halfB = b >> 13;
    unsigned int centre = (rgb & 0xfefefe) >> 1;
    unsigned int edge = (centre & 0xfcfcfc) >> 2;
    unsigned int faint = (edge & 0xf8f8f8) >> 3;
    int ax = x - halfA;
    int bx = x - halfB;
    int cx = x + fullB - halfB;
    int ay = y - halfA;
    int cy = y + fullA - halfA;
    int by = y + halfB;
    int dy = by - fullB;

    centre |= 0x3e000000;
    edge |= 0x3e000000;
    faint |= 0x3e000000;
    Flare_Quad(prim, slot, edge, faint, centre, ax, by, bx - halfA, ay + halfB, x, y, bx, ay, 0xa0, 0x40, 0x80);
    prim++;
    Flare_Quad(prim, slot, edge, faint, centre, ax, by, cx - halfA, cy + halfB, x, y, cx, cy, 0xa0, 0x40, 0xbf);
    prim++;
    Flare_Quad(prim, slot, edge, faint, centre, ax + fullA, dy, bx - halfA + fullA, ay + halfB - fullB, x, y, bx, ay,
               0xa0, 0x7f, 0x80);
    prim++;
    Flare_Quad(prim, slot, edge, faint, centre, ax + fullA, dy, cx - halfA + fullA, cy + halfB - fullB, x, y, cx, cy,
               0xa0, 0x7f, 0xbf);
    prim++;
    return prim;
}

// Projects a flare; returns 0 if its point is out of range. The DQ registers scale its
// size by depth.
static inline int Flare_Project(MeshFlare *flare, int *sz, int *size, int *xy)
{
    gte_SetCtrlReg(GTE_DQA, flare->zsize >> 16);
    gte_SetDataReg(GTE_VXY0, flare->xy);
    gte_SetDataReg(GTE_VZ0, flare->zsize);
    gte_rtps();
    if (gte_GetCtrlReg(GTE_FLAG) < 0) {
        return 0;
    }
    *sz = gte_GetDataReg(GTE_SZ3);
    *size = gte_GetDataReg(GTE_MAC0);
    *xy = gte_GetDataReg(GTE_SXY2);
    return 1;
}

// Draws a section's flares, eight OT entries in front of their depth.
static inline void Course_DrawFlares(MeshScratch *scr, CourseMesh *mesh)
{
    int count = mesh->flareCount;
    MeshFlare *flare = (MeshFlare *)mesh->flares;
    MeshFlare *end = flare + count;
    PrimGT4 *prim;

    if (count == 0) {
        return;
    }
    prim = (PrimGT4 *)scr->view.packetPtr;
    gte_SetCtrlReg(GTE_DQB, 0);
    for (; flare != end; flare++) {
        int shift = scr->view.rotShift;
        int sz, size, xy;

        if (Flare_Project(flare, &sz, &size, &xy)) {
            int z = sz >> (shift + 3);

            if (z >= 0x1000) {
                z = 0xfff;
            }
            prim = Flare_Draw(prim, scr->view.ot - 8 + z, xy, (size << shift) >> 15, flare->rgb);
        }
    }
    scr->view.packetPtr = (char *)prim;
}

// The same for an object, with the object's shift.
static inline void Obj_DrawFlares(MeshScratch *scr, CourseMesh *mesh)
{
    int count = mesh->flareCount;
    MeshFlare *flare = (MeshFlare *)mesh->flares;
    MeshFlare *end = flare + count;
    PrimGT4 *prim;

    if (count == 0) {
        return;
    }
    prim = (PrimGT4 *)scr->view.packetPtr;
    gte_SetCtrlReg(GTE_DQB, 0);
    for (; flare != end; flare++) {
        int shift = (unsigned short)scr->view.otShift;
        int sz, size, xy;

        if (Flare_Project(flare, &sz, &size, &xy)) {
            int z = (sz << shift) >> 13;

            if (z >= 0x1000) {
                z = 0xfff;
            }
            if (shift - 10 >= 0) {
                size >>= shift - 10;
            } else {
                size <<= 10 - shift;
            }
            prim = Flare_Draw(prim, scr->view.ot - 8 + z, xy, size >> 15, flare->rgb);
        }
    }
    scr->view.packetPtr = (char *)prim;
}

// A mesh's vertex.
struct MeshModelVert {
    int xy;
    short z;
    short pad;
};

// One entry of the sprite texture table (spriteTex): two of these per texture, the
// second for polygons of minArea or less. uv2 holds the third corner, then the fourth.
struct MeshTex {
    unsigned int uv0;           // then the CLUT
    unsigned int uv1;           // then the texture page
    unsigned int uv2;
    unsigned short minArea;
    unsigned short pad;
};

// A vertex of a polygon being subdivided.
struct MeshVert {
    short x, y, z;
    unsigned char u, v;
    int sxy;
    unsigned int rgb;
};

// The polygon being subdivided: its pieces chain together and link into one OT entry.
struct MeshBatch {
    unsigned int *slot;
    unsigned int rgb;           // colour and code; Gouraud pieces take only the code
    unsigned int clut;
    unsigned int tpage;
    unsigned int limit;         // pieces of a larger screen area split again
    int gouraud;
    int obj;                    // an object's mesh: the vertices keep outcodes
    char *first;
    char *last;
};

// The polygons in whole words.
struct PrimFT3Words {
    unsigned int tag;
    unsigned int rgb;
    int xy0;
    unsigned int uv0;
    int xy1;
    unsigned int uv1;
    int xy2;
    unsigned int uv2;
};

struct PrimGT3Words {
    unsigned int tag;
    unsigned int c0;
    int xy0;
    unsigned int uv0;
    unsigned int c1;
    int xy1;
    unsigned int uv1;
    unsigned int c2;
    int xy2;
    unsigned int uv2;
};

struct PrimGT4Words {
    unsigned int tag;
    unsigned int c0;
    int xy0;
    unsigned int uv0;
    unsigned int c1;
    int xy1;
    unsigned int uv1;
    unsigned int c2;
    int xy2;
    unsigned int uv2;
    unsigned int c3;
    int xy3;
    unsigned int uv3;
};

#define MESH_MAX_DEPTH 6
#define OBJ_LIMIT_TRI 0x480
#define OBJ_LIMIT_QUAD 0x800
#define MESH_ABS(x) ((x) < 0 ? -(x) : (x))

static inline MeshModelVert *Mesh_Vert(char *verts, unsigned int offset)
{
    return (MeshModelVert *)(verts + (offset & 0xff8));
}

static inline int Mesh_NClip(int a, int b, int c)
{
    gte_SetDataReg(GTE_SXY0, a);
    gte_SetDataReg(GTE_SXY1, b);
    gte_SetDataReg(GTE_SXY2, c);
    gte_nclip();
    return gte_GetDataReg(GTE_MAC0);
}

// The OT entry of a polygon: bits 27-28 of its first word pick a bank, its deepest vertex
// the entry.
static inline unsigned int *Mesh_Slot(MeshScratch *scr, unsigned int word0, int maxZ)
{
    int z = (maxZ >> 3) >> scr->view.rotShift;

    if (z >= 0x1000) {
        z = 0xfff;
    }
    return (unsigned int *)((char *)scr->view.ot + ((word0 >> 21) & 0xc0)) + z;
}

static inline int Mesh_Max(int a, int b)
{
    return a < b ? b : a;
}

// Projects a triangle (a, b, c into V0, V2, V1). The sign bit makes it one-sided.
static inline int Tri_Project(MeshScratch *scr, int word0, MeshModelVert **v, unsigned int **slot, int *area)
{
    char *verts = scr->view.verts;
    int flag;
    int maxZ;

    v[0] = Mesh_Vert(verts, word0 << 3);
    v[1] = Mesh_Vert(verts, word0 >> 6);
    v[2] = Mesh_Vert(verts, word0 >> 15);
    gte_LoadDataRegAt(GTE_VXY0, v[0], 0);
    gte_LoadDataRegAt(GTE_VZ0, v[0], 4);
    gte_LoadDataRegAt(GTE_VXY2, v[1], 0);
    gte_LoadDataRegAt(GTE_VZ2, v[1], 4);
    gte_LoadDataRegAt(GTE_VXY1, v[2], 0);
    gte_LoadDataRegAt(GTE_VZ1, v[2], 4);
    gte_rtpt();
    flag = gte_GetCtrlReg(GTE_FLAG);
    maxZ = Mesh_Max(gte_GetDataReg(GTE_SZ1), gte_GetDataReg(GTE_SZ3));
    maxZ = Mesh_Max(maxZ, gte_GetDataReg(GTE_SZ2));
    gte_nclip();
    *slot = Mesh_Slot(scr, word0, maxZ);
    *area = gte_GetDataReg(GTE_MAC0);
    return flag >= 0 && *area != 0 && (*area & word0) >= 0;
}

// Projects a quad (a, b, c into V2, V0, V1, then d). It leaves c, a and d in SXY0-2 and
// returns b's screen position in sxyB, and the signed areas of its two halves.
static inline int Quad_Project(MeshScratch *scr, int word0, int word1, MeshModelVert **v, unsigned int **slot,
                               int *area1, int *area2, int *sxyB)
{
    char *verts = scr->view.verts;
    int flag;
    int maxZ;

    v[0] = Mesh_Vert(verts, word0 << 3);
    v[1] = Mesh_Vert(verts, word0 >> 6);
    v[2] = Mesh_Vert(verts, word0 >> 15);
    v[3] = Mesh_Vert(verts, word1 << 3);
    gte_LoadDataRegAt(GTE_VXY2, v[0], 0);
    gte_LoadDataRegAt(GTE_VZ2, v[0], 4);
    gte_LoadDataRegAt(GTE_VXY0, v[1], 0);
    gte_LoadDataRegAt(GTE_VZ0, v[1], 4);
    gte_LoadDataRegAt(GTE_VXY1, v[2], 0);
    gte_LoadDataRegAt(GTE_VZ1, v[2], 4);
    gte_rtpt();
    flag = gte_GetCtrlReg(GTE_FLAG);
    *sxyB = gte_GetDataReg(GTE_SXY0);
    maxZ = Mesh_Max(gte_GetDataReg(GTE_SZ3), gte_GetDataReg(GTE_SZ1));
    maxZ = Mesh_Max(maxZ, gte_GetDataReg(GTE_SZ2));
    gte_nclip();
    *area1 = -gte_GetDataReg(GTE_MAC0);
    gte_LoadDataRegAt(GTE_VXY0, v[3], 0);
    gte_LoadDataRegAt(GTE_VZ0, v[3], 4);
    gte_rtps();
    flag |= gte_GetCtrlReg(GTE_FLAG);
    maxZ = Mesh_Max(maxZ, gte_GetDataReg(GTE_SZ3));
    gte_nclip();
    *area2 = gte_GetDataReg(GTE_MAC0);
    *slot = Mesh_Slot(scr, word0, maxZ);
    if (flag < 0 || (*area1 | *area2) == 0) {
        return 0;
    }
    return ((*area1 - 1) & (*area2 - 1) & word0) >= 0;
}

// Passes rgb through the GTE's colour pipe (IR0 0: no fog), with the code.
static inline unsigned int Mesh_Colour(unsigned int rgb)
{
    gte_SetDataReg(GTE_RGBC, rgb);
    gte_SetDataReg(GTE_IR0, 0);
    gte_dpcs();
    return gte_GetDataReg(GTE_RGB2);
}

// The same for three colours, read back from RGB0-2.
static inline void Mesh_Colours(unsigned int c0, unsigned int c1, unsigned int c2)
{
    gte_SetDataReg(GTE_RGB0, c0);
    gte_SetDataReg(GTE_RGB1, c1);
    gte_SetDataReg(GTE_RGB2, c2);
    gte_SetDataReg(GTE_IR0, 0);
    gte_dpct();
}

static inline MeshTex *Mesh_Tex(MeshScratch *scr, int word1, unsigned int area)
{
    MeshTex *tex = (MeshTex *)(scr->spriteTex + ((word1 >> 4) & 0x7ffe0));

    if (tex->minArea >= area) {
        tex++;
    }
    return tex;
}

// Moves past a primitive of size bytes; returns 1 if it was past the packet limit.
static inline int Mesh_Advance(MeshScratch *scr, char **prim, int size)
{
    int over = *prim > scr->view.packetLimit;

    *prim += size;
    return over;
}

static inline void Vert_Root(MeshVert *v, MeshModelVert *src, unsigned int uv, int sxy, unsigned int rgb)
{
    *(int *)&v->x = src->xy;
    v->z = src->z;
    *(unsigned short *)&v->u = uv;
    v->sxy = sxy;
    v->rgb = rgb;
}

static inline unsigned int Mesh_Mix(unsigned int a, unsigned int b)
{
    return ((a & 0xfefefe) + (b & 0xfefefe)) >> 1;
}

static inline void Vert_Mid(MeshVert *m, MeshVert *a, MeshVert *b, int gouraud)
{
    m->x = (a->x + b->x) >> 1;
    m->y = (a->y + b->y) >> 1;
    m->z = (a->z + b->z) >> 1;
    m->u = (a->u + b->u) >> 1;
    m->v = (a->v + b->v) >> 1;
    if (gouraud) {
        m->rgb = Mesh_Mix(a->rgb, b->rgb);
    }
}

// A vertex's outcode shares the top byte of its colour: bit 0 in front of the near
// plane, bits 1-4 off the left, right, top and bottom of the screen, bit 5 out of the
// GTE's range.
static inline unsigned char *Vert_Out(MeshVert *v)
{
    return (unsigned char *)&v->rgb + 3;
}

static inline void Vert_ProjectOnly(MeshVert *v)
{
    gte_LoadDataRegAt(GTE_VXY0, v, 0);
    gte_LoadDataRegAt(GTE_VZ0, v, 4);
    gte_rtps();
    v->sxy = gte_GetDataReg(GTE_SXY2);
}

static inline void Vert_Project(MeshScratch *scr, MeshBatch *batch, MeshVert *v)
{
    Vert_ProjectOnly(v);
    if (batch->obj) {
        int flag = gte_GetCtrlReg(GTE_FLAG);
        int x = (short)v->sxy;
        int y = v->sxy >> 16;
        int out = gte_GetDataReg(GTE_IR3) < (unsigned short)scr->view.nearZ;

        if (x < 0) {
            out |= 2;
        }
        out |= (x > (unsigned short)scr->view.screenW - 1) << 2;
        if (y < 0) {
            out |= 8;
        }
        out |= (y > (unsigned short)scr->view.screenH - 1) << 4;
        if (flag < 0) {
            out |= 0x20;
        }
        *Vert_Out(v) = out;
    }
}

// Whether a filling triangle or a part has a vertex out of the GTE's range.
static inline int Verts_Bad(MeshVert *a, MeshVert *b, MeshVert *c)
{
    return (*Vert_Out(a) | *Vert_Out(b) | *Vert_Out(c)) & 0x20;
}

// Adds a piece to the batch's chain, which keeps the packets in order for the GPU.
static inline void Mesh_AddPrim(MeshScratch *scr, MeshBatch *batch, void *prim, int size, int words)
{
    char *next = (char *)prim + size;

    *(unsigned int *)prim = words << 24 | ((unsigned int)next & 0xffffff);
    batch->last = (char *)prim;
    scr->view.packetPtr = next;
}

// A textured triangle piece. A quad's filling triangles use rgb with the triangle code.
static inline void Mesh_PutTri(MeshScratch *scr, MeshBatch *batch, MeshVert *a, MeshVert *b, MeshVert *c,
                               unsigned int rgb)
{
    if (batch->gouraud) {
        PrimGT3Words *prim = (PrimGT3Words *)scr->view.packetPtr;

        prim->c0 = (a->rgb & 0xffffff) | (rgb & 0xff000000);
        prim->c1 = b->rgb;
        prim->c2 = c->rgb;
        prim->xy0 = a->sxy;
        prim->xy1 = b->sxy;
        prim->xy2 = c->sxy;
        prim->uv0 = *(unsigned short *)&a->u | batch->clut << 16;
        prim->uv1 = *(unsigned short *)&b->u | batch->tpage << 16;
        prim->uv2 = *(unsigned short *)&c->u;
        Mesh_AddPrim(scr, batch, prim, sizeof(PrimGT3Words), 9);
    } else {
        PrimFT3Words *prim = (PrimFT3Words *)scr->view.packetPtr;

        prim->rgb = rgb;
        prim->xy0 = a->sxy;
        prim->xy1 = b->sxy;
        prim->xy2 = c->sxy;
        prim->uv0 = *(unsigned short *)&a->u | batch->clut << 16;
        prim->uv1 = *(unsigned short *)&b->u | batch->tpage << 16;
        prim->uv2 = *(unsigned short *)&c->u;
        Mesh_AddPrim(scr, batch, prim, sizeof(PrimFT3Words), 7);
    }
}

// A textured quad piece, around the centre. In a section's mesh the game gives the
// centre's colour to the third corner and the third corner's to the centre.
static inline void Mesh_PutQuad(MeshScratch *scr, MeshBatch *batch, MeshVert *a, MeshVert *b, MeshVert *c,
                                MeshVert *centre)
{
    if (batch->gouraud) {
        PrimGT4Words *prim = (PrimGT4Words *)scr->view.packetPtr;

        prim->c0 = (a->rgb & 0xffffff) | (batch->rgb & 0xff000000);
        prim->c1 = b->rgb;
        prim->c2 = batch->obj ? c->rgb : centre->rgb;
        prim->c3 = batch->obj ? centre->rgb : c->rgb;
        prim->xy0 = a->sxy;
        prim->xy1 = b->sxy;
        prim->xy2 = c->sxy;
        prim->xy3 = centre->sxy;
        prim->uv0 = *(unsigned short *)&a->u | batch->clut << 16;
        prim->uv1 = *(unsigned short *)&b->u | batch->tpage << 16;
        prim->uv2 = *(unsigned short *)&c->u;
        prim->uv3 = *(unsigned short *)&centre->u;
        Mesh_AddPrim(scr, batch, prim, sizeof(PrimGT4Words), 12);
    } else {
        PrimFT4Words *prim = (PrimFT4Words *)scr->view.packetPtr;

        prim->rgb = batch->rgb;
        prim->xy0 = a->sxy;
        prim->xy1 = b->sxy;
        prim->xy2 = c->sxy;
        prim->xy3 = centre->sxy;
        prim->uv0 = *(unsigned short *)&a->u | batch->clut << 16;
        prim->uv1 = *(unsigned short *)&b->u | batch->tpage << 16;
        prim->uv2 = *(unsigned short *)&c->u;
        prim->uv3 = *(unsigned short *)&centre->u;
        Mesh_AddPrim(scr, batch, prim, sizeof(PrimFT4Words), 9);
    }
}

// Splits a triangle at its edges' midpoints. Thin triangles along each edge come first,
// filling the cracks where a midpoint's rounded screen position leaves the edge; then
// the four parts, split again while large. A part too large at the last depth is
// dropped. Returns 0 if the packet ran out.
static int Mesh_SplitTri(MeshScratch *scr, MeshBatch *batch, MeshVert **p, int depth)
{
    MeshVert m[3];
    int gouraud = batch->gouraud;
    int i;

    Vert_Mid(&m[0], p[0], p[1], gouraud);
    Vert_Mid(&m[1], p[1], p[2], gouraud);
    Vert_Mid(&m[2], p[2], p[0], gouraud);
    Vert_Project(scr, batch, &m[0]);
    Vert_Project(scr, batch, &m[1]);
    Vert_Project(scr, batch, &m[2]);
    for (i = 2; i >= 0; i--) {
        int j = i != 0 ? i - 1 : 2;

        if (batch->obj && Verts_Bad(p[i], &m[j], p[j])) {
            continue;
        }
        if (Mesh_NClip(p[i]->sxy, m[j].sxy, p[j]->sxy) != 0) {
            Mesh_PutTri(scr, batch, p[i], &m[j], p[j], batch->rgb);
        }
    }
    if (scr->view.packetPtr > scr->view.packetLimit) {
        return 0;
    }
    for (i = 3; i >= 0; i--) {
        MeshVert *v[3];
        int area;
        int bad;

        if (i == 3) {
            v[0] = &m[0];
            v[1] = &m[1];
            v[2] = &m[2];
        } else {
            v[0] = p[i];
            v[1] = &m[i];
            v[2] = i != 0 ? &m[i - 1] : &m[2];
        }
        bad = 0;
        if (batch->obj) {
            if (*Vert_Out(v[0]) & *Vert_Out(v[1]) & *Vert_Out(v[2]) & 0x1f) {
                continue;
            }
            bad = Verts_Bad(v[0], v[1], v[2]);
        }
        area = Mesh_NClip(v[0]->sxy, v[1]->sxy, v[2]->sxy);
        if (!bad && (unsigned int)MESH_ABS(area) <= batch->limit) {
            Mesh_PutTri(scr, batch, v[0], v[1], v[2], batch->rgb);
        } else if (depth < MESH_MAX_DEPTH && !Mesh_SplitTri(scr, batch, v, depth + 1)) {
            return 0;
        }
    }
    return 1;
}

// The same with a quad: four filling triangles, then four quads around the centre.
// Gouraud quads pick the order of a part's corners differently, and in a section's mesh
// wait for the GPU between projections.
static int Mesh_SplitQuad(MeshScratch *scr, MeshBatch *batch, MeshVert **p, int depth)
{
    MeshVert centre;
    MeshVert m[4];
    int gouraud = batch->gouraud;
    int i;

    Vert_Mid(&m[0], p[0], p[1], gouraud);
    Vert_Mid(&m[1], p[1], p[2], gouraud);
    Vert_Mid(&m[2], p[2], p[3], gouraud);
    Vert_Mid(&m[3], p[3], p[0], gouraud);
    Vert_Mid(&centre, &m[0], &m[2], gouraud);
    // The corners' sum over 4, not the midpoints' mean.
    centre.x = (p[0]->x + p[1]->x + p[2]->x + p[3]->x) >> 2;
    centre.y = (p[0]->y + p[1]->y + p[2]->y + p[3]->y) >> 2;
    centre.z = (p[0]->z + p[1]->z + p[2]->z + p[3]->z) >> 2;
    centre.u = (p[0]->u + p[1]->u + p[2]->u + p[3]->u) >> 2;
    centre.v = (p[0]->v + p[1]->v + p[2]->v + p[3]->v) >> 2;
    Vert_Project(scr, batch, &centre);
    if (gouraud && !batch->obj) {
        Gpu_DrawSync();
    }
    for (i = 0; i < 4; i++) {
        Vert_Project(scr, batch, &m[i]);
        if (gouraud && !batch->obj) {
            Gpu_DrawSync();
        }
    }
    for (i = 3; i >= 0; i--) {
        int j = (i - 1) & 3;

        if (batch->obj && Verts_Bad(p[i], &m[j], p[j])) {
            continue;
        }
        if (Mesh_NClip(p[i]->sxy, m[j].sxy, p[j]->sxy) != 0) {
            Mesh_PutTri(scr, batch, p[i], &m[j], p[j], batch->rgb + 0xf8000000);
        }
    }
    if (scr->view.packetPtr > scr->view.packetLimit) {
        return 0;
    }
    for (i = 3; i >= 0; i--) {
        MeshVert *a = p[i];
        MeshVert *b = &m[i];
        MeshVert *c = &m[(i - 1) & 3];
        int bad = 0;
        int area;

        if (batch->obj) {
            if (*Vert_Out(a) & *Vert_Out(b) & *Vert_Out(c) & *Vert_Out(&centre) & 0x1f) {
                continue;
            }
            bad = Verts_Bad(a, b, c) | (*Vert_Out(&centre) & 0x20);
        }
        area = Mesh_NClip(b->sxy, a->sxy, centre.sxy) - Mesh_NClip(c->sxy, a->sxy, centre.sxy);
        if (!bad && (unsigned int)MESH_ABS(area) <= batch->limit) {
            Mesh_PutQuad(scr, batch, a, b, c, &centre);
        } else if (depth < MESH_MAX_DEPTH) {
            MeshVert *v[4];

            // The game tests the parent's centre byte, in which the outcode counts.
            if (gouraud ? ((*Vert_Out(&centre) & 0x3f) | i << 6) == 0 : (i & 1) == 0) {
                v[0] = a;
                v[1] = b;
                v[2] = &centre;
                v[3] = c;
            } else {
                v[0] = c;
                v[1] = a;
                v[2] = b;
                v[3] = &centre;
            }
            if (!Mesh_SplitQuad(scr, batch, v, depth + 1)) {
                return 0;
            }
        }
    }
    return 1;
}

static inline void Mesh_BatchBegin(MeshScratch *scr, MeshBatch *batch, unsigned int *slot, unsigned int rgb,
                                   MeshTex *tex, unsigned int limit, int gouraud)
{
    batch->slot = slot;
    batch->rgb = rgb;
    batch->clut = tex->uv0 >> 16;
    batch->tpage = tex->uv1 >> 16;
    batch->limit = limit & 0xffff;
    batch->gouraud = gouraud;
    batch->obj = 0;
    batch->first = scr->view.packetPtr;
    batch->last = scr->view.packetPtr;
}

// Links the chain into its OT entry, or drops it if the packet ran out. Returns 1 if the
// list may go on.
static inline int Mesh_BatchEnd(MeshScratch *scr, MeshBatch *batch, int ok)
{
    if (!ok || scr->view.packetPtr > scr->view.packetLimit) {
        scr->view.packetPtr = batch->first;
    } else if (scr->view.packetPtr != batch->first) {
        ot_AddPrims(batch->slot, batch->first, batch->last);
    }
    return scr->view.packetPtr < scr->view.packetLimit;
}

// An object mesh's polygons index the vertices in 10 bits. Bit 30 sorts them by their
// deepest vertex, 0x60 entries further back, instead of their nearest.
static inline MeshModelVert *ObjMesh_Vert(char *verts, unsigned int offset)
{
    return (MeshModelVert *)(verts + (offset & 0x1ff8));
}

static inline int ObjMesh_Pick(int a, int b, int far)
{
    return (a < b) == far ? b : a;
}

static inline unsigned int *ObjMesh_Slot(MeshScratch *scr, int z, int far)
{
    unsigned int index = ((unsigned int)(z << scr->view.otShift) >> 13) + (-far & 0x60);

    if ((int)index >= 0x1000) {
        index = 0xfff;
    }
    return scr->view.ot + index;
}

static inline int ObjTri_Project(MeshScratch *scr, int word0, MeshModelVert **v, unsigned int **slot, int *area)
{
    char *verts = scr->view.verts;
    int far = ((unsigned int)word0 >> 30) & 1;
    int flag;
    int z;

    v[0] = ObjMesh_Vert(verts, word0 << 3);
    v[1] = ObjMesh_Vert(verts, word0 >> 7);
    v[2] = ObjMesh_Vert(verts, word0 >> 17);
    gte_LoadDataRegAt(GTE_VXY0, v[0], 0);
    gte_LoadDataRegAt(GTE_VZ0, v[0], 4);
    gte_LoadDataRegAt(GTE_VXY2, v[1], 0);
    gte_LoadDataRegAt(GTE_VZ2, v[1], 4);
    gte_LoadDataRegAt(GTE_VXY1, v[2], 0);
    gte_LoadDataRegAt(GTE_VZ1, v[2], 4);
    gte_rtpt();
    flag = gte_GetCtrlReg(GTE_FLAG);
    z = ObjMesh_Pick(gte_GetDataReg(GTE_SZ1), gte_GetDataReg(GTE_SZ3), far);
    z = ObjMesh_Pick(z, gte_GetDataReg(GTE_SZ2), far);
    gte_nclip();
    *slot = ObjMesh_Slot(scr, z, far);
    *area = gte_GetDataReg(GTE_MAC0);
    return flag >= 0 && *area != 0 && (*area & word0) >= 0;
}

static inline int ObjQuad_Project(MeshScratch *scr, int word0, int word1, MeshModelVert **v, unsigned int **slot,
                                  int *area1, int *area2, int *sxyB)
{
    char *verts = scr->view.verts;
    int far = ((unsigned int)word0 >> 30) & 1;
    int flag;
    int z;

    v[0] = ObjMesh_Vert(verts, word0 << 3);
    v[1] = ObjMesh_Vert(verts, word0 >> 7);
    v[2] = ObjMesh_Vert(verts, word0 >> 17);
    v[3] = ObjMesh_Vert(verts, word1 << 3);
    gte_LoadDataRegAt(GTE_VXY2, v[0], 0);
    gte_LoadDataRegAt(GTE_VZ2, v[0], 4);
    gte_LoadDataRegAt(GTE_VXY0, v[1], 0);
    gte_LoadDataRegAt(GTE_VZ0, v[1], 4);
    gte_LoadDataRegAt(GTE_VXY1, v[2], 0);
    gte_LoadDataRegAt(GTE_VZ1, v[2], 4);
    gte_rtpt();
    flag = gte_GetCtrlReg(GTE_FLAG);
    *sxyB = gte_GetDataReg(GTE_SXY0);
    z = ObjMesh_Pick(gte_GetDataReg(GTE_SZ1), gte_GetDataReg(GTE_SZ2), far);
    z = ObjMesh_Pick(z, gte_GetDataReg(GTE_SZ3), far);
    gte_nclip();
    *area1 = -gte_GetDataReg(GTE_MAC0);
    gte_LoadDataRegAt(GTE_VXY0, v[3], 0);
    gte_LoadDataRegAt(GTE_VZ0, v[3], 4);
    gte_rtps();
    flag |= gte_GetCtrlReg(GTE_FLAG);
    z = ObjMesh_Pick(z, gte_GetDataReg(GTE_SZ3), far);
    gte_nclip();
    *area2 = gte_GetDataReg(GTE_MAC0);
    *slot = ObjMesh_Slot(scr, z, far);
    if (flag < 0 || (*area1 | *area2) == 0) {
        return 0;
    }
    return ((*area1 - 1) & (*area2 - 1) & word0) >= 0;
}

// The root vertices of an object's polygon stay in the scratchpad between polygons. They
// only write the outcode byte (the top of rgb) if Gouraud, so a flat polygon reads what
// was there before: the placement's matrix, or an earlier polygon's colours. A vertex
// with bit 5 set projects again, and clears the bit if it fits.
#define OBJ_ROOT(scr) ((MeshVert *)((char *)(scr) + 0xa8))

static inline void ObjRoot_Set(MeshVert *v, MeshModelVert *src, unsigned int uv, int sxy)
{
    *(int *)&v->x = src->xy;
    v->z = src->z;
    *(unsigned short *)&v->u = uv;
    v->sxy = sxy;
}

static inline void ObjRoot_Check(MeshVert *v)
{
    if (*Vert_Out(v) & 0x20) {
        Vert_ProjectOnly(v);
        if (gte_GetCtrlReg(GTE_FLAG) >= 0) {
            *Vert_Out(v) &= 0x1f;
        }
    }
}
#endif

extern "C" {
#ifdef NON_MATCHING
#define MESH_LOAD_V0(a, b, c) \
    gte_LoadDataRegAt(GTE_VXY0, a, 0); \
    gte_LoadDataRegAt(GTE_VZ0, a, 4); \
    gte_LoadDataRegAt(GTE_VXY1, b, 0); \
    gte_LoadDataRegAt(GTE_VZ1, b, 4); \
    gte_LoadDataRegAt(GTE_VXY2, c, 0); \
    gte_LoadDataRegAt(GTE_VZ2, c, 4)

// Draws a list of triangles of the sky: each entry is two vertex index words and the
// two frames' primitives. Sign bit clear: one OT entry deeper.
#define SKY_TRIS(name, stride, size, x1, x2) \
    static inline int *name(int *entry, int count, int parity) \
    { \
        ViewScratch *scr = SCRATCH; \
        unsigned int *ot = scr->ot; \
        int *end = (int *)((char *)entry + count * (stride)); \
        \
        if (count != 0) { \
            int offset = parity * (size) + 8; \
            unsigned int mask = 0x80008000; \
            \
            do { \
                int word0 = entry[0]; \
                char *verts = scr->verts; \
                int word1 = entry[1]; \
                char *a = verts + (word0 & 0xfff) * 8; \
                char *b = verts + ((word0 >> 9) & 0x7ff8); \
                char *c = verts + ((word1 >> 9) & 0x7ff8); \
                char *prim; \
                int flag; \
                int xy0, xy1, xy2; \
                int all; \
                \
                MESH_LOAD_V0(a, b, c); \
                gte_rtpt(); \
                prim = (char *)entry + offset; \
                flag = gte_GetCtrlReg(GTE_FLAG); \
                xy0 = gte_GetDataReg(GTE_SXY0); \
                xy1 = gte_GetDataReg(GTE_SXY1); \
                xy2 = gte_GetDataReg(GTE_SXY2); \
                all = xy0 & xy1 & xy2; \
                if (flag >= 0 && (all & mask) == 0) { \
                    unsigned int *slot = ot; \
                    \
                    if (word0 > 0) { \
                        slot++; \
                    } \
                    *(int *)(prim + 8) = xy0; \
                    *(int *)(prim + (x1)) = xy1; \
                    *(int *)(prim + (x2)) = xy2; \
                    ot_AddPrim(slot, prim); \
                } \
                entry = (int *)((char *)entry + (stride)); \
            } while (entry != end); \
        } \
        return entry; \
    }

// The same with quads: a second GTE command projects the fourth vertex, which goes in
// x2 (the third goes in x3).
#define SKY_QUADS(name, stride, size, x1, x2, x3) \
    static inline int *name(int *entry, int count, int parity) \
    { \
        ViewScratch *scr = SCRATCH; \
        unsigned int *ot = scr->ot; \
        int *end = (int *)((char *)entry + count * (stride)); \
        \
        if (count != 0) { \
            int offset = parity * (size) + 8; \
            unsigned int mask = 0x80008000; \
            \
            do { \
                int word0 = entry[0]; \
                char *verts = scr->verts; \
                int word1 = entry[1]; \
                char *a = verts + (word0 & 0xfff) * 8; \
                char *b = verts + ((word0 >> 9) & 0x7ff8); \
                char *c = verts + ((word1 >> 9) & 0x7ff8); \
                char *d = verts + (word1 & 0xfff) * 8; \
                char *prim; \
                int flag; \
                int xy0, xy1, xy2, xy3; \
                int all; \
                \
                MESH_LOAD_V0(a, b, c); \
                gte_rtpt(); \
                prim = (char *)entry + offset; \
                flag = gte_GetCtrlReg(GTE_FLAG); \
                gte_LoadDataRegAt(GTE_VXY0, d, 0); \
                gte_LoadDataRegAt(GTE_VZ0, d, 4); \
                xy0 = gte_GetDataReg(GTE_SXY0); \
                xy1 = gte_GetDataReg(GTE_SXY1); \
                xy2 = gte_GetDataReg(GTE_SXY2); \
                gte_rtps(); \
                all = xy0 & xy1 & xy2; \
                *(int *)(prim + 8) = xy0; \
                *(int *)(prim + (x1)) = xy1; \
                *(int *)(prim + (x3)) = xy2; \
                flag |= gte_GetCtrlReg(GTE_FLAG); \
                xy3 = gte_GetDataReg(GTE_SXY2); \
                all &= xy3; \
                if (flag >= 0 && (all & mask) == 0) { \
                    unsigned int *slot = ot; \
                    \
                    if (word0 > 0) { \
                        slot++; \
                    } \
                    *(int *)(prim + (x2)) = xy3; \
                    ot_AddPrim(slot, prim); \
                } \
                entry = (int *)((char *)entry + (stride)); \
            } while (entry != end); \
        } \
        return entry; \
    }

SKY_TRIS(Sky_DrawF3, 0x30, 0x14, 0xc, 0x10)
SKY_QUADS(Sky_DrawF4, 0x38, 0x18, 0xc, 0x10, 0x14)
SKY_TRIS(Sky_DrawG3, 0x40, 0x1c, 0x10, 0x18)
SKY_QUADS(Sky_DrawG4, 0x50, 0x24, 0x10, 0x18, 0x20)
SKY_TRIS(Sky_DrawFT3, 0x48, 0x20, 0x10, 0x18)
SKY_QUADS(Sky_DrawFT4, 0x58, 0x28, 0x10, 0x18, 0x20)
SKY_TRIS(Sky_DrawGT3, 0x58, 0x28, 0x14, 0x20)
SKY_QUADS(Sky_DrawGT4, 0x70, 0x34, 0x14, 0x20, 0x2c)

// Draws the sky mesh under the camera's rotation, turned half a circle. At frame steps
// of 2 and up it also fills the horizon, unless this is a live race of raceType 0.
void Sky_Draw(unsigned int *ot, Camera *cam, SkyMesh *sky, int mirror)
{
    ViewScratch *scr = SCRATCH;
    MATRIX turn;
    int fill;

    *(CameraView *)scr = *(CameraView *)&cam->view;
    Matrix_Identity(&turn);
    Matrix_RotY(&turn, 0x800);
    Matrix_Comp(&scr->view, &turn);
    Gte_LoadRotationOnly(scr);
    scr->ot = ot;
    if (mirror == 0) {
        int *entry;
        int parity;
        char *verts = (char *)sky->verts;

        scr->verts = verts;
        entry = (int *)(verts + sky->vertCount * 8);
        parity = g_MeshFrameParity ^ 1;
        g_MeshFrameParity = parity;
        entry = Sky_DrawF3(entry, sky->counts[0], parity);
        entry = Sky_DrawF4(entry, sky->counts[1], g_MeshFrameParity);
        entry = Sky_DrawG3(entry, sky->counts[2], g_MeshFrameParity);
        entry = Sky_DrawG4(entry, sky->counts[3], g_MeshFrameParity);
        entry = Sky_DrawFT3(entry, sky->counts[4], g_MeshFrameParity);
        entry = Sky_DrawFT4(entry, sky->counts[5], g_MeshFrameParity);
        entry = Sky_DrawGT3(entry, sky->counts[6], g_MeshFrameParity);
        Sky_DrawGT4(entry, sky->counts[7], g_MeshFrameParity);
    }
    fill = 1;
    {
        RaceSetup *race = (RaceSetup *)((char *)&g_GameState + 0xbf7c);

        if (race->frameStep < 2 || (g_RaceWorld.isReplay == 0 && race->raceType == 0)) {
            fill = 0;
        }
    }
    if (fill) {
        Sky_DrawHorizonFill(scr->ot + 1, cam, sky);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", Sky_Draw);
#endif

#ifdef NON_MATCHING
// Fills the screen with the sky's two colours split along the horizon. The view's up
// axis at the four screen corners sets flags: bits 0-3 the corners above the horizon,
// bits 4-7 the edges it crosses.
void Sky_DrawHorizonFill(unsigned int *ot, Camera *cam, SkyMesh *sky)
{
    int w = (unsigned short)cam->screenW;
    int scaleX = (w << 12) / (unsigned short)cam->projH;
    int h = (unsigned short)cam->screenH;
    int scaleY = (h << 12) / (unsigned short)cam->projH;
    int px = -cam->view.m[0][1] * scaleX;
    int py = -cam->view.m[1][1] * scaleY;
    int up = cam->view.m[2][1];
    int flags = 0;
    int dx = px >> 13;
    int mdx = -dx;
    int dy = py >> 13;
    int mdy = -dy;
    int c0, c1, c2, c3;
    unsigned int color, under, over;
    int left, top, bottom, right;

    c0 = dx + dy + up;
    if (c0 >= 0) {
        flags = 0x31;
    }
    c1 = mdx + dy + up;
    if (c1 >= 0) {
        flags |= 2;
        flags ^= 0x60;
    }
    c2 = dx + mdy + up;
    if (c2 >= 0) {
        flags |= 4;
        flags ^= 0x90;
    }
    c3 = mdx + mdy + up;
    if (c3 >= 0) {
        flags |= 8;
        flags ^= 0xc0;
    }
    color = sky->color0 & 0xffffff;
    over = sky->color1 & 0xffffff;
    under = color;
    if (flags & 8) {
        under = over;
        over = color;
    }
    left = 0;
    if (flags & 0x10) {
        left = h * c0 / (c0 - c2);
    }
    top = 0;
    if (flags & 0x20) {
        top = w * c0 / (c0 - c1);
    }
    right = 0;
    if (flags & 0x40) {
        right = h * c1 / (c1 - c3);
    }
    bottom = 0;
    if (flags & 0x80) {
        bottom = w * c2 / (c2 - c3);
    }
    switch (flags & 0xf) {
    case 0:
    case 15: {
        RECT *tile = Prim_AddTile(ot, over);

        tile->x = 0;
        tile->y = 0;
        tile->w = w;
        tile->h = h;
        break;
    }
    case 1:
    case 14:
        SKY_F3(under, 0, 0, 0, left, top, 0);
        SKY_F3(over, top, 0, w, 0, w, h);
        SKY_F4(over, top, 0, 0, left, w, h, 0, h);
        break;
    case 2:
    case 13:
        SKY_F3(under, top, 0, w, 0, w, right);
        SKY_F3(over, top, 0, 0, 0, 0, h);
        SKY_F4(over, top, 0, w, right, 0, h, w, h);
        break;
    case 3:
    case 12:
        SKY_F4(under, 0, left, 0, 0, w, right, w, 0);
        SKY_F4(over, 0, left, 0, h, w, right, w, h);
        break;
    case 4:
    case 11:
        SKY_F3(under, bottom, h, 0, h, 0, left);
        SKY_F3(over, bottom, h, 0, 0, 0, left);
        SKY_F4(over, bottom, h, 0, 0, w, h, w, 0);
        break;
    case 5:
    case 10:
        SKY_F4(under, top, 0, 0, 0, bottom, h, 0, h);
        SKY_F4(over, top, 0, w, 0, bottom, h, w, h);
        break;
    case 7:
    case 8:
        SKY_F3(under, bottom, h, w, right, w, 0);
        SKY_F3(over, bottom, h, w, right, w, h);
        SKY_F4(under, bottom, h, w, 0, 0, h, 0, 0);
        break;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", Sky_DrawHorizonFill);
#endif

// The four mesh functions are the game's handwritten assembly ($at as a temporary, GTE
// commands scheduled by hand), so C++ can't reproduce them.
#ifdef NON_MATCHING
// Draws an object's mesh, wholly in front of the near plane. Unlike a section's mesh its
// textured entries hold their own texture coordinates, its limits on screen area are
// fixed, and split parts wholly off one side of the screen are dropped.
void CourseObj_DrawMeshUnclipped(CourseMesh *mesh)
{
    MeshScratch *scr = MESH_SCRATCH;
    MeshVert *root = OBJ_ROOT(scr);
    MeshModelVert *v[4];
    unsigned int *slot;
    int area, area2, sxyB;
    int *entry;
    int *end;

    scr->view.verts = (char *)mesh->verts;

    entry = (int *)mesh->prims[0];
    end = entry + mesh->primCount[0] * 3;
    if (entry != end) {
        char *prim = scr->view.packetPtr;

        for (; entry != end; entry += 3) {
            if (ObjTri_Project(scr, entry[0], v, &slot, &area)) {
                int *p = (int *)prim;

                p[1] = Mesh_Colour(entry[2]);
                p[2] = gte_GetDataReg(GTE_SXY0);
                p[4] = gte_GetDataReg(GTE_SXY1);
                p[3] = gte_GetDataReg(GTE_SXY2);
                ((unsigned char *)p)[3] = 4;
                ot_AddPrim(slot, p);
                if (Mesh_Advance(scr, &prim, 0x14)) {
                    break;
                }
            }
        }
        scr->view.packetPtr = prim;
    }

    entry = (int *)mesh->prims[1];
    end = entry + mesh->primCount[1] * 3;
    if (entry != end) {
        char *prim = scr->view.packetPtr;

        for (; entry != end; entry += 3) {
            if (ObjQuad_Project(scr, entry[0], entry[1], v, &slot, &area, &area2, &sxyB)) {
                int *p = (int *)prim;
                int sxyC = gte_GetDataReg(GTE_SXY0);
                int sxyA = gte_GetDataReg(GTE_SXY1);
                int sxyD = gte_GetDataReg(GTE_SXY2);

                p[1] = Mesh_Colour(entry[2]);
                p[2] = sxyA;
                p[3] = sxyB;
                p[4] = sxyD;
                p[5] = sxyC;
                ((unsigned char *)p)[3] = 5;
                ot_AddPrim(slot, p);
                if (Mesh_Advance(scr, &prim, 0x18)) {
                    break;
                }
            }
        }
        scr->view.packetPtr = prim;
    }

    entry = (int *)mesh->prims[2];
    end = entry + mesh->primCount[2] * 5;
    if (entry != end) {
        char *prim = scr->view.packetPtr;

        for (; entry != end; entry += 5) {
            if (ObjTri_Project(scr, entry[0], v, &slot, &area)) {
                int *p = (int *)prim;

                p[2] = gte_GetDataReg(GTE_SXY0);
                p[6] = gte_GetDataReg(GTE_SXY1);
                p[4] = gte_GetDataReg(GTE_SXY2);
                gte_SetDataReg(GTE_RGBC, entry[2]);
                Mesh_Colours(entry[2], entry[3], entry[4]);
                p[1] = gte_GetDataReg(GTE_RGB0);
                p[3] = gte_GetDataReg(GTE_RGB1);
                p[5] = gte_GetDataReg(GTE_RGB2);
                ((unsigned char *)p)[3] = 6;
                ot_AddPrim(slot, p);
                if (Mesh_Advance(scr, &prim, 0x1c)) {
                    break;
                }
            }
        }
        scr->view.packetPtr = prim;
    }

    entry = (int *)mesh->prims[3];
    end = entry + mesh->primCount[3] * 6;
    if (entry != end) {
        char *prim = scr->view.packetPtr;

        for (; entry != end; entry += 6) {
            if (ObjQuad_Project(scr, entry[0], entry[1], v, &slot, &area, &area2, &sxyB)) {
                int *p = (int *)prim;
                int sxyC = gte_GetDataReg(GTE_SXY0);
                int sxyA = gte_GetDataReg(GTE_SXY1);
                int sxyD = gte_GetDataReg(GTE_SXY2);

                p[1] = Mesh_Colour(entry[2]);
                Mesh_Colours(entry[3], entry[4], entry[5]);
                p[3] = gte_GetDataReg(GTE_RGB0);
                p[7] = gte_GetDataReg(GTE_RGB1);
                p[5] = gte_GetDataReg(GTE_RGB2);
                p[2] = sxyA;
                p[4] = sxyB;
                p[6] = sxyD;
                p[8] = sxyC;
                ((unsigned char *)p)[3] = 8;
                ot_AddPrim(slot, p);
                if (Mesh_Advance(scr, &prim, 0x24)) {
                    break;
                }
            }
        }
        scr->view.packetPtr = prim;
    }

    entry = (int *)mesh->prims[4];
    end = entry + mesh->primCount[4] * 6;
    for (; entry != end; entry += 6) {
        if (ObjTri_Project(scr, entry[0], v, &slot, &area)) {
            int sxy0 = gte_GetDataReg(GTE_SXY0);
            int sxy1 = gte_GetDataReg(GTE_SXY1);
            int sxy2 = gte_GetDataReg(GTE_SXY2);
            unsigned int rgb = Mesh_Colour(entry[2]);
            MeshTex *tex = (MeshTex *)(entry + 3);

            if (MESH_ABS(area) <= OBJ_LIMIT_TRI) {
                PrimFT3Words *prim = (PrimFT3Words *)scr->view.packetPtr;

                prim->rgb = rgb;
                prim->uv0 = tex->uv0;
                prim->uv1 = tex->uv1;
                prim->uv2 = tex->uv2;
                prim->xy0 = sxy0;
                prim->xy2 = sxy1;
                prim->xy1 = sxy2;
                ((unsigned char *)prim)[3] = 7;
                ot_AddPrim(slot, prim);
                if (Mesh_Advance(scr, &scr->view.packetPtr, sizeof(PrimFT3Words))) {
                    break;
                }
            } else {
                MeshVert *p[3];
                MeshBatch batch;

                ObjRoot_Set(&root[0], v[0], tex->uv0, sxy0);
                ObjRoot_Set(&root[1], v[1], tex->uv1, sxy2);
                ObjRoot_Set(&root[2], v[2], tex->uv2, sxy1);
                p[0] = &root[0];
                p[1] = &root[1];
                p[2] = &root[2];
                ObjRoot_Check(&root[0]);
                ObjRoot_Check(&root[1]);
                ObjRoot_Check(&root[2]);
                Mesh_BatchBegin(scr, &batch, slot, rgb, tex, OBJ_LIMIT_TRI, 0);
                batch.obj = 1;
                if (!Mesh_BatchEnd(scr, &batch, Mesh_SplitTri(scr, &batch, p, 0))) {
                    break;
                }
            }
        }
    }

    entry = (int *)mesh->prims[5];
    end = entry + mesh->primCount[5] * 6;
    for (; entry != end; entry += 6) {
        if (ObjQuad_Project(scr, entry[0], entry[1], v, &slot, &area, &area2, &sxyB)) {
            int sxyC = gte_GetDataReg(GTE_SXY0);
            int sxyA = gte_GetDataReg(GTE_SXY1);
            int sxyD = gte_GetDataReg(GTE_SXY2);
            int total = MESH_ABS(area + area2);
            MeshTex *tex = (MeshTex *)(entry + 3);
            unsigned int rgb = Mesh_Colour(entry[2]);

            if (total <= OBJ_LIMIT_QUAD) {
                PrimFT4Words *prim = (PrimFT4Words *)scr->view.packetPtr;

                prim->rgb = rgb;
                prim->uv0 = tex->uv0;
                prim->uv1 = tex->uv1;
                prim->uv2 = tex->uv2 >> 16;
                prim->uv3 = tex->uv2;
                prim->xy0 = sxyA;
                prim->xy1 = sxyB;
                prim->xy2 = sxyD;
                prim->xy3 = sxyC;
                ((unsigned char *)prim)[3] = 9;
                ot_AddPrim(slot, prim);
                if (Mesh_Advance(scr, &scr->view.packetPtr, sizeof(PrimFT4Words))) {
                    break;
                }
            } else {
                MeshVert *p[4];
                MeshBatch batch;

                ObjRoot_Set(&root[0], v[0], tex->uv0, sxyA);
                ObjRoot_Set(&root[1], v[1], tex->uv1, sxyB);
                ObjRoot_Set(&root[2], v[2], tex->uv2, sxyC);
                ObjRoot_Set(&root[3], v[3], tex->uv2 >> 16, sxyD);
                p[0] = &root[0];
                p[1] = &root[1];
                p[2] = &root[2];
                p[3] = &root[3];
                ObjRoot_Check(&root[0]);
                ObjRoot_Check(&root[1]);
                ObjRoot_Check(&root[2]);
                ObjRoot_Check(&root[3]);
                Mesh_BatchBegin(scr, &batch, slot, rgb, tex, OBJ_LIMIT_QUAD, 0);
                batch.obj = 1;
                if (!Mesh_BatchEnd(scr, &batch, Mesh_SplitQuad(scr, &batch, p, 0))) {
                    break;
                }
            }
        }
    }

    entry = (int *)mesh->prims[6];
    end = entry + mesh->primCount[6] * 8;
    for (; entry != end; entry += 8) {
        if (ObjTri_Project(scr, entry[0], v, &slot, &area)) {
            int sxy0 = gte_GetDataReg(GTE_SXY0);
            int sxy1 = gte_GetDataReg(GTE_SXY1);
            int sxy2 = gte_GetDataReg(GTE_SXY2);
            MeshTex *tex = (MeshTex *)(entry + 3);
            unsigned int c0, c1, c2;

            gte_SetDataReg(GTE_RGBC, entry[2]);
            Mesh_Colours(entry[2], entry[6], entry[7]);
            c0 = gte_GetDataReg(GTE_RGB0);
            c1 = gte_GetDataReg(GTE_RGB1);
            c2 = gte_GetDataReg(GTE_RGB2);
            if (MESH_ABS(area) <= OBJ_LIMIT_TRI) {
                PrimGT3Words *prim = (PrimGT3Words *)scr->view.packetPtr;

                prim->c0 = c0;
                prim->c1 = c1;
                prim->c2 = c2;
                prim->uv0 = tex->uv0;
                prim->uv1 = tex->uv1;
                prim->uv2 = tex->uv2;
                prim->xy0 = sxy0;
                prim->xy2 = sxy1;
                prim->xy1 = sxy2;
                ((unsigned char *)prim)[3] = 9;
                ot_AddPrim(slot, prim);
                if (Mesh_Advance(scr, &scr->view.packetPtr, sizeof(PrimGT3Words))) {
                    break;
                }
            } else {
                MeshVert *p[3];
                MeshBatch batch;

                // As in a section's mesh, the colours turn round one corner.
                ObjRoot_Set(&root[0], v[0], tex->uv0, sxy0);
                ObjRoot_Set(&root[1], v[1], tex->uv1, sxy2);
                ObjRoot_Set(&root[2], v[2], tex->uv2, sxy1);
                root[0].rgb = c2;
                root[1].rgb = c0;
                root[2].rgb = c1;
                p[0] = &root[0];
                p[1] = &root[1];
                p[2] = &root[2];
                ObjRoot_Check(&root[0]);
                ObjRoot_Check(&root[1]);
                ObjRoot_Check(&root[2]);
                Mesh_BatchBegin(scr, &batch, slot, c2, tex, OBJ_LIMIT_TRI, 1);
                batch.obj = 1;
                if (!Mesh_BatchEnd(scr, &batch, Mesh_SplitTri(scr, &batch, p, 0))) {
                    break;
                }
            }
        }
    }

    entry = (int *)mesh->prims[7];
    end = entry + mesh->primCount[7] * 9;
    for (; entry != end; entry += 9) {
        if (ObjQuad_Project(scr, entry[0], entry[1], v, &slot, &area, &area2, &sxyB)) {
            int sxyC = gte_GetDataReg(GTE_SXY0);
            int sxyA = gte_GetDataReg(GTE_SXY1);
            int sxyD = gte_GetDataReg(GTE_SXY2);
            int total = MESH_ABS(area + area2);
            MeshTex *tex = (MeshTex *)(entry + 3);
            unsigned int c0 = Mesh_Colour(entry[2]);
            unsigned int c1, c2, c3;

            Mesh_Colours(entry[6], entry[7], entry[8]);
            c1 = gte_GetDataReg(GTE_RGB0);
            c2 = gte_GetDataReg(GTE_RGB1);
            c3 = gte_GetDataReg(GTE_RGB2);
            if (total <= OBJ_LIMIT_QUAD) {
                PrimGT4Words *prim = (PrimGT4Words *)scr->view.packetPtr;

                prim->c0 = c0;
                prim->c1 = c1;
                prim->c2 = c3;
                prim->c3 = c2;
                prim->uv0 = tex->uv0;
                prim->uv1 = tex->uv1;
                prim->uv2 = tex->uv2 >> 16;
                prim->uv3 = tex->uv2;
                prim->xy0 = sxyA;
                prim->xy1 = sxyB;
                prim->xy2 = sxyD;
                prim->xy3 = sxyC;
                ((unsigned char *)prim)[3] = 12;
                ot_AddPrim(slot, prim);
                if (Mesh_Advance(scr, &scr->view.packetPtr, sizeof(PrimGT4Words))) {
                    break;
                }
            } else {
                MeshVert *p[4];
                MeshBatch batch;

                ObjRoot_Set(&root[0], v[0], tex->uv0, sxyA);
                ObjRoot_Set(&root[1], v[1], tex->uv1, sxyB);
                ObjRoot_Set(&root[2], v[2], tex->uv2, sxyC);
                ObjRoot_Set(&root[3], v[3], tex->uv2 >> 16, sxyD);
                root[0].rgb = c0;
                root[1].rgb = c1;
                root[2].rgb = c2;
                root[3].rgb = c3;
                p[0] = &root[0];
                p[1] = &root[1];
                p[2] = &root[2];
                p[3] = &root[3];
                ObjRoot_Check(&root[0]);
                ObjRoot_Check(&root[1]);
                ObjRoot_Check(&root[2]);
                ObjRoot_Check(&root[3]);
                Mesh_BatchBegin(scr, &batch, slot, c0, tex, OBJ_LIMIT_QUAD, 1);
                batch.obj = 1;
                if (!Mesh_BatchEnd(scr, &batch, Mesh_SplitQuad(scr, &batch, p, 0))) {
                    break;
                }
            }
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseObj_DrawMeshUnclipped);
#endif

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseObj_DrawMeshClipped);

#ifdef NON_MATCHING
// A course object placement: 28 bytes after the group's count.
struct CourseObjPlace {
    short rot[3];
    short model;
    short scale[3];
    short lodScale;
    int pos[3];
};

// An object's mesh: a CourseMesh with its box and the bits of its coordinates.
struct CourseObjMesh {
    CourseMesh mesh;
    short boxMin[4];
    short boxMax[4];
    short bits;
};

// Draws a group of placed objects: a count, then the placements.
void CourseObjects_Draw(Camera *cam, unsigned int *group)
{
    MeshScratch *scr = MESH_SCRATCH;
    unsigned int i;

    for (i = 0; i < group[0]; i++) {
        CourseObjPlace *place = (CourseObjPlace *)(group + 1) + i;
        MATRIX *m = (MATRIX *)scr->corners;
        LodTable *lod;
        int depth;
        int level;
        CourseObjMesh *mesh;
        unsigned int flags;

        m->t[0] = place->pos[0];
        m->t[1] = place->pos[1];
        m->t[2] = place->pos[2];
        Matrix_FromEuler(m, -place->rot[1], place->rot[0], place->rot[2]);
        Matrix_Scale(m, place->scale[0], place->scale[1], -place->scale[2]);
        lod = (LodTable *)COURSE->modelLib[place->model + 1];
        *(CameraView *)scr = *(CameraView *)&cam->view;
        Matrix_Comp(&scr->view.view, m);
        depth = (cam->depthScale << 12) / place->lodScale;
        level = Lod_Select(lod, Lod_DistanceSq(lod, (int *)scr->view.view.t, depth));
        if (level == -1) {
            continue;
        }
        mesh = (CourseObjMesh *)lod->lods[level].mesh;
        Gte_LoadTransform(&scr->view, 0x1000, mesh->bits, 0xc);
        flags = Clip_TestBox(&scr->view, mesh->boxMin, mesh->boxMax);
        if (flags & 0x1f) {
            continue;
        }
        scr->view.packetPtr = g_PacketPtr;
        scr->view.packetLimit = g_PacketEnd - 0x2000;
        Obj_DrawSprites(scr, &mesh->mesh);
        Obj_DrawFlares(scr, &mesh->mesh);
        if (((flags >> 16) & 0x3f) == 0) {
            CourseObj_DrawMeshUnclipped(&mesh->mesh);
        } else {
            CourseObj_DrawMeshClipped(&mesh->mesh);
        }
        g_PacketPtr = scr->view.packetPtr;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseObjects_Draw);
#endif

// Draws object groups 0-31 of the course whose bit is set in mask (the visible sections
// from Course_Draw), then group 32 always.
void CourseObjects_DrawGroups(Camera *cam, CourseData *course, unsigned int mask)
{
    int i;

    for (i = 0; i < 32; i++, mask >>= 1) {
        if (mask & 1) {
            CourseObjects_Draw(cam, course->objGroups[i]);
        }
    }
    CourseObjects_Draw(cam, course->objGroups[i]);
}

// Adds base to a pointer.
static inline void Ptr_Relocate(void **p, int base)
{
    *p = (char *)*p + base;
}

// Adds base to the 11 pointers of a mesh header (vertices, 8 primitive lists, sprites,
// flares).
void CourseObjModel_Relocate(CourseMesh *mesh, int base)
{
    int i;

    mesh->verts = (char *)mesh->verts + base;
    for (i = 0; i < 8; i++) {
        Ptr_Relocate(&mesh->prims[i], base);
    }
    mesh->sprites = (int *)((char *)mesh->sprites + base);
    mesh->flares = (int *)((char *)mesh->flares + base);
}

#ifdef NON_MATCHING
// A section to draw, in the list sorted by distance.
struct CourseDrawNode {
    CourseDrawNode *next;
    CourseSection *section;
    unsigned int dist;
    unsigned short mode;        // the visibility list's top two bits
    unsigned short clip;        // CourseSection_TestBox's result
};

// Draws the sections visible from the camera's section, nearest first, and returns
// the object groups they use. Sections of mode 3 and up only show their flares, except
// in a replay. A mirror drops what is far away and uses the sections' mirror meshes.
unsigned int Course_Draw(Camera *cam, CourseData *course, int mirror)
{
    MeshScratch *scr = MESH_SCRATCH;
    CourseSectionTable *table = course->sections;
    RaceCam *raceCam = (RaceCam *)cam;
    short angles[4];
    CourseDrawNode nodes[256];
    CourseDrawNode *head = 0;
    CourseDrawNode *node;
    unsigned int mask = 0;
    unsigned short *vis;
    int count;
    int i;

    *(CameraView *)scr = *(CameraView *)&cam->view;
    Matrix_SwapYZ((MatrixRot *)&scr->view.view);
    scr->spriteTex = (char *)table->spriteTex;
    Gte_LoadRotationNoFog(&scr->view);
    if (mirror == 0) {
        scr->subdivTri = 0x480;
        scr->subdivQuad = 0x800;
    } else {
        scr->subdivTri = 0x120;
        scr->subdivQuad = 0x200;
    }
    Matrix_ToEuler(angles, &raceCam->view);
    scr->yawSin = g_SinTable[(unsigned short)angles[0] & 0xfff] >> 1;
    scr->yawCos = g_SinTable[((unsigned short)angles[0] & 0xfff) + 0x400] >> 1;
    vis = table->sections[raceCam->section]->visList;
    count = *vis;
    node = nodes;
    for (i = 0; i < count; i++, node++) {
        unsigned int id;
        CourseSection *section;
        int clip;
        CourseDrawNode *prev;
        CourseDrawNode *cur;

        vis++;
        id = *vis;
        node->next = 0;
        section = table->sections[id & 0x3fff];
        node->section = section;
        node->dist = Vec_ApproxDist(&scr->view, section->center);
        node->mode = id >> 14;
        if (mirror != 0 && node->dist > 0x63ffff) {
            continue;
        }
        mask |= section->groupMask;
        clip = CourseSection_TestBox(&scr->view, mirror == 0 ? section->bbox : section->bboxFar);
        if (clip == 2) {
            continue;
        }
        node->clip = clip;
        prev = 0;
        for (cur = head; cur != 0; cur = cur->next) {
            if (node->dist < cur->dist) {
                break;
            }
            prev = cur;
        }
        if (prev != 0) {
            prev->next = node;
        } else {
            head = node;
        }
        node->next = cur;
    }
    scr->view.packetPtr = g_PacketPtr;
    scr->view.packetLimit = g_PacketEnd - 0x2000;
    for (node = head; node != 0; node = node->next) {
        CourseSection *section = node->section;
        CourseMesh *mesh;

        CourseSection_LoadTransform(section, &scr->view);
        mesh = mirror == 0 ? &section->mesh : section->mirrorMesh;
        if (g_RaceWorld.isReplay == 0 && node->mode >= 3) {
            Course_DrawFlares(scr, mesh);
            continue;
        }
        Course_DrawSprites(scr, mesh);
        Course_DrawFlares(scr, mesh);
        if (node->clip == 0) {
            CourseMesh_DrawUnclipped(mesh);
        } else {
            CourseMesh_DrawClipped(mesh);
        }
    }
    g_PacketPtr = scr->view.packetPtr;
    return mask;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", Course_Draw);
#endif

// Swaps columns 1 and 2 of each row of a rotation, for the course's (x, z, y) axis order.
void Matrix_SwapYZ(MatrixRot *m)
{
    int x, y, z;

    // Unrolled: each row is read whole, then written back (x unchanged).
    x = m->m[0][0];
    y = m->m[0][1];
    z = m->m[0][2];
    m->m[0][0] = x;
    m->m[0][1] = z;
    m->m[0][2] = y;
    x = m->m[1][0];
    y = m->m[1][1];
    z = m->m[1][2];
    m->m[1][0] = x;
    m->m[1][1] = z;
    m->m[1][2] = y;
    x = m->m[2][0];
    y = m->m[2][1];
    z = m->m[2][2];
    m->m[2][0] = x;
    m->m[2][1] = z;
    m->m[2][2] = y;
}

// Gte_LoadScaled(scr, 0, 0, 0) with a zero translation and a black far colour (the game
// writes $zero straight to the GTE).
void Gte_LoadRotationNoFog(ViewScratch *scr)
{
    register int zero asm("$0");

    Gte_LoadScaled(scr, 0, 0, 0);
    gte_SetCtrlReg(GTE_TRX, zero);
    gte_SetCtrlReg(GTE_TRY, zero);
    gte_SetCtrlReg(GTE_TRZ, zero);
    gte_SetCtrlReg(GTE_RFC, zero);
    gte_SetCtrlReg(GTE_BFC, zero);
    gte_SetCtrlReg(GTE_GFC, zero);
}

// Frustum test of a section's box (4 corners' x and z, then y min and max: 8 points),
// relative to the camera and scaled to 16 bits into BoxScratch.box. Returns 0 inside,
// 1 crossing a plane, 2 outside.
#ifdef NON_MATCHING
// Handwritten: the game waits two nops between the LZCS write and the LZCR read, and
// steps two pointers into the points.
int CourseSection_TestBox(ViewScratch *scr, int *box)
{
    ClipPoint *out = ((BoxScratch *)scr)->box;
    int nx = scr->negPos[0];
    int ny = scr->negPos[1];
    int nz = scr->negPos[2];
    int boxY = box[8];
    int pass;
    unsigned int flags;

    for (pass = 2; pass != 0; pass--) {
        int *p;

        for (p = box; p != box + 8; p += 2) {
            int x = p[0] + nx;
            int y = boxY + ny;
            int z = p[1] + nz;
            int shift;

            gte_SetDataReg(GTE_LZCS, (x < 0 ? -x : x) | (y < 0 ? -y : y) | (z < 0 ? -z : z));
            shift = 19 - gte_GetDataReg(GTE_LZCR);
            if (shift >= 0) {
                x >>= shift;
                y >>= shift;
                z >>= shift;
            }
            // The course's y and z are the GTE's z and y.
            out->x = x;
            out->y = z;
            out->z = y;
            out++;
        }
        boxY = box[9];
    }
    flags = Clip_TestPoints(scr, ((BoxScratch *)scr)->box, 8);
    if (flags & 0x1f) {
        return 2;
    }
    return ((flags >> 16) & 0x3f) != 0;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseSection_TestBox);
#endif

// Approximate distance from the camera to pos: the largest absolute axis difference plus
// half the middle one plus a quarter of the smallest.
#ifdef NON_MATCHING
// The game computes z in $v0 and copies it to $v1 before its abs; GCC computes it in $v1.
unsigned int Vec_ApproxDist(ViewScratch *scr, int *pos)
{
    unsigned int x = pos[0] + scr->negPos[0];
    unsigned int y = pos[1] + scr->negPos[1];
    unsigned int z = pos[2] + scr->negPos[2];
    unsigned int t;

    if ((int)x < 0) {
        x = -x;
    }
    if ((int)y < 0) {
        y = -y;
    }
    if ((int)z < 0) {
        z = -z;
    }
    if (x < y) {
        t = x;
        x = y;
        y = t;
    }
    if (x < z) {
        t = x;
        x = z;
        z = t;
    }
    if (y < z) {
        t = y;
        y = z;
        z = t;
    }
    return x + (y >> 1) + (z >> 2);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", Vec_ApproxDist);
#endif

#ifdef NON_MATCHING
// Draws a section's mesh, wholly in front of the near plane: its flat, Gouraud, textured
// and Gouraud textured triangles and quads. Textured polygons with a large screen area
// split into smaller ones, which hides the PlayStation's affine texture warp. The
// triangle lists' entries hold a, b, c in word 0 and the texture in word 1; the quad
// lists add d in word 1. Colours follow.
void CourseMesh_DrawUnclipped(CourseMesh *mesh)
{
    MeshScratch *scr = MESH_SCRATCH;
    MeshModelVert *v[4];
    unsigned int *slot;
    int area, area2, sxyB;
    int *entry;
    int *end;

    scr->view.verts = (char *)mesh->verts;

    entry = (int *)mesh->prims[0];
    end = entry + mesh->primCount[0] * 3;
    if (entry != end) {
        char *prim = scr->view.packetPtr;

        for (; entry != end; entry += 3) {
            if (Tri_Project(scr, entry[0], v, &slot, &area)) {
                int *p = (int *)prim;

                p[1] = Mesh_Colour(entry[2]);
                p[2] = gte_GetDataReg(GTE_SXY0);
                p[4] = gte_GetDataReg(GTE_SXY1);
                p[3] = gte_GetDataReg(GTE_SXY2);
                ((unsigned char *)p)[3] = 4;
                ot_AddPrim(slot, p);
                if (Mesh_Advance(scr, &prim, 0x14)) {
                    break;
                }
            }
        }
        scr->view.packetPtr = prim;
    }

    entry = (int *)mesh->prims[1];
    end = entry + mesh->primCount[1] * 3;
    if (entry != end) {
        char *prim = scr->view.packetPtr;

        for (; entry != end; entry += 3) {
            if (Quad_Project(scr, entry[0], entry[1], v, &slot, &area, &area2, &sxyB)) {
                int *p = (int *)prim;
                int sxyC = gte_GetDataReg(GTE_SXY0);
                int sxyA = gte_GetDataReg(GTE_SXY1);
                int sxyD = gte_GetDataReg(GTE_SXY2);

                p[1] = Mesh_Colour(entry[2]);
                p[2] = sxyA;
                p[3] = sxyB;
                p[4] = sxyD;
                p[5] = sxyC;
                ((unsigned char *)p)[3] = 5;
                ot_AddPrim(slot, p);
                if (Mesh_Advance(scr, &prim, 0x18)) {
                    break;
                }
            }
        }
        scr->view.packetPtr = prim;
    }

    entry = (int *)mesh->prims[2];
    end = entry + mesh->primCount[2] * 5;
    if (entry != end) {
        char *prim = scr->view.packetPtr;

        for (; entry != end; entry += 5) {
            if (Tri_Project(scr, entry[0], v, &slot, &area)) {
                int *p = (int *)prim;

                p[2] = gte_GetDataReg(GTE_SXY0);
                p[6] = gte_GetDataReg(GTE_SXY1);
                p[4] = gte_GetDataReg(GTE_SXY2);
                gte_SetDataReg(GTE_RGBC, entry[2]);
                Mesh_Colours(entry[2], entry[3], entry[4]);
                p[1] = gte_GetDataReg(GTE_RGB0);
                p[3] = gte_GetDataReg(GTE_RGB1);
                p[5] = gte_GetDataReg(GTE_RGB2);
                ((unsigned char *)p)[3] = 6;
                ot_AddPrim(slot, p);
                if (Mesh_Advance(scr, &prim, 0x1c)) {
                    break;
                }
            }
        }
        scr->view.packetPtr = prim;
    }

    entry = (int *)mesh->prims[3];
    end = entry + mesh->primCount[3] * 6;
    if (entry != end) {
        char *prim = scr->view.packetPtr;

        for (; entry != end; entry += 6) {
            if (Quad_Project(scr, entry[0], entry[1], v, &slot, &area, &area2, &sxyB)) {
                int *p = (int *)prim;
                int sxyC = gte_GetDataReg(GTE_SXY0);
                int sxyA = gte_GetDataReg(GTE_SXY1);
                int sxyD = gte_GetDataReg(GTE_SXY2);

                p[1] = Mesh_Colour(entry[2]);
                Mesh_Colours(entry[3], entry[4], entry[5]);
                p[3] = gte_GetDataReg(GTE_RGB0);
                p[7] = gte_GetDataReg(GTE_RGB1);
                p[5] = gte_GetDataReg(GTE_RGB2);
                p[2] = sxyA;
                p[4] = sxyB;
                p[6] = sxyD;
                p[8] = sxyC;
                ((unsigned char *)p)[3] = 8;
                ot_AddPrim(slot, p);
                if (Mesh_Advance(scr, &prim, 0x24)) {
                    break;
                }
            }
        }
        scr->view.packetPtr = prim;
    }

    entry = (int *)mesh->prims[4];
    end = entry + mesh->primCount[4] * 3;
    for (; entry != end; entry += 3) {
        if (Tri_Project(scr, entry[0], v, &slot, &area)) {
            int sxy0 = gte_GetDataReg(GTE_SXY0);
            int sxy1 = gte_GetDataReg(GTE_SXY1);
            int sxy2 = gte_GetDataReg(GTE_SXY2);
            unsigned int rgb = Mesh_Colour(entry[2]);
            MeshTex *tex = Mesh_Tex(scr, entry[1], area);

            if (MESH_ABS(area) <= scr->subdivTri) {
                PrimFT3Words *prim = (PrimFT3Words *)scr->view.packetPtr;

                prim->rgb = rgb;
                prim->uv0 = tex->uv0;
                prim->uv1 = tex->uv1;
                prim->uv2 = tex->uv2;
                prim->xy0 = sxy0;
                prim->xy2 = sxy1;
                prim->xy1 = sxy2;
                ((unsigned char *)prim)[3] = 7;
                ot_AddPrim(slot, prim);
                if (Mesh_Advance(scr, &scr->view.packetPtr, sizeof(PrimFT3Words))) {
                    break;
                }
            } else {
                MeshVert root[3];
                MeshVert *p[3];
                MeshBatch batch;

                Vert_Root(&root[0], v[0], tex->uv0, sxy0, 0);
                Vert_Root(&root[1], v[1], tex->uv1, sxy2, 0);
                Vert_Root(&root[2], v[2], tex->uv2, sxy1, 0);
                p[0] = &root[0];
                p[1] = &root[1];
                p[2] = &root[2];
                Mesh_BatchBegin(scr, &batch, slot, rgb, tex, scr->subdivTri, 0);
                if (!Mesh_BatchEnd(scr, &batch, Mesh_SplitTri(scr, &batch, p, 0))) {
                    break;
                }
            }
        }
    }

    entry = (int *)mesh->prims[5];
    end = entry + mesh->primCount[5] * 3;
    for (; entry != end; entry += 3) {
        if (Quad_Project(scr, entry[0], entry[1], v, &slot, &area, &area2, &sxyB)) {
            int sxyC = gte_GetDataReg(GTE_SXY0);
            int sxyA = gte_GetDataReg(GTE_SXY1);
            int sxyD = gte_GetDataReg(GTE_SXY2);
            int total = MESH_ABS(area + area2);
            MeshTex *tex = Mesh_Tex(scr, entry[1], total);
            unsigned int rgb = Mesh_Colour(entry[2]);

            if (total <= scr->subdivQuad) {
                PrimFT4Words *prim = (PrimFT4Words *)scr->view.packetPtr;

                prim->rgb = rgb;
                prim->uv0 = tex->uv0;
                prim->uv1 = tex->uv1;
                prim->uv2 = tex->uv2 >> 16;
                prim->uv3 = tex->uv2;
                prim->xy0 = sxyA;
                prim->xy1 = sxyB;
                prim->xy2 = sxyD;
                prim->xy3 = sxyC;
                ((unsigned char *)prim)[3] = 9;
                ot_AddPrim(slot, prim);
                if (Mesh_Advance(scr, &scr->view.packetPtr, sizeof(PrimFT4Words))) {
                    break;
                }
            } else {
                MeshVert root[4];
                MeshVert *p[4];
                MeshBatch batch;

                Vert_Root(&root[0], v[0], tex->uv0, sxyA, 0);
                Vert_Root(&root[1], v[1], tex->uv1, sxyB, 0);
                Vert_Root(&root[2], v[2], tex->uv2, sxyC, 0);
                Vert_Root(&root[3], v[3], tex->uv2 >> 16, sxyD, 0);
                p[0] = &root[0];
                p[1] = &root[1];
                p[2] = &root[2];
                p[3] = &root[3];
                Mesh_BatchBegin(scr, &batch, slot, rgb, tex, scr->subdivQuad, 0);
                if (!Mesh_BatchEnd(scr, &batch, Mesh_SplitQuad(scr, &batch, p, 0))) {
                    break;
                }
            }
        }
    }

    entry = (int *)mesh->prims[6];
    end = entry + mesh->primCount[6] * 5;
    for (; entry != end; entry += 5) {
        if (Tri_Project(scr, entry[0], v, &slot, &area)) {
            int sxy0 = gte_GetDataReg(GTE_SXY0);
            int sxy1 = gte_GetDataReg(GTE_SXY1);
            int sxy2 = gte_GetDataReg(GTE_SXY2);
            unsigned int c0, c1, c2;
            MeshTex *tex;

            gte_SetDataReg(GTE_RGBC, entry[2]);
            Mesh_Colours(entry[2], entry[3], entry[4]);
            c0 = gte_GetDataReg(GTE_RGB0);
            c1 = gte_GetDataReg(GTE_RGB1);
            c2 = gte_GetDataReg(GTE_RGB2);
            tex = Mesh_Tex(scr, entry[1], area);
            if (MESH_ABS(area) <= scr->subdivTri) {
                PrimGT3Words *prim = (PrimGT3Words *)scr->view.packetPtr;

                prim->c0 = c0;
                prim->c1 = c1;
                prim->c2 = c2;
                prim->uv0 = tex->uv0;
                prim->uv1 = tex->uv1;
                prim->uv2 = tex->uv2;
                prim->xy0 = sxy0;
                prim->xy2 = sxy1;
                prim->xy1 = sxy2;
                ((unsigned char *)prim)[3] = 9;
                ot_AddPrim(slot, prim);
                if (Mesh_Advance(scr, &scr->view.packetPtr, sizeof(PrimGT3Words))) {
                    break;
                }
            } else {
                MeshVert root[3];
                MeshVert *p[3];
                MeshBatch batch;

                // The game turns the colours round one corner here.
                Vert_Root(&root[0], v[0], tex->uv0, sxy0, c2);
                Vert_Root(&root[1], v[1], tex->uv1, sxy2, c0);
                Vert_Root(&root[2], v[2], tex->uv2, sxy1, c1);
                p[0] = &root[0];
                p[1] = &root[1];
                p[2] = &root[2];
                Mesh_BatchBegin(scr, &batch, slot, c2, tex, scr->subdivTri, 1);
                if (!Mesh_BatchEnd(scr, &batch, Mesh_SplitTri(scr, &batch, p, 0))) {
                    break;
                }
            }
        }
    }

    entry = (int *)mesh->prims[7];
    end = entry + mesh->primCount[7] * 6;
    for (; entry != end; entry += 6) {
        if (Quad_Project(scr, entry[0], entry[1], v, &slot, &area, &area2, &sxyB)) {
            int sxyC = gte_GetDataReg(GTE_SXY0);
            int sxyA = gte_GetDataReg(GTE_SXY1);
            int sxyD = gte_GetDataReg(GTE_SXY2);
            int total = MESH_ABS(area + area2);
            MeshTex *tex = Mesh_Tex(scr, entry[1], total);
            unsigned int c0 = Mesh_Colour(entry[2]);
            unsigned int c1, c2, c3;

            Mesh_Colours(entry[3], entry[4], entry[5]);
            c1 = gte_GetDataReg(GTE_RGB0);
            c2 = gte_GetDataReg(GTE_RGB1);
            c3 = gte_GetDataReg(GTE_RGB2);
            if (total <= scr->subdivQuad) {
                PrimGT4Words *prim = (PrimGT4Words *)scr->view.packetPtr;

                prim->c0 = c0;
                prim->c1 = c1;
                prim->c2 = c3;
                prim->c3 = c2;
                prim->uv0 = tex->uv0;
                prim->uv1 = tex->uv1;
                prim->uv2 = tex->uv2 >> 16;
                prim->uv3 = tex->uv2;
                prim->xy0 = sxyA;
                prim->xy1 = sxyB;
                prim->xy2 = sxyD;
                prim->xy3 = sxyC;
                ((unsigned char *)prim)[3] = 12;
                ot_AddPrim(slot, prim);
                if (Mesh_Advance(scr, &scr->view.packetPtr, sizeof(PrimGT4Words))) {
                    break;
                }
            } else {
                MeshVert root[4];
                MeshVert *p[4];
                MeshBatch batch;

                Vert_Root(&root[0], v[0], tex->uv0, sxyA, c0);
                Vert_Root(&root[1], v[1], tex->uv1, sxyB, c1);
                Vert_Root(&root[2], v[2], tex->uv2, sxyC, c2);
                Vert_Root(&root[3], v[3], tex->uv2 >> 16, sxyD, c3);
                p[0] = &root[0];
                p[1] = &root[1];
                p[2] = &root[2];
                p[3] = &root[3];
                Mesh_BatchBegin(scr, &batch, slot, c0, tex, scr->subdivQuad, 1);
                if (!Mesh_BatchEnd(scr, &batch, Mesh_SplitQuad(scr, &batch, p, 0))) {
                    break;
                }
            }
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseMesh_DrawUnclipped);
#endif

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseMesh_DrawClipped);
}
