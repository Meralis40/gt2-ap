// Resident code 8007e0b0-8007e8a0 (prim): adding primitives to the packet pool and an
// ordering table, projecting points and a clipped 3D line.
#include "common.h"
#include "gte.h"

extern "C" {
extern unsigned int *g_PacketPtr;

// Adds a POLY_G4 after ot and returns it.
PrimG4 *Prim_AddPolyG4(uint *ot)
{
    unsigned int *p;
    unsigned int link;

    link = Mips_Lwl2(ot);
    p = g_PacketPtr;
    ((unsigned char *)p)[3] = 8;
    Mips_Swl2(link, p);
    Mips_Swl2((unsigned int)p << 8, ot);
    g_PacketPtr = p + 9;
    return (PrimG4 *)(p + 1);
}

// Adds a POLY_F3 after ot and returns it.
PrimF3 *Prim_AddPolyF3(uint *ot, uint rgb)
{
    unsigned int *p;
    unsigned int link;

    link = Mips_Lwl2(ot);
    p = g_PacketPtr;
    ((unsigned char *)p)[3] = 4;
    Mips_Swl2(link, p);
    Mips_Swl2((unsigned int)p << 8, ot);
    p[1] = rgb ^ 0x20000000;
    g_PacketPtr = p + 5;
    return (PrimF3 *)(p + 1);
}

// Loads the GTE with no translation, the view's rotation and its clip planes.
// Handwritten.
void func_8007E11C(ViewScratch *sp)
{
    gte_SetCtrlReg(GTE_TRX, 0);
    gte_SetCtrlReg(GTE_TRY, 0);
    gte_SetCtrlReg(GTE_TRZ, 0);
    gte_SetRotMatrixWords(&sp->view);
    gte_SetCtrlReg(GTE_L11L12, ((int *)sp->clipX)[0]);
    gte_SetCtrlReg(GTE_L13L21, ((int *)sp->clipX)[1]);
    gte_SetCtrlReg(GTE_L22L23, ((int *)sp->clipX)[2]);
    gte_SetCtrlReg(GTE_L31L32, ((int *)sp->clipX)[3]);
    gte_SetCtrlReg(GTE_L33, ((int *)sp->clipX)[4]);
    gte_SetCtrlReg(GTE_LR1LR2, ((int *)sp->clipY)[0]);
    gte_SetCtrlReg(GTE_LR3LG1, ((int *)sp->clipY)[1]);
    gte_SetCtrlReg(GTE_LG2LG3, ((int *)sp->clipY)[2]);
    gte_SetCtrlReg(GTE_LB1LB2, ((int *)sp->clipY)[3]);
    gte_SetCtrlReg(GTE_LB3, ((int *)sp->clipY)[4]);
}

// An end of func_8007E1A8's line.
struct LineEnd {
    short x, y, z, pad;         // the position, scaled
    short vx, vy, vz, pad2;     // in view space
    int sxy;
    short dist[5];              // to each plane, negative outside (as in ClipVert)
    short failed;               // -1 when the GTE's error flag was set
};

// The two ends together, for the copy.
struct LineEnds {
    LineEnd e[2];
};

// Draws a flat line in 3D between two points (moved by sp->negPos) in sp's OT entry. A
// line with an end that failed to project is clipped against the near plane and the view's
// side planes, as func_8007E11C has set them up. Each cut starts from the original ends and
// replaces the outside end's copy, so of the planes that cut one end the last one wins.
// Handwritten.
static void LineEnd_Project(LineEnd *e)
{
    gte_LoadDataReg(GTE_VXY0, &e->x);
    gte_LoadDataReg(GTE_VZ0, &e->z);
    gte_rtps();
    gte_StoreDataReg(GTE_SXY2, &e->sxy);
    e->vx = gte_GetDataReg(GTE_IR1);
    e->vy = gte_GetDataReg(GTE_IR2);
    e->vz = gte_GetDataReg(GTE_IR3);
}

// The distances to the planes of the point in view space, of depth z.
static void LineEnd_Classify(ViewScratch *sp, LineEnd *e, int z)
{
    gte_LoadDataReg(GTE_VXY0, &e->vx);
    gte_LoadDataReg(GTE_VZ0, &e->vz);
    gte_mvmva(1, GTE_MX_LL, GTE_V_V0, GTE_CV_NONE, 0);
    e->dist[0] = z - (unsigned short)sp->nearZ;
    e->dist[1] = gte_GetDataReg(GTE_IR1);
    e->dist[2] = gte_GetDataReg(GTE_IR2);
    gte_mvmva(1, GTE_MX_LC, GTE_V_V0, GTE_CV_NONE, 0);
    e->dist[3] = gte_GetDataReg(GTE_IR1);
    e->dist[4] = gte_GetDataReg(GTE_IR2);
}

void func_8007E1A8(ViewScratch *sp, int *pts, unsigned int rgb)
{
    LineEnds ends;
    LineEnds clipped;
    LineF2 *line;
    int i;

    for (i = 0; i < 2; i++) {
        LineEnd *e = &ends.e[i];
        int x = pts[0] + sp->negPos[0];
        int y = pts[1] + sp->negPos[1];
        int z = pts[2] + sp->negPos[2];
        int ax = x < 0 ? -x : x;
        int ay = y < 0 ? -y : y;
        int az = z < 0 ? -z : z;
        int shift = 31 - gte_Lzc(ax | ay | az) - 12;

        if (shift >= 0) {
            x >>= shift;
            y >>= shift;
            z >>= shift;
        } else {
            x <<= -shift;
            y <<= -shift;
            z <<= -shift;
        }
        e->x = x;
        e->y = y;
        e->z = z;
        LineEnd_Project(e);
        e->failed = gte_GetCtrlReg(GTE_FLAG) >> 31;
        pts += 3;
    }

    if (!((ends.e[0].failed | ends.e[1].failed) & 0x8000)) {
        line = Prim_AddLineF2(sp->ot, rgb);
        *(int *)&line->x0 = ends.e[0].sxy;
        *(int *)&line->x1 = ends.e[1].sxy;
        return;
    }

    for (i = 0; i < 2; i++) {
        LineEnd_Classify(sp, &ends.e[i], ends.e[i].vz);
    }
    clipped = ends;
    for (i = 0; i < 5; i++) {
        int d = clipped.e[0].dist[i];

        if ((d ^ clipped.e[1].dist[i]) >= 0) {
            // Both ends on one side.
            if (d < 0) {
                return;
            }
        } else {
            LineEnd *in = &ends.e[0];
            LineEnd *out = &ends.e[1];
            LineEnd *cut = &clipped.e[1];
            int t;
            int x;
            int y;
            int z;

            if (d < 0) {
                in = &ends.e[1];
                out = &ends.e[0];
                cut = &clipped.e[0];
            }
            t = (in->dist[i] << 12) / (in->dist[i] - out->dist[i]);
            x = in->x;
            y = in->y;
            z = in->z;
            cut->x = x - (((x - out->x) * t) >> 12);
            cut->y = y - (((y - out->y) * t) >> 12);
            cut->z = z - (((z - out->z) * t) >> 12);
            LineEnd_Project(cut);
            LineEnd_Classify(sp, cut, gte_GetDataReg(GTE_IR3));
        }
    }
    line = Prim_AddLineF2(sp->ot, rgb);
    *(int *)&line->x0 = clipped.e[0].sxy;
    *(int *)&line->x1 = clipped.e[1].sxy;
}

// Transforms and projects count 32-bit points (moved by the view's negPos, scaled down
// to fit the GTE when needed): dst gets the screen position, the depth (>> 13) and -1 when
// the GTE's error flag was set.
// Handwritten.
void Points_Project(void *camera, ScreenPoint *dst, void *src, int count)
{
    ViewScratch *sp = (ViewScratch *)camera;
    int *p = (int *)src;
    int tx = sp->negPos[0];
    int ty = sp->negPos[1];
    int tz = sp->negPos[2];

    gte_SetCtrlReg(GTE_TRX, 0);
    gte_SetCtrlReg(GTE_TRY, 0);
    gte_SetCtrlReg(GTE_TRZ, 0);
    gte_SetRotMatrixWords(&sp->view);
    for (; count != 0; count--) {
        int x = p[0] + tx;
        int y = p[1] + ty;
        int z = p[2] + tz;
        int ax = x < 0 ? -x : x;
        int ay = y < 0 ? -y : y;
        int az = z < 0 ? -z : z;
        int shift = 31 - gte_Lzc(ax | ay | az) - 12;
        int scale = 0;
        int flags;

        if (shift > 0) {
            scale = shift;
            x >>= scale;
            y >>= scale;
            z >>= scale;
        }
        gte_SetDataReg(GTE_VXY0, (x & 0xffff) | (y << 16));
        gte_SetDataReg(GTE_VZ0, z);
        gte_rtps();
        gte_StoreDataReg(GTE_SXY2, dst);
        z = gte_GetDataReg(GTE_IR3);
        flags = gte_GetCtrlReg(GTE_FLAG);
        dst->z = (z << scale) >> 13;
        dst->pad = flags >> 31;
        dst++;
        p += 3;
    }
}

// Adds a POLY_G3 after ot and returns it.
PrimG3 *Prim_AddPolyG3(uint *ot)
{
    unsigned int *p;
    unsigned int link;

    link = Mips_Lwl2(ot);
    p = g_PacketPtr;
    ((unsigned char *)p)[3] = 6;
    Mips_Swl2(link, p);
    Mips_Swl2((unsigned int)p << 8, ot);
    g_PacketPtr = p + 7;
    return (PrimG3 *)(p + 1);
}

// Adds a closed outline of 4 points (a polyline with its terminator) after ot and returns it.
PrimLineF4 *Prim_AddLineF4(void *ot, unsigned int rgb)
{
    unsigned int *p;
    unsigned int link;

    link = Mips_Lwl2(ot);
    p = g_PacketPtr;
    ((unsigned char *)p)[3] = 7;
    Mips_Swl2(link, p);
    Mips_Swl2((unsigned int)p << 8, ot);
    p[1] = rgb ^ 0x48000000;
    p[7] = 0x50005000;
    g_PacketPtr = p + 8;
    return (PrimLineF4 *)(p + 1);
}

// Adds a rectangle's outline, or a line when w or h is 1.
void Prim_AddRectOutline(void *ot, unsigned int rgb, RECT *rect)
{
    int x = rect->x;
    int y = rect->y;
    int w = rect->w;
    int h = rect->h;

    if (w != 0 && h != 0) {
        if (h == 1) {
            LineF2 *line = Prim_AddLineF2((uint *)ot, rgb);

            line->x0 = x;
            line->y0 = y;
            line->x1 = x + w;
            line->y1 = y;
        } else if (w == 1) {
            LineF2 *line = Prim_AddLineF2((uint *)ot, rgb);

            line->x0 = x;
            line->y0 = y;
            line->x1 = x;
            line->y1 = y + h;
        } else {
            PrimLineF4 *p = Prim_AddLineF4(ot, rgb);
            int right = x + w - 1;
            int bottom;

            p->x2 = p->x1 = right;
            bottom = y + h - 1;
            p->x4 = x;
            p->y4 = y;
            p->x0 = x;
            p->y0 = y;
            p->y1 = y;
            p->y2 = bottom;
            p->x3 = x;
            p->y3 = bottom;
        }
    }
}

// Adds a POLY_FT4 after ot and returns it.
PrimFT4 *Prim_AddPolyFT4(uint *ot, uint rgb)
{
    unsigned int *p;
    unsigned int link;

    link = Mips_Lwl2(ot);
    p = g_PacketPtr;
    ((unsigned char *)p)[3] = 9;
    Mips_Swl2(link, p);
    Mips_Swl2((unsigned int)p << 8, ot);
    p[1] = rgb ^ 0x2c000000;
    g_PacketPtr = p + 10;
    return (PrimFT4 *)(p + 1);
}

}
