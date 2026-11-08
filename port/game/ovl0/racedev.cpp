// OVL0 80011ec8-800122c4 (racedev): RaceDevelopment and the race entries.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"

// The input logs a race starts from, copied whole.
struct RaceReplayBuf {
    int data[0x17e8 / 4];
};

extern "C" {
extern GameState (&g_GameState);
extern RaceMem (&g_Race_IsDemo);
void RaceText_Load(void);
void RaceFont_Load(int tpage);
void RaceMem_ResetFlags(void *mem);
void Race_LoadSfxBank(void *mem, int bank);
int ArcadeRace_Run(int arg);
int GTRace_Run(int arg);
void RaceDevelopment_Run(int arg);
void RaceDevelopment_Run(int arg)
{
    RaceViewLoop view;

    view.v_Start(arg);
}

// Entry of the race overlay: sets up the race's memory and runs the mode's race.
void Race_Entry(int arg)
{
    int next;
    GameState *state;
    RaceSetup *setup;
    RaceMem *race;
    unsigned int flags;
    int slot;
    int garage;

    RaceText_Load();
    next = 1;
    state = &g_GameState;
    setup = (RaceSetup *)((char *)state + 0xbf7c);
    garage = setup->unk582;
    slot = setup->garageSlot;
    flags = setup->flags & ~8;
    setup->flags = flags;
    if ((garage >= 0) & (slot >= 0)) {
        Garage *g = &state->garages[garage];

        setup->flags = (flags & ~8) | ((g->cars[slot].flags98 & 0x4000) != 0) << 3;
    }
    switch (setup->raceType) {
    case 6:
        *(RaceReplayBuf *)rt::from_guest((((int)rt::to_guest(&g_Race_IsDemo) + 0xc5397) & ~3)) =
            *(RaceReplayBuf *)rt::from_guest((((int)rt::to_guest(state) + 0xc6c0) & ~3));
        break;
    case 1:
        *(RaceReplayBuf *)rt::from_guest((((int)rt::to_guest(&g_Race_IsDemo) + 0xc5397) & ~3)) =
            *(RaceReplayBuf *)rt::from_guest((((int)rt::to_guest(state) + 0x10bd8) & ~3));
        break;
    case 10:
        *(RaceReplayBuf *)rt::from_guest((((int)rt::to_guest(&g_Race_IsDemo) + 0xc5397) & ~3)) =
            *(RaceReplayBuf *)rt::from_guest((((int)rt::to_guest(state) + 0xc6c0) & ~3));
        break;
    }
    RaceFont_Load(6);
    race = &g_Race_IsDemo;
    RaceMem_ResetFlags(race);
    g_Race_IsDemo.isDemo = arg;
    Race_LoadSfxBank(race, 0xed);
    switch (setup->game) {
    case 0:
        next = ArcadeRace_Run(arg);
        break;
    case 1:
        next = GTRace_Run(arg);
        break;
    case 2:
        RaceDevelopment_Run(arg);
        break;
    }
    SpuAlloc_Restore((SpuBank *)((char *)&g_Race_IsDemo + 0xd4394));
    Overlay_Start(next);
}

// Default entry of the race overlay (Overlay_Start(0)): goes straight back to OVL1.
void Race_DefaultEntry(void)
{
    Overlay_Start(1);
}

}
