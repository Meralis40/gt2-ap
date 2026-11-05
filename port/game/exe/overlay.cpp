// Resident code 8005d9bc-8005db90 (overlay): starts the overlays. The port loads them through
// the runtime, which also holds the setjmp point the game has in Overlay_Dispatch.
#include <stdarg.h>
#include "common.h"
#include "rt.h"

extern "C" {
extern char g_MenuContext[];    // start of the overlays' .bss, and the CD read buffer
extern char g_MainInitialized[]; // end of the overlays' .bss

void Race_DefaultEntry();
void MainMenu_Run();
void ArcadeMode_Run();
void ArcadeGarageRace_Entry();
void GTMode_Run();
void Movie_DefaultEntry();

// g_OverlayDefaultEntries, with host functions.
static void *const s_OverlayDefaultEntries[6] = {
    (void *)Race_DefaultEntry, (void *)MainMenu_Run, (void *)ArcadeMode_Run,
    (void *)ArcadeGarageRace_Entry, (void *)GTMode_Run, (void *)Movie_DefaultEntry,
};

void Overlay_ClearBss(void)
{
    memset(g_MenuContext, 0, g_MainInitialized - g_MenuContext);
}

void Overlay_Start(int index)
{
    Overlay_StartAt(index, s_OverlayDefaultEntries[index], 0, 0, 0, 0);
}

// Loads overlay index and runs entry with the four ints after it, from the runtime's loop.
void Overlay_StartAt(int index, void *entry, ...)
{
    va_list ap;
    int args[4];

    va_start(ap, entry);
    args[0] = va_arg(ap, int);
    args[1] = va_arg(ap, int);
    args[2] = va_arg(ap, int);
    args[3] = va_arg(ap, int);
    va_end(ap);
    Overlay_Load(index);
    Rt_RunEntry(entry, args[0], args[1], args[2], args[3]);
}

void Overlay_Load(int index)
{
    Spu_WaitVoicesSilent();
    Sfx_StopAllIfReady();
    Rt_LoadOverlay(index);
    FlushCache();
    Overlay_ClearBss();
}
}
