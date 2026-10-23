// CarWork helpers, in OVL4 and copied into OVL3 (docs/arcade.md). Both overlays
// build this file.
#include "common.h"

struct CarWork;

extern "C" void CarWork_Clear(CarWork *work)
{
    memset(work, -1, 0x17e8);
}
