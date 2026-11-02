// OVL4 80013b28-80013b60 (timload): Reads a TIM file. OVL2 builds it too.
#include "common.h"

extern "C" {
// Reads a TIM file from the volume into dst.
void Tim_LoadFile(void *dst, VolBootId id)
{
    VolId_ReadFile(id, dst);
    VolId_GetSize(id);
}
}
