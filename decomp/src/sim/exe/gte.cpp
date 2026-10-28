// Resident code 8007adc8-8007c268 (gte). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"
#include "gte.h"

#ifdef NON_MATCHING
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
#endif

extern "C" {
void Dma_WaitGpu(...);
void Gpu_DrawSync(void);
void *Gpu_LoadImageBlock(int *block, RECT *rect);
int Gpu_TransferImage(RECT *rect, unsigned int *data, int store);
void Gpu_WaitReady(...);
void Matrix_Mul(MATRIX *dst, MATRIX *a, MATRIX *b);
MATRIX *Matrix_Translate(MATRIX *m, VECTOR *v);
void *Tim_LoadClut(void *tim, RECT *rect);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", LodTable_SquareDistances);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", FUN_8007ae08);

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

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Lod_Select);

void Gpu_DrawSync(void)
{
    Gpu_WaitReady();
    Dma_WaitGpu();
    Gpu_WaitReady();
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Identity);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_SetEuler);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Copy);

MATRIX *Matrix_Comp(MATRIX *m, MATRIX *n)
{
    Matrix_Translate(m, (VECTOR *)n->t);
    Matrix_Mul(m, m, n);
    return m;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Translate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_RotEuler);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_RotX);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_RotY);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_RotZ);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Scale);

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

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Camera_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", FUN_8007b5ac);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", FUN_8007b5fc);

// Transforms count points with the loaded view (Gte_LoadScaled) and codes each against
// the view's planes: bit 0 nearer than nearZ, bits 1-4 outside the left, right, top and
// bottom planes, bit 5 a GTE error; and against the screen: bits 1-4 left of, right of,
// above and below it (none when the GTE failed). Returns the AND of the plane codes, ORed
// with the AND of the screen codes, then the OR of the plane codes in bits 16 on. With no
// points that is 0xffffffff.
#ifdef NON_MATCHING
struct ClipPoint {
    short x;
    short y;
    short z;
    short pad;
};

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

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gte_LoadTransform);

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Clip_TestBox);

// dst = a * b, rotations only (the translation is left alone). dst may be a or b.
#ifdef NON_MATCHING
// Handwritten: the original loads all of b's columns into V0-V2 before storing, and also
// returns dst.
void Matrix_Mul(MATRIX *dst, MATRIX *a, MATRIX *b)
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
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Matrix_Mul);
#endif

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Gpu_TransferImage);

void *Gpu_LoadImageBlock(int *block, RECT *rect)
{
    if (rect == 0) {
        rect = (RECT *)(block + 1);
    }
    Gpu_TransferImage(rect, (unsigned int *)(block + 3), 0);
    Gpu_DrawSync();
    return (void *)((int)block + *block);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/gte", Tim_LoadClut);

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
