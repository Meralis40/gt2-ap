// Resident code 8007c268-8007c36c (dma): starts SPU transfers, and waits with a timeout
// for a DMA channel to go idle.
#include "common.h"

extern "C" {

// Starts a DMA of size bytes, rounded up to 64-byte blocks, between RAM and the SPU RAM
// address spuAddr: dir 0 writes to the SPU, otherwise reads back.
// Scheduling only: the game shifts spuAddr early and splits a constant's lui and ori.
void Spu_DmaTransfer(void *src, int spuAddr, unsigned int size, int dir)
{
    int blocks = (int)(size + 0x3f) >> 6;

    if (blocks != 0) {
        volatile unsigned int *dma = (volatile unsigned int *)HW_REG(0x080);
        volatile unsigned short *spu = (volatile unsigned short *)HW_REG(0xc00);
        int addr = spuAddr >> 3;
        unsigned int chcr = 0x1000201;
        volatile unsigned int *delay = (volatile unsigned int *)HW_REG(0x014);
        unsigned int cnt = spu[0x1aa / 2] & 0xffcf;
        unsigned int mode = cnt | 0x20;
        unsigned int timing = (*delay & 0xf0ffffff) | 0x20000000;
        unsigned int bcr = (blocks + 1) << 16 | 0x10;

        if (dir != 0) {
            chcr = 0x1000200;
            timing |= 0x2000000;
            mode = cnt | 0x30;
        }
        spu[0x1a6 / 2] = addr;
        spu[0x1aa / 2] = mode;
        *delay = timing;
        dma[0x40 / 4] = (unsigned int)src;
        dma[0x44 / 4] = bcr;
        dma[0x48 / 4] = chcr;
        Hw_SpuDma(src, (blocks + 1) * 0x40, dir != 0);
        Rt_DmaDone(4);
    }
}

// The channels' waits are a `b Dma_WaitChannel` each with the channel in its delay slot,
// the last one falling into it: GCC makes neither.
void func_8007C300(void)
{
    Dma_WaitChannel(0);
}

// The second entry of the same 16 bytes.
void func_8007C308(void)
{
    Dma_WaitChannel(1);
}

void Dma_WaitGpu(...)
{
    Dma_WaitChannel(2);
}

// The second entry of the same 16 bytes.
void func_8007C318(void)
{
    Dma_WaitChannel(3);
}

void Spu_DmaWait(void)
{
    Dma_WaitChannel(4);
}

void Dma_WaitOtc(void)
{
    Dma_WaitChannel(6);
}

// Waits for the channel's CHCR busy bit to clear, 0x2efb44 polls at most.
void Dma_WaitChannel(int ch)
{
    volatile unsigned int *chan = (volatile unsigned int *)HW_REG(0x080) + ch * 4;
    int timeout = 0x2efb44;
    unsigned int busy = 0x1000000;

loop:
    {
        unsigned int b = chan[2] & busy;

        timeout--;
        if (b == 0) return;
    }
    if (timeout != 0) goto loop;
}

}
