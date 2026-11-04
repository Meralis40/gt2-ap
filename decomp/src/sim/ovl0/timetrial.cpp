// OVL0 800122c4-80012cd4 (timetrial): Time trial ghosts and lap logs.
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

// A lap's record in a time trial session: its header and input log.
struct TTLapHead {
    int lap;
    unsigned int time;
    char unk8[0xd8];
};

struct TTLapSlot {
    TTLapHead head;
    int log[0x101c / 4];
};

// The session's laps in g_GameState (0xc5a8): the last four laps, used as a ring.
struct TTSession {
    RaceRecord record;
    short used;
    short slot;
    TTLapSlot slots[4];
};

static inline TTSession *TTSession_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xc5a8;

    return (TTSession *)(base + offset);
}

// Blocks copied whole.
struct TTWheel {
    int data[4];
};

struct CarPhysHead {
    int data[0x45c / 4];
};

struct RaceCarHead {
    int data[0x8c / 4];
};

struct RaceCarInit {
    short data[0xe0];
};

struct TTLapLog {
    short data[0x342 / 2];
};

struct TTInputLog {
    int data[0x101c / 4];
};

#define CARPHYS_FIELD(car, type, off) (*(type *)((char *)(car) + (off)))

extern "C" {
extern RaceMem g_Race_IsDemo;
extern int g_TT_BuffersInit;
extern unsigned char g_TT_HasBestLap;
extern unsigned char g_TT_GhostReady;
void CarPhys_LinkTables(CarPhys *car);
void InputLog_Close(InputLog *log, int unused);
void GhostPlay_RestartLap(CarPhys *car, short lap, short section);
void GhostRec_StartLap(CarPhys *car, short lap, int a2);
void *TimeTrial_GetShownLap(void);
void TimeTrial_CopyPlayerPhys(CarPhys *dst);
int TimeTrial_NextLapSlot(void);

static inline RaceMem *Race_Get(void)
{
    return &g_Race_IsDemo;
}
extern unsigned char g_TT_GhostState;
extern unsigned char g_TT_BestLapLog;
extern unsigned char g_TT_LapLog;
extern RaceWorld g_RaceWorld;

// &g_TT_GhostState (0xc4 bytes, cleared by GhostPlayer_Clear).
void *TimeTrial_GetGhostState(void)
{
    return &g_TT_GhostState;
}

void *TimeTrial_GetLapSlot(void)
{
    TTSession *s = TTSession_Get();
    int slot = s->slot;
    TTLapSlot *slots = s->slots;

    return &slots[slot];
}

#ifdef NON_MATCHING
void *TimeTrial_GetShownLap(void)
{
    RaceMem *race = &g_Race_IsDemo;
    void *lap;

    if (race->world.isReplay != 0) {
        char *base = (char *)&g_GameState;
        int offset = 0xc6a8;
        TTLapSlot *slots = (TTLapSlot *)(base + offset);

        lap = &slots[(signed char)race->world.vehicles[0].ghostLap];
    } else {
        int offset = 0x10bc0;
        char *base = (char *)&g_GameState;

        lap = base + offset;
    }
    return lap;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_GetShownLap);
#endif

// &g_TT_BestLapLog (0x342 bytes; +1 state, +2 frame count).
void *TimeTrial_GetBestLapLog(void)
{
    return &g_TT_BestLapLog;
}

// &g_TT_LapLog (0x342 bytes) of the lap being driven.
void *TimeTrial_GetLapLog(void)
{
    return &g_TT_LapLog;
}

int Race_IsTimeTrial(void)
{
    return RaceSetup_Get()->raceType == 6;
}

void TimeTrial_CopyPlayerPhys(CarPhys *dst)
{
    RaceMem *race = &g_Race_IsDemo;

    *(CarPhysHead *)dst = *(CarPhysHead *)&race->world.vehicles[0].phys;
    CarPhys_LinkTables(dst);
}

void TimeTrial_InitBuffers(void)
{
    GhostPlayer_Clear(TimeTrial_GetGhostState());
    if (g_TT_BuffersInit == 0) {
        int replay = g_RaceWorld.isReplay != 0;
        int one = 1;

        g_TT_BuffersInit = one;
        if ((replay ^ one) != 0) {
            GhostRecord_Clear(TimeTrial_GetShownLap());
            GhostRecState_Clear(TimeTrial_GetBestLapLog());
        }
        if (replay == 0) {
            GhostRecord_Clear(TimeTrial_GetLapSlot());
            GhostRecState_Clear(TimeTrial_GetLapLog());
        }
    }
}

// Rewinds the ghost car's input log for playback and the best lap log frame count.
void TimeTrial_RestartGhost(void)
{
    void *pvVar1;

    InputLog_Reset(g_RaceWorld.vehicles[1].inputLog, 1, 0x1000);
    pvVar1 = TimeTrial_GetBestLapLog();
    *(unsigned short *)((int)pvVar1 + 2) = 0;
}

void TimeTrial_StepGhost(void)
{
    RaceMem *race = &g_Race_IsDemo;
    int input[4];
    unsigned short *log;

    RaceVehicle_GetLoggedInput(&race->world.vehicles[1], input);
    log = (unsigned short *)TimeTrial_GetBestLapLog();
    log[1]++;
}

#ifdef NON_MATCHING
int TimeTrial_GhostEnded(void)
{
    RaceMem *race = &g_Race_IsDemo;

    if (race->world.isReplay == 0) {
        InputLog *log = race->world.vehicles[1].inputLog;

        if (log != 0) {
            return log->ended != 0;
        } else {
            return 1;
        }
    }
    return 0;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_GhostEnded);
#endif

int TimeTrial_NextLapSlot(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xc5a8;
    TTSession *s = (TTSession *)(base + offset);
    int four = 4;
    int used;
    int slot;

    slot = s->slot;
    used = s->used;
    slot++;
    if (slot == four) {
        slot = 0;
    }
    used++;
    if (used >= 4) {
        used = 4;
    }
    s->slot = slot;
    s->used = used;
    return slot;
}

#ifdef NON_MATCHING
void TimeTrial_EndSession(void)
{
    TTSession *s = TTSession_Get();
    int used = s->used;
    int slot = s->slot;

    if (used >= 4) {
        TTLapSlot sorted[4];
        int i = 0;

        if (used > 0) {
            char *src = (char *)s + slot * (int)sizeof(TTLapSlot);

            do {
                sorted[i] = ((TTSession *)src)->slots[0];
                slot++;
                src += sizeof(TTLapSlot);
                if (slot >= 4) {
                    src = (char *)s;
                    slot = 0;
                }
                i++;
            } while (i < used);
        }
        {
            TTLapSlot *dst = s->slots;
            int left = 3;
            TTLapSlot *src = sorted;

            do {
                *dst = *src;
                dst++;
                left--;
                src++;
            } while (left != -1);
        }
        s->slot = 0;
    }
    if (g_TT_HasBestLap != 0) {
        RaceMem *race;
        RaceWorld *world;
        RaceSetup *setup;

        g_TT_GhostReady = 1;
        race = &g_Race_IsDemo;
        world = &race->world;
        world->vehicles[1] = world->vehicles[0];
        setup = RaceSetup_Get();
        world->carModels[1] = world->carModels[0];
        setup->cars[1] = setup->cars[0];
        {
            char *base = (char *)&g_GameState;
            int offset = 0xc0a8;
            int dst;
            int src;

            ((RaceCar *)(base + offset))->driverType = 2;
            dst = 0x1519a;
            src = 0x14fda;
            *(RaceCarInit *)(base + dst) = *(RaceCarInit *)(base + src);
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_EndSession);
#endif

#ifdef NON_MATCHING
void TimeTrial_OnLap(CarPhys *car, unsigned int time, short lap, int a3, int noGhost, int restart)
{
    RaceMem *race;

    if (car->carIndex != 0) {
        return;
    }
    race = &g_Race_IsDemo;
    if (race->world.isReplay == 0) {
        unsigned int lapTime;
        RaceVehicle *player = &race->world.vehicles[0];
        RaceVehicle *ghost = &race->world.vehicles[1];

        InputLog_Close(player->inputLog, 0);
        if (time != 0x405f7dfd) {
            unsigned char better = 0;

            lapTime = time / 3 - CARPHYS_FIELD(car, int, 0x780);
            if (noGhost == 0) {
                if (ghost->present == 0 ||
                    lapTime < ((TTLapSlot *)TimeTrial_GetShownLap())->head.time) {
                    better = 1;
                }
            }
            if (better) {
                TTLapSlot *slot;
                RaceSetup *setup;
                int i;
                TTWheel *from;
                TTWheel *to;

                g_TT_HasBestLap = 1;
                slot = (TTLapSlot *)TimeTrial_GetLapSlot();
                setup = RaceSetup_Get();
                slot->head.time = lapTime;
                *(RaceCarHead *)&setup->cars[1] = *(RaceCarHead *)&setup->cars[0];
                {
                    char *base = (char *)&g_GameState;
                    int dst = 0x1519a;
                    int src = 0x14fda;

                    *(RaceCarInit *)(base + dst) = *(RaceCarInit *)(base + src);
                }
                {
                    TTLapHead *shown = (TTLapHead *)TimeTrial_GetShownLap();

                    *shown = *(TTLapHead *)TimeTrial_GetLapSlot();
                }
                {
                    TTLapLog *best = (TTLapLog *)TimeTrial_GetBestLapLog();

                    *best = *(TTLapLog *)TimeTrial_GetLapLog();
                }
                *(TTInputLog *)ghost->inputLog = *(TTInputLog *)player->inputLog;
                TimeTrial_CopyPlayerPhys(&Race_Get()->world.vehicles[1].phys);
                RaceVehicle_StopSound(ghost);
                to = (TTWheel *)ghost->wheelDraw;
                i = 3;
                from = (TTWheel *)player->wheelDraw;
                ghost->model = player->model;
                do {
                    *to = *from;
                    to++;
                    i--;
                    from++;
                } while (i != -1);
                ghost->present = 1;
                ghost->hidden = 0;
                RaceVehicle_StartSound(ghost);
            }
        }
        if (restart != 0) {
            TTSession *s;
            InputLog *log;

            if (ghost->present != 0) {
                GhostPlay_RestartLap(&Race_Get()->world.vehicles[1].phys, car->lap, lap);
            }
            s = TTSession_Get();
            log = (InputLog *)&s->slots[TimeTrial_NextLapSlot()].log;
            player->inputLog = log;
            InputLog_Reset(log, 0, 0x1000);
            GhostRec_StartLap(car, lap, a3);
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/timetrial", TimeTrial_OnLap);
#endif
}
