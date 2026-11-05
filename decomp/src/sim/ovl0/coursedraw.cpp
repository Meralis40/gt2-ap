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
    int subdivFar;              // 0x398
    int subdivNear;
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
// commands scheduled by hand), so C++ can't reproduce them. They have no C++ yet.
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseObj_DrawMeshUnclipped);

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
        scr->subdivFar = 0x480;
        scr->subdivNear = 0x800;
    } else {
        scr->subdivFar = 0x120;
        scr->subdivNear = 0x200;
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

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseMesh_DrawUnclipped);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseMesh_DrawClipped);
}
