// OVL0 80014e28-800157a8 (raceworld): Race world and the lap callbacks.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline int RaceSetup_FrameStep(void)
{
    return RaceSetup_Get()->frameStep;
}

static inline int RaceSetup_RaceType(void)
{
    return RaceSetup_Get()->raceType;
}

// The second car of g_RaceSetup, reached the same way.
static inline RaceCar *RaceSetup_GetCar1(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xc0a8;

    return (RaceCar *)(base + offset);
}

// A car's start data in g_GameState (0x1c0 bytes), copied whole, from 0x14fda.
struct RaceCarInit {
    short data[0xe0];
};

// RaceVehicle's fields from 0x880, as the HUD callbacks reach them.
struct RaceCarHud {
    unsigned char unk0[0x20c];
    byte lapInvalid;
    byte message;
    short lapTimeTimer;
    short refTimeTimer;
    short messageTimer;
    int lapTimeShown;
    void *lapLabel;
    int refTime;
    int refTime2;
};

extern "C" {
extern RaceMem g_Race_IsDemo;
extern RaceWorld g_RaceWorld;
extern RaceViewLoop *g_RaceView;
extern int g_FrameRate;
extern int g_FrameDt;
extern int g_TT_GhostResync;
void RaceVehicle_UpdateSound(RaceVehicle *car, int rank, RaceCam *cam);
void RaceVehicle_Init(RaceVehicle *car, int index, int init);
void RaceVehicle_Shutdown(RaceVehicle *car);
void RaceVehicle_Draw(RaceVehicle *car, RaceCam *cam, int mirror);
void RaceVehicle_OnLap(RaceVehicle *car, int lap, int time, int invalid);
void RaceVehicle_OnLapDone(RaceVehicle *car, int lap, int best, int time, int extra);
void Race_InitPhysicsParams(void);
void Race_InitCarState(void);
void TimeTrial_NextLapSlot(void);
void TimeTrial_EndSession(void);
void Particles_Draw(ParticlePool *pool, RaceCam *cam);
void RaceMusic_PlayFinish(RaceViewLoop *view);

// Clears the world, stores isReplay and the course record of the course
// (CourseRecord_GetCurrent).
void RaceWorld_Init(RaceWorld *w, unsigned char isReplay)
{
    CourseRecord *pCVar1;

    memset(w, 0, 0x5d30);
    w->isReplay = isReplay;
    pCVar1 = CourseRecord_GetCurrent();
    w->courseRecord = pCVar1;
}

#ifdef NON_MATCHING
void RaceWorld_UpdateSounds(RaceWorld *w, RaceCam *cams)
{
    int rank = 0;
    RaceVehicle *car = w->drawList;

    for (; car != 0; car = car->next) {
        rank++;
        RaceVehicle_UpdateSound(car, rank, cams);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_UpdateSounds);
#endif

void RaceWorld_UpdateSounds2P(RaceWorld *w, RaceCam *cams)
{
    int i;

    for (i = 0; i < 2; i++) {
        RaceVehicle *car = &w->vehicles[i];

        RaceVehicle_UpdateSound(car, 0, &cams[i]);
    }
}

void RaceWorld_MuteSounds(RaceWorld *w)
{
    int i;

    for (i = 0; i < g_RaceWorld.carCount; i++) {
        RaceVehicle *car = &w->vehicles[i];

        if (car->present != 0) {
            RaceVehicle_MuteSound(car);
        }
    }
}

void RaceWorld_UnmuteSounds(RaceWorld *w)
{
    int i;

    for (i = 0; i < g_RaceWorld.carCount; i++) {
        RaceVehicle *car = &w->vehicles[i];

        if (car->present != 0) {
            RaceVehicle_UnmuteSound(car);
        }
    }
}

#ifdef NON_MATCHING
void RaceWorld_SetupCars(RaceWorld *w)
{
    RaceSetup *setup;

    if (Course_Get(w->course)->flags & 0x20) {
        RaceSetup_Get()->lapCount = 1;
    }
    setup = RaceSetup_Get();
    w->carCount = setup->carCount;
    if (Race_IsTimeTrial()) {
        if (g_RaceWorld.isReplay == 0) {
            RaceCar *ghost = &setup->cars[1];
            char *base;
            int offset;
            int dst;
            int src;

            if (ghost->active == 0) {
                *ghost = setup->cars[0];
                base = (char *)&g_GameState;
                offset = 0xc0a8;
                ((RaceCar *)(base + offset))->active = 0;
                dst = 0x1519a;
                src = 0x14fda;
                *(RaceCarInit *)(base + dst) = *(RaceCarInit *)(base + src);
            }
            w->carCount = 2;
        }
        RaceSetup_GetCar1()->driverType = 2;
    } else {
        int one = 1;
        int two = 0;

        if (setup->carCount == one) {
            int active = setup->cars[1].active;

            two = active != 0;
        }
        if (two) {
            int type = setup->raceType;

            if (type != one && type != 10) {
                setup->cars[0].gridSlot = 0;
                setup->cars[1].driverType = 2;
                w->carCount = 2;
            }
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", RaceWorld_SetupCars);
#endif

void RaceWorld_Start(RaceWorld *w)
{
    RaceMem *race;
    RaceWorld *world;
    RaceWorld *counted;
    int i;
    char *state;

    switch (RaceSetup_FrameStep()) {
    case 1:
        g_FrameRate = 60;
        g_FrameDt = 0x444;
        break;
    case 0:
    case 2:
        g_FrameRate = 30;
        g_FrameDt = 0x888;
        break;
    }
    race = &g_Race_IsDemo;
    world = &race->world;
    if (world->isReplay == 0) {
        char *base = (char *)&g_GameState;

        RaceRecord_Init((RaceRecord *)(base + 0xc5a8));
        RaceRecord_Init((RaceRecord *)(base + 0x10ac0));
        RaceResult_Init(base + 0xc508);
    }
    i = 0;
    Race_InitPhysicsParams();
    g_TT_GhostResync = 0;
    counted = world;
    state = (char *)&g_GameState;
    for (; i < counted->carCount; i++) {
        RaceVehicle *car = &w->vehicles[i];
        int offset = i * 0x1c0 + 0x14fda;

        RaceVehicle_Init(car, i, offset + (int)state);
    }
    Race_InitCarState();
    PadDevice_FlushInput(&w->pads[0].dev);
    PadDevice_FlushInput(&w->pads[1].dev);
}

void RaceWorld_Shutdown(RaceWorld *w)
{
    int i;
    int ended;

    for (i = 0; i < g_RaceWorld.carCount; i++) {
        RaceVehicle *car = &w->vehicles[i];

        RaceVehicle_Shutdown(car);
    }
    ended = 0;
    if (g_RaceWorld.isReplay == 0) {
        ended = Race_IsTimeTrial() != 0;
    }
    if (ended) {
        TimeTrial_NextLapSlot();
        TimeTrial_EndSession();
    }
}

void RaceWorld_DrawCars(RaceWorld *w, RaceCam *cam, int mirror)
{
    RaceMem *race;
    RaceWorld *world;
    int i;

    if (mirror == 0) {
        g_RaceWorld.drawList = 0;
    }
    i = 0;
    race = &g_Race_IsDemo;
    world = &race->world;
    for (; i < world->carCount; i++) {
        RaceVehicle *car = &w->vehicles[i];

        RaceVehicle_Draw(car, cam, mirror);
    }
    if (mirror == 0) {
        Particles_Draw(&race->world.particles, cam);
    }
}

#ifdef NON_MATCHING
void Race_OnLapCb(int index, int lap, int time, int invalid)
{
    RaceMem *race = &g_Race_IsDemo;
    RaceVehicle *car = &race->world.vehicles[index];

    invalid = invalid != 0;
    car->lapInvalid = invalid;
    RaceVehicle_OnLap(car, lap, time, invalid);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_OnLapCb);
#endif

#ifdef NON_MATCHING
void Race_OnLapDoneCb(int index, int lap, int time, int best, unsigned short extra, int invalid)
{
    RaceMem *race = &g_Race_IsDemo;
    RaceVehicle *car = &race->world.vehicles[index];

    car->lapInvalid = invalid != 0;
    RaceVehicle_OnLapDone(car, lap, best, time, extra);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_OnLapDoneCb);
#endif

#ifdef NON_MATCHING
void Race_ShowGap(int index, int gap)
{
    switch (RaceSetup_RaceType()) {
    case 0: {
        int other = 1 - index;
        RaceMem *race = &g_Race_IsDemo;
        RaceWorld *world = &race->world;
        RaceCarHud *hud = (RaceCarHud *)world->vehicles[other].unk880;

        hud->refTime = gap;
        hud->refTimeTimer = 0x78;
        hud->refTime2 = 0;
        {
            RaceCarHud *own = (RaceCarHud *)world->vehicles[index].unk880;

            own->refTime = 0;
            own->refTimeTimer = 0x78;
            own->refTime2 = gap;
        }
        return;
    }
    case 2:
    case 4:
    case 11:
        if (index == 0) {
            RaceMem *race = &g_Race_IsDemo;
            RaceCarHud *hud = (RaceCarHud *)race->world.vehicles[0].unk880;

            hud->refTime = 0;
            hud->refTimeTimer = 0x78;
            hud->refTime2 = gap;
            return;
        }
        break;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_ShowGap);
#endif

// Empty race logic callback.
void Race_Nop(void)
{
}

#ifdef NON_MATCHING
void Race_ShowMessage(int index, int message)
{
    RaceMem *race = &g_Race_IsDemo;
    RaceCarHud *hud = (RaceCarHud *)race->world.vehicles[index].unk880;

    hud->messageTimer = 0x78;
    hud->message = message;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceworld", Race_ShowMessage);
#endif

void Race_OnFinish(int index, int unused, int place, int time, unsigned short extra)
{
    char *state = (char *)&g_GameState;
    int *result = (int *)(state + 0xc508);
    RaceMem *race;
    RaceWorld *world;
    RaceRecord *record;

    result[1] = place;
    result[2] = time;
    race = &g_Race_IsDemo;
    world = &race->world;
    world->vehicles[index].finishTime = -1;
    record = (RaceRecord *)(state + 0xc5a8);
    if (place == 1) {
        RaceRecord_AddLap(record, 1, time, extra, 0);
    }
    if (world->isReplay == 0) {
        Result_Start((short *)((char *)race + 0x5d26));
        RaceMusic_PlayFinish(g_RaceView);
    }
}
}
