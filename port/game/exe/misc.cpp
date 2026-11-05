// Resident code 8007f7f4-8007f924 (misc): a line primitive, the display enable and the
// SPU's reset.
#include "common.h"

extern "C" {
extern void *g_PacketPtr;

struct GpuPorts {
    unsigned int data;
    unsigned int ctrl;
};

// Allocates a LINE_F2 from g_PacketPtr, links it into ot, and returns its first point.
LineF2 *Prim_AddLineF2(uint *ot, uint rgb)
{
    unsigned int *p;
    unsigned int link;

    __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
    p = (unsigned int *)g_PacketPtr;
    ((unsigned char *)p)[3] = 3;
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(p) : "memory");
    __asm__ volatile("swl\t%0,2(%1)" : : "r"((unsigned int)p << 8), "r"(ot) : "memory");
    p[1] = rgb ^ 0x40000000;
    g_PacketPtr = (char *)(p + 4);
    return (LineF2 *)(p + 2);
}

// GP1(03).
void EnableDisplay(int on)
{
    volatile GpuPorts *gpu = (volatile GpuPorts *)0x1f801810;

    gpu->ctrl = on | 0x3000000;
}

// Enables the SPU's DMA, zeroes every volume and voice mode, and sets SPUCNT.
// The game loads D_801C93F4's address in $a0 for nothing first, like Spu_DmaDone (the address in $a0 first, not solved).
void Spu_Reset(void)
{
    volatile unsigned int *dma;
    volatile ushort *spu;
    volatile ushort *voice;

    dma = (volatile unsigned int *)0x1f801080;
    dma[0x70 / 4] |= 0xb0000;
    DMACallback(4, Spu_DmaDone);
    spu = (volatile ushort *)0x1f801c00;
    voice = (volatile ushort *)0x1f801c00;
    spu[0x180 / 2] = 0;
    spu[0x182 / 2] = 0;
    spu[0x1ac / 2] = 4;
    spu[0x1b0 / 2] = 0;
    spu[0x1b2 / 2] = 0;
    spu[0x1b4 / 2] = 0;
    spu[0x1b6 / 2] = 0;
    do {
        voice[0] = 0;
        voice[1] = 0;
        voice += 8;
    } while (voice != (volatile ushort *)0x1f801d80);
    spu[0x184 / 2] = 0;
    spu[0x186 / 2] = 0;
    spu[0x190 / 2] = 0;
    spu[0x192 / 2] = 0;
    spu[0x194 / 2] = 0;
    spu[0x196 / 2] = 0;
    spu[0x198 / 2] = 0;
    spu[0x19a / 2] = 0;
    spu[0x1aa / 2] = 0xc001;
}

// The SPU's DMA finished: clears SPUCNT's transfer mode.
// The game loads D_801C93F4's address in $a0 for nothing first.
void Spu_DmaDone(void)
{
    volatile ushort *spu = (volatile ushort *)0x1f801c00;

    spu[0x1aa / 2] &= 0xffcf;
}

}
