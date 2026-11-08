// OVL4 80018f6c-8001907c (menubgm): Menu background music requests.
#include "common.h"

extern "C" {
extern int (&g_MenuBgm_Track);
extern short (&g_MenuBgm_Delay);
extern short (&g_MenuBgm_Playing);

/* No track, no delay; loads the music sample bank. */

void MenuBgm_Init(void)
{
    g_MenuBgm_Track = -1;
    g_MenuBgm_Delay = 0;
    g_MenuBgm_Playing = 0;
    MenuBgm_LoadBank();
}

/* Stops the music and starts track two frames later, unless it is the
   current one. */

void MenuBgm_Request(int track)
{
    if (g_MenuBgm_Track != track) {
        MenuBgm_Stop();
        g_MenuBgm_Track = track;
        g_MenuBgm_Delay = 2;
    }
}

/* Stops the music if a track was started (GTMenu quitting). */

void MenuBgm_StopIfPlaying(void)
{
    if (g_MenuBgm_Playing != 0) {
        MenuBgm_Stop();
        g_MenuBgm_Playing = 0;
    }
}

/* Counts the request's delay down and plays the track when it ends. */

void MenuBgm_Update(void)
{
    if (g_MenuBgm_Delay > 0 && --g_MenuBgm_Delay == 0) {
        MenuBgm_Play(g_MenuBgm_Track);
        g_MenuBgm_Playing = 1;
    }
}
}
