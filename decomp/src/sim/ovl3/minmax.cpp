// Colour helpers: largest and smallest of three values, and RGB555 to HSV.
#include "common.h"

extern "C" int Max3(int a, int b, int c)
{
    if (b < a) {
        if (a < c) {
            return c;
        }
    } else {
        if (b < c) {
            return c;
        }
        return b;
    }
    return a;
}

extern "C" int Min3(int a, int b, int c)
{
    if (a < b) {
        if (c < a) {
            return c;
        }
    } else {
        if (c < b) {
            return c;
        }
        return b;
    }
    return a;
}

// Converts a 15-bit colour (red in the low bits) to hue (0-359), saturation
// (0-32) and value (0-31) in hsv[0..2].
extern "C" void Rgb555_ToHsv(unsigned int rgb, short *hsv)
{
    int r = rgb & 0x1f;
    int g = (rgb & 0x3e0) >> 5;
    int b = (rgb & 0x7c00) >> 10;
    int max = Max3(r, g, b);
    int min = Min3(r, g, b);
    int s;
    int h;

    if (max == 0) {
        s = 0;
    } else {
        s = ((max - min) << 5) / max;
    }
    h = 0;
    if (s != 0) {
        int rc = ((max - r) << 5) / (max - min);
        int gc = ((max - g) << 5) / (max - min);
        int bc = ((max - b) << 5) / (max - min);
        if (r == max) {
            h = bc - gc;
        }
        if (g == max) {
            h = 64 + rc - bc;
        }
        if (b == max) {
            h = 128 + gc - rc;
        }
        h *= 60;
        while (h < 0) {
            h += 360 << 5;
        }
        while (h >= 360 << 5) {
            h -= 360 << 5;
        }
    }
    hsv[0] = h >> 5;
    hsv[1] = s;
    hsv[2] = max;
}
