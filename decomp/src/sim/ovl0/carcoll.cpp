// OVL0 8003fe8c-80041e4c (carcoll): Car to car collisions and body footprints.
// The functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

extern "C" {
extern RaceWorld g_RaceWorld;
extern unsigned char g_Race_IsDemo;

// One corner of a car against another car (g_CarColl_Pairs), per footprint buffer.
struct CarCollCorner {
    unsigned char outcode[2]; /* 0xf: not tested yet */
    unsigned char pad02[2];
    int x[2];
    int z[2];
};

extern int g_CarColl_Buf;
extern short g_CarColl_Toi[6][5];
extern signed char g_CarColl_Corner[6][5];
extern signed char g_CarColl_Side[6][5];
extern CarCollCorner g_CarColl_Pairs[6][5][4];
extern short g_SinTable[];
extern short g_CosTable[];

// A footprint corner swept against the course walls (World_CollideWalls), in the
// scratchpad from 0x1f800004 on.
struct WallSweep {
    int from[3]; /* previous corner x, z, 0 (x16) */
    int to[3]; /* current corner */
    short fraction; /* first wall hit, 0x1000: none */
    short pad1a;
    short normal[2]; /* wall normal x, z of the hit */
    unsigned char unk20[12];
};
#define WALL_SWEEPS ((WallSweep *)0x1f800004)

// Resets the car-to-car collision tables of both buffers and sets g_CarColl_Buf to 1.
#ifdef NON_MATCHING
// The game keeps the Toi / Corner / Side addresses and constants of the inner loop in
// registers set up before it (s4, t7, t1); here they are rebuilt in the loop.
void CarColl_ResetAll(void)
{
    int buf;
    int car;

    for (buf = 0; buf < 2; buf++) {
        for (car = 0; car < 6; car++) {
            int other;
            for (other = 0; other < 5; other++) {
                int c;
                for (c = 0; c < 4; c++) {
                    g_CarColl_Pairs[car][other][c].outcode[buf] = 0xf;
                }
                for (c = 0; c < 4; c++) {
                    g_CarColl_Pairs[car][other][c].x[buf] = 0;
                    g_CarColl_Pairs[car][other][c].z[buf] = 0;
                    g_CarColl_Toi[car][other] = 0x1000;
                    g_CarColl_Corner[car][other] = -1;
                    g_CarColl_Side[car][other] = -1;
                }
            }
        }
    }
    g_CarColl_Buf = 1;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_ResetAll);
#endif

// Marks the g_CarColl_Pairs entries of car carIdx against every other car, and of
// every other car against it, as not tested (both buffers). Used when a car enters
// its pit stop.
#ifdef NON_MATCHING
// Marks the 4 corners of car `car` against its other car `slot` as not tested.
static inline void CarColl_ResetPair(int car, int slot, int buf)
{
    int c;

    for (c = 0; c < 4; c++) {
        g_CarColl_Pairs[car][slot][c].outcode[buf] = 0xf;
    }
}

// Register allocation differs (the game reuses one register for the carIdx copy, the
// corner counter and the second car counter) and one more move is emitted.
void CarColl_ResetCar(int carIdx, int count)
{
    int buf;

    for (buf = 0; buf < 2; buf++) {
        int car = carIdx;
        int other;
        for (other = 0; other < count - 1; other++) {
            CarColl_ResetPair(car, other, buf);
        }
        for (car = 0; car < count; car++) {
            if (car != carIdx) {
                CarColl_ResetPair(car, carIdx - (carIdx >= car), buf);
            }
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_ResetCar);
#endif

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_ComputeCorners);

// 1 when lo <= v <= hi.
int Int_InRange(int lo, int v, int hi)
{
    int result = 0;

    if (lo <= v) {
        result = 1;
        if (v > hi) {
            goto out;
        }
        return result;
    }
out:
    return 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_FindImpact);

// Car to car collisions, step 2: CarColl_FindImpact for every pair of cars that
// collide (not ghosts, not in the pit lane), into g_CarColl_Toi / Corner / Side.
#ifdef NON_MATCHING
// The game computes &side once before the loops and keeps it on the stack (frame
// 0x60); here it is rebuilt before each call (frame 0x58).
void CarColl_FindImpacts(RaceVehicle *cars, int count)
{
    int i;
    int j;
    int corner;
    int side;

    for (i = 0; i < count; i++) {
        CarPhys *a = &cars[i].phys;
        if (a->ghost == 0 && a->aiLine != 4) {
            int slot;
            for (j = 0, slot = 0; j < count; j++) {
                CarPhys *b = &cars[j].phys;
                if (i != j) {
                    if (b->ghost == 0 && b->aiLine != 4) {
                        g_CarColl_Toi[i][slot] = CarColl_FindImpact(i, slot, a, &corner, &side);
                        g_CarColl_Corner[i][slot] = corner;
                        g_CarColl_Side[i][slot] = side;
                    }
                    slot++;
                }
            }
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_FindImpacts);
#endif

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_ResolveImpacts);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_RecomputeCorners);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_SeparateOverlaps);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", Race_GetCollisionRule);

// Race_GetCollisionRule, or 0 for a CPU car or one under AI control (pits).
int Body_GetCollisionRule(CarPhys *body)
{
    if ((signed char)body->control != 0) {
        return 0;
    }
    if (body->aiMode != 0) {
        return 0;
    }
    return Race_GetCollisionRule();
}

// 1 when leaving the course fails the race: raceType 3 or 6, a course without
// flag 4, and a player car not under AI control.
int Body_ChecksCourseOut(CarPhys *body)
{
    int result = 0;

    if ((signed char)body->control == 0 && body->aiMode == 0) {
        GameState *gs = &g_GameState;
        char *base0 = (char *)gs;
        int offset0 = 0xbf7c;
        RaceSetup *setup = (RaceSetup *)(base0 + offset0);
        int type = setup->raceType;

        if (type == 3 || type == 6) {
            if ((Course_Get(g_RaceWorld.course)->flags & 4) == 0) {
                result = 1;
            }
        }
    }
    return result;
}

// 1 unless raceType is 3 or 6.
int Race_AllowsOffCourseFlag(void)
{
    GameState *gs = &g_GameState;
    char *base0 = (char *)gs;
    int offset0 = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base0 + offset0);
    int type = setup->raceType;

    if (type == 6) {
        return 0;
    }
    return type != 3;
}

// Fills the current footprint buffer with the 4 corners of the car: the position
// plus or minus half the width sideways and the front / rear extents along the yaw.
void Body_ComputeFootprint(CarPhys *body)
{
    int buf = body->bufIdx;
    int angle = body->yaw & 0xfff;
    int sin = g_SinTable[angle];
    int cos = g_CosTable[angle];
    int frontX = -Fixed_Mul32(body->lenFront, sin);
    int frontZ = Fixed_Mul32(body->lenFront, cos);
    int rearX = -Fixed_Mul32(body->lenRear, sin);
    int rearZ = Fixed_Mul32(body->lenRear, cos);
    int sideX = Fixed_Mul32Shift(body->width, cos, 1);
    int sideZ = Fixed_Mul32Shift(body->width, sin, 1);
    // The footprint buffer, as an offset of the whole body.
    CarPhys *fp = (CarPhys *)((char *)body + buf * 32);

    fp->footprint[0] = body->pos[0] - sideX + frontX;
    fp->footprint[1] = body->pos[1] - sideZ + frontZ;
    fp->footprint[2] = body->pos[0] + sideX + frontX;
    fp->footprint[3] = body->pos[1] + sideZ + frontZ;
    fp->footprint[4] = body->pos[0] - sideX - rearX;
    fp->footprint[5] = body->pos[1] - sideZ - rearZ;
    fp->footprint[6] = body->pos[0] + sideX - rearX;
    fp->footprint[7] = body->pos[1] + sideZ - rearZ;
}

// Copies the current footprint corners into the other buffer (previous frame).
void Body_SaveFootprint(CarPhys *body)
{
    int i = 0;
    int src = body->bufIdx * 32;
    int dst = (1 - body->bufIdx) * 32;

    do {
        CarPhys *s = (CarPhys *)((char *)body + src);
        CarPhys *d = (CarPhys *)((char *)body + dst);
        d->footprint[0] = s->footprint[0];
        d->footprint[1] = s->footprint[1];
        src += 8;
        dst += 8;
        i++;
    } while (i < 4);
}

// Sweeps the 4 footprint corners from the previous to the current buffer against the
// course walls. Returns the mask of the corners that hit, with the earliest hit
// fraction and its corner; on a hit copies the wall normal of that corner.
#ifdef NON_MATCHING
// The game sets off (li t1,4) after loading the scratchpad base (lui t2); here the
// two are swapped.
unsigned int Body_SweepFootprint(CarPhys *body, int *outFraction, unsigned int *outCorner)
{
    unsigned int i;
    unsigned int hits;
    unsigned int bit;
    unsigned int corner;
    short fraction;
    int off = 4;
    int cur = body->bufIdx * 32;
    int prev = (1 - body->bufIdx) * 32;

    for (i = 0; i < 4; i++) {
        CarPhys *to = (CarPhys *)((char *)body + cur);
        CarPhys *from = (CarPhys *)((char *)body + prev);
        WallSweep *s = (WallSweep *)(0x1f800000 + off);
        cur += 8;
        prev += 8;
        s->from[0] = from->footprint[0] << 4;
        s->from[1] = from->footprint[1] << 4;
        s->from[2] = 0;
        s->to[0] = to->footprint[0] << 4;
        s->to[1] = to->footprint[1] << 4;
        s->to[2] = 0;
        off += sizeof(WallSweep);
    }
    World_CollideWalls(&g_Race_IsDemo, body->section, (int *)WALL_SWEEPS);
    hits = 0;
    fraction = 0x1000;
    corner = 0;
    bit = 1;
    for (i = 0; i < 4; i++) {
        short f = WALL_SWEEPS[i].fraction;
        if (f != 0x1000) {
            hits |= bit;
            if (f < fraction) {
                corner = i;
                fraction = f;
            }
        }
        bit <<= 1;
    }
    *outCorner = corner;
    *outFraction = fraction;
    if (hits == 0) {
        return 0;
    }
    body->wallNormalX = WALL_SWEEPS[corner].normal[0];
    body->wallNormalZ = WALL_SWEEPS[corner].normal[1];
    return hits;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", Body_SweepFootprint);
#endif
}
