// Resident code 800686a0-80068734 (soundvsync): the sound driver's vsync callback.
#include "resident.h"

extern "C" {
void Seq_Tick(...);
void Sound_Tick(...);
void Sound_VSyncTick(void);
void VSyncList_Add(VSyncCallback **list, VSyncCallback *node);
void VSyncList_Remove(VSyncCallback **list, VSyncCallback *node);
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

// Removes the callback Sound_InstallVSync added.
void FUN_80068708(void)
{
    VSyncCallback *cb = &g_SoundVSync;

    VSyncList_Remove(&g_GfxIrq.vsyncCallbacks, cb);
}
}
