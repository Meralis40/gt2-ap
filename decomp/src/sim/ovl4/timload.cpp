// OVL4 80013a28-80013bd4 (timload): Wheel codes and TIM files.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void VolId_ReadFile(VolBootId id, void *dst);
int VolId_GetSize(VolBootId id);
void *Gpu_LoadImageBlock(int *block, RECT *rect);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/timload", Wheel_ParseCode);

// Reads a TIM file from the volume into dst.
void Tim_LoadFile(void *dst, VolBootId id)
{
    VolId_ReadFile(id, dst);
    VolId_GetSize(id);
}

// Uploads the pixel block of a TIM (skipping the CLUT block when flag bit 3 is set) to
// the top-left of texture page page: x = (page & 0xf) * 64, y = (page & 0x10) * 16.
void Tim_UploadToPage(void *tim, unsigned int page)
{
    int *block;
    RECT local_10;

    block = (int *)((int)tim + 8);
    if ((*(unsigned int *)((int)tim + 4) >> 3 & 1) != 0) {
        block = (int *)((int)block + *(int *)((int)tim + 8));
    }
    local_10.x = (short)((page & 0xf) << 6);
    local_10.y = (short)((page & 0x10) << 4);
    local_10.w = (short)block[2];
    local_10.h = *(short *)((int)block + 10);
    Gpu_LoadImageBlock(block, &local_10);
}
}
