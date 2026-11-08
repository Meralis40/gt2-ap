// OVL3 entry point: race with the GT-mode garage cars picked in the OVL2 arcade
// menus (docs/arcade.md).
#include "gptr.h"
#include "common.h"
#include "gamestate.h"

extern "C" {
extern ArcadeRaceSel (&g_Ovl3_RaceSel);
extern char (&g_Ovl3_GtData)[];
extern game_pointer<void> (&g_EventDataBuf);

}

extern "C" void ArcadeGarageRace_Entry(void)
{
    // 0x801d5fa0, past the end of g_GameState, rounded down to a word.
    char *base = (char *)&g_GameState;
    int offset = 0xc6c0;
    ArcadeRaceSel *sel = (ArcadeRaceSel *)rt::from_guest(((int)rt::to_guest((base + offset)) & ~3));

    g_Ovl3_RaceSel = *sel;
    if (sel->useGarageCars) {
        g_EventDataBuf = GameData_LoadGtMode(g_Ovl3_GtData, 0xc8000);
        ArcadeSel_SetupRaceCars(&g_Ovl3_RaceSel);
        if (sel->raceType == 6) {
            CarWork_LoadGarageCarAlt(sel->garage, sel->slot);
        }
    }
    Overlay_StartAt(0, (void *)Race_Entry, 0);
}
