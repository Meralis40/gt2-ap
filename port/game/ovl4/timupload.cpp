// OVL4 80013b60-80013bd4 (timupload): Uploads a TIM to a texture page.
#include "common.h"

extern "C" {
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
