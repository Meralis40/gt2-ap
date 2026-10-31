// OVL4 80018f6c-8001907c (menubgm): Menu background music requests.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void MenuBgm_LoadBank(void);
extern int g_MenuBgm_Track;
extern short g_MenuBgm_Delay;
extern short g_MenuBgm_Playing;
void MenuBgm_Stop(void);

/* No track, no delay; loads the music sample bank. */

void MenuBgm_Init(void)
{
    g_MenuBgm_Track = -1;
    g_MenuBgm_Delay = 0;
    g_MenuBgm_Playing = 0;
    MenuBgm_LoadBank();
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menubgm", MenuBgm_Request);

/* Stops the music if a track was started (GTMenu quitting). */

void MenuBgm_StopIfPlaying(void)
{
    if (g_MenuBgm_Playing != 0) {
        MenuBgm_Stop();
        g_MenuBgm_Playing = 0;
    }
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menubgm", MenuBgm_Update);
}
