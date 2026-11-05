// OVL1 80010edc-800110c4 (racestart): Prepares the race of a replay.
#include "copies.h"
#include "common.h"
#include "gamestate.h"

extern "C" {
extern void *g_EventDataBuf;
extern void *g_GtModeRaceDataBuf;
extern unsigned char g_OVL1_DataBuf[];
extern unsigned char g_OVL1_FontInfo[];

// Before jumping to Race_Entry (OVL0) for a replay loaded into g_RaceSetup (Replay
// Theater or demo replay): reloads the data files matching g_RaceSetup.game and
// raceType and recomputes each car's g_RaceCarParams. Game 0 (arcade): the arcade
// data, and the GT mode data when a car needs it. Game 1: the GT mode race data for
// race types 1, 2 and 7 to 10; for 11 the same, with the recorded car file of the
// event (GhostEvent_GetFileId); the license data for the others.
void Race_PrepareStart(void)
{
    GameState *gs = &g_GameState;
    char *base0 = (char *)gs;
    int offset0 = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base0 + offset0);
    int j;

    switch (setup->game) {
    case 0:
        g_EventDataBuf = GameData_LoadArcade(g_OVL1_DataBuf, 0x30000);
        if (RaceSetup_ReloadArcade()) {
            g_EventDataBuf = GameData_LoadGtMode(g_OVL1_FontInfo, 0xc8000);
            RaceSetup_BuildGtCars();
        }
        break;
    case 1:
        switch (setup->raceType) {
        case 1:
        case 2:
        case 7:
        case 8:
        case 9:
        case 10:
            g_GtModeRaceDataBuf = GameData_LoadGtModeRace(g_OVL1_DataBuf, 0x30000);
            g_EventDataBuf = GameData_LoadGtMode(g_OVL1_FontInfo, 0xc8000);
            RaceSetup_ReloadGtRace();
            break;
        case 11:
            VolId_ReadFileAsync(Event_GetGhostFileId(setup->eventCode), g_OVL1_DataBuf);
            GhostEvent_ApplyFile((char *)g_OVL1_DataBuf);
            g_GtModeRaceDataBuf = GameData_LoadGtModeRace(g_OVL1_DataBuf, 0x30000);
            g_EventDataBuf = GameData_LoadGtMode(g_OVL1_FontInfo, 0xc8000);
            RaceSetup_ReloadGtRace();
            setup->carCount = 1;
            for (j = 0; j < setup->carCount; j++) {
                // g_RaceCarParams[j], past the end of g_GameState.
                char *base = (char *)&g_GameState;
                int offset = 0x14fda;
                CarParams_BuildFromSetup(&setup->cars[j].setup, (CarParams *)(base + (offset + j * 0x1c0)));
            }
            break;
        default:
            g_EventDataBuf = GameData_LoadLicense(g_OVL1_DataBuf, 0x30000);
            RaceSetup_Reload();
            break;
        }
        break;
    }
}
}
