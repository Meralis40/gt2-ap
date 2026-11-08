// OVL0 80010000-80011ec8 (racecam): Race cameras.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"
#include "gte.h"

// Three words, copied as one.
struct Vec3 {
    int v[3];
};

// A key of a straight-line camera path.
struct TrackCamLinearKey {
    int length;
    int pos[3];
};

// A key of a spline camera path; x, y and z are its cubic's factors.
struct TrackCamSplineKey {
    int length;
    int pos[3];
    uint scale;
    int x[3];
    int y[3];
    int z[3];
};

struct TrackCamPath {
    ushort kind;                // 0 straight lines, 1 spline
    ushort count;
};

struct TrackCamLinear : TrackCamPath {
    TrackCamLinearKey keys[1];
};

struct TrackCamSpline : TrackCamPath {
    TrackCamSplineKey keys[1];
};

// A track-side camera: where it applies, then what its kind (flags & 0xf) needs.
struct TrackCamEntry {
    ushort flags;
    ushort firstLap;
    ushort lastLap;
    ushort section;
    int start;
    int end;
    union {
        struct {
            ushort proj0;
            ushort proj1;
            uchar pad14[4];
            TrackCamPath path;
        } tv;
        struct {
            int proj;
            Vec3 pos;
            short angles[3];
        } fixed;
        struct {
            ushort proj0;
            ushort proj1;
            ushort zoom;
            uchar pad16[6];
            TrackCamPath path;
        } zoomed;
    };
};

struct TrackCamTable {
    ushort count;
    ushort loop;                // laps of the cycle; 0 for none
    game_pointer<TrackCamEntry> entries[1];
};

// A car-mounted view.
struct CarViewPreset {
    int pos[3];
    short rot[3];
    short rot2[3];
};

// The part of a car's body data that RaceCam_TvZoom reads.
struct CarBodyScale {
    char unk0[0x4c];
    ushort shift;
    ushort scale;
};

extern "C" {
extern GameState (&g_GameState);
extern short (&g_RaceCam_FovTable)[4];
extern RaceMem (&g_Race_IsDemo);
extern unsigned char (&D_800A951C);
extern unsigned short (&D_800A9520);
extern unsigned char (&D_801C9992);
extern signed char (&D_800A9E04);
extern unsigned char (&g_RaceCarCount);
extern int (&g_TvCam_Progress);
extern int (&g_ChaseCam_Height)[2];
extern int (&g_ChaseCam_Dist)[2];
extern int (&g_ChaseCam_HeightSplit)[2];
extern int (&g_ChaseCam_DistSplit)[2];
extern CarViewPreset (&g_CarViewPresets)[];
void RaceCam_CopyTargetCar(RaceCam *cam);
void RaceCam_UpdateLive(RaceCam *cam);
void RaceCam_UpdateIntro(RaceCam *cam);
void RaceCam_UpdateReplay(RaceCam *cam);
void RaceCam_SetOnboard(RaceCam *cam);
void RaceCam_TvPath(RaceCam *cam, TrackCamEntry *entry);
void RaceCam_TvFixed(RaceCam *cam, TrackCamEntry *entry);
void RaceCam_TvZoom(RaceCam *cam, TrackCamEntry *entry);
int Math_LerpDiv(int a, int b, int t, int d);
int Math_Lerp12(int a, int b, int t);
int Race_FindCarByPlace(int place);
void Matrix_LookAlong(MATRIX *m, VECTOR *dir);
int *Vector_Lerp(int *out, int *a, int *b, int t);
TrackCamEntry *TrackCam_Find(TrackCamTable *table, int lap, int pos);
void TrackCam_EvalPath(TrackCamPath *path, int *out);
void TrackCam_EvalLinear(TrackCamLinear *path, Vec3 *out);
void TrackCam_EvalSpline(TrackCamSpline *path, Vec3 *out);
int CourseSect_Locate(CourseSectionTable *table, int index, int *pos);

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// The course, as the game reaches it: the race plus 0xb534, added at run time.
static inline CourseData *Race_Course(void)
{
    char *base = (char *)&g_Race_IsDemo;
    int offset = 0xb534;

    return (CourseData *)(base + offset);
}

static inline char *Race_Base(void)
{
    return (char *)&g_Race_IsDemo;
}

// The game indexes the race memory by car, then adds where the cars start.
#define RACE_FIRST_CAR(car) ((RaceVehicle *)((char *)(car) + 0x188))

// The world, as the game reaches it: the race plus 4, added at run time.
static inline RaceWorld *Race_World(void)
{
    return (RaceWorld *)(Race_Base() + 4);
}

static inline RaceVehicle *RaceCam_Car(RaceCam *cam)
{
    return &g_Race_IsDemo.world.vehicles[cam->targetCar];
}

static inline void Vec_Sub(int *dst, int *a, int *b)
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
}

static inline void Vec_Set(short *dst, int x, int y, int z)
{
    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
}

static inline int Lerp12(int a, int b, int t)
{
    return (int)((long long)(b - a) * t >> 12) + a;
}

static inline int Spline_Eval(uint scale, int *c, int t3, int t2, int t)
{
    long long sum = ((long long)c[0] * t3 >> 24) + ((long long)c[1] * t2 >> 24) + ((long long)c[2] * t >> 24);

    return (long long)((unsigned long long)scale * sum) >> 24;
}

void RaceCam_Init(RaceCam *cam, unsigned char index)
{
    memset(cam, 0, 0x110);
    cam->targetCar = index;
    cam->chaseView = g_GameState.options2[1];
    cam->replayOpt = 1;
    if (D_800A951C != 0) {
        if (RaceSetup_Get()->raceType == 3) {
            cam->replayOpt = 2;
        } else {
            cam->replayOpt = g_GameState.options2[0];
        }
    }
}

void RaceCam_SetProjection(RaceCam *cam, int projH, int offsetX, int offsetY)
{
    int height = 0xf0;
    int half = 0x84;
    int shift;

    shift = cam->splitScreen != 0;
    cam->cam.vpX = 0;
    cam->cam.vpY = 0;
    cam->cam.width = 0x140;
    height >>= shift;
    half >>= shift;
    offsetY >>= shift;
    cam->cam.height = height;
    Camera_SetFrustum(&cam->cam, offsetX - 0xa0, offsetX + 0xa0, offsetY + half, offsetY - half, projH, 0x7fff);
}

void RaceCam_Update(RaceCam *cam)
{
    cam->lookBack = 0;
    cam->insideCar = 0;
    cam->rearMirror = 0;
    *(Vec3 *)cam->prevPos = *(Vec3 *)cam->view.t;
    cam->tvView = 1;
    cam->innerView = 0;
    if (D_800A951C == 0) {
        RaceCam_UpdateLive(cam);
    } else {
        RaceCam_UpdateReplay(cam);
    }
    Vec_Sub(cam->vel, (int *)cam->view.t, cam->prevPos);
    cam->speed = Vector_NormalizeToS(&cam->velDir, (VECTOR *)cam->vel);
    Matrix_ToEuler((short *)&cam->angles, &cam->view);
    Vec_Set(cam->forward, cam->view.m[0][2], cam->view.m[1][2], cam->view.m[2][2]);
    Vec_Set(cam->right, cam->view.m[0][0], cam->view.m[1][0], cam->view.m[2][0]);
}

void RaceCam_CopyTargetCar(RaceCam *cam)
{
    RaceVehicle *car = (RaceVehicle *)&g_Race_IsDemo;

    car = cam->targetCar + car;
    car = RACE_FIRST_CAR(car);
    cam->section = car->section;
    cam->view = car->world;
    Matrix_ToEuler((short *)&cam->angles, &cam->view);
}

// Projection distance from the field-of-view option (g_RaceCam_FovTable[options2[4]]).
void RaceCam_SetDefaultProjection(RaceCam *cam)
{
    RaceCam_SetProjection(cam, (unsigned int)(unsigned short)g_RaceCam_FovTable[g_GameState.options2[4]], 0, 0);
}

void RaceCam_UpdateLive(RaceCam *cam)
{
    if (D_800A951C == 0) {
        RacePad *pad = cam->pad;
        uint held = pad->actHeld;
        uint pressed = pad->actPressed;

        if (held & 0x200) {
            cam->lookBack = 1;
        }
        if (pressed & 0x100) {
            if (++cam->chaseView == 3) {
                cam->chaseView = 0;
            }
        }
        if (D_800A9520 != 0) {
            RaceCam_UpdateIntro(cam);
            return;
        }
    }
    RaceCam_SetDefaultProjection(cam);
    RaceCam_SetChase(cam, cam->chaseView);
}

// RaceCam_SetProjection(projH) then RaceCam_SetChase(chaseView).
void RaceCam_SetChaseProjection(RaceCam *cam, int projH)
{
    RaceCam_SetProjection(cam, projH, 0, 0);
    RaceCam_SetChase(cam, (unsigned int)cam->chaseView);
}

void RaceCam_SetChase(RaceCam *cam, int view)
{
    RaceMem *race = &g_Race_IsDemo;
    RaceVehicle *car = &race->world.vehicles[cam->targetCar];
    VECTOR pos;

    RaceCam_CopyTargetCar(cam);
    cam->tvView = 0;
    if (view == 0) {
        MATRIX *m = &cam->view;

        pos.vx = 0;
        pos.vy = 0xcccc;
        pos.vz = 0;
        Matrix_Translate(m, &pos);
        if (cam->lookBack == 0) {
            cam->rearMirror = 1;
        } else {
            Matrix_RotY(m, 0x800);
        }
        cam->insideCar = 1;
        cam->innerView = 1;
    } else {
        int x = cam->view.t[0];
        int y = cam->view.t[1];
        int z = cam->view.t[2];
        int yaw = cam->angles.vx - car->phys.chaseSway[0];
        int pitch = cam->angles.vy + car->phys.chaseSway[1];
        MATRIX *m;

        if (cam->lookBack != 0) {
            yaw += 0x800;
            pitch = -pitch;
        }
        m = &cam->view;
        if (cam->splitScreen != 0) {
            pitch += 0x20;
        }
        Matrix_Identity(m);
        pos.vx = x;
        pos.vy = y;
        pos.vz = z;
        Matrix_Translate(m, &pos);
        Matrix_RotEuler(m, yaw, pitch, 0);
        x = 0;
        if (cam->splitScreen == 0) {
            y = g_ChaseCam_Height[view - 1];
            z = g_ChaseCam_Dist[view - 1];
        } else {
            y = g_ChaseCam_HeightSplit[view - 1];
            z = g_ChaseCam_DistSplit[view - 1];
        }
        pos.vx = x;
        pos.vy = y;
        pos.vz = z;
        Matrix_Translate(&cam->view, &pos);
    }
}

int Math_LerpDiv(int a, int b, int t, int d)
{
    return (long long)(b - a) * t / d + a;
}

int Math_Lerp12(int a, int b, int t)
{
    return (int)((long long)(b - a) * t >> 12) + a;
}

void RaceCam_UpdateIntro(RaceCam *cam)
{
    RaceWorld *world = Race_World();
    int timer = world->introTimer;
    int frames = world->introFrames;
    int fov = (unsigned short)g_RaceCam_FovTable[D_801C9992];
    int start = timer;
    VECTOR pos;

    RaceCam_CopyTargetCar(cam);
    if (timer < 0x3c) {
        RaceCam_SetChaseProjection(cam, Math_LerpDiv(fov, 0x399, timer, 0x3c));
        return;
    }
    timer -= 0x3c;
    if (timer < 0x3c) {
        int x, y, z, p, yaw, pitch;
        MATRIX *m;

        timer -= 6;
        if (timer < 0) {
            timer = 0;
        }
        p = (timer << 12) / 54;
        y = cam->view.t[1];
        x = cam->view.t[0];
        z = cam->view.t[2];
        y += 0x4ccc;
        yaw = Math_Lerp12(0, 0x5c0, p);
        pitch = Math_Lerp12(0xaa, 0, p);
        m = &cam->view;
        yaw = cam->angles.vx + yaw;
        pitch = cam->angles.vy + pitch;
        Matrix_Identity(m);
        pos.vx = x;
        pos.vy = y;
        pos.vz = z;
        Matrix_Translate(m, &pos);
        Matrix_RotEuler(m, yaw, pitch, 0);
        z = Math_Lerp12(0x354bc, 0x28000, p);
        pos.vx = 0;
        pos.vy = 0;
        pos.vz = z;
        Matrix_Translate(m, &pos);
        yaw = Math_Lerp12(0, 0x8c, p);
        pitch = Math_Lerp12(0, 0x100, p);
        Matrix_RotEuler(m, yaw, -pitch, 0);
        RaceCam_SetProjection(cam, 0xa0, 0, 0);
    } else {
        int p, far, x, y, z, yaw, pitch, a;
        MATRIX *m;

        int span = frames - 0x78;

        timer = start - 0x8a;
        if (timer < 0) {
            timer = 0;
        }
        p = (timer << 12) / (frames - 0x8a);
        far = span * 0x4ccc;
        x = cam->view.t[0];
        z = cam->view.t[2];
        y = cam->view.t[1] + 0xcccc;
        yaw = cam->angles.vx + 0x800;
        pitch = Math_Lerp12(0x18, 0xaa, p);
        m = &cam->view;
        pitch = cam->angles.vy + pitch;
        Matrix_Identity(m);
        pos.vx = x;
        pos.vy = y;
        pos.vz = z;
        Matrix_Translate(m, &pos);
        Matrix_RotEuler(m, yaw, pitch, 0);
        z = Math_Lerp12(0x5d4bc, far, p);
        pos.vx = 0;
        pos.vy = 0;
        pos.vz = z;
        Matrix_Translate(m, &pos);
        yaw = 0;
        a = Math_Lerp12(0x50, yaw, p);
        if (a < 0) {
            a = yaw;
        } else if (a > 0x50) {
            a = 0x50;
        }
        Matrix_RotEuler(&cam->view, yaw, -a, 0);
        x = 0;
        y = -0x50;
        a = Math_Lerp12(0x1c2, 0x399, p);
        if (a > 0x399) {
            z = 0x399;
        } else {
            z = a;
            if (z < 0x1c2) {
                z = 0x1c2;
            }
        }
        RaceCam_SetProjection(cam, z, x, y);
    }
}

int Race_FindCarByPlace(int place)
{
    RaceWorld *world;
    int count;
    int i;

    if (Race_IsTimeTrial() != 0) {
        return 0;
    }
    count = Race_World()->carCount;
    world = Race_World();
    for (i = 0; i < count; i++) {
        RaceVehicle *car = (RaceVehicle *)&world->vehicles[i].next;

        if ((signed char)car->phys.racePos == place) {
            return i;
        }
    }
    return 0;
}

void RaceCam_UpdateReplay(RaceCam *cam)
{
    int carCount;
    int changed;
    int type;
    uint pressed;
    CourseData *course;
    RaceVehicle *car;
    RaceWorld *world;
    TrackCamEntry *entry;
    int place;

    if (Race_IsTimeTrial() != 0) {
        carCount = 1;
    } else {
        carCount = g_RaceCarCount;
    }
    changed = 0;
    type = RaceSetup_Get()->raceType;
    pressed = cam->pad->raw.pressed;
    if (pressed & 0x800) {
        changed = 1;
        if (++cam->replayMode == 3) {
            cam->replayMode = 0;
        }
    }
    switch (type) {
    case 6:
        world = Race_World();
        if (world->introTimer == 0) {
            switch (pressed & 0x1010) {
            case 0x10:
                world->vehicles[cam->targetCar].ghostLapDelta = -1;
                break;
            case 0x1000:
                world->vehicles[cam->targetCar].ghostLapDelta = 1;
                break;
            }
        }
        break;
    case 0:
        if (pressed & 0x100) {
            cam->splitScreen = cam->splitScreen == 0;
        }
        break;
    default:
        place = (signed char)g_Race_IsDemo.world.vehicles[cam->targetCar].phys.racePos;
        switch (pressed & 3) {
        case 1:
            if (--place == 0) {
                place = carCount;
            }
            break;
        case 2:
            if (++place > carCount) {
                place = 1;
            }
            break;
        default:
            goto noPlace;
        }
        cam->targetCar = Race_FindCarByPlace(place);
    }
noPlace:
    if (pressed & 0x400) {
        if (++cam->onboardView == 9) {
            cam->onboardView = 0;
        }
    }
    if (pressed & 0x200) {
        if (++cam->replayOpt == 3) {
            cam->replayOpt = 0;
        }
    }
    switch (cam->replayMode) {
    case 0:
        break;
    case 1:
        RaceCam_SetOnboard(cam);
        return;
    case 2:
        if (type != 0) {
            cam->replayMode = 0;
        } else {
            if (changed != 0) {
                RaceCam_SetOnboard(cam);
                return;
            }
            if (cam->splitScreen == 0) {
                if (D_800A9E04 == 1) {
                    cam->targetCar = 0;
                } else {
                    cam->targetCar = 1;
                }
            }
        }
        break;
    default:
        return;
    }
    course = Race_Course();
    car = RaceCam_Car(cam);
    entry = TrackCam_Find((TrackCamTable *)course->table1c, car->phys.lap, car->phys.trackPos);
    if (entry == 0) {
        RaceCam_UpdateLive(cam);
    } else {
        int pos = car->phys.trackPos;
        int end = entry->end;
        int start = entry->start;
        int length = course->sections->unk00;

        if (end < start) {
            end += length;
        }
        if (pos < start) {
            pos += length;
        }
        end -= start;
        pos -= start;
        if (end != 0) {
            g_TvCam_Progress = ((long long)pos << 12) / end;
        } else {
            g_TvCam_Progress = 0;
        }
        cam->tvFlags = entry->flags;
        switch (entry->flags & 0xf) {
        case 0:
            RaceCam_TvPath(cam, entry);
            break;
        case 1:
            RaceCam_TvFixed(cam, entry);
            break;
        case 2:
            RaceCam_TvCarView(cam, entry);
            break;
        case 3:
            RaceCam_TvZoom(cam, entry);
            break;
        }
    }
}

void Matrix_LookAlong(MATRIX *m, VECTOR *dir)
{
    int x = dir->vx;
    int y = dir->vy;
    int z = dir->vz;
    int absX, absY, absZ;
    int shift;
    int sx, sy, sz;
    int xx, yy, zz;
    int sinP, cosP, sinY, cosY;
    int len;

    absX = x;
    if (x < 0) {
        absX = -x;
    }
    absY = y;
    if (y < 0) {
        absY = -y;
    }
    absZ = z;
    if (z < 0) {
        absZ = -z;
    }
    shift = 0x1f - gte_Lzc(absX | absY | absZ) - 0xe;
    if (shift >= 0) {
        sx = x >> shift;
        sy = y >> shift;
        sz = z >> shift;
    } else {
        shift = -shift;
        sx = x << shift;
        sy = y << shift;
        sz = z << shift;
    }
    xx = sx * sx;
    yy = sy * sy;
    zz = sz * sz;
    sinP = 0;
    cosP = 0x1000;
    len = Math_ISqrt(xx + yy + zz, 0);
    if (len != 0) {
        int flat = Math_ISqrt(xx + zz, 0);

        sinP = (sy << 12) / len;
        cosP = (flat << 12) / len;
    }
    shift = 0x1f - gte_Lzc(absX | absZ) - 0xe;
    if (shift >= 0) {
        sx = x >> shift;
        sz = z >> shift;
    } else {
        shift = -shift;
        sx = x << shift;
        sz = z << shift;
    }
    sinY = 0;
    cosY = 0x1000;
    len = Math_ISqrt(sx * sx + sz * sz, 0);
    if (len != 0) {
        sinY = (sx << 12) / len;
        cosY = (sz << 12) / len;
    }
    sinY = -sinY;
    sinP = -sinP;
    cosY = -cosY;
    m->m[0][0] = cosY;
    m->m[1][0] = 0;
    m->m[1][1] = cosP;
    m->m[1][2] = sinP;
    m->m[2][0] = -sinY;
    m->m[0][1] = -(sinY * sinP >> 12);
    m->m[0][2] = sinY * cosP >> 12;
    m->m[2][1] = -(cosY * sinP >> 12);
    m->m[2][2] = cosY * cosP >> 12;
}

int *Vector_Lerp(int *out, int *a, int *b, int t)
{
    int x = Lerp12(a[0], b[0], t);
    int y = Lerp12(a[1], b[1], t);
    int z = Lerp12(a[2], b[2], t);

    out[0] = x;
    out[1] = y;
    out[2] = z;
    return out;
}

// Length of v (components >> 10 before squaring).
int Vector_Length(VECTOR *v)
{
    int iVar1;
    int iVar2;
    int iVar3;

    iVar1 = v->vx >> 10;
    iVar2 = v->vy >> 10;
    iVar3 = v->vz >> 10;
    iVar1 = Math_ISqrt(iVar1 *iVar1 + iVar2 *iVar2 + iVar3 *iVar3, 0);
    return iVar1;
}

void RaceCam_TvPath(RaceCam *cam, TrackCamEntry *entry)
{
    RaceVehicle *car = (RaceVehicle *)&g_Race_IsDemo;
    int offset = 0xb534;
    CourseData *course = (CourseData *)((char *)car + offset);
    int d[3];

    car += cam->targetCar;
    car = RACE_FIRST_CAR(car);
    TrackCam_EvalPath(&entry->tv.path, (int *)cam->view.t);
    cam->section = CourseSect_Locate(course->sections, entry->section, (int *)cam->view.t);
    car = (RaceVehicle *)car->world.t;
    Vec_Sub(d, (int *)car, (int *)cam->view.t);
    d[1] += 0xcccc;
    Matrix_LookAlong(&cam->view, (VECTOR *)d);
    RaceCam_SetProjection(cam, ((entry->tv.proj1 - entry->tv.proj0) * g_TvCam_Progress >> 12) + entry->tv.proj0, 0, 0);
}

void RaceCam_TvFixed(RaceCam *cam, TrackCamEntry *entry)
{
    CourseData *course = Race_Course();

    RaceCam_SetProjection(cam, entry->fixed.proj, 0, 0);
    *(Vec3 *)cam->view.t = entry->fixed.pos;
    cam->view.t[2] = -cam->view.t[2];
    Matrix_FromEuler(&cam->view, -entry->fixed.angles[1], -entry->fixed.angles[0], entry->fixed.angles[2]);
    cam->section = CourseSect_Locate(course->sections, entry->section, (int *)cam->view.t);
}

// Track-side camera type 2: car-mounted view entry +0x10 (RaceCam_SetCarView) with
// projection +0x12.
void RaceCam_TvCarView(RaceCam *cam, void *entry)
{
    RaceCam_SetProjection(cam, (unsigned int)*(unsigned short *)rt::from_guest(((int)rt::to_guest(entry) + 0x12)), 0, 0);
    RaceCam_SetCarView(cam, (unsigned int)*(unsigned short *)rt::from_guest(((int)rt::to_guest(entry) + 0x10)));
}

void RaceCam_SetCarView(RaceCam *cam, int view)
{
    char *base = (char *)&g_Race_IsDemo;
    RaceVehicle *car;
    CarViewPreset *preset = &g_CarViewPresets[view];
    int length;
    int yaw, pitch, roll;
    int x, y, z;
    MATRIX *m;
    VECTOR pos;

    base += cam->targetCar * (int)sizeof(RaceVehicle);
    base += 0x188;
    car = (RaceVehicle *)base;
    switch (view) {
    case 0:
    case 7:
        cam->insideCar = 1;
        cam->innerView = 1;
        cam->tvView = 0;
        break;
    case 8:
        break;
    }
    length = car->bodyLength + 0x4ccc;
    g_CarViewPresets[8].pos[0] = -length;
    g_CarViewPresets[9].pos[0] = length;
    g_CarViewPresets[10].pos[0] = -length;
    g_CarViewPresets[11].pos[0] = length;
    RaceCam_CopyTargetCar(cam);
    yaw = cam->angles.vx;
    pitch = cam->angles.vy;
    roll = cam->angles.vz;
    x = cam->view.t[0];
    y = cam->view.t[1];
    z = cam->view.t[2];
    if (view >= 12) {
        yaw = 0;
        pitch = 0;
        roll = 0;
    }
    m = &cam->view;
    Matrix_Identity(m);
    pos.vx = x;
    pos.vy = y;
    pos.vz = z;
    Matrix_Translate(m, &pos);
    Matrix_RotEuler(m, yaw, pitch, roll);
    Matrix_RotEuler(m, preset->rot[0], preset->rot[1], preset->rot[2]);
    {
        int px = preset->pos[0];
        int py = preset->pos[1];
        int pz = preset->pos[2];

        pos.vx = px;
        pos.vy = py;
        pos.vz = pz;
    }
    Matrix_Translate(m, &pos);
    Matrix_RotEuler(m, preset->rot2[0], preset->rot2[1], preset->rot2[2]);
}

void RaceCam_TvZoom(RaceCam *cam, TrackCamEntry *entry)
{
    char *course = (char *)&g_Race_IsDemo;
    int offset = 0xb534;
    RaceVehicle *car = (RaceVehicle *)course + cam->targetCar;
    CarBodyScale *body;
    int d[3];
    int proj;
    int shift;

    car = RACE_FIRST_CAR(car);
    body = *(game_pointer<CarBodyScale> *)((char *)car->model + 0x870);
    course += offset;
    TrackCam_EvalPath(&entry->zoomed.path, (int *)cam->view.t);
    cam->section = CourseSect_Locate(((CourseData *)course)->sections, entry->section, (int *)cam->view.t);
    car = (RaceVehicle *)car->world.t;
    Vec_Sub(d, (int *)car, (int *)cam->view.t);
    d[1] += 0xcccc;
    Matrix_LookAlong(&cam->view, (VECTOR *)d);
    proj = Vector_Length((VECTOR *)d) * 0x3600;
    shift = body->shift - 0x10;
    if (shift >= 0) {
        proj >>= shift;
    } else {
        proj <<= -shift;
    }
    proj /= body->scale;
    proj = proj * entry->zoomed.zoom >> 12;
    {
        int high = entry->zoomed.proj0;
        int low = entry->zoomed.proj1;

        if (proj > high) {
            proj = high;
        } else if (proj < low) {
            proj = low;
        }
    }
    RaceCam_SetProjection(cam, proj, 0, 0);
}

void RaceCam_SetOnboard(RaceCam *cam)
{
    int view = 0;

    RaceCam_SetDefaultProjection(cam);
    switch (cam->onboardView) {
    case 1:
        view = 1;
        goto chase;
    case 2:
        view = 2;
    chase:
        RaceCam_SetChase(cam, view);
        return;
    case 3:
        view = 12;
        break;
    case 0:
        view = 0;
        break;
    case 4:
        view = 7;
        break;
    case 5:
        view = 8;
        break;
    case 6:
        view = 9;
        break;
    case 7:
        view = 10;
        break;
    case 8:
        view = 11;
        break;
    }
    RaceCam_SetCarView(cam, view);
}

TrackCamEntry *TrackCam_Find(TrackCamTable *table, int lap, int pos)
{
    int length = Race_Course()->sections->unk00;
    int loop = table->loop;
    int count = table->count;
    int i;

    if (loop != 0 && lap > loop) {
        lap = (lap - 1) % loop + 1;
    }
    for (i = 0; i < count; i++) {
        TrackCamEntry *entry = table->entries[i];
        int first = entry->firstLap;
        int last = entry->lastLap;

        if (lap >= first && lap <= last) {
            int end = entry->end;
            int start = entry->start;
            int p = pos;

            if (end < start) {
                end += length;
            }
            if (first < lap) {
                p += length;
            }
            if (p >= start && p < end) {
                return entry;
            }
        }
    }
    return 0;
}

void TrackCam_EvalPath(TrackCamPath *path, int *out)
{
    switch (path->kind) {
    case 0:
        TrackCam_EvalLinear((TrackCamLinear *)path, (Vec3 *)out);
        break;
    case 1:
        TrackCam_EvalSpline((TrackCamSpline *)path, (Vec3 *)out);
        break;
    }
    out[2] = -out[2];
}

void TrackCam_EvalLinear(TrackCamLinear *path, Vec3 *out)
{
    int total = 0;
    int count = path->count;
    int i = 0;

    if (count != 0) {
        do {
            total += path->keys[i].length;
            i++;
        } while (i < count);
    }
    if (total == 0 || count < 2) {
        *out = *(Vec3 *)path->keys[0].pos;
    } else {
        int dist = (long long)total * g_TvCam_Progress >> 12;
        int t;
        int next;

        i = 0;
        count--;
        while (i < count) {
            next = i + 1;
            total = path->keys[next].length;
            if (dist < total) {
                break;
            }
            i = next;
            dist -= total;
        }
        if (total != 0) {
            t = ((long long)dist << 12) / total;
        } else {
            t = 0;
        }
        if (t < 0) {
            t = 0;
        } else if (t >= 0x1000) {
            t = 0x1000;
        }
        Vector_Lerp(out->v, path->keys[i].pos, path->keys[i + 1].pos, t);
    }
}

void TrackCam_EvalSpline(TrackCamSpline *path, Vec3 *out)
{
    int total = 0;
    int count = path->count;
    int i = 0;

    if (count != 0) {
        do {
            total += path->keys[i].length;
            i++;
        } while (i < count);
    }
    if (total == 0 || count < 2) {
        *out = *(Vec3 *)path->keys[0].pos;
    } else {
        int dist = (long long)total * g_TvCam_Progress >> 12;
        int t, t2, t3;
        TrackCamSplineKey *key;

        for (i = 0; i < count; i++) {
            total = path->keys[i].length;
            if (dist < total) {
                break;
            }
            dist -= total;
        }
        if (total != 0) {
            t = ((long long)dist << 12) / total;
        } else {
            t = 0;
        }
        if (t < 0) {
            t = 0;
        } else if (t >= 0x1000) {
            t = 0x1000;
        }
        t <<= 12;
        t2 = (long long)t * t >> 24;
        t3 = (long long)t2 * t >> 24;
        key = &path->keys[i];
        out->v[0] = key->pos[0] + Spline_Eval(key->scale, key->x, t3, t2, t);
        out->v[1] = key->pos[1] + Spline_Eval(key->scale, key->y, t3, t2, t);
        out->v[2] = key->pos[2] + Spline_Eval(key->scale, key->z, t3, t2, t);
    }
}
}
