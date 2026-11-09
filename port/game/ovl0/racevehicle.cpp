// OVL0 80012cd4-80014b54 (racevehicle): Race vehicles: setup, transforms, input and
// sounds.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"
#include "ap.h"

// Three words, copied as one.
struct Vec3 {
    int v[3];
};

// Two words, copied as one: a drawn wheel's x, y, z.
struct short4 {
    short v[4];
};

// The car sizes RaceVehicle_Init hands on.
struct CarDims {
    char unk0[0x2d];
    unsigned char turbo;
};

// A body frame of the physics: position, then the three axes.
struct CarPose {
    int pos[3];
    short forward[4];
    short right[4];
    short up[4];
};

// What the input functions hand the physics.
struct CarControls {
    short flags;
    short steer;
    short brake;
    short throttle;
    signed char gear;
    unsigned char a;
    unsigned char b;
};

// One input packet of an InputLog.
struct RaceInput {
    unsigned char flags;
    unsigned char buttons;
    unsigned char steer;
    unsigned char throttle;
    unsigned char brake;
};

// A player's records: the laps, then the lap ghosts of a time trial.
struct RaceRecordSlot {
    char record[0xfc];
    short lapLogs;
    short unkfe;
    char pad100[0xe0];
    char logs[3][0x10fc];
    char pad[0x4518 - 0x1e0 - 3 * 0x10fc];
};

struct GhostLogHead {
    unsigned char state;
    unsigned char ok;
    unsigned short count;
};

// A car's HUD state, as the lap functions reach it.
struct VehicleHud {
    char pad[0x20c];
    unsigned char lapInvalid;
    unsigned char message;
    short lapTimeTimer;
    short refTimeTimer;
    short messageTimer;
    int lapTimeShown;
    game_pointer<void> lapLabel;
    int refTime;
    int refTime2;
};

extern "C" {
extern GameState (&g_GameState);
extern RaceSetup (&g_RaceSetup);
extern RaceMem (&g_Race_IsDemo);
extern RaceWorld (&g_RaceWorld);
extern game_pointer<RaceViewLoop> (&g_RaceView);
extern unsigned char (&D_800A951C);
extern unsigned short (&D_800A9522);
extern game_pointer<void> (&D_800A9524);
extern unsigned char (&D_800AF232);
extern unsigned char (&D_801C9995);
extern game_pointer<RaceVehicle> (&D_800ADA08);
extern char (&D_8017DDA0)[];
extern char (&D_8017DDB8)[];
extern unsigned char (&g_TT_HasBestLap);
extern int (&g_TT_GhostResync);
extern unsigned char (&g_Race_EndRequest);
extern game_pointer<void> (&g_LapLabels)[];
extern char (&sR_LapTime)[];
extern short (&g_PedalCurve)[];
extern signed char (&g_ChampPoints)[];
extern short (&g_SinTable)[];
extern int (&g_FrameRate);
void RaceVehicle_SetupModelDims(RaceVehicle *v, CarDims *dims);
void RaceVehicle_SetupWheels(RaceVehicle *v);
void RaceVehicle_InitLapGhosts(RaceVehicle *v);
int RaceVehicle_SelectGhostLap(RaceVehicle *v, int lap);
int RaceVehicle_GetInput(RaceVehicle *v, RacePad *pad, CarControls *out, int playback);
void Matrix_FromCarPose(MATRIX *m, CarPose *pose);
int CourseSect_FindNearest(CourseSectionTable *table, int *pos);
int CourseSect_Locate(CourseSectionTable *table, int index, int *pos);
void CarPhys_Setup(CarPhys *phys, CarDims *dims, int section, int x, int z, int sin, int cos, int control, int unk8f,
                   int phase, int zero, int index, int advance);
void InputLog_Read(InputLog *log, RaceInput *out);
void InputLog_Write(InputLog *log, RaceInput *in);
void InputLog_Close(InputLog *log, int discard);
Particle *Particles_Spawn(ParticlePool *pool, int kind);
void Ai_EnterDefaultMode(CarPhys *car);
void RaceMusic_PlayFinish(RaceViewLoop *view);
void CarSound_Init(CarSound *cs, SpuBank *banks, unsigned char mode);
void CarSound_PlayOneShot(CarSound *cs, int id, int a, int b);
void CarSound_Update(CarSound *cs);
int SVector_Dot(SVECTOR *a, SVECTOR *b);

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline char *GameState_Base(void)
{
    return (char *)&g_GameState;
}

static inline RaceRecordSlot *RecordSlot(int index)
{
    char *base = (char *)&g_GameState;
    int offset = index * (int)sizeof(RaceRecordSlot) + 0xc5a8;

    return (RaceRecordSlot *)(base + offset);
}

static inline char *Race_Base(void)
{
    return (char *)&g_Race_IsDemo;
}

static inline RaceWorld *Race_World(void)
{
    return (RaceWorld *)(Race_Base() + 4);
}

static inline int InputLog_HasEnded(InputLog *log)
{
    if (log != 0) {
        return log->ended != 0;
    }
    return 1;
}

static inline int NonZero(int x)
{
    return x != 0;
}

static inline void Vec_Sub(int *dst, int *a, int *b)
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
}

void RaceVehicle_Init(RaceVehicle *v, int index, CarDims *dims)
{
    char *course = (char *)&g_Race_IsDemo + 0xb534;
    RaceSetup *setup = RaceSetup_Get();
    RaceCar *car = &setup->cars[index];
    int section = CourseSect_FindNearest(((CourseData *)course)->sections, (int *)(course + 0x58));
    int handicap = (signed char)setup->options[6];
    int single = 0;
    int slot;
    int phase;
    int control;
    int unk8f;
    int yaw;
    int dirt;
    int advance;
    RaceWorld *world;

    memset(v, 0, 0xb40);
    v->present = 1;
    v->index = index;
    v->hasTurbo = single < dims->turbo;
    world = Race_World();
    v->model = world->carModels[index];
    Matrix_Identity(&v->world);
    slot = car->gridSlot;
    if (Race_IsTimeTrial() != 0) {
        single = (Course_Get(world->course)->flags & 0x20) == 0;
    }
    if (single != 0) {
        slot = 2;
    }
    Matrix_Translate(&v->world, (VECTOR *)(course + 0x58 + slot * 12));
    phase = 0;
    control = 0;
    dirt = 0;
    advance = 0;
    yaw = *(int *)(course + 0x54);
    Matrix_RotY(&v->world, yaw);
    RaceVehicle_SetupModelDims(v, dims);
    section = CourseSect_Locate(((CourseData *)course)->sections, section, (int *)v->world.t);
    unk8f = car->unk8f;
    switch (car->driverType) {
    case 3:
        if (Race_IsTimeTrial() == 0) {
            InputLog *log = (InputLog *)((char *)&g_GameState + 0xc6a4);

            v->inputLog = log;
            InputLog_Reset(log, world->isReplay != 0, 0x4400);
        } else {
            RaceVehicle_InitLapGhosts(v);
            if (world->isReplay != 0) {
                phase = 2;
            }
        }
        v->controlType = 2;
        if (handicap != 0 && handicap > 0) {
            advance = handicap * 10;
        }
        dirt = RaceSetup_Get()->dirt;
        break;
    case 4: {
        InputLog *log = (InputLog *)((char *)&g_GameState + 0x10bbc);

        v->inputLog = log;
        InputLog_Reset(log, world->isReplay != 0, 0x4400);
        v->controlType = 3;
        if (handicap != 0 && handicap < 0) {
            advance = -handicap * 10;
        }
        break;
    }
    case 2:
        g_TT_HasBestLap = 0;
        if (Race_IsTimeTrial() == 0) {
            InputLog *log = (InputLog *)((char *)&g_GameState + 0x10bbc);

            v->inputLog = log;
            InputLog_Reset(log, 1, 0x4400);
            phase = 1;
        } else {
            RaceRecordSlot *slots = (RaceRecordSlot *)((char *)&g_GameState + 0xc5a8);
            InputLog *log = (InputLog *)slots[v->index].logs[0];

            v->inputLog = log;
            if (car->active == 0) {
                v->model = g_Race_IsDemo.world.vehicles[0].model;
                InputLog_Reset(log, 0, 0x1000);
                InputLog_Close(log, 0);
                v->present = 0;
            }
            v->hidden = 1;
            InputLog_Reset(log, 1, 0x1000);
            phase = 2;
        }
        v->controlType = 1;
        break;
    default:
        v->controlType = 0;
        control = 2;
        break;
    }
    if (RaceSetup_Get()->raceType != 0) {
        advance = 0;
    }
    CarPhys_Setup(&v->phys, dims, section, v->world.t[0] >> 4, -v->world.t[2] >> 4, g_SinTable[yaw & 0xfff],
                  g_SinTable[(yaw & 0xfff) + 0x400], control, unk8f, phase, 0, index, -advance);
    if (Race_IsTimeTrial() != 0) {
        int type = car->driverType;

        if (type != 2 && type == 3) {
            if (D_800A951C == 0) {
                GhostRec_StartFree(&v->phys);
            } else {
                g_TT_GhostResync = 1;
            }
        }
    }
    v->phys.dirt = (long long)dirt * 600000 >> 12;
    RaceVehicle_SetupWheels(v);
    RaceVehicle_StartSound(v);
    RaceVehicle_UpdateTransform(v);
}

void RaceVehicle_Shutdown(RaceVehicle *v)
{
    RaceVehicle_StopSound(v);
    if (v->controlType != 1) {
        InputLog_Close(v->inputLog, D_800A951C != 0);
    }
    if (v->controlType == 2 && D_800A951C == 0) {
        *(short *)(GameState_Base() + 0x14fd8) = ((int)v->phys.dirt << 12) / 600000;
    }
}

void RaceVehicle_InitLapGhosts(RaceVehicle *v)
{
    RaceRecordSlot *slot = RecordSlot(v->index);

    if (D_800A951C == 0) {
        slot->lapLogs = 0;
        slot->unkfe = 0;
    } else {
        v->ghostLap = 0;
    }
    v->inputLog = (InputLog *)slot->logs[0];
    InputLog_Reset((InputLog *)slot->logs[0], D_800A951C != 0, 0x1000);
}

int RaceVehicle_SelectGhostLap(RaceVehicle *v, int lap)
{
    RaceRecordSlot *slot = RecordSlot(v->index);
    int result;

    if (lap < slot->lapLogs) {
        InputLog *log = (InputLog *)rt::from_guest((lap * 0x10fc + (int)rt::to_guest(slot) + 0x1e0));

        v->ghostLap = lap;
        v->inputLog = log;
        InputLog_Reset(log, 1, 0x1000);
        result = 0;
    } else {
        result = 1;
    }
    return result;
}

void Matrix_FromCarPose(MATRIX *m, CarPose *pose)
{
    int x, y, z;

    x = pose->pos[0];
    z = pose->pos[2];
    m->t[0] = x << 4;
    y = pose->pos[1];
    m->t[1] = z << 4;
    m->t[2] = -y << 4;
    x = pose->right[0];
    z = pose->right[2];
    m->m[0][0] = x;
    y = pose->right[1];
    m->m[1][0] = z;
    m->m[2][0] = -y;
    x = pose->up[0];
    z = pose->up[2];
    m->m[0][1] = x;
    y = pose->up[1];
    m->m[1][1] = z;
    m->m[2][1] = -y;
    x = pose->forward[0];
    z = pose->forward[2];
    m->m[0][2] = -x;
    y = pose->forward[1];
    m->m[1][2] = -z;
    m->m[2][2] = y;
}

void RaceVehicle_UpdateTransform(RaceVehicle *v)
{
    int i;
    MATRIX *world = &v->world;
    VECTOR d;
    SVECTOR local;
    MATRIX a;
    MATRIX b;
    RaceWorld *race;

    if (v->present != 0) {
        v->hidden = 0;
    }
    v->section = v->phys.section;
    *(Vec3 *)v->prevPos = *(Vec3 *)v->world.t;
    Matrix_FromCarPose(world, (CarPose *)v->phys.pos);
    Matrix_FromCarPose(&v->shadow, (CarPose *)((char *)v + 0x6ac));
    Vec_Sub(v->vel, (int *)v->world.t, v->prevPos);
    local.vx = 0;
    local.vy = 0;
    local.vz = *(short *)(v->phys.unk2c + 2);
    Matrix_RotateVector(&d, world, &local);
    {
        int x = d.vx >> 8;
        int y = d.vy >> 8;
        int z = d.vz >> 8;

        v->world.t[0] += x;
        v->world.t[1] += y;
        v->world.t[2] += z;
        v->shadow.t[0] += x;
        v->shadow.t[1] += y;
        v->shadow.t[2] += z;
    }
    for (i = 0; i < 4; i++) {
        RaceWheelDraw *draw = &v->wheelDraw[i];
        CarPhysWheel *wheel = &v->phys.wheels[i];

        *(short4 *)draw->pos = *(short4 *)draw->rest;
        draw->pos[1] = *(short *)((char *)v + 0x160 + (i >> 1) * 0x34) - *(short *)((char *)wheel + 0x10);
        if ((signed char)wheel->fxSmoke < 0) {
            Particle *p = Particles_Spawn(&g_Race_IsDemo.world.particles, 0);

            if (p != 0) {
                Matrix_Copy(&a, &v->world);
                Matrix_Identity(&b);
                b.t[0] = draw->pos[0] << 4;
                b.t[1] = draw->pos[1] << 4;
                b.t[2] = draw->pos[2] << 4;
                Matrix_Comp(&a, &b);
                *(Vec3 *)p->pos = *(Vec3 *)a.t;
                p->seed = (ushort)a.t[2] ^ ((ushort)a.t[0] ^ (ushort)a.t[1]);
                p->size = 0;
                p->life = wheel->fxSmoke << 4;
            }
        }
    }
    race = Race_World();
    if (race->isReplay == 0 && v->controlType < 4 && v->controlType >= 2) {
        int index = v->index;
        char *pad = (char *)&race->pads[index];
        short *motor = (short *)(pad + 0x5a);

        motor[3] = 2;
        if (((char *)&g_GameState)[index * 0x52 + 0x36] == 0 && v->finishTime == 0) {
            *(short *)(pad + 0x5a) = *(short *)v->phys.unk760;
            *(short *)(pad + 0x5c) = v->phys.unk762 << 4;
            *(short *)(pad + 0x5e) = v->phys.unk763 << 4;
        } else {
            motor[0] = 0;
            motor[1] = 0;
            motor[2] = 0;
        }
    }
}

void RaceVehicle_OnLap(RaceVehicle *v, int index, int time)
{
    char *state = (char *)&g_GameState;
    char *record = state + 0xc5a8;
    VehicleHud *hud;
    int frames;
    void *label;

    switch (v->controlType) {
    case 3:
        record = state + 0x10ac0;
    case 2:
        if (D_800A951C == 0) {
            *(int *)rt::from_guest(((int)rt::to_guest(record) + index * 4 + 0xe8)) = time;
        }
        break;
    }
    label = g_LapLabels[index];
    hud = (VehicleHud *)((char *)v + 0x880);
    frames = 0x78;
    hud->lapTimeTimer = frames;
    hud->lapTimeShown = time;
    hud->lapLabel = label;
    if (RaceSetup_Get()->raceType == 6) {
        int *best = (int *)D_800A9524;

        hud->refTime = -1;
        if (~best[0] != 0) {
            hud->refTime = *(int *)rt::from_guest(((int)rt::to_guest(best) + index * 4 + 4));
        }
        hud->refTimeTimer = frames;
        hud->refTime2 = time;
    }
}

void RaceVehicle_OnLapDone(RaceVehicle *v, int lap, unsigned int time, int total, unsigned short extra)
{
    int finished = 0;
    RaceSetup *setup = &g_RaceSetup;
    int type = setup->raceType;
    CourseEntry *course = Course_Get(Course_FindByHash(setup->courseHash));
    char *state;
    RaceRecord *record;
    TimeRecord *best;
    VehicleHud *hud;

    switch (type) {
    case 1:
    case 6:
    case 10:
        if (lap >= 999) {
            return;
        }
        finished = course->flags >> 5 & 1;
        break;
    default: {
        int laps = RaceSetup_Get()->lapCount;

        if (laps == 0) {
            laps = 100;
        }
        if (lap > laps) {
            return;
        }
        if (lap == laps || Race_IsEnduranceTimeUp() != 0) {
            finished = 1;
        }
        if (finished != 0) {
            Ai_EnterDefaultMode(&v->phys);
            v->finishTime = total;
        }
        break;
    }
    }
    state = (char *)&g_GameState;
    record = (RaceRecord *)(state + 0xc5a8);
    best = (TimeRecord *)g_Race_IsDemo.world.courseRecord;
    hud = (VehicleHud *)((char *)v + 0x880);
    switch (v->controlType) {
    case 3:
        record = (RaceRecord *)(state + 0x10ac0);
    case 2:
        if (g_Race_IsDemo.world.isReplay == 0) {
            RaceRecord_AddLap(record, lap, time, extra, (signed char)hud->lapInvalid != 0);
            if (((RaceSetup *)(state + 0xbf7c))->raceType == 6) {
                hud->refTime = -1;
                if (~best->time != 0) {
                    hud->refTime = best->time;
                }
                hud->refTimeTimer = 0x78;
                hud->refTime2 = time;
            }
            if ((signed char)v->lapInvalid == 0) {
                int better = 1;

                if (~best->time != 0) {
                    better = time < best->time;
                }
                if (better != 0 && ((RaceSetup *)(state + 0xbf7c))->raceType == 6) {
                    *best = record->laps[record->lapsKept - 1];
                    (state + 0xc508)[1] = 1;
                }
            }
            if (finished != 0) {
                *(state + 0xc508) = v->phys.racePos;
                record->unkf8 = total;
                record->unk00 = (signed char)v->phys.racePos;
                Result_Start(g_Race_IsDemo.world.resultAnim);
                if (type == 2 || type == 11) {
                    ChampRaceList *list = (ChampRaceList *)(state + 0xc514);
                    short order[6];
                    int i;

                    if (v->controlType == 2) {
                        Ap_RaceFinished(list->unk00);
                        if ((signed char)v->phys.racePos == 1) {
                            Ap_RaceWon(list->unk00);
                        }
                    }
                    for (i = 0; i < 6; i++) {
                        order[i] = -1;
                        list->racePoints[i] = 0;
                    }
                    for (i = 0; i < g_RaceSetup.carCount; i++) {
                        unsigned int place = (signed char)g_RaceWorld.vehicles[i].phys.racePos - 1;

                        if (place < 6) {
                            order[place] = i;
                        }
                    }
                    for (i = 0; i < 6; i++) {
                        if (order[i] >= 0) {
                            list->racePoints[order[i]] = g_ChampPoints[i];
                        }
                    }
                    ChampRaceList_AddPoints(list);
                }
            }
        } else {
            hud->refTime = -1;
            if (~best->time != 0) {
                hud->refTime = best->time;
            }
        }
        break;
    }
    hud->lapTimeTimer = 0x78;
    hud->lapTimeShown = time;
    hud->lapLabel = sR_LapTime;
    if (finished != 0 && v->controlType < 4 && v->controlType >= 2) {
        RaceMusic_PlayFinish(g_RaceView);
    }
}

int RaceVehicle_GetInput(RaceVehicle *v, RacePad *pad, CarControls *out, int playback)
{
    RaceInput in;
    RaceInput *packet = &in;
    int done;
    int flags, buttons, throttle, steer, brake;
    int gear;

    memset(packet, 0, 5);
    done = 0;
    if (playback == 0) {
        unsigned int analog = pad->analogActs;
        int limit;

        flags = 0;
        steer = 0;
        throttle = 0;
        buttons = pad->actHeld;
        brake = 0;
        if (analog & 1) {
            steer = pad->analogValue[0];
            flags = 1;
        }
        if (analog & 4) {
            throttle = pad->analogValue[2];
            flags |= 2;
            throttle >>= 4;
        }
        if (analog & 8) {
            brake = pad->analogValue[3];
            flags |= 4;
            brake >>= 4;
        }
        if (buttons & 0x400) {
            flags |= 8;
        }
        in.flags = flags;
        in.buttons = buttons;
        in.steer = steer;
        in.throttle = throttle;
        in.brake = brake;
        limit = 0x12c;
        if (RaceSetup_Get()->raceType == 3) {
            limit = 0x3c;
        }
        if (D_800A9522 >= limit) {
            InputLog_Close(v->inputLog, 0);
        } else {
            InputLog_Write(v->inputLog, packet);
        }
    } else {
        InputLog_Read(v->inputLog, packet);
        flags = in.flags;
        buttons = in.buttons;
        steer = in.steer;
        throttle = in.throttle;
        brake = in.brake;
        done = InputLog_HasEnded(v->inputLog);
    }
    out->flags = flags;
    if ((flags & 1) == 0) {
        steer = (buttons & 1) << 1;
        if (buttons & 2) {
            steer -= 2;
        }
    } else {
        steer = (0x80 - steer) << 5;
    }
    out->steer = steer;
    if ((flags & 2) == 0) {
        throttle = NonZero(buttons & 4);
    } else {
        throttle = g_PedalCurve[throttle];
    }
    out->throttle = throttle;
    if ((flags & 4) == 0) {
        brake = NonZero(buttons & 8);
    } else {
        brake = g_PedalCurve[brake];
    }
    gear = NonZero(buttons & 0x40);
    out->b = NonZero(buttons & 0x10);
    out->a = NonZero(buttons & 0x20);
    out->brake = brake;
    if (buttons & 0x80) {
        gear--;
    }
    out->gear = gear;
    return done;
}

void RaceVehicle_GetPlayer1Input(RaceVehicle *v, void *input)
{
    int result = 1;
    int review = 0;
    int delta = (signed char)v->ghostLapDelta;
    int lap;
    RacePad *pad;
    RaceMem *race = &g_Race_IsDemo;

    v->ghostLapDelta = 0;
    lap = (signed char)v->ghostLap;
    pad = &race->world.pads[0];
    if (race->world.isReplay != 0) {
        review = Race_IsTimeTrial() != 0;
    }
    if (review != 0) {
        int target = lap + delta;
        int count = RecordSlot(0)->lapLogs;

        if (target < 0) {
            target = 0;
        } else if (target >= count) {
            target = count - 1;
        }
        if (lap != target) {
            lap = target;
            goto select;
        }
    }
    if (RaceVehicle_GetInput(v, pad, (CarControls *)input, D_800A951C != 0) != 0) {
        if (Race_IsTimeTrial() != 0) {
            lap++;
        select:
            g_TT_GhostResync = 1;
            result = RaceVehicle_SelectGhostLap(v, lap);
        }
        if (result != 0) {
            int one = 1;

            g_Race_EndRequest = one;
        }
    }
    ((GhostLogHead *)TimeTrial_GetLapLog())->count++;
}

void RaceVehicle_GetPlayer2Input(RaceVehicle *v, void *input)
{
    RaceMem *race = &g_Race_IsDemo;

    if (RaceVehicle_GetInput(v, &race->world.pads[1], (CarControls *)input, race->world.isReplay != 0) != 0) {
        int one = 1;

        g_Race_EndRequest = one;
    }
}

void RaceVehicle_GetLoggedInput(RaceVehicle *v, void *input)
{
    RaceMem *race = &g_Race_IsDemo;

    RaceVehicle_GetInput(v, &race->world.pads[1], (CarControls *)input, 1);
}

void RaceVehicle_Draw(RaceVehicle *v, RaceCam *cam, int mirror)
{
    CarModelInst inst;
    int dist, a, b, c, t, level, scale, near;
    int i, n0, n1, n2;
    int lod, shadow;
    int inside;

    Vec_Sub(v->relPos, (int *)v->world.t, (int *)cam->view.t);
    a = v->relPos[0];
    b = v->relPos[1];
    c = v->relPos[2];
    if (a < 0) {
        a = -a;
    }
    if (b < 0) {
        b = -b;
    }
    if (c < 0) {
        c = -c;
    }
    if ((unsigned int)a < (unsigned int)b) {
        t = a;
        a = b;
        b = t;
    }
    if ((unsigned int)a < (unsigned int)c) {
        t = a;
        a = c;
        c = t;
    }
    if ((unsigned int)b < (unsigned int)c) {
        t = b;
        b = c;
        c = t;
    }
    dist = a + ((unsigned int)b >> 1) + ((unsigned int)c >> 2);
    scale = 0x80000;
    near = 0;
    if (g_Race_IsDemo.world.isReplay == 0) {
        near = v->controlType < 2;
    }
    if (near != 0) {
        scale = 0x18000;
    }
    if (mirror != 0 && dist > 0x63ffff) {
        return;
    }
    level = scale / ((dist >> 16) + 1);
    v->camDist = dist;
    if (mirror == 0) {
        RaceVehicle *car = g_Race_IsDemo.world.drawList;
        RaceVehicle *prev = 0;

        while (car != 0) {
            if (dist < car->camDist) {
                break;
            }
            prev = car;
            car = car->next;
        }
        if (prev == 0) {
            D_800ADA08 = v;
        } else {
            prev->next = v;
        }
        v->next = car;
    }
    if (level > 0x4000) {
        level = 0x4000;
    }
    if (level < 0x400 && v->controlType < 2) {
        level = 0;
    }
    v->soundLevel = level;
    if (v->hidden != 0) {
        return;
    }
    inside = 0;
    if (v->index == cam->targetCar) {
        inside = cam->insideCar != 0;
    }
    if (inside != 0) {
        return;
    }
    n0 = 0;
    n1 = 0;
    n2 = 0;
    for (i = 0; i < 4; i++) {
        switch (v->phys.wheels[i].surface2) {
        case 0:
            n0++;
            break;
        case 1:
            n1++;
            break;
        case 2:
            n2++;
            break;
        }
    }
    shadow = 0;
    if (n0 < n1) {
        shadow = 1;
        if (n1 < n2) {
            shadow = 2;
        }
    } else if (n0 < n2) {
        shadow = 2;
    }
    lod = 3;
    if (mirror == 0) {
        lod = v->index == cam->targetCar;
        if (v->controlType == 1) {
            int distance = v->camDist;
            int show;

            if (Race_IsTimeTrial() != 0) {
                show = D_800AF232;
            } else {
                RaceWorld *world = Race_World();

                if (world->finishFrames != 0 || world->isReplay != 0) {
                    goto draw;
                }
                show = world->showGhost;
            }
            if (show == 0) {
                return;
            }
            switch (D_801C9995) {
            case 0:
                if (Race_IsTimeTrial() != 0) {
                    return;
                }
            case 1:
                goto ghost;
            case 2:
                if (distance <= 0x2ffff) {
                    goto ghost;
                }
                break;
            case 3:
                if (distance <= 0x4ffff) {
                    goto ghost;
                }
                break;
            }
            goto draw;
        ghost:
            shadow = 3;
            lod = 3;
        }
    }
draw:
    inst.lod = lod;
    inst.detail = shadow;
    inst.unk02 = v->phys.brake != 0;
    inst.drawShadow = 1;
    inst.model = v->model;
    inst.world = v->world;
    inst.shadow = v->shadow;
    {
        int dirt = ((int)v->phys.dirt << 12) / 600000;

        if (dirt >= 0x1000) {
            dirt = 0x1000;
        }
        inst.brightness = (unsigned int)((0x1000 - dirt) * 3) >> 7;
    }
    inst.unk06 = 0x7fd7;
    inst.tpage = 9;
    for (i = 0; i < 4; i++) {
        WheelInst *wheel = &inst.wheels[i];
        short steer = *(short *)((char *)v + 0x394 + (i >> 1) * 2);
        short heading = v->phys.wheels[i].heading;
        short spin = v->phys.wheels[i].spinAngle;

        *(short4 *)wheel = *(short4 *)v->wheelDraw[i].pos;
        if (i & 1) {
            wheel->spin = heading + 0x400;
            wheel->angleA = steer;
        } else {
            wheel->spin = heading - 0x400;
            wheel->angleA = -steer;
        }
        wheel->angleB = spin;
    }
    CarModel_Draw(&cam->cam, &inst, mirror);
}

void RaceVehicle_StartSound(RaceVehicle *v)
{
    CarSound *sound = (CarSound *)&v->soundStart;
    int player = v->controlType >= 2;

    if (player) {
        CarSound_Init(sound, (SpuBank *)((char *)D_8017DDA0 + v->index * 0x18), 0);
    } else {
        CarSound_Init(sound, (SpuBank *)((char *)D_8017DDB8 + v->hasTurbo * 12), 1);
    }
    v->soundMuted = 0;
}

// CarSound_Stop on the car sound.
void RaceVehicle_StopSound(RaceVehicle *v)
{
    CarSound_Stop((CarSound *)&v->soundStart);
}

// soundMuted = 1 and stops the sound (pause).
void RaceVehicle_MuteSound(RaceVehicle *v)
{
    v->soundMuted = 1;
    RaceVehicle_StopSound(v);
}

// soundMuted = 0 and restarts the sound.
void RaceVehicle_UnmuteSound(RaceVehicle *v)
{
    v->soundMuted = 0;
    RaceVehicle_StartSound(v);
}

void RaceVehicle_UpdateSound(RaceVehicle *v, int slot, RaceCam *cam)
{
    int level = v->soundLevel;
    CarSound *sound = (CarSound *)&v->soundStart;
    SVECTOR dir;
    SVECTOR move;
    int doppler;
    int scale;
    int volume;
    int engine = 0x2000;
    int engine2 = 0x4000;
    int blowOff = 0x1333;
    int replay;
    int i;
    int n0, n1, n2, skid, squeal, gravel, sum1, sum2, sum0;
    RaceWorld *world;

    if (v->soundMuted != 0) {
        level = 0;
    }
    if (cam->targetCar == v->index) {
        slot = 1;
    } else {
        slot = slot + 1;
    }
    sound->carIndex = slot;
    sound->volume = level;
    Vector_NormalizeToS(&dir, (VECTOR *)v->relPos);
    sound->unk0e = SVector_Dot(&dir, (SVECTOR *)cam->forward);
    sound->pan = SVector_Dot(&dir, (SVECTOR *)cam->right);
    world = Race_World();
    doppler = 0x1000;
    if (cam->speed < world->frameScale >> 1) {
        int speed = Vector_NormalizeToS(&move, (VECTOR *)v->vel);
        int own = speed * SVector_Dot(&dir, &move) >> 12;
        int listener = cam->speed * SVector_Dot(&dir, &cam->velDir) >> 12;

        doppler = ((long long)(world->frameScale + listener) << 12) / (world->frameScale + own);
    }
    volume = level;
    sound->doppler = doppler;
    replay = world->isReplay != 0;
    scale = 0x1000;
    if (replay) {
        scale = 0x3000;
    }
    if (v->index == cam->targetCar) {
        if ((replay ^ 1) != 0 && v->phys.unk73e == g_FrameRate) {
            int hit = v->phys.wallHit << 3;

            if (hit >= 0x1000) {
                hit = 0x1000;
            }
            CarSound_PlayOneShot(sound, 5, 0x40, (hit << 14) >> 12);
        }
        if (v->controlType < 4 && v->controlType >= 2 && cam->tvView == 0) {
            scale = 0x3000;
            sound->doppler = 0x1000;
            sound->unk0e = 0;
            sound->pan = 0;
            if (cam->innerView != 0) {
                volume = 0x4000;
                engine = scale;
                engine2 = 0x1800;
                blowOff = scale;
            } else {
                engine = 0x2199;
                engine2 = 0x2b33;
                blowOff = 0x2199;
            }
        }
    }
    sound->volume = volume;
    sound->rpm = v->phys.rpm;
    sound->volEngine = engine * v->phys.unk757 >> 7;
    sound->volEngine2 = engine2 * v->phys.unk758 >> 7;
    if (v->phys.blowOffState >= 0) {
        sound->volOneShot = v->phys.blowOffState << 2;
    } else {
        sound->volOneShot = 0;
    }
    sound->pitchLoop0 = (v->phys.boostDisplay << 12) / 6144;
    sound->volLoop[0] = blowOff * v->phys.blowOffVolume >> 7;
    n0 = 0;
    n1 = 0;
    n2 = 0;
    sum0 = 0;
    sum1 = 0;
    sum2 = 0;
    skid = 0;
    squeal = 0;
    gravel = 0;
    for (i = 0; i < 4; i++) {
        CarPhysWheel *wheel = &v->phys.wheels[i];

        switch (wheel->surface) {
        case 0:
            n0++;
            sum0 += wheel->fxSqueal;
            break;
        case 1:
            n1++;
            sum1 += wheel->fxSqueal;
            skid += wheel->fxGravel;
            break;
        case 5:
            n2++;
            sum2 += wheel->fxSqueal;
            gravel += wheel->fxGravel;
            break;
        }
    }
    if (n0 != 0) {
        sum0 = (sum0 << 4) * scale >> 12;
    }
    if (n1 != 0) {
        sum1 = (sum1 << 6) * scale >> 12;
        skid = (skid << 7) / n1;
    }
    if (n2 != 0) {
        gravel = (gravel << 7) / n2;
        sum2 = (sum2 << 4) * scale >> 12;
        gravel = gravel * 0x4cc >> 12;
    }
    squeal = sum0;
    if (squeal > 0x4000) {
        squeal = 0x4000;
    }
    sound->volLoop[1] = squeal;
    squeal = sum1;
    if (squeal > 0x4000) {
        squeal = 0x4000;
    }
    sound->volLoop[2] = squeal;
    squeal = skid;
    if (squeal > 0x3fff) {
        squeal = 0x3fff;
    }
    sound->pitchLoop2 = squeal;
    squeal = sum2;
    if (squeal > 0x4000) {
        squeal = 0x4000;
    }
    sound->volLoop[3] = squeal;
    squeal = gravel;
    if (squeal > 0x3fff) {
        squeal = 0x3fff;
    }
    sound->pitchLoop3 = squeal;
    CarSound_Update(sound);
}
}
