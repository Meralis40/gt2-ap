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

// The moves CarColl_ResolveImpacts hands CarPhys_MoveAndCollide, in the scratchpad.
struct MoveScratch {
    unsigned char unk0[0xb4];
    int moveA[4];
    int moveB[4];
};
#define MOVE_SCRATCH ((MoveScratch *)0x1f800000)

extern int g_FrameDt;
int CarPhys_MoveAndCollide(CarPhys *body, int *move, int *rest, int count);

// 1 when v is outside lo..hi, for values whose difference can't overflow.
#define OUT_OF_RANGE(v, lo, hi) ((((lo) - (v)) ^ ((hi) - (v))) > 0)

// Car to car collisions, step 1 for one car: the corners of every other car, in this
// car's frame, into the current buffer of g_CarColl_Pairs. The outcode has one bit
// per side the corner is beyond (8 front, 4 rear, 2 left, 1 right).
static inline void CarColl_UpdateCar(RaceVehicle *cars, int count, int i, CarPhys *a)
{
    int buf = g_CarColl_Buf;
    int pos[3];
    int rel[2];

    if (a->ghost == 0 && a->aiLine != 4) {
        int j;
        int slot = 0;
        int angle = a->yaw & 0xfff;
        int sin = g_SinTable[angle];
        int cos = g_CosTable[angle];

        for (j = 0; j < count; j++) {
            CarPhys *b;
            int k;
            int c;

            if (i == j) {
                continue;
            }
            b = &cars[j].phys;
            if (b->ghost == 0 && b->aiLine != 4) {
                int dy;

                for (k = 0; k < 3; k++) {
                    pos[k] = a->pos[k];
                }
                dy = b->pos[2] - pos[2];
                if (!OUT_OF_RANGE(dy, -0x2000, 0x2000)) {
                    CarCollCorner *pair = g_CarColl_Pairs[i][slot];

                    for (c = 0; c < 4; c++, pair++) {
                        for (k = 0; k < 2; k++) {
                            rel[k] = b->footprint[b->bufIdx * 8 + c * 2 + k] - pos[k];
                        }
                        if (OUT_OF_RANGE(rel[1], -0x10000, 0xffff) || OUT_OF_RANGE(rel[0], -0x10000, 0xffff)) {
                            pair->outcode[buf] = 0xf;
                        } else {
                            int x = ((long long)cos * rel[0] + (long long)sin * rel[1]) / 4096;
                            int z;
                            int half;
                            int code;

                            pair->x[buf] = x;
                            z = ((long long)-sin * rel[0] + (long long)cos * rel[1]) / 4096;
                            pair->z[buf] = z;
                            code = (a->lenFront < z) << 3;
                            half = a->width / 2;
                            if (z < -a->lenRear) {
                                code |= 4;
                            }
                            if (x < -half) {
                                code |= 2;
                            }
                            if (half < x) {
                                code |= 1;
                            }
                            pair->outcode[buf] = code;
                        }
                    }
                }
            }
            slot++;
        }
    }
}

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

// Car to car collisions, step 1: flips g_CarColl_Buf and fills its side of
// g_CarColl_Pairs for every car.
#ifdef NON_MATCHING
// Register pressure: the game keeps s6 and s7 as scratch for the 64-bit products and
// spills the count and the corner pointer (frame 0x50); here the products themselves
// go to the stack (frame 0x90).
void CarColl_ComputeCorners(RaceVehicle *cars, int count)
{
    int i;

    g_CarColl_Buf = 1 - g_CarColl_Buf;
    for (i = 0; i < count; i++) {
        CarColl_UpdateCar(cars, count, i, &cars[i].phys);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_ComputeCorners);
#endif

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

// How far into the frame (0x1000: not at all) a corner of the other car first crossed
// a side of this car, between the two buffers of g_CarColl_Pairs. Sets the corner and
// the side (0 front, 1 rear, 2 left, 3 right).
#ifdef NON_MATCHING
// Only the register of the pair pointer differs (a2 in the game, a1 here).
int CarColl_FindImpact(int i, int slot, CarPhys *body, int *outCorner, int *outSide)
{
    int cur;
    int prev;
    int bestCorner;
    int bestSide;
    CarPhys *car = body;
    int carOffset;
    int slotOffset;
    int front;
    int rear;
    int cornerOffset;
    int best;
    int half;
    int c;

    best = 0x1000;
    bestCorner = -1;
    bestSide = -1;
    carOffset = i * sizeof(g_CarColl_Pairs[0]);
    slotOffset = slot * sizeof(g_CarColl_Pairs[0][0]);
    cur = g_CarColl_Buf;
    cornerOffset = 0;
    prev = 1 - cur;

    for (c = 0; c < 4; c++, cornerOffset += sizeof(CarCollCorner)) {
        int from;
        CarCollCorner *pair = (CarCollCorner *)((char *)g_CarColl_Pairs + carOffset + slotOffset + cornerOffset);
        int to;
        int crossed;
        int x0;
        int z0;
        int x1;
        int z1;
        int t;

        from = pair->outcode[prev];
        if (from == 0 || from == 0xf) {
            continue;
        }
        to = pair->outcode[cur];
        if (to == 0xf || (from & to) != 0) {
            continue;
        }
        crossed = from ^ to;
        half = car->width / 2;
        front = car->lenFront;
        rear = car->lenRear;
        x0 = pair->x[prev];
        z0 = pair->z[prev];
        x1 = pair->x[cur];
        z1 = pair->z[cur];
        if (crossed & 8) {
            t = Fixed_Div(front - z0, z1 - z0, 0);
            if (t < best && Int_InRange(-half, x0 + Fixed_Mul32(t, x1 - x0), half)) {
                best = t;
                bestCorner = c;
                bestSide = 0;
            }
        }
        if (crossed & 4) {
            t = Fixed_Div(-rear - z0, z1 - z0, 0);
            if (t < best && Int_InRange(-half, x0 + Fixed_Mul32(t, x1 - x0), half)) {
                best = t;
                bestCorner = c;
                bestSide = 1;
            }
        }
        if (crossed & 2) {
            t = Fixed_Div(-half - x0, x1 - x0, 0);
            if (t < best && Int_InRange(-rear, z0 + Fixed_Mul32(t, z1 - z0), front)) {
                best = t;
                bestCorner = c;
                bestSide = 2;
            }
        }
        if (crossed & 1) {
            t = Fixed_Div(half - x0, x1 - x0, 0);
            if (t < best && Int_InRange(-rear, z0 + Fixed_Mul32(t, z1 - z0), front)) {
                best = t;
                bestCorner = c;
                bestSide = 3;
            }
        }
    }
    *outCorner = bestCorner;
    *outSide = bestSide;
    return best;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_FindImpact);
#endif

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

// Car to car collisions, step 3: for every pair that hit, the impulse along the normal
// of the side that was hit, weighted by catchUp. Moves both cars out of the contact.
#ifdef NON_MATCHING
// The game keeps &normal, &delta and &impulse on the stack from the function's start
// and walks them in the loops; here they stay frame offsets. Frame 0x88, game 0x90.
void CarColl_ResolveImpacts(RaceVehicle *cars, int count)
{
    int impulse[3];
    short delta[3];
    short normal[3];
    int restA;
    int restB;
    int i;
    int j;
    int k;

    for (i = 1; i < count; i++) {
        for (j = 0; j < i; j++) {
            short toiA = g_CarColl_Toi[i][j];
            short toiB = *(g_CarColl_Toi[j] + i - 1);
            short toi;
            short left;
            CarPhys *a;
            CarPhys *b;
            int side;
            int angle;
            short sin;
            short cos;
            int speedA;
            int speedB;
            int total;
            int power;
            short weightA;
            short weightB;
            int movedA;
            int movedB;

            if (toiA == 0x1000 && toiB == 0x1000) {
                continue;
            }
            if (toiB >= toiA) {
                toi = toiA;
                a = &cars[i].phys;
                b = &cars[j].phys;
                side = g_CarColl_Side[i][j];
            } else {
                toi = toiB;
                a = &cars[j].phys;
                b = &cars[i].phys;
                side = g_CarColl_Side[j][i - 1];
            }
            left = 0x1000 - toi;
            angle = a->yaw & 0xfff;
            sin = g_SinTable[angle];
            cos = g_CosTable[angle];
            switch (side) {
            case 0:
                normal[0] = -sin;
                normal[1] = cos;
                break;
            case 1:
                normal[0] = sin;
                normal[1] = -cos;
                break;
            case 2:
                normal[0] = -cos;
                normal[1] = -sin;
                break;
            case 3:
                normal[0] = cos;
                normal[1] = sin;
                break;
            }
            speedB = 0;
            speedA = 0;
            normal[2] = 0;
            for (k = 0; k < 3; k++) {
                speedA += Fixed_Mul(normal[k], a->vel[k]);
                speedB -= Fixed_Mul(normal[k], b->vel[k]);
            }
            weightA = a->catchUp;
            weightB = b->catchUp;
            total = ((long long)speedA * weightA + (long long)speedB * weightB) / (weightA + weightB);
            for (k = 0; k < 3; k++) {
                impulse[k] = Fixed_Mul(normal[k], total);
            }
            if (total < 0) {
                total = -total;
            }
            power = total * 164 >> 12;
            a->wallHit += power;
            b->wallHit += power;
            a->wallContact |= 2;
            b->wallContact |= 2;
            for (k = 0; k < 3; k++) {
                int push = impulse[k] + Fixed_Mul(normal[k], 0);

                a->vel[k] -= push;
                b->vel[k] += push;
            }
            for (k = 0; k < 3; k++) {
                delta[k] = Fixed_Mul32_16(g_FrameDt, Fixed_Mul(left, impulse[k]));
                delta[k] += Fixed_MulShiftTrunc(normal[k], 0x1000, 5);
            }
            for (k = 0; k < 3; k++) {
                MOVE_SCRATCH->moveA[k] = -delta[k];
                MOVE_SCRATCH->moveB[k] = delta[k];
            }
            MOVE_SCRATCH->moveA[3] = 0;
            MOVE_SCRATCH->moveB[3] = 0;
            movedA = CarPhys_MoveAndCollide(a, MOVE_SCRATCH->moveA, &restA, 2);
            movedB = CarPhys_MoveAndCollide(b, MOVE_SCRATCH->moveB, &restB, 2);
            if (movedA != 0) {
                if (movedB == 0) {
                    for (k = 0; k < 3; k++) {
                        b->pos[k] += Fixed_Mul(restA, delta[k]);
                    }
                }
            } else if (movedB != 0) {
                for (k = 0; k < 3; k++) {
                    a->pos[k] -= Fixed_Mul(restB, delta[k]);
                }
            }
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_ResolveImpacts);
#endif

// Step 1 again for the cars CarColl_ResolveImpacts moved (wallContact bit 1).
#ifdef NON_MATCHING
// Same register pressure as CarColl_ComputeCorners.
void CarColl_RecomputeCorners(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *body = &cars[i].phys;

        if (body->wallContact & 2) {
            Body_ComputeFootprint(body);
            CarColl_UpdateCar(cars, count, i, body);
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_RecomputeCorners);
#endif

// Sets pushVec of cars whose corners are inside each other, away from the side of
// least overlap.
#ifdef NON_MATCHING
// Registers differ throughout; the logic follows the game's branch by branch.
void CarColl_SeparateOverlaps(RaceVehicle *cars, int count)
{
    int cornerX[4];
    int cornerZ[4];
    int inside[4];
    int push[2];
    int i;
    int j;

    for (i = 0; i < count; i++) {
        int k;

        for (k = 0; k < 2; k++) {
            cars[i].phys.pushVec[k] = 0;
        }
    }
    for (i = 0; i < count; i++) {
        CarPhys *a = &cars[i].phys;
        int front = a->lenFront * 15 / 16;
        int rear = a->lenRear * 15 / 16;
        int half = a->width * 15 / 32;
        int slot;

        if (a->ghost != 0 || a->aiLine == 4) {
            continue;
        }
        slot = 0;
        for (j = 0; j < count; j++) {
            CarPhys *b;
            int any;
            int c;
            int buf;
            int angle;
            int dx;
            int dz;
            int along;
            int across;
            int ratioX;
            int ratioZ;
            int best;
            int side;
            short sin;
            short cos;

            if (i == j) {
                continue;
            }
            b = &cars[j].phys;
            if (b->ghost != 0 || b->aiLine == 4) {
                slot++;
                continue;
            }
            any = 0;
            buf = g_CarColl_Buf;
            for (c = 0; c < 4; c++) {
                inside[c] = 0;
                if (g_CarColl_Pairs[i][slot][c].outcode[buf] != 0xf) {
                    int x = g_CarColl_Pairs[i][slot][c].x[buf];

                    if (x < half && -half < x) {
                        int z = g_CarColl_Pairs[i][slot][c].z[buf];

                        if (z < front && -rear < z) {
                            any = 1;
                            cornerX[c] = x;
                            cornerZ[c] = z;
                            inside[c] = 1;
                        }
                    }
                }
            }
            if (any == 0) {
                slot++;
                continue;
            }
            dx = b->pos[0] - a->pos[0];
            dz = b->pos[1] - a->pos[1];
            angle = a->yaw & 0xfff;
            sin = g_SinTable[angle];
            cos = g_CosTable[angle];
            along = Fixed_Dot2Shift(g_CosTable[angle], dx, g_SinTable[angle], dz, 0);
            across = Fixed_Dot2Shift(-g_SinTable[angle], dx, g_CosTable[angle], dz, 0);
            ratioX = (along < 0 ? -along : along);
            ratioX = (ratioX << 12) / half;
            if (across >= 0) {
                ratioZ = (across << 12) / front;
            } else {
                ratioZ = (-across << 12) / rear;
            }
            best = 0;
            side = -1;
            if (ratioZ < ratioX) {
                for (c = 0; c < 4; c++) {
                    if (inside[c] != 0) {
                        int t;

                        if (along > 0) {
                            t = Fixed_Div(half - cornerX[c], half * 2, 0);
                        } else {
                            t = Fixed_Div(half + cornerX[c], half * 2, 0);
                        }
                        if (best < t) {
                            best = t;
                            side = 2;
                            if (along > 0) {
                                side = 3;
                            }
                        }
                    }
                }
            } else {
                for (c = 0; c < 4; c++) {
                    if (inside[c] != 0) {
                        int t;

                        if (across > 0) {
                            t = Fixed_Div(front - cornerZ[c], front + rear, 0);
                        } else {
                            t = Fixed_Div(rear + cornerZ[c], front + rear, 0);
                        }
                        if (best < t) {
                            best = t;
                            side = across < 1;
                        }
                    }
                }
            }
            if (best != 0) {
                int k;

                switch (side) {
                case 0:
                    push[0] = -sin;
                    push[1] = cos;
                    break;
                case 1:
                    push[0] = sin;
                    push[1] = -cos;
                    break;
                case 2:
                    push[0] = -cos;
                    push[1] = -sin;
                    break;
                case 3:
                    push[0] = cos;
                    push[1] = sin;
                    break;
                }
                best += 0x800;
                if (best > 0x1000) {
                    best = 0x1000;
                }
                best = Fixed_MulFloor(best, 0x4c901);
                for (k = 0; k < 2; k++) {
                    int amount = Fixed_Mul(push[k], best);

                    a->pushVec[k] += amount;
                    b->pushVec[k] -= amount;
                }
            }
            slot++;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/carcoll", CarColl_SeparateOverlaps);
#endif

// 2 when the race options turn on collision penalties (raceType 0, or 2, 4, 12 outside
// game 1) on a course without flag 4, 1 for raceType 3, else 0.
int Race_GetCollisionRule(void)
{
    GameState *gs = &g_GameState;
    char *base0 = (char *)gs;
    int offset0 = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base0 + offset0);
    int result = 0;

    switch (setup->raceType) {
    case 3:
        result = 1;
        break;
    case 0:
        if ((Course_Get(g_RaceWorld.course)->flags & 4) == 0) {
            GameState *gs2 = &g_GameState;
            char *base2 = (char *)gs2;
            int offset2 = 0xbf7c;

            if (((RaceSetup *)(base2 + offset2))->options[4] == 1) {
                result = 2;
            }
        }
        break;
    case 2:
    case 4:
    case 12:
        if ((Course_Get(g_RaceWorld.course)->flags & 4) == 0) {
            GameState *gs2 = &g_GameState;
            char *base2 = (char *)gs2;
            int offset2 = 0xbf7c;
            RaceSetup *setup2 = (RaceSetup *)(base2 + offset2);

            if (setup2->game != 1 && setup2->options[1] == 1) {
                result = 2;
            }
        }
        break;
    }
    return result;
}

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
