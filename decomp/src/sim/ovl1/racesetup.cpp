// OVL1 800109c0-80010e6c (racesetup): Reloads the event of a replay's g_RaceSetup and
// builds its cars' g_RaceCarParams.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

extern "C" {
extern void *g_EventDataBuf;
extern void *g_GtModeRaceDataBuf;
extern RaceEventParams g_RaceEventParams;

// Like RaceSetup_Reload, with the event from g_GtModeRaceDataBuf.
RaceSetup *RaceSetup_ReloadGtRace(void)
{
    GameState *gs = &g_GameState;
    char *base0 = (char *)gs;
    int offset0 = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base0 + offset0);
    void *event = GameData_FindEvent((GameData *)g_GtModeRaceDataBuf, setup->eventCode);
    int j;

    g_RaceEventParams = *(RaceEventParams *)((char *)event + 0x44);
    for (j = 0; j < setup->carCount; j++) {
        // g_RaceCarParams[j], past the end of g_GameState.
        char *base = (char *)&g_GameState;
        int offset = 0x14fda;
        CarParams_BuildFromSetup(&setup->cars[j].setup, (CarParams *)(base + (offset + j * 0x1c0)));
    }
    return setup;
}

// Looks g_RaceSetup.eventCode up in g_EventDataBuf, copies its params to
// g_RaceEventParams and builds every car's g_RaceCarParams.
RaceSetup *RaceSetup_Reload(void)
{
    GameState *gs = &g_GameState;
    char *base0 = (char *)gs;
    int offset0 = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base0 + offset0);
    void *event = GameData_FindEvent((GameData *)g_EventDataBuf, setup->eventCode);
    int j;

    g_RaceEventParams = *(RaceEventParams *)((char *)event + 0x44);
    for (j = 0; j < setup->carCount; j++) {
        char *base = (char *)&g_GameState;
        int offset = 0x14fda;
        CarParams_BuildFromSetup(&setup->cars[j].setup, (CarParams *)(base + (offset + j * 0x1c0)));
    }
    return setup;
}

// Like RaceSetup_Reload, but builds only the params of the cars that don't use GT
// mode data. Returns 1 when some car does (CarSetup.flags_7a & 0x40): GT mode data
// must be loaded and RaceSetup_BuildGtCars run.
#ifdef NON_MATCHING
// The game keeps the car setup in v0 and copies it to a0 for the call.
int RaceSetup_ReloadArcade(void)
{
    GameState *gs = &g_GameState;
    char *base0 = (char *)gs;
    int offset0 = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base0 + offset0);
    int needGt;
    void *event = GameData_FindEvent((GameData *)g_EventDataBuf, setup->eventCode);
    int j;

    needGt = 0;
    g_RaceEventParams = *(RaceEventParams *)((char *)event + 0x44);
    for (j = 0; j < setup->carCount; j++) {
        CarSetup *cs = &setup->cars[j].setup;

        if ((cs->flags_7a >> 6) & 1) {
            needGt = 1;
            continue;
        }
        char *base = (char *)&g_GameState;
        int offset = 0x14fda;
        CarParams_BuildFromSetup(cs, (CarParams *)(base + (offset + j * 0x1c0)));
    }
    return needGt;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/racesetup", RaceSetup_ReloadArcade);
#endif

// Builds g_RaceCarParams for the cars flagged as using GT mode data.
void RaceSetup_BuildGtCars(void)
{
    GameState *gs = &g_GameState;
    char *base0 = (char *)gs;
    int offset0 = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base0 + offset0);
    int j;

    for (j = 0; j < setup->carCount; j++) {
        CarSetup *cs = &setup->cars[j].setup;

        if ((cs->flags_7a >> 6) & 1) {
            char *base = (char *)&g_GameState;
            int offset = 0x14fda;
            CarParams_BuildFromSetup(cs, (CarParams *)(base + (offset + j * 0x1c0)));
        }
    }
}
}
