// Resident code 800810f8-80081c5c (math): direct GPU packets, vectors and matrices through
// the GTE, integer square root and arctangent, and a polygon clipper.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gte.h"

extern "C" {
extern unsigned int *g_PacketPtr;
extern short g_SinTable[];
extern short g_CosTable[];
extern short g_SqrtOneMinusSqTable[];
extern unsigned short D_800A2AA4[];     // a ROM row nibble, spread into 12 bits

// A VRAM to VRAM copy: packet[1] is the source, [2] the destination, [3] the size.
#ifdef NON_MATCHING
// A tail call (j Gpu_SendDirect).
void Gpu_MoveImage(unsigned int *packet)
{
    packet[0] = 0x80000000;
    Gpu_SendDirect(packet, 4);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Gpu_MoveImage);
#endif

// Sends a primitive (its length is in the tag's top byte) straight to the GPU.
#ifdef NON_MATCHING
// Falls through into Gpu_SendDirect.
void func_80081108(unsigned char *prim)
{
    Gpu_SendDirect((unsigned int *)(prim + 4), prim[3]);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", func_80081108);
#endif

// Waits for the GPU, resets its command buffer and writes count words to GP0.
void Gpu_SendDirect(unsigned int *cmds, int count)
{
    unsigned int *gpu;

    Gpu_DrawSync();
    gpu = (unsigned int *)0x1f801810;
    gpu[1] = 0x04000000;
    do {
        gpu[0] = *cmds++;
        count--;
    } while (count != 0);
}

// Normalizes a 32-bit vector into 4.12; returns its length.
int Vector_NormalizeToS(SVECTOR *out, VECTOR *in)
{
    SVECTOR v;
    int shift = Vector_ToShort(&v, in);
    int length = SVector_Normalize(out, &v);
    int result = length << shift;

    if (shift < 0) {
        result = length >> -shift;
    }
    return result;
}

// The angles of a rotation matrix (4096 to the turn): out[1] is the pitch from m[1][2].
// Unless the matching cosine is 0, out[2] comes from row 1 and out[0] from column 2;
// otherwise out[2] is 0 and out[0] comes from column 1.
short *Matrix_ToEuler(short *out, MATRIX *m)
{
    int c;
    int z;
    short y;
    short x;

    out[1] = Math_Asin(m->m[1][2]);
    c = m->m[2][1];
    if (c < 0) {
        c = -c;
    }
    if (c < 0x1000) {
        c = g_SqrtOneMinusSqTable[c];
    } else {
        c = 0;
    }
    if (c == 0) {
        z = 0;
        y = -m->m[0][1];
        x = -m->m[2][1];
    } else {
        z = Math_Atan2Short(m->m[1][0], m->m[1][1]);
        y = m->m[0][2];
        x = m->m[2][2];
    }
    out[0] = Math_Atan2Short(y, x);
    out[2] = z;
    return out;
}

// The integer square root of x (taken as unsigned), with fracBits extra fractional bits.
#ifdef NON_MATCHING
// Handwritten.
int Math_ISqrt(int x, int fracBits)
{
    unsigned int bits = x;
    int digits;
    int root;
    int twice;
    int rem;
    int t;

    if (x == 0) {
        return 0;
    }
    digits = 16;
    if (x >= 0) {
        digits = (((31 - gte_Lzc(x)) & ~1) + 2) >> 1;
    }
    root = 0;
    twice = 0;
    rem = 0;
    bits <<= 32 - digits - digits;
    do {
        root <<= 1;
        rem = (rem << 2) | (bits >> 30);
        twice <<= 1;
        t = twice + 1;
        rem -= t;
        bits <<= 2;
        if (rem < 0) {
            rem += t;
        } else {
            root++;
            twice += 2;
        }
    } while (--digits != 0);
    while (fracBits != 0) {
        fracBits--;
        rem <<= 2;
        twice <<= 1;
        t = twice + 1;
        rem -= t;
        root <<= 1;
        if (rem < 0) {
            rem += t;
        } else {
            root++;
            twice += 2;
        }
    }
    return root;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Math_ISqrt);
#endif

// Builds a rotation matrix from three angles: the product of the first two is worked out
// by hand and loaded into the GTE, which then rotates the third angle's two axes.
#ifdef NON_MATCHING
// Handwritten. The cosine and -sine loaded as R11 and R31 are whole words, so a negative
// one also sets R12 or R32 to -1, as in the game.
MATRIX *Matrix_FromEuler(MATRIX *m, int x, int y, int z)
{
    int sinX = g_SinTable[x & 0xfff];
    int cosX = g_CosTable[x & 0xfff];
    int sinY = g_SinTable[y & 0xfff];
    int cosY = g_CosTable[y & 0xfff];
    int sinZ = g_SinTable[z & 0xfff];
    int cosZ = g_CosTable[z & 0xfff];

    gte_SetCtrlReg(GTE_R11R12, cosX);
    gte_SetCtrlReg(GTE_R13R21, (sinX * -sinY >> 12) & 0xffff);
    gte_SetCtrlReg(GTE_R22R23, cosY << 16);
    gte_SetCtrlReg(GTE_R31R32, -sinX);
    gte_SetCtrlReg(GTE_R33, cosX * -sinY >> 12);
    gte_SetDataReg(GTE_VXY0, cosZ & 0xffff);
    gte_SetDataReg(GTE_VZ0, sinZ);
    gte_SetDataReg(GTE_VXY1, -sinZ & 0xffff);
    gte_SetDataReg(GTE_VZ1, cosZ);
    gte_mvmva(1, GTE_MX_RT, GTE_V_V0, GTE_CV_NONE, 0);
    m->m[0][2] = sinX * cosY >> 12;
    m->m[0][0] = gte_GetDataReg(GTE_IR1);
    m->m[1][0] = gte_GetDataReg(GTE_IR2);
    m->m[2][0] = gte_GetDataReg(GTE_IR3);
    gte_mvmva(1, GTE_MX_RT, GTE_V_V1, GTE_CV_NONE, 0);
    m->m[1][2] = sinY;
    m->m[2][2] = cosX * cosY >> 12;
    m->m[0][1] = gte_GetDataReg(GTE_IR1);
    m->m[1][1] = gte_GetDataReg(GTE_IR2);
    m->m[2][1] = gte_GetDataReg(GTE_IR3);
    return m;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Matrix_FromEuler);
#endif

// Adds a SPRT after ot and returns its position.
PrimSprite *Prim_AddSprite(void *ot, unsigned int rgb)
{
    unsigned int *p;
    unsigned int link;

    __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
    p = g_PacketPtr;
    ((unsigned char *)p)[3] = 4;
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(p) : "memory");
    __asm__ volatile("swl\t%0,2(%1)" : : "r"((unsigned int)p << 8), "r"(ot) : "memory");
    p[1] = rgb ^ 0x64000000;
    g_PacketPtr = p + 5;
    return (PrimSprite *)(p + 2);
}

// out = m * v, with no shift and no translation.
#ifdef NON_MATCHING
// Handwritten.
VECTOR *Matrix_RotateVector(VECTOR *out, MATRIX *m, SVECTOR *v)
{
    gte_SetRotMatrixWords(m);
    gte_LoadDataReg(GTE_VXY0, &v->vx);
    gte_LoadDataReg(GTE_VZ0, &v->vz);
    gte_mvmva(0, GTE_MX_RT, GTE_V_V0, GTE_CV_NONE, 0);
    gte_StoreDataReg(GTE_MAC1, &out->vx);
    gte_StoreDataReg(GTE_MAC2, &out->vy);
    gte_StoreDataReg(GTE_MAC3, &out->vz);
    return out;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Matrix_RotateVector);
#endif

// a . b >> 12: a is loaded as the rotation's first row, and the result is clamped to
// 16 bits.
#ifdef NON_MATCHING
// Handwritten.
int SVector_Dot(SVECTOR *a, SVECTOR *b)
{
    gte_SetCtrlReg(GTE_R11R12, ((int *)a)[0]);
    gte_SetCtrlReg(GTE_R13R21, ((int *)a)[1]);
    gte_SetDataReg(GTE_VXY0, ((int *)b)[0]);
    gte_SetDataReg(GTE_VZ0, ((int *)b)[1]);
    gte_mvmva(1, GTE_MX_RT, GTE_V_V0, GTE_CV_NONE, 0);
    return gte_GetDataReg(GTE_IR1);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", SVector_Dot);
#endif

// Expands a 15-bit colour to 0xBBGGRR.
#ifdef NON_MATCHING
// The game shifts each field down to bit 0 first; GCC folds that shift into the two after it.
unsigned int Rgb555ToRgb24(unsigned int c)
{
    unsigned int r = c & 0x1f;
    unsigned int g = (c & 0x3e0) >> 5;
    unsigned int b = (c & 0x7c00) >> 10;

    r = r << 3 | r >> 2;
    g = g << 3 | g >> 2;
    b = b << 3 | b >> 2;
    return r | g << 8 | b << 16;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Rgb555ToRgb24);
#endif

// Clips a flat triangle or quad as Poly_Clip does (count points, from the scratchpad at
// 0x9c) but with no colours: what is left is a strip of POLY_F4s ending in a POLY_F3 when
// the point count is odd, all in the input's colour. A point keeps its input screen
// position unless projecting the input as it is sets the GTE's error flag. For the course
// and its objects' meshes.
#ifdef NON_MATCHING
// Handwritten.

// The two packets.
struct ClipPolyF3 {
    unsigned int tag;
    unsigned int rgbc;
    int xy0, xy1, xy2;
};

struct ClipPolyF4 {
    unsigned int tag;
    unsigned int rgbc;
    int xy0, xy1, xy2, xy3;
};

// The shift that brings the top bit of extent to bit 12 (negative to shift left).
static inline int Clip_Shift(int extent)
{
    return 19 - gte_Lzc(extent);
}

// Stores a point scaled by shift and returns its scaled depth.
static inline int ClipVert_SetPos(ClipVert *v, int x, int y, int z, int shift)
{
    if (shift >= 0) {
        x >>= shift;
        y >>= shift;
        z >>= shift;
    } else {
        x <<= -shift;
        y <<= -shift;
        z <<= -shift;
    }
    v->x = x;
    v->y = y;
    v->z = z;
    return z;
}

// Codes the point in the GTE's V0, of depth z.
static void ClipVert_Classify(ViewScratch *sp, ClipVert *v, int z)
{
    int near = z - (unsigned short)sp->nearZ;
    int d1;
    int d2;
    int d3;
    int d4;
    unsigned int code;

    gte_mvmva(1, GTE_MX_LL, GTE_V_V0, GTE_CV_NONE, 0);
    d1 = gte_GetDataReg(GTE_IR1);
    d2 = gte_GetDataReg(GTE_IR2);
    gte_mvmva(1, GTE_MX_LC, GTE_V_V0, GTE_CV_NONE, 0);
    d3 = gte_GetDataReg(GTE_IR1);
    d4 = gte_GetDataReg(GTE_IR2);
    code = (unsigned int)near >> 31;
    v->dist[0] = near;
    v->dist[1] = d1;
    if (d1 < 0) {
        code |= 2;
    }
    v->dist[2] = d2;
    if (d2 < 0) {
        code |= 4;
    }
    v->dist[3] = d3;
    if (d3 < 0) {
        code |= 8;
    }
    v->dist[4] = d4;
    if (d4 < 0) {
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
    z = ClipVert_SetPos(v, x, y, z, Clip_Shift(ax | ay | az));
    gte_LoadDataReg(GTE_VXY0, &v->x);
    gte_LoadDataReg(GTE_VZ0, &v->z);
    gte_rtps();
    gte_StoreDataReg(GTE_SXY2, &v->sxy);
    ClipVert_Classify(sp, v, z);
}

void Poly_ClipLit(ViewScratch *sp, int count)
{
    PolyClipIn *in = (PolyClipIn *)((char *)sp + 0x9c);
    PolyClip *pc = (PolyClip *)((char *)sp + 0xe8);
    ClipVert *v = pc->pool;
    ClipVert *end = &pc->pool[count];
    unsigned int codeOr = 0;
    unsigned int codeAnd = 0xffffffff;
    int bit;
    int i = 0;

    pc->head = pc->pool;
    do {
        int shift;
        int flags;
        int z;

        v->sxy = in->v[i].sxy;
        // The input point is projected as it is, for the GTE's error flag.
        shift = Clip_Shift(in->v[i].extent);
        gte_LoadDataReg(GTE_VXY0, &in->v[i].xy);
        gte_LoadDataReg(GTE_VZ0, &in->v[i].z);
        gte_rtps();
        z = ClipVert_SetPos(v, (short)in->v[i].xy, in->v[i].xy >> 16, in->v[i].z, shift);
        gte_LoadDataReg(GTE_VXY0, &v->x);
        gte_LoadDataReg(GTE_VZ0, &v->z);
        flags = gte_GetCtrlReg(GTE_FLAG);
        gte_rtps();
        if (flags < 0) {
            gte_StoreDataReg(GTE_SXY2, &v->sxy);
        }
        ClipVert_Classify(sp, v, z);
        (v + 1)->prev = v;
        v->next = v + 1;
        codeOr |= v->code;
        codeAnd &= v->code;
        v++;
        i++;
    } while (v != end);
    pc->free = end;
    if (codeAnd != 0) {
        return;
    }
    // The points make a ring, and the remaining records another for the free list.
    pc->head->prev = end - 1;
    (end - 1)->next = pc->head;
    v = end;
    do {
        (v + 1)->prev = v;
        v->next = v + 1;
        v++;
    } while (v != &pc->pool[13]);
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
                n++;
                pc->free = v->next;
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
        // With no point left, the game still stores the head at address 0x14 (tail->next
        // with tail 0).
        if (!tail) {
            return;
        }
        tail->next = pc->head;
        pc->head->prev = tail;
        if (n < 3) {
            return;
        }
    }

    // A strip from both ends of the ring, as in Poly_Clip. Each POLY_F4 links to the next
    // packet; the last packet gets the OT entry's old link, and the entry the first packet.
    {
        unsigned int tri = in->code & 0xf7ffffff;
        unsigned int quad = tri | 0x08000000;
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
                ClipPolyF3 *f = (ClipPolyF3 *)pkt;

                f->tag = 0x04000000;
                f->xy0 = a->sxy;
                f->xy1 = z->sxy;
                f->xy2 = b->sxy;
                f->rgbc = tri;
                pkt += sizeof(ClipPolyF3);
                break;
            } else {
                ClipPolyF4 *f = (ClipPolyF4 *)pkt;

                f->tag = 0x05000000 | ((unsigned int)(pkt + sizeof(ClipPolyF4)) & 0xffffff);
                f->xy0 = a->sxy;
                f->xy1 = z->sxy;
                f->xy2 = b->sxy;
                f->xy3 = y->sxy;
                f->rgbc = quad;
                pkt += sizeof(ClipPolyF4);
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
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Poly_ClipLit);
#endif

// Copies a rotation with every element shifted left by shift. Returns out.
#ifdef NON_MATCHING
// Scheduling only: the game stores each element right after its shift and steps m after the
// stores. (A do-while gets the elements their own pointers, so the loop is a goto.)
short *Matrix_ShiftCopy(short *out, short *m, int shift)
{
    short *p = out;
    short *end = m + 9;
    int x;
    int y;
    int z;

loop:
    x = m[0];
    y = m[1];
    z = m[2];
    p[0] = x << shift;
    p[1] = y << shift;
    p[2] = z << shift;
    m += 3;
    p += 3;
    if (m != end) {
        goto loop;
    }
    return out;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Matrix_ShiftCopy);
#endif

// The dot product of an int vector and a 4.12 short vector, >> 12 (64 bits inside).
#ifdef NON_MATCHING
// The game keeps the 64-bit sum in two registers with no spill, adding the low words first;
// GCC adds the high word before the carry and needs a stack slot for the third product.
int Vec_DotFix12(int *v, short *n)
{
    int a = n[0];
    long long sum = (long long)a * v[0];

    a = n[1];
    sum += (long long)a * v[1];
    a = n[2];
    sum += (long long)a * v[2];
    return sum >> 12;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Vec_DotFix12);
#endif

// Scales y and x down to 18 bits and returns Math_Atan2Short(y, x): 4096 to the turn.
#ifdef NON_MATCHING
// Handwritten.
int Math_Atan2(int y, int x)
{
    int shift = 18 - gte_Lzc((x ^ (x >> 31)) | (y ^ (y >> 31)));

    if (shift > 0) {
        x >>= shift;
        y >>= shift;
    }
    return (short)Math_Atan2Short(y, x);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", Math_Atan2);
#endif

// Renders a glyph's 15 ROM rows into its cell, each nibble of a row spread through a
// table into 8 bytes of pixels, then an empty row.
#ifdef NON_MATCHING
// Registers only: the game loads the code into $a0 and the flag into $v0, and the table's
// address through $v1; GCC uses $v0 and $v1 and then $v0.
void func_80081B5C(KanjiFont *font, KanjiGlyph *glyph)
{
    int code = glyph->code & 0x7f7f;
    unsigned short *src;
    unsigned short *end;
    unsigned char *dst;
    int stride;

    if (!glyph->rendered) {
        src = KanjiFont_Krom2RawAdd(code);
        dst = KanjiFont_GetPixelPtr(font, (unsigned short)glyph->x, (unsigned short)glyph->y);
        end = src + 15;
        stride = font->cols << 3;
        do {
            unsigned int bits = *src++;
            unsigned int a = D_800A2AA4[bits & 0xf];
            unsigned int b = D_800A2AA4[bits >> 4 & 0xf];
            unsigned int c = D_800A2AA4[bits >> 8 & 0xf];
            unsigned int d = D_800A2AA4[bits >> 12];

            ((unsigned int *)dst)[0] = a << 12 | b | d << 24;
            ((unsigned int *)dst)[1] = c << 4 | d >> 8;
            dst += stride;
        } while (src != end);
        ((unsigned int *)dst)[0] = 0;
        ((unsigned int *)dst)[1] = 0;
        glyph->rendered = 1;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/math", func_80081B5C);
#endif

}
