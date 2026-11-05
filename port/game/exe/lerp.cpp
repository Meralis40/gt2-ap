// Resident code 80074844-8007489c (lerp): clamped linear interpolation. OVL2 builds
// it too.
#include "common.h"

extern "C" {
// y0 + (y1 - y0) * (x - x0) / (x1 - x0), clamped to y0 and y1 outside [x0, x1];
// -1 when x0 is negative (no point).
int Lerp_Clamped(int x, int x0, int x1, int y0, int y1)
{
    if (x0 < 0) {
        return -1;
    }
    if (x <= x0) {
        return y0;
    }
    if (x >= x1) {
        return y1;
    }
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}
}
