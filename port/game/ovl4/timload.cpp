// OVL4 80013b28-80013b60 (timload): Reads a TIM file. OVL2 builds it too.
#include "common.h"

extern "C" {
// Reads a TIM file from the volume into dst; returns its size.
int Tim_LoadFile(void *dst, VolBootId id)
{
    VolId_ReadFile(id, dst);
    return VolId_GetSize(id);
}
}
