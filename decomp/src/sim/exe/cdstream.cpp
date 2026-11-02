// Resident code 80081c5c-8008264c (cdstream): the CD sector ring that streamed reads
// fill, the critical section and frame timers, and a few GPU and matrix helpers.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"
#include "gte.h"

extern "C" {
extern CdAudio g_CdAudio;
extern int g_CriticalDepth;
extern void *g_PacketPtr;
extern void *g_HeapBase;
extern unsigned long g_HeapSize;
extern char D_80090894[];
extern char D_800908B0[];
extern char D_800908C4[];
extern char D_800908D8[];

int CdAudio_RunScript(int script);
int CdAudio_RunScript_8007C4F8(CdAudio *cd, int index);
void CdAudio_ClearPending(void *cd);
int CdGetSector_Thunk(void *dst, int words);
int CheckCallback(void);
int EnterCriticalSection(void);
int ExitCriticalSection(void);
int printf(const char *fmt, ...);
void Gpu_Reset(void);
void FUN_80082f74(void *p, unsigned int area0, unsigned int area1, unsigned int offset);
void func_80082EE4(void);
int Sys_CriticalSection(int leave);
int Timer_Sample(void);
void CdStream_AddFillCount(CdAudio *cd, short delta);
unsigned char *CdStream_GetReadPtr(CdAudio *cd);

extern TimerSample D_801C951C;
extern unsigned short D_801C9520;
extern int D_801C9524;
extern int D_801C9528;
extern int D_801C952C;

static inline void CdRing_Reset(void *req)
{
    CdReq *r = (CdReq *)req;

    r->fill = 0;
    r->read = 0;
    r->write = 0;
}

#ifdef NON_MATCHING
// The game loads &g_CdAudio into $a0 before the prologue and copies it to a saved
// register; GCC loads it after (as CdAudio_Init, docs/decomp.md).
void CdAudio_ResetRead(void)
{
    CdAudio_ClearPending(&g_CdAudio);
    g_CdAudio.remaining = g_CdAudio.reqSize;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_ResetRead);
#endif

// 1 while a command is in flight, 2 if the read stopped before its end, else 0.
int CdAudio_GetReadStatus(void)
{
    CdAudio *cd = &g_CdAudio;

    if (cd->state != 0) {
        return 1;
    }
    if (cd->unk167 != 0) {
        return 2;
    }
    return 0;
}

// Links a 13-word packet from g_PacketPtr in after ot and returns its body.
// The 24-bit links are copied with lwl and swl, one asm statement each.
void *FUN_80081cc8(OtTag *ot)
{
    unsigned int *p;
    unsigned int link;

    __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
    p = (unsigned int *)g_PacketPtr;
    ((unsigned char *)p)[3] = 12;
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(p) : "memory");
    __asm__ volatile("swl\t%0,2(%1)" : : "r"((unsigned int)p << 8), "r"(ot) : "memory");
    g_PacketPtr = (unsigned char *)p + 0x34;
    return p + 1;
}

// Enters (leave == 0) or leaves a nested critical section; returns what to pass to
// leave it again (0 inside a callback, where interrupts are already off).
int Sys_CriticalSection(int leave)
{
    int depth;
    int inc;
    int (*fn)(void);

    if (CheckCallback() != 0) {
        return 0;
    }
    depth = g_CriticalDepth;
    inc = 1;
    fn = EnterCriticalSection;
    if (leave != 0) {
        fn = ExitCriticalSection;
        depth--;
        inc = 0;
    }
    g_CriticalDepth = depth + inc;
    if (depth == 0) {
        fn();
    }
    return -1;
}

// Adds the clock ticks since the last sample to D_801C9524 and returns it. The
// clock counter is 16 bits, so the hblanks since the last sample (about 0.63 of
// its rate) say how many times it wrapped.
int Timer_Sample(void)
{
    TimerSample *t = &D_801C951C;
    volatile unsigned short *hblankCounter = (volatile unsigned short *)0x1f801110;
    volatile unsigned short *clockCounter = (volatile unsigned short *)0x1f801120;
    unsigned int hblank;
    unsigned int prevHblank;
    unsigned int clock;
    unsigned int prevClock;
    unsigned int ticks;
    unsigned int expect;
    unsigned int lo;
    unsigned int hi;

    do {
        hblank = *hblankCounter;
    } while (hblank != *hblankCounter);
    prevHblank = t->hblank;
    t->hblank = hblank;
    clock = *clockCounter;
    prevClock = t->clock;
    t->clock = clock;
    if (hblank < prevHblank) {
        hblank += 0xffff;
    }
    hblank -= prevHblank;
    t->hblankDelta = hblank;
    expect = (hblank * 41339) >> 16;
    lo = 0;
    if (expect >= 0x869) {
        lo = expect - 0x868;
    }
    hi = expect + 0x868;
    if (clock < prevClock) {
        clock += 0xffff;
    }
    ticks = clock - prevClock;
    while (ticks < lo) {
        ticks += 0xffff;
    }
    while (ticks > hi && ticks > 0xfffe) {
        ticks -= 0xffff;
    }
    D_801C9524 += ticks;
    return D_801C9524;
}

int Timer_StartFrame(void)
{
    int t = Timer_Sample();

    D_801C9528 = t;
    D_801C952C = D_801C9520;
    return t;
}

int Timer_SinceFrameStart(void)
{
    int iVar1;

    iVar1 = Timer_Sample();
    return iVar1 - D_801C9528;
}

int Timer_HblanksSinceFrameStart(void)
{
    unsigned int uVar1;

    uVar1 = (unsigned int)D_801C9520;
    if ((int)uVar1 < D_801C952C) {
        uVar1 = uVar1 + 0xffff;
    }
    return uVar1 - D_801C952C;
}

int FUN_80081efc(void)
{
    int iVar1;
    int iVar2;

    iVar1 = D_801C9524;
    iVar2 = Timer_Sample();
    return iVar2 - iVar1;
}

void Gpu_SetDisplayStart(int x, int y)
{
    volatile int *gpu = (volatile int *)0x1f801810;

    gpu[1] = y << 10 | 0x05000000 | x;
}

void CdAudio_StartRead(void)
{
    int iVar1;

    do {
        iVar1 = CdAudio_RunScript(7);
    } while (iVar1 == 0);
}

#ifdef NON_MATCHING
// The game loads &g_CdAudio into $a0 before the prologue and copies it to a saved
// register; GCC loads it after (as CdAudio_Init, docs/decomp.md).
void CdAudio_ResetStream(void)
{
    CdAudio_ClearPending(&g_CdAudio);
    CdRing_Reset(&g_CdAudio.flag);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_ResetStream);
#endif

void CdStream_AddFillCount(CdAudio *cd, short delta)
{
    int leave;

    leave = Sys_CriticalSection(0);
    cd->ringFill = cd->ringFill + delta;
    Sys_CriticalSection(leave);
}

#ifdef NON_MATCHING
// The game's version has no return: it loads &g_CdAudio and falls through into
// CdStream_GetReadPtr, the next function.
unsigned char *FUN_80081ff8(void)
{
    return CdStream_GetReadPtr(&g_CdAudio);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", FUN_80081ff8);
#endif

unsigned char *CdStream_GetReadPtr(CdAudio *cd)
{
    return (unsigned char *)((int)cd->dst + (unsigned int)cd->ringRead * 0x800);
}

#ifdef NON_MATCHING
// The game loads &g_CdAudio into $a0 before the prologue; GCC loads it after.
void CdStream_ReleaseSector(void)
{
    CdAudio *cd = &g_CdAudio;

    int read;

    read = cd->ringRead + 1;
    cd->ringRead = read;
    if (read == cd->remaining) {
        cd->ringRead = 0;
    }
    CdStream_AddFillCount(cd, -1);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdStream_ReleaseSector);
#endif

// Waits for a sector in the ring, restarting the read (script 8) while the ring
// isn't full and the drive hasn't stalled, and returns it.
#ifdef NON_MATCHING
// The game loads &g_CdAudio into $a0 before the prologue and copies it to a saved
// register; GCC loads it after (as CdAudio_Init, docs/decomp.md).
unsigned char *CdStream_WaitSector(void)
{
    volatile CdAudio *cd = &g_CdAudio;
    int leave;
    int stall;
    int fill;
    unsigned int size;

    do {
        leave = Sys_CriticalSection(0);
        stall = cd->state;
        fill = cd->ringFill;
        size = cd->remaining;
        cd->curSector;
        cd->endSector;
        if (fill != size && stall == 0) {
            CdAudio_RunScript_8007C4F8((CdAudio *)cd, 8);
        }
        Sys_CriticalSection(leave);
    } while (fill == 0);
    return CdStream_GetReadPtr((CdAudio *)cd);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdStream_WaitSector);
#endif

// Returns the bytes between the stack pointer and the end of the heap, printing
// the heap and stack bounds when they overlap.
#ifdef NON_MATCHING
// The game copies $sp to $s3 after the register saves; the asm here copies it two
// instructions later.
int Stack_Check(void)
{
    unsigned char *heap = (unsigned char *)g_HeapBase;
    unsigned char *heapEnd = heap + g_HeapSize;
    unsigned char *stack;
    int free;

    asm volatile("move %0,$sp" : "=r"(stack));
    free = stack - heapEnd;

    if (free < 0) {
        printf(D_80090894, -free);
        printf(D_800908B0, heap);
        printf(D_800908C4, heapEnd);
        printf(D_800908D8, stack);
    }
    return free;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Stack_Check);
#endif

void CdAudio_SetupStream(void *buf, int bufSectors, int start, unsigned int end)
{
    int mode = 0x80;

    g_CdAudio.reqMode = mode;
    g_CdAudio.reqStart = start;
    g_CdAudio.reqEnd = end;
    g_CdAudio.onSector = (void *)func_80082EE4;
    g_CdAudio.reqDst = buf;
    g_CdAudio.reqSize = bufSectors;
    CdRing_Reset(&g_CdAudio.reqFlag);
}

// Waits for the GPU to take a command, resetting it after about 3 million polls.
void Gpu_WaitReady(void)
{
    volatile int *gpu = (volatile int *)0x1f801810;
    int n = 0x2efb44;

    do {
        if (gpu[1] & 0x04000000) {
            return;
        }
    } while (--n != 0);
    Gpu_Reset();
}

// dst = m * v >> 12 + add, with v's components taken as 12-bit parts so that large
// vectors keep their precision (m's translation isn't used).
#ifdef NON_MATCHING
// The part of a vector's components selected by shift (0 and 12: 12 bits, 24: the rest,
// signed) in a GTE vector register pair.
#define VECTOR_PART_XY(v, shift, mask) \
    ((((v)->vx >> (shift)) & (mask)) | ((v)->vy >> (shift) & (mask)) << 16)

// Handwritten. The top part's product goes through IR1-IR3, which saturate at 16 bits:
// components past about 2^27 give wrong results.
static inline void Matrix_ApplyParts(MATRIX *m, VECTOR *v, int *out)
{
    int lo[3];
    int hi[3];

    gte_SetRotMatrixWords(m);
    gte_SetDataReg(GTE_VXY0, VECTOR_PART_XY(v, 0, 0xfff));
    gte_SetDataReg(GTE_VZ0, v->vz & 0xfff);
    gte_mvmva(1, GTE_MX_RT, GTE_V_V0, GTE_CV_NONE, 0);
    lo[0] = gte_GetDataReg(GTE_IR1);
    lo[1] = gte_GetDataReg(GTE_IR2);
    lo[2] = gte_GetDataReg(GTE_IR3);
    gte_SetDataReg(GTE_VXY2, (v->vx >> 24 & 0xffff) | v->vy >> 24 << 16);
    gte_SetDataReg(GTE_VZ2, v->vz >> 24);
    gte_mvmva(0, GTE_MX_RT, GTE_V_V2, GTE_CV_NONE, 0);
    hi[0] = gte_GetDataReg(GTE_IR1);
    hi[1] = gte_GetDataReg(GTE_IR2);
    hi[2] = gte_GetDataReg(GTE_IR3);
    gte_SetDataReg(GTE_VXY1, VECTOR_PART_XY(v, 12, 0xfff));
    gte_SetDataReg(GTE_VZ1, v->vz >> 12 & 0xfff);
    gte_mvmva(0, GTE_MX_RT, GTE_V_V1, GTE_CV_NONE, 0);
    out[0] = lo[0] + (hi[0] << 12) + gte_GetDataReg(GTE_MAC1);
    out[1] = lo[1] + (hi[1] << 12) + gte_GetDataReg(GTE_MAC2);
    out[2] = lo[2] + (hi[2] << 12) + gte_GetDataReg(GTE_MAC3);
}

VECTOR *Matrix_ApplyAdd(VECTOR *dst, MATRIX *m, VECTOR *v, VECTOR *add)
{
    int r[3];

    Matrix_ApplyParts(m, v, r);
    dst->vx = r[0] + add->vx;
    dst->vy = r[1] + add->vy;
    dst->vz = r[2] + add->vz;
    return dst;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ApplyAdd);
#endif

// dst = src's rotation transposed. In place (dst == src), each element is stored
// back where it was, so the matrix is left as it is.
void Matrix_Transpose(MATRIX *dst, MATRIX *src)
{
    int a;
    int b;
    int c;

    if (src != dst) {
        a = src->m[0][0];
        b = src->m[1][0];
        c = src->m[2][0];
        dst->m[0][0] = a;
        dst->m[0][1] = b;
        dst->m[0][2] = c;
        a = src->m[0][1];
        b = src->m[1][1];
        c = src->m[2][1];
        dst->m[1][0] = a;
        dst->m[1][1] = b;
        dst->m[1][2] = c;
        a = src->m[0][2];
        b = src->m[1][2];
        c = src->m[2][2];
        dst->m[2][0] = a;
        dst->m[2][1] = b;
        dst->m[2][2] = c;
        return;
    }
    b = src->m[0][1];
    a = src->m[1][0];
    src->m[0][1] = b;
    src->m[1][0] = a;
    b = src->m[0][2];
    a = src->m[2][0];
    src->m[0][2] = b;
    src->m[2][0] = a;
    b = src->m[1][2];
    a = src->m[2][1];
    src->m[1][2] = b;
    src->m[2][1] = a;

}

// dst = m * v >> 12, as Matrix_ApplyAdd.
#ifdef NON_MATCHING
VECTOR *Matrix_ApplyLV(VECTOR *dst, MATRIX *m, VECTOR *v)
{
    int r[3];

    Matrix_ApplyParts(m, v, r);
    dst->vx = r[0];
    dst->vy = r[1];
    dst->vz = r[2];
    return dst;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ApplyLV);
#endif

// Sets the drawing area to r and the drawing offset to its corner (GP0 E3-E5).
#ifdef NON_MATCHING
// The registers and the order of the loads and ors differ (permuter didn't find it).
void FUN_800824ac(void *p, RECT *r)
{
    int w = r->w;
    int x = r->x;
    int h = r->h;
    int y = r->y;
    int x1 = x + w - 1;
    int y1 = y + h - 1;

    FUN_80082f74(p, 0xe3000000 | (y & 0x3ff) << 10 | (x & 0x3ff), 0xe4000000 | (y1 & 0x3ff) << 10 | (x1 & 0x3ff),
                 0xe5000000 | (y & 0x7ff) << 11 | (x & 0x7ff));
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", FUN_800824ac);
#endif

// dst = src's rotation with each element shifted right by shift.
#ifdef NON_MATCHING
// The game walks src with a pointer up to its end and loads a row before storing
// it; GCC keeps a counter (permuter didn't find a form that matches).
short *Matrix_ShiftRot(short *dst, MATRIX *src, int shift)
{
    int i;

    for (i = 0; i < 3; i++) {
        dst[i * 3 + 0] = src->m[i][0] >> shift;
        dst[i * 3 + 1] = src->m[i][1] >> shift;
        dst[i * 3 + 2] = src->m[i][2] >> shift;
    }
    return dst;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", Matrix_ShiftRot);
#endif

// The data read's sector callback: copies sector's data, past the bytes to skip,
// to dst. Returns 8 for a sector it didn't expect, 2 after the last one, else 0.
#ifdef NON_MATCHING
// The game loads &g_CdAudio into $a0 first, moving the arguments up to $a1 and $a2
// (as CdAudio_OnSync); GCC keeps them where they are.
int CdAudio_OnReadSector(unsigned int sector, int unused)
{
    CdAudio *cd = &g_CdAudio;
    unsigned int buf[0x200];
    int ret;
    int skip;
    unsigned char *dst;
    unsigned int left;
    unsigned int n;

    ret = 8;
    cd->curSector = cd->nextSector;
    skip = cd->skip;
    if (sector == cd->nextSector) {
        dst = (unsigned char *)cd->dst;
        left = cd->remaining;
        n = 0x800 - skip;
        if (left < n) {
            n = left;
        }
        left -= n;
        ret = 0;
        cd->dst = dst + 0x800;
        cd->nextSector = sector + 1;
        cd->skip = 0;
        cd->remaining = left;
        if (skip >> 2 != 0) {
            CdGetSector_Thunk(buf, skip >> 2);
        }
        if (n != 0) {
            if (left == 0) {
                cd->unk167 = 0;
                ret = 2;
            }
            CdGetSector_Thunk(dst, (n + 3) >> 2);
        }
    }
    return ret;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdstream", CdAudio_OnReadSector);
#endif
}
