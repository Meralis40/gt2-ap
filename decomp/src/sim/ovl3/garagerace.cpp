// OVL3 entry point: race with the GT-mode garage cars picked in the OVL2 arcade
// menus (docs/arcade.md).
#include "common.h"
#include "gamestate.h"

// The arcade race selection that OVL2 leaves at 0x801d5fa0 (g_RaceCarWorkAlt).
struct ArcadeRaceSel {
    char unk0[2];
    signed char raceType;  // 6: time trial
    char unk3[3];
    short garage;          // 0x6
    short slot;            // 0x8
    char unkA[0x2c8 - 0xa];
    int useGarageCars;     // 0x2c8
    char unk2CC[0x2d4 - 0x2cc];
};

extern "C" {
extern ArcadeRaceSel g_Ovl3_RaceSel;
extern char g_Ovl3_GtData[];
extern void *g_EventDataBuf;
extern char D_80011F64[];  // Race_Entry (OVL0)

void *GameData_LoadGtMode(void *buffer, int size);
void ArcadeSel_SetupRaceCars(ArcadeRaceSel *sel);
void CarWork_LoadGarageCarAlt(int garage, int slot);
void Overlay_StartAt(int index, void *entry, int arg0);
}

extern "C" void ArcadeGarageRace_Entry(void)
{
    // 0x801d5fa0, past the end of g_GameState, rounded down to a word.
    char *base = (char *)&g_GameState;
    int offset = 0xc6c0;
    ArcadeRaceSel *sel = (ArcadeRaceSel *)((int)(base + offset) & ~3);

    g_Ovl3_RaceSel = *sel;
    if (sel->useGarageCars) {
        g_EventDataBuf = GameData_LoadGtMode(g_Ovl3_GtData, 0xc8000);
        ArcadeSel_SetupRaceCars(&g_Ovl3_RaceSel);
        if (sel->raceType == 6) {
            CarWork_LoadGarageCarAlt(sel->garage, sel->slot);
        }
    }
    Overlay_StartAt(0, D_80011F64, 0);
}
