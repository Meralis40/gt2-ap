// Saves a car being worked on back to its garage slot, with the stats
// computed from its parts.
#include "common.h"
#include "carwork.h"

struct Hi3 { unsigned short lo:13; unsigned short hi:3; };

extern "C" void Car_StoreSetup(Car *car, CarWork *work)
{
    unsigned short stats[4];
    unsigned short kept;
    int on;

    car->setup = work->setup;
    CarWork_ComputeStats(work, stats);
    kept = car->flags98 & 0xc000;
    car->flags98 = kept | stats[1];
    car->stat96 = stats[2];
    kept = car->stat94 & 0xe000;
    car->stat94 = kept | stats[0];
    ((Hi3 *)&car->stat94)->hi = ((unsigned char *)stats)[6];
    on = work->partSlots[0x13] > 0;
    car->flags98 = (car->flags98 & 0x7fff) | (on << 15);
}
