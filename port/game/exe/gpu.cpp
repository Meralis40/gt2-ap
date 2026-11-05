// Resident code 8007d024-8007d32c (gpu): two primitive allocators, the interrupt handlers
// and the GP1 display registers.
#include "common.h"

extern "C" {
extern void *g_PacketPtr;
extern unsigned char D_80090520[]; // dot clock divider of each horizontal mode
extern unsigned char D_80090528[]; // first dot of each horizontal mode

// The GPU's two ports.
struct GpuPorts {
    unsigned int data;
    unsigned int ctrl;
};

// Allocates a TILE from g_PacketPtr, links it into ot, and returns its position and size.
RECT *Prim_AddTile(void *ot, unsigned int rgb)
{
    unsigned int *p;
    unsigned int link;

    link = Mips_Lwl2(ot);
    p = (unsigned int *)g_PacketPtr;
    ((unsigned char *)p)[3] = 3;
    Mips_Swl2(link, p);
    Mips_Swl2((unsigned int)p << 8, ot);
    p[1] = rgb ^ 0x60000000;
    g_PacketPtr = (char *)(p + 4);
    return (RECT *)(p + 2);
}

// Allocates a POLY_F4 and returns its colour word, the vertices after it.
unsigned int *Prim_AddPolyF4(void *ot, unsigned int rgb)
{
    unsigned int *p;
    unsigned int link;

    link = Mips_Lwl2(ot);
    p = (unsigned int *)g_PacketPtr;
    ((unsigned char *)p)[3] = 5;
    Mips_Swl2(link, p);
    Mips_Swl2((unsigned int)p << 8, ot);
    p[1] = rgb ^ 0x28000000;
    g_PacketPtr = (char *)(p + 6);
    return p + 1;
}

// Called first by Boot_Init.
// The game loads the address in $a0 first and copies it (matching-quirks.md, not solved).
void Irq_Install()
{
    GfxIrqState *irq = &g_GfxIrq;

    memset(irq, 0, sizeof(GfxIrqState));
    irq->vsyncCallbacks = 0;
    DMACallback(2, Irq_GpuDmaHandler);
    Psyq_VSyncCallback(Irq_VSyncHandler);
}

// GP1(08), remembered for the range functions.
// The address in $a0 first (see Irq_Install).
void Gpu_SetDisplayMode(unsigned int mode)
{
    GfxIrqState *irq = &g_GfxIrq;

    volatile GpuPorts *gpu = (volatile GpuPorts *)0x1f801810;

    gpu->ctrl = mode | 0x8000000;
    irq->displayMode = mode;
}

// GP1(06): the horizontal display range, in the mode's dots.
// The address in $a0 first (see Irq_Install).
void Gpu_SetHRange(int x0, int x1)
{
    GfxIrqState *irq = &g_GfxIrq;
    unsigned int mode = irq->displayMode;
    volatile GpuPorts *gpu;
    int i = 4;
    int div;

    if (!(mode & 0x40)) i = mode & 3;
    div = D_80090520[i];
    x0 = (x0 + D_80090528[i]) * div;
    x1 = ((x1 + 3) & ~3) * div;
    gpu = (volatile GpuPorts *)0x1f801810;
    gpu->ctrl = (x1 + x0) << 12 | x0 | 0x6000000;
}

// GP1(07): the vertical display range, halved when interlaced.
// The address in $a0 first (see Irq_Install).
void Gpu_SetVRange(int y0, int y1)
{
    GfxIrqState *irq = &g_GfxIrq;
    unsigned int mode = irq->displayMode;
    volatile GpuPorts *gpu;

    if (mode & 4) {
        y0 >>= 1;
        y1 >>= 1;
    }
    y0 += 0x10;
    if (mode & 8) y0 += 0x15;
    gpu = (volatile GpuPorts *)0x1f801810;
    gpu->ctrl = (y1 + y0) << 10 | y0 | 0x7000000;
}

// Waits for GPUSTAT's field bit to differ from the one Irq_WaitVSyncs saw. Returns 1 for
// an even field.
// The address in $a0 first; pinned there, one load is still scheduled differently.
// common.h declares it void, for the callers that ignore the result.
int Gpu_WaitField(void)
{
    GfxIrqState *irq = &g_GfxIrq;
    unsigned int stat = irq->gpustatAtWait;
    volatile unsigned int *status = (volatile unsigned int *)0x1f801814;
    unsigned int field = 0x80000000;

    do {
    } while (!((stat ^ *status) & field));
    irq->vblanksSinceFlip = 0;
    return (stat & field) == 0;
}

// Waits until count vblanks have passed since the last wait, then starts counting again;
// a negative count starts again now and waits for one. Returns the vblank count.
// The address in $a0 first, with an empty delay slot after it.
int Irq_WaitVSyncs(int count)
{
    GfxIrqState *irq = &g_GfxIrq;

    if (count < 0) {
        irq->vblanksSinceFlip = 0;
        count = 1;
    }
    if (count > 0) {
        irq->gpustatAtWait = ((volatile GpuPorts *)0x1f801810)->ctrl;
        do {
        } while (*(volatile int *)&irq->vblanksSinceFlip < count);
        irq->vblanksSinceFlip = 0;
    }
    return irq->vblankTotal;
}

// The address in $a0 first (see Irq_Install).
void Irq_VSyncHandler(void)
{
    GfxIrqState *irq = &g_GfxIrq;

    irq->vblankTotal++;
    irq->vblanksSinceFlip++;
    VSyncList_Run(&irq->vsyncCallbacks);
}

// DMA channel 2 finished: records the time and calls gpuDmaHook.
// The address in $a0 first (see Irq_Install).
void Irq_GpuDmaHandler(void)
{
    GfxIrqState *irq = &g_GfxIrq;

    irq->gpuDmaDoneTime = Timer_SinceFrameStart();
    irq->gpuDmaDoneHblanks = Timer_HblanksSinceFrameStart();
    irq->imageDmaPending = 0;
    irq->gpuDmaCount++;
    if (irq->gpuDmaHook != 0) {
        irq->gpuDmaHook();
    }
}

}
