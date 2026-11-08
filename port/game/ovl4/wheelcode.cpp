// OVL4 80013a28-80013b28 (wheelcode): Wheel codes.
#include "common.h"

extern "C" {

// The wheel brands' two-letter codes ("bb", "br", "du"...), 0-terminated.
extern unsigned short (&D_80050904)[];

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
}
