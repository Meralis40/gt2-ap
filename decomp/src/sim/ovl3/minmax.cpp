// Largest and smallest of three values, used by Rgb555_ToHsv.
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
