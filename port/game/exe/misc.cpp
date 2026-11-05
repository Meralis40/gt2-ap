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

    link = Mips_Lwl2(ot);
    p = (unsigned int *)g_PacketPtr;
    ((unsigned char *)p)[3] = 3;
    Mips_Swl2(link, p);
    Mips_Swl2((unsigned int)p << 8, ot);
    p[1] = rgb ^ 0x40000000;
    g_PacketPtr = (char *)(p + 4);
    return (LineF2 *)(p + 2);
}

// GP1(03).
void EnableDisplay(int on)
{
    Hw_Gp1(on | 0x3000000);
}

// Enables the SPU's DMA, zeroes every volume and voice mode, and sets SPUCNT.
// The game loads D_801C93F4's address in $a0 for nothing first, like Spu_DmaDone (the address in $a0 first, not solved).
void Spu_Reset(void)
{
    volatile unsigned int *dma;
    volatile ushort *spu;
    volatile ushort *voice;

    dma = (volatile unsigned int *)HW_REG(0x080);
    dma[0x70 / 4] |= 0xb0000;
    DMACallback(4, Spu_DmaDone);
    spu = (volatile ushort *)HW_REG(0xc00);
    voice = (volatile ushort *)HW_REG(0xc00);
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
    } while (voice != (volatile ushort *)HW_REG(0xd80));
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
    volatile ushort *spu = (volatile ushort *)HW_REG(0xc00);

    spu[0x1aa / 2] &= 0xffcf;
}

}
