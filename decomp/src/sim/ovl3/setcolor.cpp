// Picks the colour of a race car: a fixed colour from the table when colorIdx
// is set, otherwise a random one among the car's colours.
#include "common.h"
#include "race.h"

extern "C" {
extern unsigned char D_80091620[];
int CarInfoA_GetColors(unsigned int carId, unsigned short **outRgb, unsigned char **outCodes);
unsigned short CarInfoA_GetColorRgb(unsigned int carId, signed char colorCode);
unsigned int Rand_Next(unsigned int *seed);

void RaceCarPick_SetColor(RaceCarPick *pick, unsigned int carId, unsigned int colorIdx,
                          unsigned int *seed)
{
    unsigned char code;
    unsigned short rgb;

    if (colorIdx != 0) {
        int c = D_80091620[colorIdx & 0x3f];
        code = c;
        rgb = CarInfoA_GetColorRgb(carId, c);
    } else {
        unsigned short *rgbs;
        unsigned char *codes;
        unsigned int count = CarInfoA_GetColors(carId, &rgbs, &codes);
        unsigned int i = Rand_Next(seed) % count;
        code = codes[i];
        rgb = rgbs[i];
    }
    pick->colorCode = code;
    pick->colorRgb = rgb;
}
}
