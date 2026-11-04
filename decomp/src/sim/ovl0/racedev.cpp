// OVL0 80011ec8-800122c4 (racedev): RaceDevelopment and the race entries.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

// The input logs a race starts from, copied whole.
struct RaceReplayBuf {
    int data[0x17e8 / 4];
};

extern "C" {
extern GameState g_GameState;
extern RaceMem g_Race_IsDemo;
void RaceText_Load(void);
void RaceFont_Load(int tpage);
void RaceMem_ResetFlags(void *mem);
void Race_LoadSfxBank(void *mem, int bank);
int ArcadeRace_Run(int arg);
int GTRace_Run(int arg);
void RaceDevelopment_Run(int arg);
extern void *RaceDevelopment__vtable[];
extern int RaceDevelopment__typeinfo[];
extern int RaceViewLoop__typeinfo[];
extern char D_8002EF08[];
void *RaceViewLoop__RaceViewLoop(void *self);
void RaceViewLoop__RaceViewLoop_dtor(void *self, int inCharge);
void RaceViewLoop__Start(void *self, int arg);
void *RaceViewLoop____tf(void);
void __rtti_si(void *ti, char *name, void *base);
void *RaceDevelopment__RaceDevelopment(void *self)
{
    RaceViewLoop__RaceViewLoop(self);
    *(void **)self = RaceDevelopment__vtable;
    return self;
}

void RaceDevelopment__RaceDevelopment_dtor(void *self, int inCharge)
{
    *(void **)self = RaceDevelopment__vtable;
    RaceViewLoop__RaceViewLoop_dtor(self, inCharge);
}

void RaceDevelopment_Run(int arg)
{
    char view[0x2f4];

    RaceDevelopment__RaceDevelopment(view);
    RaceViewLoop__Start(view, arg);
    RaceDevelopment__RaceDevelopment_dtor(view, 2);
}

#ifdef NON_MATCHING
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
        *(RaceReplayBuf *)(((int)&g_Race_IsDemo + 0xc5397) & ~3) =
            *(RaceReplayBuf *)(((int)state + 0xc6c0) & ~3);
        break;
    case 1:
        *(RaceReplayBuf *)(((int)&g_Race_IsDemo + 0xc5397) & ~3) =
            *(RaceReplayBuf *)(((int)state + 0x10bd8) & ~3);
        break;
    case 10:
        *(RaceReplayBuf *)(((int)&g_Race_IsDemo + 0xc5397) & ~3) =
            *(RaceReplayBuf *)(((int)state + 0xc6c0) & ~3);
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
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racedev", Race_Entry);
#endif

// Default entry of the race overlay (Overlay_Start(0)): goes straight back to OVL1.
void Race_DefaultEntry(void)
{
    Overlay_Start(1);
}

void *RaceDevelopment____tf(void)
{
    if (RaceDevelopment__typeinfo[0] == 0) {
        RaceViewLoop____tf();
        __rtti_si(RaceDevelopment__typeinfo, D_8002EF08, RaceViewLoop__typeinfo);
    }
    return RaceDevelopment__typeinfo;
}
}
