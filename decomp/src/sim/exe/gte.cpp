// Resident code 8007adc8-8007c268 (gte). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"
#include "gte.h"

// The scratchpad's first 0x9c bytes as the drawing code lays them out (carmodel.cpp's
// CarScratch): a copy of a Camera from its view matrix on, then the drawing state.
struct ViewScratch {
    MATRIX view;
    int negPos[3];
    short clipX[9];             // 0x2c: loaded as the light matrix; rows 0 and 1 are the
    short pad3e;                // left and right planes' normals
    short clipY[9];             // 0x40: loaded as the light colour matrix; the top and
    short pad52;                // bottom planes' normals
    int ofx;
    int ofy;
    short projH;
    short screenW;
    short screenH;
    short nearZ;                // 0x62
    unsigned int *ot;
    char *packetPtr;            // 0x68
    char *packetLimit;
    char *verts;
    char *normals;
    int savedTrans[3];          // 0x78: the view, scaled down by Gte_LoadScaled
    short savedRot[10];
    short otShift;              // 0x98: the translation's shift
    short rotShift;             // 0x9a: the rotation's shift
};

// A point for Clip_TestPoints.
struct ClipPoint {
    short x;
    short y;
    short z;
    short pad;
};

// As carmodel.cpp: a model's LOD count and entries.
struct LodEntry {
    int dist;
    void *mesh;
};

struct LodTable {
    int count;
    LodEntry lods[1];
};

// A MATRIX's rotation alone, for the temporary matrices that Matrix_Mul takes.
struct MatrixRot {
    short m[3][3];
};

extern "C" {
extern short g_SinTable[];
extern GfxIrqState g_GfxIrq;

unsigned int Clip_TestPoints(ViewScratch *sp, ClipPoint *pts, int count);
void Dma_WaitGpu(...);
void FUN_800824ac(void *ot, RECT *r);
void Gte_LoadScaled(ViewScratch *sp, int msb, int bits, int rotBits);
int Gte_TransMsb(MATRIX *m, int bias);
VECTOR *Matrix_ApplyAdd(VECTOR *dst, MATRIX *m, VECTOR *v, VECTOR *add);
VECTOR *Matrix_ApplyLV(VECTOR *dst, MATRIX *m, VECTOR *v);
MATRIX *Matrix_FromEuler(MATRIX *m, int x, int y, int z);
void Matrix_Transpose(MATRIX *dst, MATRIX *src);
RECT *Prim_AddTile(void *ot, unsigned int rgb);
void Gpu_DrawSync(void);
void *Gpu_LoadImageBlock(int *block, RECT *rect);
int Gpu_TransferImage(RECT *rect, unsigned int *data, int store);
void Gpu_WaitReady(...);
MATRIX *Matrix_Mul(MATRIX *dst, MATRIX *a, MATRIX *b);
MATRIX *Matrix_Translate(MATRIX *m, VECTOR *v);
void *Tim_LoadClut(void *tim, RECT *rect);

// Squares each LOD distance (in 14-bit units), for comparing with Lod_DistanceSq.
#ifdef NON_MATCHING
// The game keeps the pointer in $a0 and the end in $v1; GCC 2.8 swaps them. Permuter
// didn't find a form.
void LodTable_SquareDistances(LodTable *table)
{
    LodEntry *e = table->lods;
    LodEntry *end = e + table->count;
    unsigned int d;

loop:
    if (e == end) {
        return;
    }
    d = (unsigned int)e->dist >> 14;
    e->dist = d * d;
    e++;
    goto loop;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", LodTable_SquareDistances);
#endif

// Sets LOD i's distance to dist (squared, in 14-bit units), if there is one.
void FUN_8007ae08(LodTable *table, int i, int dist)
{
    if (i < table->count) {
        table->lods[i].dist = (dist >> 14) * (dist >> 14);
    }
}

// The squared length of pos (a view translation, in 14-bit units), scaled twice by
// depthScale (a 4.12 fraction).
#ifdef NON_MATCHING
// Handwritten. The original returns the 64-bit result, its high word in $v1; callers use
// the low word.
int Lod_DistanceSq(void *table, int *pos, int depthScale)
{
    long long d;

    gte_SetDataReg(GTE_IR1, pos[0] >> 14);
    gte_SetDataReg(GTE_IR2, pos[1] >> 14);
    gte_SetDataReg(GTE_IR3, pos[2] >> 14);
    gte_sqr(0);
    d = gte_GetDataReg(GTE_MAC1) + gte_GetDataReg(GTE_MAC2) + gte_GetDataReg(GTE_MAC3);
    d = (d * depthScale) >> 12;
    // The second product takes the first's low word as unsigned.
    return (int)(((unsigned long long)(unsigned int)d * (long long)depthScale) >> 12);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Lod_DistanceSq);
#endif

// The first LOD whose squared distance is beyond distSq, or -1.
#ifdef NON_MATCHING
// The game walks the entries with a pointer from table - 8, loading the next distance
// one pass ahead, and returns the index from that pointer; no loop form tried gives it.
int Lod_Select(LodTable *table, unsigned int distSq)
{
    int n = table->count;
    int i;

    for (i = 0; n != 0; n--, i++) {
        if (distSq < (unsigned int)table->lods[i].dist) {
            return i;
        }
    }
    return -1;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Lod_Select);
#endif

void Gpu_DrawSync(void)
{
    Gpu_WaitReady();
    Dma_WaitGpu();
    Gpu_WaitReady();
}

MATRIX *Matrix_Identity(MATRIX *m)
{
    int *w = (int *)m;

    w[0] = 0x1000;
    w[1] = 0;
    w[2] = 0x1000;
    w[3] = 0;
    w[4] = 0x1000;
    w[5] = 0;
    w[6] = 0;
    w[7] = 0;
    return m;
}

// m = the rotation by the Euler angles x, y and z, with no translation.
MATRIX *Matrix_SetEuler(MATRIX *m, int x, int y, int z)
{
    Matrix_FromEuler(m, x, y, z);
    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    return m;
}

MATRIX *Matrix_Copy(MATRIX *dst, MATRIX *src)
{
    *dst = *src;
    return dst;
}

MATRIX *Matrix_Comp(MATRIX *m, MATRIX *n)
{
    Matrix_Translate(m, (VECTOR *)n->t);
    Matrix_Mul(m, m, n);
    return m;
}

// m's translation += m's rotation * v.
MATRIX *Matrix_Translate(MATRIX *m, VECTOR *v)
{
    Matrix_ApplyAdd((VECTOR *)m->t, m, v, (VECTOR *)m->t);
    return m;
}

// m's rotation *= the rotation by the Euler angles x, y and z.
MATRIX *Matrix_RotEuler(MATRIX *m, int x, int y, int z)
{
    MatrixRot r;

    Matrix_FromEuler((MATRIX *)&r, x, y, z);
    Matrix_Mul(m, m, (MATRIX *)&r);
    return m;
}

// m's rotation *= a rotation by angle (4096 to a turn) about X, Y or Z.
MATRIX *Matrix_RotX(MATRIX *m, int angle)
{
    MatrixRot r;
    int c;
    int s;

    c = g_SinTable[(angle & 0xfff) + 0x400];
    s = g_SinTable[angle & 0xfff];
    r.m[0][0] = 0x1000;
    r.m[0][1] = 0;
    r.m[0][2] = 0;
    r.m[1][0] = 0;
    r.m[1][1] = c;
    r.m[1][2] = -s;
    r.m[2][0] = 0;
    r.m[2][1] = s;
    r.m[2][2] = c;
    Matrix_Mul(m, m, (MATRIX *)&r);
    return m;
}

MATRIX *Matrix_RotY(MATRIX *m, int angle)
{
    MatrixRot r;
    int c;
    int s;

    c = g_SinTable[(angle & 0xfff) + 0x400];
    s = g_SinTable[angle & 0xfff];
    r.m[0][0] = c;
    r.m[0][1] = 0;
    r.m[0][2] = s;
    r.m[1][0] = 0;
    r.m[1][1] = 0x1000;
    r.m[1][2] = 0;
    r.m[2][0] = -s;
    r.m[2][1] = 0;
    r.m[2][2] = c;
    Matrix_Mul(m, m, (MATRIX *)&r);
    return m;
}

MATRIX *Matrix_RotZ(MATRIX *m, int angle)
{
    MatrixRot r;
    int c;
    int s;

    c = g_SinTable[(angle & 0xfff) + 0x400];
    s = g_SinTable[angle & 0xfff];
    r.m[0][0] = c;
    r.m[0][1] = -s;
    r.m[0][2] = 0;
    r.m[1][0] = s;
    r.m[1][1] = c;
    r.m[1][2] = 0;
    r.m[2][0] = 0;
    r.m[2][1] = 0;
    r.m[2][2] = 0x1000;
    Matrix_Mul(m, m, (MATRIX *)&r);
    return m;
}

// m's rotation *= a scale by x, y and z (4.12), unless all three are 1.
MATRIX *Matrix_Scale(MATRIX *m, int x, int y, int z)
{
    MatrixRot r;

    if (x == y && y == z && z == 0x1000) {
        return m;
    }
    r.m[0][0] = x;
    r.m[0][1] = 0;
    r.m[0][2] = 0;
    r.m[1][0] = 0;
    r.m[1][1] = y;
    r.m[1][2] = 0;
    r.m[2][0] = 0;
    r.m[2][1] = 0;
    r.m[2][2] = z;
    Matrix_Mul(m, m, (MATRIX *)&r);
    return m;
}

// The index of the highest bit set in any of m's translation components (as magnitudes,
// each plus bias), from the GTE's leading zero counter.
#ifdef NON_MATCHING
// Handwritten.
int Gte_TransMsb(MATRIX *m, int bias)
{
    int x = m->t[0];
    int y = m->t[1];
    int z = m->t[2];

    if (x < 0) {
        x = -x;
    }
    if (y < 0) {
        y = -y;
    }
    if (z < 0) {
        z = -z;
    }
    gte_SetDataReg(GTE_LZCS, (x + bias) | (y + bias) | (z + bias));
    return 31 - gte_GetDataReg(GTE_LZCR);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gte_TransMsb);
#endif

void Camera_SetFrustum(Camera *cam, int left, int right, int top, int bottom, int h)
{
    cam->extentX = (short)right - (short)left;
    cam->extentY = (short)top - (short)bottom;
    cam->frustumH = (short)h;
    cam->centerX = (short)(((left + right >> 1) << 0xc) / h);
    cam->centerY = (short)(((bottom + top >> 1) << 0xc) / h);
}

// Sets up cam for drawing from the camera transform view into ot (an OT and its packet
// area, 0x417c bytes on): the view matrix (the inverse of view, then mirrored in Y and Z
// and offset to the frustum's centre), the projection (the screen centre and a
// projection distance of frustumH / 2), the LOD depth scale and the side planes of the
// frustum, widened by margin pixels.
#ifdef NON_MATCHING
// Register allocation differs throughout: the game keeps cam in $s1, &cam->invRot in
// $s2 and one product in $s0. Permuter didn't find a form.
void Camera_Update(Camera *cam, MATRIX *view, void *ot, int margin)
{
    MatrixRot proj;
    int w = cam->width;
    int h = cam->height;
    int sx = (w << 12) / cam->extentX;
    int sy = (h << 12) / cam->extentY;
    int fh;
    int size;
    int c;
    int s;
    int ex;
    int ey;

    proj.m[0][0] = sx;
    proj.m[0][1] = 0;
    proj.m[0][2] = cam->centerX * sx >> 12;
    proj.m[1][0] = 0;
    proj.m[1][1] = -sy;
    proj.m[1][2] = -(cam->centerY * sy >> 12);
    proj.m[2][0] = 0;
    proj.m[2][1] = 0;
    proj.m[2][2] = -0x1000;
    fh = cam->frustumH;
    cam->ot2 = ot;
    cam->otEnd = (char *)ot + 0x417c;
    size = h;
    if (h < w) {
        size = w;
    }
    cam->ofx = w << 15;
    cam->ofy = h << 15;
    cam->projH = fh;
    cam->screenW = w;
    cam->screenH = h;
    cam->nearZ = fh >> 1;
    cam->ot = ot;
    cam->depthScale = ((size >> 1) << 12) / fh;
    ex = (w >> 1) + margin;
    ey = (h >> 1) + margin;
    if (ex < fh) {
        c = (ex << 12) / fh;
        s = 0x1000;
    } else {
        c = 0x1000;
        s = (fh << 12) / ex;
    }
    cam->clipX[0] = s;
    cam->clipX[1] = 0;
    cam->clipX[2] = c;
    cam->clipX[3] = -s;
    cam->clipX[4] = 0;
    cam->clipX[5] = c;
    cam->clipX[6] = 0;
    cam->clipX[7] = 0;
    cam->clipX[8] = 0;
    if (ey < fh) {
        c = (ey << 12) / fh;
        s = 0x1000;
    } else {
        c = 0x1000;
        s = (fh << 12) / ey;
    }
    cam->clipY[0] = 0;
    cam->clipY[1] = s;
    cam->clipY[2] = c;
    cam->clipY[3] = 0;
    cam->clipY[4] = -s;
    cam->clipY[5] = c;
    cam->clipY[6] = 0;
    cam->clipY[7] = 0;
    cam->clipY[8] = 0;
    cam->negPos[0] = -view->t[0];
    cam->negPos[1] = -view->t[1];
    cam->negPos[2] = -view->t[2];
    Matrix_Transpose((MATRIX *)cam->invRot, view);
    Matrix_Mul(&cam->view, (MATRIX *)&proj, (MATRIX *)cam->invRot);
    Matrix_ApplyLV((VECTOR *)cam->view.t, &cam->view, (VECTOR *)cam->negPos);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Camera_Update);
#endif

// Queues a clear of cam's viewport, moved down by dy, at the end of its OT.
void FUN_8007b5ac(Camera *cam, int dy)
{
    RECT r;
    int x = cam->vpX;
    int y = cam->vpY;
    int w = cam->width;
    int h = cam->height;

    r.x = x;
    r.y = y + dy;
    r.w = w - 1;
    r.h = h - 1;
    FUN_800824ac(cam->otEnd, &r);
}

// Fills cam's screen with rgb, at the end of its OT.
void FUN_8007b5fc(Camera *cam, unsigned int rgb)
{
    RECT *r = Prim_AddTile(cam->otEnd, rgb);
    int w = cam->width;
    int h = cam->height;

    r->x = 0;
    r->y = 0;
    r->w = w;
    r->h = h;
}

// Transforms count points with the loaded view (Gte_LoadScaled) and codes each against
// the view's planes: bit 0 nearer than nearZ, bits 1-4 outside the left, right, top and
// bottom planes, bit 5 a GTE error; and against the screen: bits 1-4 left of, right of,
// above and below it (none when the GTE failed). Returns the AND of the plane codes, ORed
// with the AND of the screen codes, then the OR of the plane codes in bits 16 on. With no
// points that is 0xffffffff.
#ifdef NON_MATCHING
// Handwritten: the original projects each point one pass ahead (its last pass projects
// the word after the array, unused).
unsigned int Clip_TestPoints(ViewScratch *sp, ClipPoint *pts, int count)
{
    unsigned int planeAnd = 0xffffffff;
    unsigned int screenAnd = 0xffffffff;
    unsigned int planeOr = 0;

    for (; count != 0; count--, pts++) {
        int flags;
        int z;
        int sxy;
        int x;
        int y;
        unsigned int plane;
        unsigned int screen;

        gte_LoadDataReg(GTE_VXY0, &pts->x);
        gte_LoadDataReg(GTE_VZ0, &pts->z);
        gte_rtps();
        flags = gte_GetCtrlReg(GTE_FLAG);
        x = gte_GetDataReg(GTE_IR1);
        y = gte_GetDataReg(GTE_IR2);
        z = gte_GetDataReg(GTE_IR3);
        sxy = gte_GetDataReg(GTE_SXY2);
        // The view-space position (still in IR1-IR3) against the side planes, then
        // (through V1) against the top and bottom ones.
        gte_mvmva(1, GTE_MX_LL, GTE_V_IR, GTE_CV_NONE, 0);
        gte_SetDataReg(GTE_VXY1, (x & 0xffff) | y << 16);
        gte_SetDataReg(GTE_VZ1, z);
        plane = (unsigned int)(z - (unsigned short)sp->nearZ) >> 31;
        if (gte_GetDataReg(GTE_IR1) < 0) {
            plane |= 2;
        }
        if (gte_GetDataReg(GTE_IR2) < 0) {
            plane |= 4;
        }
        gte_mvmva(1, GTE_MX_LC, GTE_V_V1, GTE_CV_NONE, 0);
        if (flags < 0) {
            plane |= 0x20;
        }
        if (gte_GetDataReg(GTE_IR1) < 0) {
            plane |= 8;
        }
        if (gte_GetDataReg(GTE_IR2) < 0) {
            plane |= 0x10;
        }
        x = (short)sxy;
        y = sxy >> 16;
        screen = 0;
        if (x < 0) {
            screen |= 2;
        }
        if (x >= (unsigned short)sp->screenW) {
            screen |= 4;
        }
        if (y < 0) {
            screen |= 8;
        }
        if (y >= (unsigned short)sp->screenH) {
            screen |= 0x10;
        }
        if (flags < 0) {
            screen = 0;
        }
        planeOr |= plane;
        planeAnd &= plane;
        screenAnd &= screen;
    }
    return (planeAnd | screenAnd) | planeOr << 16;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Clip_TestPoints);
#endif

// Loads the scratchpad's view into the GTE scaled down so that it fits: the rotation by
// rotShift (how far msb, the translation's top bit from Gte_TransMsb, goes past bits + 2)
// and the translation by bits - rotBits more. Both shifts are kept at 0x98 (the
// translation's is the OT shift) and the scaled transform at 0x78. The clip planes go in
// the light matrices, for Clip_TestPoints and Poly_Clip; the depth cue is off.
#ifdef NON_MATCHING
void *Matrix_ShiftRot(short *dst, MATRIX *src, int shift);

void Gte_LoadScaled(ViewScratch *sp, int msb, int bits, int rotBits)
{
    int rotShift = msb - bits - 2;
    int transShift;
    int *w;

    if (rotShift < 0) {
        rotShift = 0;
    }
    transShift = bits - rotBits + rotShift;
    sp->otShift = transShift;
    sp->rotShift = rotShift;
    sp->savedTrans[0] = sp->view.t[0] >> transShift;
    sp->savedTrans[1] = sp->view.t[1] >> transShift;
    sp->savedTrans[2] = sp->view.t[2] >> transShift;
    Matrix_ShiftRot(sp->savedRot, &sp->view, rotShift);
    gte_SetCtrlReg(GTE_OFX, sp->ofx);
    gte_SetCtrlReg(GTE_OFY, sp->ofy);
    gte_SetCtrlReg(GTE_H, (unsigned short)sp->projH);
    gte_SetCtrlReg(GTE_TRX, sp->savedTrans[0]);
    gte_SetCtrlReg(GTE_TRY, sp->savedTrans[1]);
    gte_SetCtrlReg(GTE_TRZ, sp->savedTrans[2]);
    gte_SetRotMatrixWords(sp->savedRot);
    w = (int *)sp->clipX;
    gte_SetCtrlReg(GTE_L11L12, w[0]);
    gte_SetCtrlReg(GTE_L13L21, w[1]);
    gte_SetCtrlReg(GTE_L22L23, w[2]);
    gte_SetCtrlReg(GTE_L31L32, w[3]);
    gte_SetCtrlReg(GTE_L33, w[4]);
    w = (int *)sp->clipY;
    gte_SetCtrlReg(GTE_LR1LR2, w[0]);
    gte_SetCtrlReg(GTE_LR3LG1, w[1]);
    gte_SetCtrlReg(GTE_LG2LG3, w[2]);
    gte_SetCtrlReg(GTE_LB1LB2, w[3]);
    gte_SetCtrlReg(GTE_LB3, w[4]);
    gte_SetCtrlReg(GTE_DQA, 0x1000);
    gte_SetCtrlReg(GTE_DQB, 0);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gte_LoadScaled);
#endif

// Gte_LoadScaled with the shift that the scratchpad's view (scaled by scale) needs.
void Gte_LoadTransform(ViewScratch *sp, int scale, int bits, int rotBits)
{
    Gte_LoadScaled(sp, Gte_TransMsb(&sp->view, scale << (bits - rotBits)), bits, rotBits);
}

// The scratchpad with Clip_TestBox's corners after the drawing state.
struct BoxScratch {
    ViewScratch view;
    ClipPoint box[8];
};

// Clip_TestPoints on the eight corners of the box from min to max.
unsigned int Clip_TestBox(ViewScratch *sp, short *min, short *max)
{
    BoxScratch *b = (BoxScratch *)sp;
    int x0 = min[0];
    int y0 = min[1];
    int z0 = min[2];
    int x1 = max[0];
    int y1 = max[1];
    int z1 = max[2];

    b->box[0].x = x0;
    b->box[0].y = y0;
    b->box[0].z = z0;
    b->box[1].x = x1;
    b->box[1].y = y0;
    b->box[1].z = z0;
    b->box[2].x = x0;
    b->box[2].y = y1;
    b->box[2].z = z0;
    b->box[3].x = x1;
    b->box[3].y = y1;
    b->box[3].z = z0;
    b->box[4].x = x0;
    b->box[4].y = y0;
    b->box[4].z = z1;
    b->box[5].x = x1;
    b->box[5].y = y0;
    b->box[5].z = z1;
    b->box[6].x = x0;
    b->box[6].y = y1;
    b->box[6].z = z1;
    b->box[7].x = x1;
    b->box[7].y = y1;
    b->box[7].z = z1;
    return Clip_TestPoints(sp, b->box, 8);
}

// dst = a * b, rotations only (the translation is left alone). dst may be a or b.
#ifdef NON_MATCHING
// Handwritten: the original loads all of b's columns into V0-V2 before storing.
MATRIX *Matrix_Mul(MATRIX *dst, MATRIX *a, MATRIX *b)
{
    int i;

    gte_SetRotMatrixWords(a);
    for (i = 0; i < 3; i++) {
        gte_SetDataReg(GTE_VXY0, (b->m[0][i] & 0xffff) | b->m[1][i] << 16);
        gte_SetDataReg(GTE_VZ0, b->m[2][i]);
        gte_mvmva(1, GTE_MX_RT, GTE_V_V0, GTE_CV_NONE, 0);
        dst->m[0][i] = gte_GetDataReg(GTE_IR1);
        dst->m[1][i] = gte_GetDataReg(GTE_IR2);
        dst->m[2][i] = gte_GetDataReg(GTE_IR3);
    }
    return dst;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Mul);
#endif

// Copies rect of VRAM from data (store 0) or to it (store 1), the first words (the
// size mod 16) through GP0 and the 16-word blocks by DMA, which is left running. Returns
// the number of blocks.
#define GPU ((volatile unsigned int *)0x1f801810)
#define DMA ((volatile unsigned int *)0x1f801080)
#ifdef NON_MATCHING
// Two registers swapped (the DMA mode and CHCR values) and the w * h multiply one slot
// later than the game's. Permuter didn't find a form.
int Gpu_TransferImage(RECT *rect, unsigned int *data, int store)
{
    volatile unsigned int *gpu;
    volatile unsigned int *dma;
    unsigned int cmd;
    unsigned int mode;
    unsigned int chcr;
    int x;
    int y;
    int w;
    int h;
    int n;
    int blocks;

    unsigned char *f = &g_GfxIrq.imageTransferBusy;

    f[0] = 1;
    Gpu_DrawSync();
    dma = (volatile unsigned int *)0x1f801080;
    gpu = (volatile unsigned int *)0x1f801810;
    cmd = 0xa0000000;
    mode = 0x4000002;
    chcr = 0x1000201;
    f[1] = 1;
    x = rect->x;
    y = rect->y;
    w = rect->w;
    h = rect->h;
    if (store) {
        cmd = 0xc0000000;
        mode = 0x4000003;
        chcr = 0x1000200;
    }
    gpu[1] = 0x4000000;
    gpu[0] = 0x1000000;
    gpu[0] = cmd;
    gpu[0] = x | y << 16;
    gpu[0] = w | h << 16;
    blocks = w * h + 1 >> 1;
    n = blocks & 0xf;
    blocks >>= 4;
    if (n) {
        unsigned int *p = data;
        if (!store) {
            do {
                *gpu = *p++;
            } while (--n);
        } else {
            do {
                *p++ = *gpu;
            } while (--n);
        }
        data = p;
    }
    if (blocks) {
        gpu[1] = mode;
        dma[8] = (unsigned int)data;
        dma[9] = blocks << 16 | 0x10;
        dma[10] = chcr;
    } else {
        g_GfxIrq.imageDmaPending = 0;
    }
    g_GfxIrq.imageTransferBusy = 0;
    return blocks;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gpu_TransferImage);
#endif

void *Gpu_LoadImageBlock(int *block, RECT *rect)
{
    if (rect == 0) {
        rect = (RECT *)(block + 1);
    }
    Gpu_TransferImage(rect, (unsigned int *)(block + 3), 0);
    Gpu_DrawSync();
    return (void *)((int)block + *block);
}

// Loads tim's CLUT block, if it has one (flag bit 3), to rect or its own rectangle.
// Returns the block after it: the image block.
void *Tim_LoadClut(void *tim, RECT *rect)
{
    int has = *(unsigned int *)((int)tim + 4) >> 3 & 1;
    // The test as the game's xori: !has gives sltiu.
    int none = has ^ 1;

    if (none) {
        return (int *)((int)tim + 8);
    }
    return Gpu_LoadImageBlock((int *)((int)tim + 8), rect);
}

void Tim_LoadImage(void *tim, RECT *rect)
{
    int *block;

    block = (int *)((int)tim + 8);
    if ((*(unsigned int *)((int)tim + 4) >> 3 & 1) != 0) {
        block = (int *)((int)block + *(int *)((int)tim + 8));
    }
    Gpu_LoadImageBlock(block, rect);
}

void Tim_LoadClutAndImage(void *tim, RECT *clutRect, RECT *imageRect)
{
    int *block;

    block = Tim_LoadClut(tim, clutRect);
    Gpu_LoadImageBlock(block, imageRect);
}

// Clips a Gouraud triangle or quad (count points, from the scratchpad at 0x9c) against
// the near plane and the view's side planes, and draws what is left as a strip of
// POLY_G4s ending in a POLY_G3 when the point count is odd, all in the input's OT entry.
// The GTE must hold the identity rotation, no translation and Gte_LoadScaled's planes.
// The points go in a ring of records; each plane a point is outside of is cut in turn
// (Sutherland-Hodgman), a crossing edge getting a new point interpolated in position and
// colour, and the face is dropped once fewer than three points are left. Each record's
// position is scaled to about 13 bits on its own: a cut point is then a multiple of the
// true one, which projects and tests against the side planes (through the eye) the same,
// but the near plane test compares the scaled depth with nearZ.
#ifdef NON_MATCHING
// Poly_Clip's input.
struct PolyClipIn {
    unsigned int *ot;           // 0x9c
    unsigned int code;          // the POLY_G3 or POLY_G4 code, in the top byte
    int pad;
    struct {
        int xy;                 // view-space position
        short z;
        short extent;           // |x| | |y| | |z|
        int sxy;
        unsigned int rgbc;      // the colour (the top byte isn't used)
    } v[4];
};

struct ClipVert {
    short x;                    // the position, scaled
    short y;
    short z;
    short pad;
    int sxy;
    unsigned int rgbc;          // a cut point keeps the record's old top byte
    ClipVert *prev;
    ClipVert *next;
    unsigned short code;        // 0x18: bit 0 nearer than nearZ, bits 1-4 outside a side
    short dist[5];              // the distance to each plane, negative outside
};

// The working state after the input, through 0x2e8.
struct PolyClip {
    ClipVert pool[14];          // 0xe8
    ClipVert *head;             // 0x2e0
    ClipVert *free;             // 0x2e4
};

struct ClipG3 {
    unsigned int tag;
    unsigned int rgbc0;
    int xy0;
    unsigned int rgb1;
    int xy1;
    unsigned int rgb2;
    int xy2;
};

struct ClipG4 {
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

// The shift that brings the top bit of extent to bit 12 (negative to shift left).
static inline int Clip_Shift(int extent)
{
    gte_SetDataReg(GTE_LZCS, extent);
    return 19 - gte_GetDataReg(GTE_LZCR);
}

static inline void ClipVert_SetPos(ClipVert *v, int x, int y, int z, int shift)
{
    if (shift >= 0) {
        v->x = x >> shift;
        v->y = y >> shift;
        v->z = z >> shift;
    } else {
        v->x = x << -shift;
        v->y = y << -shift;
        v->z = z << -shift;
    }
}

// Projects a record (storing its screen position when project is set) and codes it.
static void ClipVert_Classify(ViewScratch *sp, ClipVert *v, int project)
{
    int z = v->z;
    int d1;
    int d2;
    unsigned int code;

    gte_LoadDataReg(GTE_VXY0, &v->x);
    gte_LoadDataReg(GTE_VZ0, &v->z);
    gte_rtps();
    if (project) {
        gte_StoreDataReg(GTE_SXY2, &v->sxy);
    }
    gte_mvmva(1, GTE_MX_LL, GTE_V_V0, GTE_CV_NONE, 0);
    d1 = gte_GetDataReg(GTE_IR1);
    d2 = gte_GetDataReg(GTE_IR2);
    gte_mvmva(1, GTE_MX_LC, GTE_V_V0, GTE_CV_NONE, 0);
    v->dist[0] = z - (unsigned short)sp->nearZ;
    code = (unsigned int)(z - (unsigned short)sp->nearZ) >> 31;
    v->dist[1] = d1;
    if (d1 < 0) {
        code |= 2;
    }
    v->dist[2] = d2;
    if (d2 < 0) {
        code |= 4;
    }
    d1 = gte_GetDataReg(GTE_IR1);
    d2 = gte_GetDataReg(GTE_IR2);
    v->dist[3] = d1;
    if (d1 < 0) {
        code |= 8;
    }
    v->dist[4] = d2;
    if (d2 < 0) {
        code |= 0x10;
    }
    v->code = code;
}

// The point where the edge from in (inside) to out crosses a plane, t (a 4.12 fraction)
// of the way.
static void ClipVert_Cut(ViewScratch *sp, ClipVert *v, ClipVert *in, ClipVert *out, int t)
{
    int x;
    int y;
    int z;
    int ax;
    int ay;
    int az;
    unsigned char *c;

    gte_SetDataReg(GTE_IR0, 0x1000 - t);
    gte_SetDataReg(GTE_IR1, in->x);
    gte_SetDataReg(GTE_IR2, in->y);
    gte_SetDataReg(GTE_IR3, in->z);
    gte_gpf(0);
    gte_SetDataReg(GTE_IR0, t);
    gte_SetDataReg(GTE_IR1, out->x);
    gte_SetDataReg(GTE_IR2, out->y);
    gte_SetDataReg(GTE_IR3, out->z);
    gte_gpl(0);
    x = gte_GetDataReg(GTE_MAC1);
    y = gte_GetDataReg(GTE_MAC2);
    z = gte_GetDataReg(GTE_MAC3);
    ax = x < 0 ? -x : x;
    ay = y < 0 ? -y : y;
    az = z < 0 ? -z : z;
    ClipVert_SetPos(v, x, y, z, Clip_Shift(ax | ay | az));

    c = (unsigned char *)&in->rgbc;
    gte_SetDataReg(GTE_IR0, 0x1000 - t);
    gte_SetDataReg(GTE_IR1, c[0]);
    gte_SetDataReg(GTE_IR2, c[1]);
    gte_SetDataReg(GTE_IR3, c[2]);
    gte_gpf(1);
    c = (unsigned char *)&out->rgbc;
    gte_SetDataReg(GTE_IR0, t);
    gte_SetDataReg(GTE_IR1, c[0]);
    gte_SetDataReg(GTE_IR2, c[1]);
    gte_SetDataReg(GTE_IR3, c[2]);
    gte_gpl(1);
    c = (unsigned char *)&v->rgbc;
    c[0] = gte_GetDataReg(GTE_IR1);
    c[1] = gte_GetDataReg(GTE_IR2);
    c[2] = gte_GetDataReg(GTE_IR3);

    ClipVert_Classify(sp, v, 1);
}

// Handwritten.
void Poly_Clip(ViewScratch *sp, int count)
{
    PolyClipIn *in = (PolyClipIn *)((char *)sp + 0x9c);
    PolyClip *pc = (PolyClip *)((char *)sp + 0xe8);
    ClipVert *v;
    ClipVert *end = &pc->pool[count];
    unsigned int codeOr = 0;
    unsigned int codeAnd = 0xffffffff;
    int bit;
    int i;

    pc->head = pc->pool;
    for (i = 0, v = pc->pool; v != end; i++, v++) {
        int flags;

        v->sxy = in->v[i].sxy;
        v->rgbc = in->v[i].rgbc;
        // The input point is projected (as is) only for the GTE's error flag: the record
        // keeps the input's screen position when that projection failed.
        gte_LoadDataReg(GTE_VXY0, &in->v[i].xy);
        gte_LoadDataReg(GTE_VZ0, &in->v[i].z);
        gte_rtps();
        flags = gte_GetCtrlReg(GTE_FLAG);
        ClipVert_SetPos(v, (short)in->v[i].xy, in->v[i].xy >> 16, in->v[i].z,
                        Clip_Shift(in->v[i].extent));
        ClipVert_Classify(sp, v, flags >= 0);
        v->next = v + 1;
        (v + 1)->prev = v;
        codeOr |= v->code;
        codeAnd &= v->code;
    }
    pc->free = end;
    if (codeAnd != 0) {
        return;
    }
    // The points make a ring, and the remaining records another for the free list.
    pc->head->prev = end - 1;
    (end - 1)->next = pc->head;
    for (v = end; v != &pc->pool[13]; v++) {
        v->next = v + 1;
        (v + 1)->prev = v;
    }
    pc->free->prev = &pc->pool[13];
    pc->pool[13].next = pc->free;

    for (bit = 0; codeOr != 0; bit++) {
        unsigned int mask = 1 << bit;
        ClipVert *start;
        ClipVert *cur;
        ClipVert *tail;
        int n;

        if (!(codeOr & mask)) {
            continue;
        }
        codeOr -= mask;
        start = pc->head;
        cur = start;
        tail = 0;
        n = 0;
        do {
            ClipVert *next = cur->next;
            unsigned int out = cur->code & mask;

            if (!out) {
                cur->prev = tail;
                if (tail) {
                    tail->next = cur;
                } else {
                    pc->head = cur;
                }
                tail = cur;
                n++;
            }
            if (out ^ (next->code & mask)) {
                ClipVert *inside = out ? next : cur;
                ClipVert *outside = out ? cur : next;
                int t = (inside->dist[bit] << 12) / (inside->dist[bit] - outside->dist[bit]);

                v = pc->free;
                pc->free = v->next;
                n++;
                v->prev = tail;
                if (tail) {
                    tail->next = v;
                } else {
                    pc->head = v;
                }
                tail = v;
                ClipVert_Cut(sp, v, inside, outside, t);
            }
            cur = next;
        } while (cur != start);
        // With no point left, the original still stores the head at address 0x14
        // (tail->next with tail 0).
        if (!tail) {
            return;
        }
        tail->next = pc->head;
        pc->head->prev = tail;
        if (n < 3) {
            return;
        }
    }

    // A strip from both ends of the ring: the first point, the last, the second, the one
    // before the last... Each POLY_G4 links to the next packet; the last packet gets the
    // OT entry's old link, and the entry the first packet.
    {
        unsigned int tri = (in->code & 0xf7ffffff) >> 24;
        unsigned int quad = ((in->code & 0xf7ffffff) | 0x08000000) >> 24;
        char *first = sp->packetPtr;
        char *pkt = first;
        unsigned int *last = (unsigned int *)pkt;
        ClipVert *a = pc->head;
        ClipVert *z = a->prev;
        ClipVert *b = a->next;
        ClipVert *y = z->prev;

        while (b != z) {
            last = (unsigned int *)pkt;
            if (b == y) {
                ClipG3 *g = (ClipG3 *)pkt;

                g->tag = 0x06000000;
                g->xy0 = a->sxy;
                g->xy1 = z->sxy;
                g->xy2 = b->sxy;
                g->rgbc0 = a->rgbc;
                g->rgb1 = z->rgbc;
                ((unsigned char *)&g->rgbc0)[3] = tri;
                g->rgb2 = b->rgbc;
                pkt += sizeof(ClipG3);
                break;
            } else {
                ClipG4 *g = (ClipG4 *)pkt;

                g->tag = 0x08000000 | ((unsigned int)(pkt + sizeof(ClipG4)) & 0xffffff);
                g->xy0 = a->sxy;
                g->xy1 = z->sxy;
                g->xy2 = b->sxy;
                g->xy3 = y->sxy;
                g->rgbc0 = a->rgbc;
                g->rgb1 = z->rgbc;
                g->rgb2 = b->rgbc;
                ((unsigned char *)&g->rgbc0)[3] = quad;
                g->rgb3 = y->rgbc;
                pkt += sizeof(ClipG4);
                a = b;
                z = y;
                b = b->next;
                y = y->prev;
            }
        }
        sp->packetPtr = pkt;
        *last = (*last & 0xff000000) | (*in->ot & 0xffffff);
        *in->ot = (*in->ot & 0xff000000) | ((unsigned int)first & 0xffffff);
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Poly_Clip);
#endif
}
