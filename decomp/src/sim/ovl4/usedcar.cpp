// OVL4 800224b0-800226a0 (usedcar): Used car tables.
#include "ovl4.h"

extern "C" {
extern UsedCarPeriod g_OVL4_StorageUsedCar;

void VolId_ReadFileAsync(VolBootId id, void *dst);
void bcopy(const void *src, void *dst, int n);
int CarInfoA_IsAvailable(unsigned int carId);

UsedCarEntry *UsedCar_GetMakerCars(UsedCarPeriod *period, int makerId)
{
    return (UsedCarEntry *)((int)&period->makers[0].offset + (unsigned int)period->makers[makerId].offset);
}

unsigned short *UsedCar_GetMakerCount(UsedCarPeriod *period, int makerId)
{
    return &period->makers[makerId].count;
}

UsedCarPeriod *UsedCar_GetPeriod(void)
{
    return &g_OVL4_StorageUsedCar;
}

// Reads the used car file into buf and copies the period for day (one of 60)
// to g_OVL4_StorageUsedCar, then drops the cars that aren't available.
void UsedCar_LoadPeriod(unsigned int day, char *buf)
{
    unsigned int period = day % 60;
    UsedCarFile *file = (UsedCarFile *)buf;
    UsedCarPeriod *p;
    int maker;

    VolId_ReadFileAsync(VOLID_USEDCAR_USA, buf);
    bcopy(buf + file->offsets[period], &g_OVL4_StorageUsedCar,
          file->offsets[period + 1] - file->offsets[period]);
    p = UsedCar_GetPeriod();
    for (maker = 0; maker < 39; maker++) {
        UsedCarEntry *cars = UsedCar_GetMakerCars(p, maker);
        unsigned short *count = UsedCar_GetMakerCount(p, maker);
        int i;
        int n = 0;

        for (i = 0; i < *count; i++) {
            if (CarInfoA_IsAvailable(cars[i].carId)) {
                if (i != n)
                    cars[n] = cars[i];
                n++;
            }
        }
        *count = n;
    }
}

// Used cars of makerId for the current period (UsedCar_GetPeriod).
void UsedCarList_GetCars(int makerId, unsigned int *count, UsedCarEntry **cars)
{
    UsedCarPeriod *period;
    unsigned short *puVar1;
    UsedCarEntry *pUVar2;

    period = UsedCar_GetPeriod();
    puVar1 = UsedCar_GetMakerCount(period, makerId);
    *count = (unsigned int)*puVar1;
    pUVar2 = UsedCar_GetMakerCars(period, makerId);
    *cars = pUVar2;
}
}
