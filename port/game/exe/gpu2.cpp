// Resident code 80082ba8-80082fac (gpu2): GPU and ordering table DMA, more vector and
// arctangent maths, a sector callback and a three-word packet.
#include "common.h"
#include "gte.h"

extern "C" {
extern CdAudio g_CdAudio;
extern unsigned int *g_PacketPtr;
extern short g_AsinTable[];
extern short g_AtanTable[];
extern signed char g_AtanOctantSign[];
extern short g_AtanOctantBase[];

// Waits for the GPU, then starts its DMA down the linked list at list.
void Gpu_DrawOTag(void *list)
{
    Gpu_DrawSync();
    Hw_Gp1(0x04000002);
    Hw_GpuDrawList(list);
    Rt_DmaDone(2);
}

// Clears an ordering table of n entries by OTC DMA, each linked to the one before.
void Dma_ClearOTagR(void *ot, int n)
{
    unsigned int *entry = (unsigned int *)ot + n - 1;

    while (--n != 0) {
        *entry = (unsigned int)(entry - 1) & 0xffffff;
        entry--;
    }
    *entry = 0xffffff;
}

// Acknowledges the GPU's interrupt and resets its command buffer.
void Gpu_ResetCommandBuffer()
{
    Hw_Gp1(0x02000000);
    Hw_Gp1(0x01000000);
}

// Shifts a 32-bit vector to fit 13 bits; returns the shift (negative to the left).
// Handwritten.
int Vector_ToShort(SVECTOR *out, VECTOR *in)
{
    int x = in->vx;
    int y = in->vy;
    int z = in->vz;
    int ax = x < 0 ? -x : x;
    int ay = y < 0 ? -y : y;
    int az = z < 0 ? -z : z;
    int shift = 19 - gte_Lzc(ax | ay | az);

    if (shift >= 0) {
        x >>= shift;
        y >>= shift;
        z >>= shift;
    } else {
        x <<= -shift;
        y <<= -shift;
        z <<= -shift;
    }
    out->vx = x;
    out->vy = y;
    out->vz = z;
    return shift;
}

// Normalizes in to length 0x1000; returns its length (a zero vector stays as it is).
// Handwritten.
int SVector_Normalize(SVECTOR *out, SVECTOR *in)
{
    int x = in->vx;
    int y = in->vy;
    int z = in->vz;
    int length = 0;

    gte_SetDataReg(GTE_IR1, x);
    gte_SetDataReg(GTE_IR2, y);
    gte_SetDataReg(GTE_IR3, z);
    gte_sqr(0);
    if ((x | y | z) != 0) {
        int sum = gte_GetDataReg(GTE_MAC1);

        sum += gte_GetDataReg(GTE_MAC2);
        sum += gte_GetDataReg(GTE_MAC3);
        length = Math_ISqrt(sum, 0);
        gte_SetDataReg(GTE_IR1, in->vx);
        gte_SetDataReg(GTE_IR2, in->vy);
        gte_SetDataReg(GTE_IR3, in->vz);
        gte_SetDataReg(GTE_IR0, 0x1000000 / length);
        gte_gpf(1);
        x = gte_GetDataReg(GTE_IR1);
        y = gte_GetDataReg(GTE_IR2);
        z = gte_GetDataReg(GTE_IR3);
    }
    out->vx = x;
    out->vy = y;
    out->vz = z;
    return length;
}

// The arc sine of x (4.12), 4096 to the turn; +-0x400 beyond +-1.
int Math_Asin(short x)
{
    if (x < -0xfff) {
        return -0x400;
    }
    if (x >= 0x1000) {
        return 0x400;
    }
    if (x < 0) {
        return (short)-g_AsinTable[-x];
    }
    return g_AsinTable[x];
}

// A table atan2, 4096 to the turn, 0 for (0, 0): the octant from the signs and which of
// |y| and |x| is bigger, then the table at the smaller over the bigger, in 4.12.
// Not compiler output as it stands: the game's mflo comes after the octant's two branches,
// far from its div.
int Math_Atan2Short(short y, short x)
{
    int iy = y;
    int ix = x;
    int ax;
    int ay;
    int den;
    int q;
    int octant;
    int base;
    int sign;

    if ((ix | iy) == 0) {
        return 0;
    }
    ax = ix;
    if (ax < 0) {
        ax = -ax;
    }
    ay = iy;
    if (ay < 0) {
        ay = -ay;
    }
    if (ax >= ay) {
        q = ay << 12;
        den = ax;
    } else {
        q = ax << 12;
        den = ay;
    }
    q /= den;
    octant = (iy >> 31) & 4;
    if (ix < 0) {
        octant |= 2;
    }
    if (ax < ay) {
        octant |= 1;
    }
    base = g_AtanOctantBase[octant];
    sign = g_AtanOctantSign[octant];
    return (short)((g_AtanTable[q] ^ sign) + base);
}

// The sector callback of a stream into a ring buffer (set by cdstream.cpp): takes the
// sector expected next and returns 2 when that fills the ring, 0 otherwise; 8 for any
// other sector.
// The game loads &g_CdAudio into $a0 first, moving the arguments up to $a1 and $a2 (as
// CdAudio_OnSync).
int func_80082EE4(unsigned int sector, void *header)
{
    CdAudio *cd = &g_CdAudio;
    int result = 8;

    cd->curSector = cd->nextSector;
    if (sector == cd->nextSector) {
        unsigned int write = cd->ringWrite;
        unsigned int fill = cd->ringFill;
        char *dst = (char *)cd->dst + (write << 11);

        write++;
        fill++;
        if (write == cd->remaining) {
            write = 0;
        }
        result = (fill == cd->remaining) << 1;
        cd->nextSector = sector + 1;
        cd->ringFill = fill;
        cd->ringWrite = write;
        CdGetSector_Thunk(dst, 0x200);
    }
    return result;
}

// Adds a 3-word packet after p.
void FUN_80082f74(void *p, unsigned int area0, unsigned int area1, unsigned int offset)
{
    unsigned int *pkt;
    unsigned int link;

    link = Mips_Lwl2(p);
    pkt = g_PacketPtr;
    ((unsigned char *)pkt)[3] = 3;
    Mips_Swl2(link, pkt);
    Mips_Swl2((unsigned int)pkt << 8, p);
    pkt[1] = area0;
    pkt[2] = area1;
    pkt[3] = offset;
    g_PacketPtr = pkt + 4;
}

}
