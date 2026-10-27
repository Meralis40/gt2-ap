// Resident code 800686a0-80068734 (soundvsync). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Seq_Tick(...);
void Sound_Tick(...);
void Sound_VSyncTick(void);
void VSyncList_Add(VSyncCallback **list, VSyncCallback *node);
extern GfxIrqState g_GfxIrq;
extern VSyncCallback g_SoundVSync;

void Sound_VSyncTick(void)
{
    Sound_Tick();
    Seq_Tick();
}

void Sound_InstallVSync(void)
{
    g_SoundVSync.func = Sound_VSyncTick;
    g_SoundVSync.priority = 0x20000;
    VSyncList_Add(&g_GfxIrq.vsyncCallbacks, &g_SoundVSync);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/soundvsync", FUN_80068708);
}
