// OVL3 copy of the OVL4 CarWork helpers (docs/arcade.md).
#include "common.h"

struct CarWork;

extern "C" void CarWork_Clear(CarWork *work)
{
    memset(work, -1, 0x17e8);
}
