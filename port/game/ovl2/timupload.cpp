// OVL2 80013674-800136f0 (timupload): Uploads a TIM to a texture page: another version
// of OVL4's.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"

extern "C" {


// Copy of Tim_UploadToPage / Gpu_UploadImageBlock: uploads the image block of tim to
// texture page page and waits.
void Tim_UploadToPage_OVL2(unsigned char *tim, unsigned int page)
{
    int *block;
    RECT local_10;

    block = (int *)(tim + 8);
    if ((*(unsigned int *)(tim + 4) >> 3 & 1) != 0) {
        block = (int *)rt::from_guest(((int)rt::to_guest(block) + *(int *)(tim + 8)));
    }
    local_10.x = (short)((page & 0xf) << 6);
    local_10.y = (short)((page & 0x10) << 4);
    local_10.w = (short)block[2];
    local_10.h = *(short *)rt::from_guest(((int)rt::to_guest(block) + 10));
    Gpu_LoadImageBlock(block, &local_10);
    Gpu_DrawSync();
}
}
