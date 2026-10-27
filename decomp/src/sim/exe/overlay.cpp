// Resident code 8005d9bc-8005db90 (overlay). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Overlay_StartAt(int index, void *entry, int arg0, int arg1, int arg2, int arg3);
extern void *g_OverlayDefaultEntries;

INCLUDE_ASM("asm/sim/exe/nonmatchings/overlay", Overlay_ClearBss);

INCLUDE_ASM("asm/sim/exe/nonmatchings/overlay", Overlay_Dispatch);

void Overlay_Start(int index)
{
    Overlay_StartAt(index, (&g_OverlayDefaultEntries)[index], 0, 0, 0, 0);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/overlay", Overlay_StartAt);

INCLUDE_ASM("asm/sim/exe/nonmatchings/overlay", Overlay_Load);
}
