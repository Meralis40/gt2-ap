// OVL4 800222e4-800224b0 (carcolor): Car colour names.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern int g_CarColorIsLatin;
extern char *g_CarColor;
extern char *g_CarColorNames;

// Reads the car colour file (VolId 0) to buf and the colour names after it, the
// Latin ones (VolId 5) or the others (VolId 4). Returns buf.
char *CarColor_Load(int latin, char *buf)
{
    g_CarColorIsLatin = latin;
    g_CarColor = buf;
    VolId_ReadFile((VolBootId)0, buf);
    buf += (VolId_GetSize((VolBootId)0) + 3) & ~3;
    VolId_ReadFile((VolBootId)(latin == 0 ? 4 : 5), g_CarColorNames = buf);
    return g_CarColor;
}

// The colour list of car carIdx in the colour file.
unsigned short *CarColor_GetCarEntry(char *file, int carIdx)
{
    unsigned short *offsets = (unsigned short *)(file + 8);

    return (unsigned short *)(file + offsets[carIdx]);
}

// The name of the colour colorIdx of a car's colour list.
char *CarColor_GetName(unsigned short *entry, int colorIdx)
{
char *names = g_CarColorNames;
    unsigned short *id = entry + colorIdx;

    return ColorNames_Get(names, *id);
}

// The name id in the colour names.
char *ColorNames_Get(char *names, int id)
{
unsigned short *offset = (unsigned short *)names + id;

    return names + *offset;
}

// Copies the name of colorCode of carId to out as a wide string, widening the
// Latin names. Returns out.
unsigned short *CarColor_GetNameW(unsigned short *out, unsigned int carId, int colorCode)
{
    int carIdx = CarInfoA_FindIndex(carId);
    int colorIdx = CarInfoA_FindColorIndex(carId, colorCode);
    char *name = CarColor_GetName(CarColor_GetCarEntry(g_CarColor, carIdx), colorIdx);

    if (!g_CarColorIsLatin) {
        unsigned short *src = (unsigned short *)name;
        unsigned short *dst = out;

        while ((*dst++ = *src++) != 0)
            ;
    } else {
        unsigned char *src = (unsigned char *)name;
        unsigned short *dst = out;

        while ((*dst++ = *src++) != 0)
            ;
    }
    return out;
}
}
