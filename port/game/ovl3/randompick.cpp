// Picks a random race car from a list of entries (car and colour).
#include "common.h"
#include "gamestate.h"

extern "C" {
extern int g_GameDataMode;
extern char *g_GtModeRaceData;
extern unsigned char D_80091620[];

}

inline unsigned int CarIdAt(GameDataTable *table, int index)
{
    return *(unsigned int *)GameDataTable_At(table, index - 1);
}

// Picks an available car from idList for out. A car already in picked
// (same entry or same car) is re-rolled with a 29/32 chance, at most 64
// times. The colour is the entry's colour index if set, otherwise a random
// colour of the car, re-rolled once if washed out (saturation below 6).
// Returns the entry's colour index, or 0 if the list is empty.
extern "C" unsigned int RaceCarPick_Random(RaceCarPick *out, void *data, IdList *idList, unsigned int *seed,
                                             RaceCarPick *picked, int pickedCount)
{
    GameDataTable *table;
    int tries;
    int count;
    unsigned int entry;
    unsigned short colorIdx;
    unsigned int carId;
    unsigned char code;
    unsigned short rgb;
    int i;
    unsigned int k;
    short index;

    {
        GameDataTable *t;
        if (g_GameDataMode == 2) {
            t = (GameDataTable *)(g_GtModeRaceData + 0x10);
        } else {
            t = (GameDataTable *)((char *)data + 0x100);
        }
        table = t;
    }
    tries = 64;
    count = IdList_Count(idList);
    if (count <= 0) {
        return 0;
    }
    goto pick;
retry:
    tries--;
pick:
    {
        unsigned int e;
        k = Rand_Next(seed) % count;
        e = idList->ids[k];
        colorIdx = e >> 26;
        entry = e;
    }
    if (!CarInfoA_IsAvailable(CarIdAt(table, entry & 0xffff))) {
        goto pick;
    }
    for (i = 0; i < pickedCount; i++) {
        if (((unsigned short)picked[i].entryIndex == (entry & 0xffff)) | (picked[i].carId == CarIdAt(table, entry & 0xffff))) {
            if (((int)(Rand_Next(seed) & 0x1f) < 29) & (tries > 0)) {
                goto retry;
            }
        }
    }
    carId = CarIdAt(table, entry & 0xffff);
    index = entry;
    if (colorIdx != 0) {
        int c = D_80091620[colorIdx];
        code = c;
        rgb = CarInfoA_GetColorRgb(carId, (signed char)c);
    } else {
        unsigned short *rgbs;
        unsigned char *codes;
        short hsv[3];
        unsigned int n = CarInfoA_GetColors(carId, &rgbs, (char **)&codes);
        for (i = 0; i < 2; i++) {
            k = Rand_Next(seed) % n;
            Rgb555_ToHsv(rgbs[k], hsv);
            if (hsv[1] >= 6) {
                break;
            }
        }
        code = codes[k];
        rgb = rgbs[k];
    }
    out->carId = carId;
    out->colorCode = code;
    out->colorRgb = rgb;
    out->entryIndex = index;
    return colorIdx;
}
