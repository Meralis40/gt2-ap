// OVL4 80013a28-80013bd4 (timload): Wheel codes and TIM files.
#include "ovl4.h"

extern "C" {
void VolId_ReadFile(VolBootId id, void *dst);
int VolId_GetSize(VolBootId id);
void *Gpu_LoadImageBlock(int *block, RECT *rect);

// The wheel brands' two-letter codes ("bb", "br", "du"...), 0-terminated.
extern unsigned short D_80050904[];

// Packs a wheel code (a two-letter brand, a three-digit size, the type digit
// at 6 and a variant byte at 7) as brand << 28 | size << 16 | type << 13 |
// variant. The brand is its index in D_80050904; types '4' to '6' are 1 to 3.
unsigned int Wheel_ParseCode(signed char *code)
{
    unsigned int r = 0;
    int brand = code[0] | code[1] << 8;
    int i;
    int size;
    int type;
    int c;

    for (i = 0; (c = D_80050904[i]) != 0; i++) {
        if (c == brand)
            break;
    }
    size = (code[2] - '0') * 100;
    size += (code[3] - '0') * 10;
    size += code[4] - '0';
    brand = i;
    switch (code[6]) {
    case '4':
        type = 1;
        break;
    case '5':
        type = 2;
        break;
    case '6':
        type = 3;
        break;
    default:
        type = 0;
        break;
    }
    r = r << 4 | brand;
    r = r << 12 | size;
    r = r << 3 | type;
    return r << 13 | code[7];
}

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
