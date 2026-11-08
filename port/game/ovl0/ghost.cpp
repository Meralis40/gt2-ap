// OVL0 8003ef18-8003fe8c (ghost): Ghost recording and playback.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"
#include "gamestate.h"

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// Nine words of a car's pose.
struct GhostPose {
    int data[9];
};

// A car's pose at one ghost frame.
struct GhostFrame {
    int pos[3];
    int rot[6];
    int posB[2];
    int y;
    int rotB[6];
    int trackPos;
    int unk4c;
    short angles[4];
    short angleX;
    short angleZ;
};

// The ghost player: the two frames it interpolates between.
struct GhostState {
    GhostFrame frames[2];
    short index;
    short pending;
    short lap;
    short weight;
    int clock;
};

// The part of CarPhys a lap log keeps.
struct CarTail {
    unsigned char data[0x33c];
};

// A lap's log.
struct GhostLog {
    unsigned char state;
    unsigned char ok;
    unsigned short count;
    unsigned char pad4[2];
    CarTail tail;
};

// A lap's stored start.
struct GhostLap {
    short time;
    unsigned char clock;
    signed char phase;
    char pad4[4];
    char start[0xd8];
};

#define CAR_POSE(car) (*(GhostPose *)&(car)->pos)
#define CAR_POSE_B(car) (*(GhostPose *)((char *)(car) + 0x680))
#define CAR_TAIL(car) (*(CarTail *)&(car)->carIndex)
#define CAR_CLOCK(car) ((car)->raceTime)

extern "C" {
extern RaceMem (&g_Race_IsDemo);
extern unsigned int (&g_RaceClock);
extern unsigned char (&g_RaceCourseId);
void *TimeTrial_GetShownLap(void);
void TimeTrial_StepGhost(void);
void CarPhys_RestoreState(CarPhys *car, void *state);
int Ghost_SumHeader(unsigned char *p);
int Angle_Lerp(int w0, int a, int w1, int b);
void GhostFrame_Capture(GhostFrame *frame, CarPhys *car);
void GhostFrame_Apply(CarPhys *car, GhostFrame *frame);
int TrackDist_Lerp(short w0, int a, short w1, int b);
void GhostPlay_RestartLap(CarPhys *car, int lap, short section);

static inline RaceMem *Race_Get(void)
{
    return &g_Race_IsDemo;
}

static inline CourseData *RaceCourse_Get(void)
{
    char *base = (char *)&g_Race_IsDemo;
    int offset = 0xb534;

    return (CourseData *)(base + offset);
}

int Ghost_SumHeader(unsigned char *p)
{
    int sum = 0;
    unsigned int i;

    for (i = 0; i < 0xd8; i++) {
        sum += *p++;
    }
    return sum;
}

// Starts the best lap's ghost, if its data is there.
int GhostCar_Init(CarPhys *car)
{
    GhostLog *log = (GhostLog *)TimeTrial_GetBestLapLog();
    GhostLap *lap = (GhostLap *)TimeTrial_GetShownLap();
    unsigned char *start;
    unsigned char ghost;
    unsigned char index;

    log->state = 3;
    TimeTrial_RestartGhost();
    start = (unsigned char *)lap->start;
    if (Ghost_SumHeader(start) == 0) {
        log->ok = 0;
    }
    if (log->ok == 0) {
        return 0;
    }
    memset(&log->tail, 0, 0x33c);
    log->ok = 1;
    ghost = car->ghost;
    index = car->carIndex;
    CarPhys_RestoreState(car, start);
    car->ghost = ghost;
    car->carIndex = index;
    return 1;
}

// Weighted angle, the short way round.
int Angle_Lerp(int w0, int a, int w1, int b)
{
    int diff;

    a = Angle_Wrap(a);
    b = Angle_Wrap(b);
    diff = a - b;
    if (diff >= 0x800) {
        b += 0x1000;
    } else if (diff < -0x800) {
        a += 0x1000;
    }
    return Angle_Wrap((w0 * a + w1 * b) / 0x1000);
}

void GhostFrame_Capture(GhostFrame *frame, CarPhys *car)
{
    int i;

    *(GhostPose *)frame->pos = CAR_POSE(car);
    *(GhostPose *)frame->posB = CAR_POSE_B(car);
    frame->trackPos = car->trackPos;
    for (i = 0; i < 3; i++) {
        frame->angles[i] = (&car->pitch)[i];
    }
    frame->angleX = car->unk6f4;
    frame->angleZ = car->unk6f6;
}

void GhostFrame_Apply(CarPhys *car, GhostFrame *frame)
{
    CAR_POSE(car) = *(GhostPose *)frame->pos;
    CAR_POSE_B(car) = *(GhostPose *)frame->posB;
    car->trackPos = frame->trackPos;
}

// Weighted course distance, across the start line if that is closer.
int TrackDist_Lerp(short w0, int a, short w1, int b)
{
    int length = RaceCourse_Get()->sections->unk00;
    int half = length / 2;
    int sum;

    if (a + half < b) {
        sum = Fixed_Mul(w0, a) + Fixed_Mul(w1, b - length);
        if (sum < 0) {
            sum += length;
        }
    } else if (b + half < a) {
        sum = Fixed_Mul(w0, a - length) + Fixed_Mul(w1, b);
        if (sum < 0) {
            sum += length;
        }
    } else {
        sum = Fixed_Mul(w0, a) + Fixed_Mul(w1, b);
    }
    return sum;
}

// Places the ghost between its two frames.
void GhostCar_Interpolate(int carIdx)
{
    RaceVehicle *vehicles = Race_Get()->world.vehicles;
    GhostState *g = (GhostState *)TimeTrial_GetGhostState();
    CarPhys *car = &vehicles[carIdx].phys;
    GhostFrame *prev = &g->frames[1 - g->index];
    GhostFrame *cur = &g->frames[g->index];
    short w1;
    short w0;
    int i;
    int a0;
    int a1;
    int a2;
    int a3;

    GhostFrame_Capture(prev, car);
    w1 = g->weight;
    w0 = 0x1000 - w1;
    for (i = 0; i < 3; i++) {
        car->pos[i] = Fixed_Mul(w0, cur->pos[i]) + Fixed_Mul(w1, prev->pos[i]);
    }
    a0 = Angle_Lerp(w0, cur->angles[0], w1, prev->angles[0]);
    a1 = Angle_Lerp(w0, cur->angles[1], w1, prev->angles[1]);
    a2 = Angle_Lerp(w0, cur->angles[2], w1, prev->angles[2]);
    Matrix_FromAngles((short *)((char *)car + 0x668), (short *)((char *)car + 0x670), (short *)((char *)car + 0x678), a0,
                      a1, a2);
    *(int *)((char *)car + 0x680) = car->pos[0];
    *(int *)((char *)car + 0x684) = car->pos[1];
    *(int *)((char *)car + 0x688) = Fixed_Mul(w0, cur->y) + Fixed_Mul(w1, prev->y);
    a0 = Angle_Lerp(w0, cur->angleX, w1, prev->angleX);
    a3 = Angle_Lerp(w0, cur->angleZ, w1, prev->angleZ);
    Matrix_FromAngles((short *)((char *)car + 0x68c), (short *)((char *)car + 0x694), (short *)((char *)car + 0x69c), a0,
                      a3, a2);
    car->trackPos = TrackDist_Lerp(w0, cur->trackPos, w1, prev->trackPos);
}

void GhostCar_ApplyFrame(int carIdx)
{
    RaceVehicle *vehicles = Race_Get()->world.vehicles;
    GhostState *g = (GhostState *)TimeTrial_GetGhostState();
    CarPhys *car = &vehicles[carIdx].phys;

    GhostFrame_Apply(car, &g->frames[1 - g->index]);
    g->index = 1 - g->index;
}

void GhostRec_StartLap(CarPhys *car, short time, int clock)
{
    GhostLog *log = (GhostLog *)TimeTrial_GetLapLog();
    GhostLap *lap = (GhostLap *)TimeTrial_GetLapSlot();
    unsigned int ticks;

    CarPhys_SaveState((int *)lap->start, car);
    log->ok = 1;
    log->state = 1;
    log->count = 0;
    lap->time = time;
    lap->clock = clock;
    if (RaceSetup_Get()->frameStep == 1) {
        ticks = g_RaceClock + 50;
    } else {
        ticks = g_RaceClock + 100;
    }
    lap->phase = ticks % 3;
}

// Same as GhostRec_StartLap with mode 2 and no lap time.
void GhostRec_StartFree(CarPhys *car)
{
    unsigned char *puVar1;
    unsigned short *puVar2;

    puVar1 = (unsigned char *)TimeTrial_GetLapLog();
    puVar2 = (unsigned short *)TimeTrial_GetLapSlot();
    CarPhys_SaveState((int *)(puVar2 + 4), car);
    puVar1[1] = 2;
    *puVar1 = 1;
    *(unsigned short *)(puVar1 + 2) = 0;
    *puVar2 = 0;
    *(unsigned char *)(puVar2 + 1) = 0;
    *(unsigned char *)rt::from_guest(((int)rt::to_guest(puVar2) + 3)) = 0;
}

// Starts the shown lap's ghost, or holds it for the next lap if it is longer.
void GhostPlay_RestartLap(CarPhys *car, int lap, short section)
{
    GhostLog *log = (GhostLog *)TimeTrial_GetBestLapLog();
    GhostLap *shown = (GhostLap *)TimeTrial_GetShownLap();
    GhostState *g = (GhostState *)TimeTrial_GetGhostState();
    unsigned char ghost;
    unsigned char index;
    int control;
    unsigned int clock;
    int delta;

    if (g->pending == 0) {
        unsigned short time = shown->time;

        if (shown->time >= section) {
            g->weight = time - section;
        } else {
            short wrapped = section - 0x1000;

            g->weight = time - wrapped;
            g->lap = lap;
            g->pending = 2;
            return;
        }
    } else {
        lap = g->lap;
        g->pending = 0;
    }
    g->index = 0;
    ghost = car->ghost;
    index = car->carIndex;
    control = *(signed char *)&car->control;
    CarPhys_RestoreState(car, shown->start);
    GhostFrame_Capture(&g->frames[g->index], car);
    CAR_TAIL(car) = log->tail;
    GhostFrame_Capture(&g->frames[g->index], car);
    car->ghost = ghost;
    car->carIndex = index;
    car->control = control;
    car->lap = lap;
    log->state = 2;
    TimeTrial_RestartGhost();
    TimeTrial_StepGhost();
    clock = g_RaceClock;
    delta = shown->phase - clock % 3;
    g->clock = delta;
    CAR_CLOCK(car) = (clock + delta - shown->clock) / 3;
}

void GhostPlay_Start(CarPhys *car)
{
    GhostLog *log = (GhostLog *)TimeTrial_GetBestLapLog();
    GhostLap *shown = (GhostLap *)TimeTrial_GetShownLap();
    GhostState *g = (GhostState *)TimeTrial_GetGhostState();
    unsigned char ghost;
    unsigned char index;
    int control;

    g->pending = 0;
    g->lap = 0;
    g->weight = 0;
    g->index = 0;
    ghost = car->ghost;
    index = car->carIndex;
    control = *(signed char *)&car->control;
    CarPhys_RestoreState(car, shown->start);
    GhostFrame_Capture(&g->frames[g->index], car);
    GhostFrame_Capture(&g->frames[1 - g->index], car);
    car->ghost = ghost;
    car->carIndex = index;
    car->control = control;
    log->state = 2;
    TimeTrial_RestartGhost();
    if (car->lap == 0) {
        g_RaceClock = 0;
        g->clock = 0;
        CAR_CLOCK(car) = 0;
    } else {
        unsigned int clock;
        int delta;

        g_RaceClock = 180000;
        clock = g_RaceClock;
        delta = shown->phase;
        g->clock = delta;
        CAR_CLOCK(car) = (clock + delta - shown->clock) / 3;
    }
    log->ok = 2;
}

// 2 with no ghost, 1 once it is over, and on loop courses 2 when it has run out.
int Ghost_ShouldHold(void)
{
    GhostLog *log = (GhostLog *)TimeTrial_GetBestLapLog();
    int ended;

    if (log->ok == 0) {
        return 2;
    }
    if (*(unsigned short *)log == 0x203) {
        return 1;
    }
    ended = 0;
    if ((Course_Get(g_RaceCourseId)->flags & 0x20) == 0) {
        ended = TimeTrial_GhostEnded() != 0;
    }
    return ended * 2;
}

void Ghost_CommitLap(CarPhys *car)
{
    GhostLog *lapLog = (GhostLog *)TimeTrial_GetLapLog();
    GhostLog *best = (GhostLog *)TimeTrial_GetBestLapLog();
    GhostState *g = (GhostState *)TimeTrial_GetGhostState();
    RaceVehicle *vehicles = Race_Get()->world.vehicles;

    if (car->ghost == 2 && best->ok == 1) {
        best->tail = CAR_TAIL(car);
        best->ok = 2;
    }
    if (car->ghost == 0) {
        if (g->pending != 0) {
            if (g->pending == 1) {
                GhostPlay_RestartLap(&vehicles[1].phys, 0, 0);
            } else {
                g->pending--;
            }
        }
        if (lapLog->state == 1 && lapLog->count == 1) {
            lapLog->tail = CAR_TAIL(car);
            lapLog->ok = 2;
        }
    }
}

// Clears a 0x342-byte ghost recording state.
void GhostRecState_Clear(void *state)
{
    memset(state, 0, 0x342);
    *(unsigned char *)state = 0;
    *(unsigned char *)rt::from_guest(((int)rt::to_guest(state) + 1)) = 0;
}

// Clears a ghost record header (0xe0 bytes) and sets its time to 359999999.
void GhostRecord_Clear(void *record)
{
    memset(record, 0, 0xe0);
    *(unsigned int *)rt::from_guest(((int)rt::to_guest(record) + 4)) = 359999999;
}

// Clears the ghost player (two 0x5c-byte frames, frame index, pending restart, weight
// 0x800, clock phase).
void GhostPlayer_Clear(void *player)
{
    memset(player, 0, 0xb8);
    *(unsigned short *)rt::from_guest(((int)rt::to_guest(player) + 0xb8)) = 0;
    *(unsigned short *)rt::from_guest(((int)rt::to_guest(player) + 0xbe)) = 0x800;
    *(unsigned short *)rt::from_guest(((int)rt::to_guest(player) + 0xba)) = 0;
    *(unsigned int *)rt::from_guest(((int)rt::to_guest(player) + 0xc0)) = 0;
}
}
