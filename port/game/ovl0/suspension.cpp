// OVL0 80043388-80046bac (suspension): Suspension and drivetrain.
// The functions that don't match yet are the original assembly.
#include "common.h"
#include "gamestate.h"

extern "C" {
// The drivetrain block of CarPhys, from +0x370.
struct CarDrive {
    unsigned char layout; /* 0, 4, 5 rear; 1, 2 front; 3, 6 four wheel */
    unsigned char unk1;
    unsigned char gearCount;
    unsigned char driveAxle;
    short centerSplit;
    short lockLimit; /* non-zero: the centre coupling can lock */
    short engineInertia;
    unsigned char diffType[2]; /* front, rear: 0 open, 1 locked, 2 viscous, 3-5 limited slip, 6-7 active */
    int diffPreload[2];
    int diffAccel[2];
    int diffDecel[2];
    unsigned short launchLimit;
    unsigned short launchRpm;
    unsigned short shiftRpm[6];
    int gearRatio[8];
    int gearSpeed[8];
    short tyreRadius[2];
    short invTyreRadius[2];
    unsigned char unk7c[8];
    short slipTorque;
    short dragTorque;
    int diffInertia[2];
    int axleInertia[2];
    int centerGain;
    int inertia;
};

// A wheel's ground query in the scratchpad, from 0x1f800004 (World_GetGroundInfo).
struct WheelProbe {
    unsigned char surface;
    unsigned char surface2;
    unsigned char flags;
    unsigned char unk3;
    unsigned short section;
    unsigned char unk6[2];
    int pos[3]; /* x, y, z x16 */
    int height;
};
#define WHEEL_PROBES ((WheelProbe (*)[4])0x1f800004)

// Per-wheel values in the scratchpad, 4 per 0x90 bytes of car, from 0x1f800004.
struct WheelScratch {
    int brakeForce;
    int forceFwd;
    int forceSide;
    int torque;
    unsigned char unk10[0xc];
};
#define WHEEL_SCRATCH(car, wheel) ((WheelScratch *)(0x1f800004 + (car) * 0x90 + (wheel) * sizeof(WheelScratch)))

// Per-car values in the scratchpad (0x1f800000 + carIndex * 0x90).
struct PhysCarScratch {
    short bias; /* the previous car's: ABS slip (CarPhysScratch) */
    unsigned char unk2[0x72];
    int axleTorque[2];
    int engineSpeed;
    int wheelSpeed;
    int invMass;
    int rollDrag;
    int torqueRatio;
};
#define SCRATCH ((PhysCarScratch *)0x1f800000)

// Per-car suspension values in the scratchpad, every 0x34 bytes.
struct SuspScratch {
    int unk0;
    int rate;
    int height[4];
    int load[4];
    int damper[4];
};
#define SUSP_SCRATCH(car) ((SuspScratch *)(0x1f800000 + (car) * 0x34))

#define g_FrameDt (*(int *)0x1f800000)
#define CARPHYS_FIELD(car, type, offset) (*(type *)((char *)(car) + (offset)))
#define CAR_UP(car) ((short *)((char *)(car) + 0x678))

extern unsigned char g_Race_IsDemo;
extern RaceSetup g_RaceSetup;
extern short g_SinTable[];
extern short g_CosTable[];
extern short g_SurfaceGrip[];
extern short g_SurfaceTable3[];
extern unsigned char g_SurfaceTable4[];
struct AiLineSet {
    unsigned char unk0[0x18];
    char *unk18;
};
extern AiLineSet *g_AiLines;

int Race_AllowsOffCourseFlag(void);
int Body_ChecksCourseOut(CarPhys *body);
int Pit_IsInPitZone(int trackPos, int unused);
void Race_ShowMessage(int car, int message);
void World_GetGroundInfo(void *world, WheelProbe *probe);
int PitStop_GetLift(int timer, int *phase);
int Engine_ApplyRevLimiter(CarPhys *car, int throttle);
int Engine_UpdateTurbo(CarPhys *car, int throttle);
int Gearbox_GearForSpeed(CarPhys *car, int speed);
int Diff_GetCouplingTorque(CarPhys *car, int torque, int front, int rear);

// Tilts *angle by the difference of two penetration depths over a wheelbase, and
// returns half of the smaller depth.
static inline int Suspension_Tilt(short a, short b, int span, short *angle)
{
    if (b < a) {
        *angle += Math_Atan2(a - b, span);
        return (unsigned int)(b / 2);
    }
    if (a < b) {
        *angle += Math_Atan2(a - b, span);
        return (unsigned int)(a / 2);
    }
    return (unsigned int)(a / 2);
}

// Updates the wheel offsets, then fills each wheel's ground query.
void RaceCars_PrepareWheelProbes(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        Body_UpdateWheelOffsets(&cars[i].phys);
    }
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        int w;

        for (w = 0; w < 4; w++) {
            CarPhysWheel *wheel = &car->wheels[w];
            int fade = Fixed_MulFloor(Math_ApproxDist2(wheel->velFwd, wheel->velLat), 0x52);
            WheelProbe *probe;

            if (fade > 0x1000) {
                fade = 0x1000;
            }
            wheel->bumpFade = fade;
            probe = &WHEEL_PROBES[i][w];
            probe->pos[0] = (car->pos[0] + wheel->offX) << 4;
            probe->pos[1] = (car->pos[2] + wheel->offZ) << 4;
            probe->pos[2] = (car->pos[1] + wheel->offY) << 4;
            probe->section = car->section;
        }
    }
}

// World_GetGroundInfo for every wheel.
// Only two registers swap: the count and the scratchpad base (s6, s5 in the game).
void RaceCars_ProbeGround(RaceVehicle *cars, int count)
{
    int i;
    char *scratch = (char *)0x1f800000;

    for (i = 0; i < count; i++) {
        WheelProbe *probe = (WheelProbe *)(i * sizeof(WheelProbe[4]) + 4 + scratch);
        int w;

        for (w = 0; w < 4; w++) {
            WheelProbe *p = probe + w;

            World_GetGroundInfo(&g_Race_IsDemo, p);
        }
    }
}

// Takes the ground height, surface and bumps from the queries. Counts wheels off the
// course, and shows the course-out message in a race that checks it.
// Registers differ; the game keeps more in memory (frame 0x50).
void RaceCars_ApplyGroundProbes(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        int rough = 0;
        int dirt = 0;
        int off = 0;
        int w;
        int result;

        car->section = WHEEL_PROBES[i][0].section;
        for (w = 0; w < 4; w++) {
            WheelProbe *probe = &WHEEL_PROBES[i][w];
            CarPhysWheel *wheel = &car->wheels[w];
            int height = probe->height;
            int amplitude;

            if (height == 0x7fffffff) {
                off++;
                wheel->bump = 0;
                continue;
            }
            wheel->groundY = (height + wheel->groundY16) >> 5;
            wheel->groundY16 = height;
            wheel->surface = probe->surface;
            wheel->surface2 = probe->surface2;
            if (probe->flags & 2) {
                dirt++;
            }
            if (wheel->surface >= 2) {
                rough++;
            }
            if (wheel->surface == 6) {
                wheel->surface = 5;
                off++;
            }
            amplitude = g_SurfaceGrip[wheel->surface];
            if (amplitude != 0) {
                int scale = g_SurfaceTable3[wheel->surface];
                int sum = g_SinTable[Fixed_MulFloor(scale, probe->pos[0]) & 0xfff] +
                          g_CosTable[Fixed_MulFloor(scale, probe->pos[2]) & 0xfff];

                if (g_SurfaceTable4[wheel->surface] != 0) {
                    sum = Fixed_Mul32(sum, wheel->bumpFade);
                }
                wheel->bump = sum * amplitude >> 12;
            } else {
                wheel->bump = 0;
            }
        }
        if (dirt != 0 && Race_AllowsOffCourseFlag() != 0) {
            car->flags |= 0x10;
        } else {
            car->flags &= 0xef;
        }
        if (Body_ChecksCourseOut(car) == 1 && car->hitFeedback == 0) {
            result = 0;
            if (off == 4) {
                car->unk6fb = 0xff;
                result = 1;
            } else {
                if ((dirt == 4 && Pit_IsInPitZone(car->trackPos, 0) != 0) ||
                    (((RaceSetup *)((char *)&g_GameState + 0xbf7c))->raceType == 3 && rough == 4)) {
                    car->unk6fb++;
                    result = 1;
                }
            }
            if (result == 0) {
                car->unk6fb = 0;
            } else if (car->unk6fb != 0) {
                Race_ShowMessage(car->carIndex, 2);
                car->hitFeedback = 2;
            }
        }
    }
}

// The spring, bump rubber and damper forces of one wheel. Returns spring + damper.
// The game keeps height in s3 and force in s4; here they swap.
int Wheel_ComputeSpringForce(CarPhys *car, CarPhysWheel *wheel, int height, SuspAxle *axle)
{
    int force;
    int clamped;
    int speed;
    int prev;
    int average;
    int damper;
    int total;

    if (axle->travelMax < height) {
        wheel->suspPos = axle->travelMax;
        wheel->suspLength = 0;
        wheel->damper = 0;
        wheel->spring = -axle->preload;
        return -axle->preload;
    }
    if (height < axle->travelMin) {
        clamped = axle->travelMin;
    } else {
        clamped = height;
    }
    force = -Fixed_Mul32(clamped, axle->springRate);
    if (clamped < axle->bumpStart) {
        short depth = axle->bumpStart - clamped;

        force += Fixed_MulFloor(depth * depth >> 12, axle->bumpRate);
    }
    speed = Fixed_Mul(car->rateScale, height - wheel->suspPos);
    prev = wheel->suspSpeed;
    wheel->suspSpeed = speed;
    average = (speed + prev) / 2;
    if (average < 0) {
        if (average < -0x1000) {
            average = -0x1000;
        }
        damper = axle->bumpBase + Fixed_Mul(axle->bumpGain, Math_ISqrt(-average - axle->bumpDead, 6));
    } else {
        if (average >= 0x1001) {
            average = 0x1000;
        }
        damper = -axle->reboundBase - Fixed_Mul(axle->reboundGain, Math_ISqrt(average - axle->reboundDead, 6));
    }
    wheel->spring = force;
    total = force;
    wheel->suspPos = clamped;
    wheel->damper = damper;
    total += damper;
    wheel->suspLength = total + axle->preload;
    if (wheel->suspLength < 0) {
        wheel->suspLength = 0;
    }
    return total;
}

// Pit jack: raises the body to height + the car's rest offset. 0 while it is above that.
int Body_LiftToHeight(CarPhys *car, int height)
{
    int y = CARPHYS_FIELD(car, int, 0x688) + height;

    if (y < car->pos[2]) {
        return 0;
    }
    car->pos[2] = y;
    CARPHYS_FIELD(car, short, 0x652) = 0;
    CARPHYS_FIELD(car, short, 0x650) = 0;
    car->roll = 0;
    car->pitch = 0;
    car->vel[2] = 0;
    return 1;
}

// One suspension step: wheel heights, spring forces, weight transfer, then the body's
// pitch, roll and vertical speed, and any wheel at its stop lifts the body directly.
// Draft from the assembly, not tuned. The stop handling is written with an inline, Suspension_Tilt.
void RaceCars_UpdateSuspension(RaceVehicle *cars, int count)
{
    int base[4];
    int phase;
    int i;

    // The suspension height of each wheel, and which ones are at the stop.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        if (car->aiMode != 7) {
            SuspScratch *s = SUSP_SCRATCH(i);
            int rate = (CARPHYS_FIELD(car, short, 0x6a0) << 12) / CARPHYS_FIELD(car, short, 0x67c);
            int y;
            int w;
            int contact;
            int bit;

            s->rate = rate;
            base[1] = base[0] = car->susp[0].rideHeight - car->tyreRadius[0];
            base[3] = base[2] = car->susp[1].rideHeight - car->tyreRadius[1];
            y = car->pos[2];
            for (w = 0; w < 4; w++) {
                CarPhysWheel *wheel = &car->wheels[w];

                s->height[w] = Fixed_Mul32(y + wheel->offZ - wheel->groundY - wheel->bump, rate) + base[w];
            }
            contact = 0;
            bit = 1;
            for (w = 0; w < 4; w++) {
                if (s->height[w] < car->susp[w >> 1].travelMin) {
                    contact |= bit;
                }
                bit <<= 1;
            }
            car->wheelContact = contact;
        }
    }

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        if (car->aiMode != 7) {
            int w;

            for (w = 0; w < 4; w++) {
                Wheel_ComputeSpringForce(car, &car->wheels[w], SUSP_SCRATCH(i)->height[w], &car->susp[w >> 1]);
            }
        }
    }

    // The load of each wheel with the anti-roll bars and the weight transfer.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        if (car->aiMode != 7) {
            SuspScratch *s = SUSP_SCRATCH(i);
            int front = car->wheels[0].suspPos - car->wheels[1].suspPos;
            int rear;
            int rollFront;
            int rollRear;
            int side;
            int along;
            int w;

            if (front >= 0) {
                rollFront = Fixed_MulFloor(car->susp[0].antiRoll, front * front >> 12);
            } else {
                rollFront = -Fixed_MulFloor(car->susp[0].antiRoll, front * front >> 12);
            }
            rear = car->wheels[2].suspPos - car->wheels[3].suspPos;
            if (rear >= 0) {
                rollRear = Fixed_MulFloor(car->susp[1].antiRoll, rear * rear >> 12);
            } else {
                rollRear = -Fixed_MulFloor(car->susp[1].antiRoll, rear * rear >> 12);
            }
            side = Fixed_Mul32(CARPHYS_FIELD(car, short, 0x1e), -CARPHYS_FIELD(car, int, 0x728));
            along = Fixed_Mul32(CARPHYS_FIELD(car, short, 0x20),
                                CARPHYS_FIELD(car, int, 0x734) - CARPHYS_FIELD(car, int, 0x72c));
            s->load[0] = car->wheels[0].spring - rollFront - CARPHYS_FIELD(car, int, 0x71c) / 2 + along + side;
            s->load[1] = car->wheels[1].spring + rollFront - CARPHYS_FIELD(car, int, 0x71c) / 2 + along - side;
            s->load[2] = car->wheels[2].spring - rollRear - CARPHYS_FIELD(car, int, 0x720) / 2 - along + side;
            s->load[3] = car->wheels[3].spring + rollRear - CARPHYS_FIELD(car, int, 0x720) / 2 - along - side;
            for (w = 0; w < 4; w++) {
                s->damper[w] = car->wheels[w].damper;
            }
        }
    }

    // The body's pitch, roll and vertical speed.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        SuspScratch *s = SUSP_SCRATCH(i);
        int old;
        int rate;
        int next;
        int lift;
        int k;

        if (car->aiMode == 7) {
            continue;
        }
        if (car->pitTimer >= 2 && Body_LiftToHeight(car, PitStop_GetLift(car->pitTimer, &phase)) != 0) {
            if (phase == 2) {
                int target = *(int *)((char *)g_AiLines->unk18 + (signed char)car->pitBox * 40 + 0x28);
                int diff = Angle_Diff(target, car->yaw);

                if (diff != 0) {
                    int step = g_FrameDt * 341 >> 16;

                    if (step < diff) {
                        car->yaw += step;
                    } else if (diff < -step) {
                        car->yaw -= step;
                    } else {
                        car->yaw = target;
                    }
                }
            }
            Matrix_FromAngles(car->fwdDir, car->rightDir, CAR_UP(car), car->pitch, car->roll, car->yaw);
            continue;
        }
        g_FrameDt = car->timeStep;
        rate = Fixed_Mul32(s->load[0] + s->load[1], CARPHYS_FIELD(car, int, 0x6c));
        CARPHYS_FIELD(car, short, 0x650) +=
            Fixed_Mul32_16(rate - Fixed_Mul32(s->load[2] + s->load[3], CARPHYS_FIELD(car, int, 0x70)), g_FrameDt);
        rate = Fixed_Mul32(s->damper[0] + s->damper[1], CARPHYS_FIELD(car, int, 0x6c));
        rate = Fixed_Mul32_16(rate - Fixed_Mul32(s->damper[2] + s->damper[3], CARPHYS_FIELD(car, int, 0x70)),
                              g_FrameDt);
        old = CARPHYS_FIELD(car, short, 0x650);
        next = old + rate;
        if ((old > 0 && next < 0) || (old < 0 && next > 0)) {
            CARPHYS_FIELD(car, short, 0x650) = 0;
        } else {
            if (next >= 0x4000) {
                next = 0x3fff;
            } else if (next < -0x4000) {
                next = -0x4000;
            }
            CARPHYS_FIELD(car, short, 0x650) = next;
        }
        car->pitch += Fixed_Mul16Shift(g_FrameDt, CARPHYS_FIELD(car, short, 0x650), 4);
        if (car->pitch < -0x200) {
            car->pitch = -0x200;
            CARPHYS_FIELD(car, short, 0x650) = 0;
        } else if (car->pitch >= 0x201) {
            car->pitch = 0x200;
            CARPHYS_FIELD(car, short, 0x650) = 0;
        }
        rate = Fixed_Mul32(s->load[0] - s->load[1], CARPHYS_FIELD(car, int, 0x74));
        CARPHYS_FIELD(car, short, 0x652) +=
            Fixed_Mul32_16(rate + Fixed_Mul32(s->load[2] - s->load[3], CARPHYS_FIELD(car, int, 0x78)), g_FrameDt);
        rate = Fixed_Mul32(s->damper[0] - s->damper[1], CARPHYS_FIELD(car, int, 0x6c));
        rate = Fixed_Mul32_16(rate + Fixed_Mul32(s->damper[2] - s->damper[3], CARPHYS_FIELD(car, int, 0x70)),
                              g_FrameDt);
        old = CARPHYS_FIELD(car, short, 0x652);
        next = old + rate;
        if ((old > 0 && next < 0) || (old < 0 && next > 0)) {
            CARPHYS_FIELD(car, short, 0x652) = 0;
        } else {
            if (next >= 0x4000) {
                next = 0x3fff;
            } else if (next < -0x4000) {
                next = -0x4000;
            }
            CARPHYS_FIELD(car, short, 0x652) = next;
        }
        car->roll += Fixed_Mul16Shift(g_FrameDt, CARPHYS_FIELD(car, short, 0x652), 4);
        if (car->roll < -0x200) {
            car->roll = -0x200;
            CARPHYS_FIELD(car, short, 0x652) = 0;
        } else if (car->roll >= 0x201) {
            car->roll = 0x200;
            CARPHYS_FIELD(car, short, 0x652) = 0;
        }
        {
            int loaded = car->wheels[0].suspLength + car->wheels[1].suspLength + car->wheels[2].suspLength +
                         car->wheels[3].suspLength;

            if ((signed char)car->airborne == 0) {
                if (loaded == 0) {
                    car->airborne = 1;
                }
            } else if (loaded != 0) {
                car->airborne = 0;
            }
        }
        Matrix_FromAngles(car->fwdDir, car->rightDir, CAR_UP(car), car->pitch, car->roll, car->yaw);
        lift = Fixed_Mul(s->load[0] + s->load[1] + s->load[2] + s->load[3], CARPHYS_FIELD(car, short, 0x2c));
        lift = Fixed_Mul32_16(lift, g_FrameDt);
        for (k = 0; k < 3; k++) {
            car->vel[k] += Fixed_Mul32(CAR_UP(car)[k], lift);
        }
        // Gravity.
        car->vel[2] -= g_FrameDt * 40141 >> 16;
        old = car->vel[2];
        lift = Fixed_Mul(s->damper[0] + s->damper[1] + s->damper[2] + s->damper[3], CARPHYS_FIELD(car, short, 0x2c));
        lift = Fixed_Mul32_16(lift, g_FrameDt);
        for (k = 0; k < 3; k++) {
            car->vel[k] += Fixed_Mul32(CAR_UP(car)[k], lift);
        }
        if ((car->vel[2] ^ old) < 0) {
            car->vel[2] = 0;
        }
    }

    // The wheels at their stops lift and tilt the body straight away.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        SuspScratch *s = SUSP_SCRATCH(i);
        int contact;
        int w;
        int bit;
        int sumA;
        int sumB;
        int floor;
        int k;
        int y;
        int rate;

        if (car->aiMode == 7 || car->wheelContact == 0) {
            continue;
        }
        Body_UpdateWheelHeights(car);
        rate = s->rate;
        base[1] = base[0] = car->susp[0].rideHeight - car->tyreRadius[0];
        base[3] = base[2] = car->susp[1].rideHeight - car->tyreRadius[1];
        y = car->pos[2];
        for (w = 0; w < 4; w++) {
            CarPhysWheel *wheel = &car->wheels[w];

            s->height[w] = Fixed_Mul32(y + wheel->offZ - wheel->groundY - wheel->bump, rate) + base[w];
        }
        contact = car->wheelContact << 4;
        bit = 1;
        for (w = 0; w < 4; w++) {
            if (s->height[w] < car->susp[w >> 1].travelMin) {
                contact |= bit;
            }
            bit <<= 1;
        }
        car->wheelContact = contact;
        if (contact & 0xf) {
            short *depth = (short *)base;
            int raise = 0;
            short midFront;
            short midRear;
            short left;
            short right;

            for (w = 0; w < 4; w++) {
                depth[w] = car->susp[w >> 1].travelMin - (short)s->height[w];
                if (depth[w] < 0) {
                    depth[w] = 0;
                }
            }
            midFront = (unsigned int)((depth[0] + depth[1]) / 2);
            midRear = (unsigned int)((depth[2] + depth[3]) / 2);
            left = depth[0] + Fixed_Mul32(depth[2] - depth[0], car->rearWeight);
            right = depth[1] + Fixed_Mul32(depth[3] - depth[1], car->rearWeight);
            if (left != 0 || right != 0) {
                raise = Suspension_Tilt(left, right, CARPHYS_FIELD(car, short, 0x12), &car->roll);
            }
            if (midFront != 0 || midRear != 0) {
                raise += Suspension_Tilt(midFront, midRear, CARPHYS_FIELD(car, short, 0x10), &car->pitch);
            }
            car->pos[2] += (short)raise;
        }
        sumA = 0;
        sumB = 0;
        for (k = 0; k < 3; k++) {
            int v = car->vel[k];

            sumA += Fixed_MulFloor(CARPHYS_FIELD(car, short, 0x68c + k * 2), v);
            sumB += CARPHYS_FIELD(car, short, 0x694 + k * 2) * v >> 12;
        }
        floor = Fixed_Mul32(sumA, CARPHYS_FIELD(car, short, 0x690));
        floor += Fixed_Mul32(sumB, CARPHYS_FIELD(car, short, 0x698));
        if (floor > 0) {
            floor = 0;
        }
        if (car->vel[2] < floor) {
            car->vel[2] = floor;
        }
    }
}

// The lowest forward gear that can reach speed (0 when going backwards).
// The game keeps the /60 constant in s4 across the loop.
int Gearbox_GearForSpeed(CarPhys *car, int speed)
{
    CarDrive *d = (CarDrive *)&car->driveLayout;
    int gear;

    if (speed < 0) {
        return 0;
    }
    if (d->gearCount < 3) {
        return 1;
    }
    for (gear = 1; gear < d->gearCount; gear++) {
        if (speed < ((long long)(((d->launchRpm + 500) << 12) / 60) << 12) / d->gearSpeed[gear]) {
            return gear;
        }
    }
    return d->gearCount;
}

// Picks the next gear, or -1 to stay: automatic with unk642 0, manual with 1, and
// always first gear out of a pit stop. input[0] is reverse, input[1] the shift.
// Draft from the assembly; the structure differs.
int Gearbox_SelectGear(CarPhys *car, signed char *input)
{
    CarDrive *d;
    int mode;
    int gear;
    int low;
    int high;
    int front;
    int rear;
    int next;
    int w;

    if (car->pitTimer != 0) {
        return car->gear == 1 ? -1 : 1;
    }
    mode = 0;
    if (car->aiMode == 0) {
        mode = car->unk642;
    }
    if (mode == 0) {
        gear = car->gear;
        d = (CarDrive *)&car->driveLayout;
        if (car->startDelay != 0) {
            car->startDelay--;
            return -1;
        }
        if (input[0] != 0) {
            return gear != 0 ? 0 : -1;
        }
        if (car->speedFwd >= 0x473 && gear == 0) {
            return 1;
        }
        if (car->throttleTarget != 0) {
            if (car->speedFwd < 0x472) {
                return gear != 1 ? 1 : -1;
            }
        } else if (car->speedFwd < -0x472) {
            if (gear != 0) {
                return 0;
            }
            goto down;
        }
        if (gear > 0 && gear < d->gearCount) {
            int top = ((long long)(((car->engine.redline + 500) << 12) / 60) << 12) / d->gearSpeed[gear];

            if (top < car->speedFwd) {
                return gear + 1;
            }
        }
    down:
        if (car->clutchIn != 1 || gear == 0) {
            return -1;
        }
        if (gear == 1) {
            low = -0x4e20;
            high = d->launchRpm;
        } else {
            high = 0x4e20;
            if (gear < d->gearCount) {
                high = d->launchRpm;
            }
            low = d->shiftRpm[gear - 2];
            if (car->throttleTarget >= 0xc01) {
                int floor = (unsigned short)(d->launchLimit + 500);

                low -= 500;
                if (low < floor) {
                    low = floor;
                }
            }
        }
        front = 0;
        rear = 0;
        switch (d->layout) {
        case 1:
        case 2:
            front = 1;
            rear = 0;
            break;
        case 0:
        case 4:
        case 5:
            front = 0;
            rear = 1;
            break;
        case 3:
        case 6:
            front = 1;
            rear = 1;
            break;
        }
        if (car->rpm < low) {
            next = car->gear - 1;
            for (w = 0; w < 4; w++) {
                if (front == 0 && w < 2) {
                    continue;
                }
                if (rear == 0 && w >= 2) {
                    return next;
                }
                if (car->wheels[w].suspLength == 0) {
                    return -1;
                }
                if (car->tyres[w >> 1].slipLimitDown < car->wheels[w].unk44) {
                    return -1;
                }
            }
            return next;
        }
        if (high < car->rpm) {
            next = car->gear + 1;
            for (w = 0; w < 4; w++) {
                if (front == 0 && w < 2) {
                    continue;
                }
                if (rear == 0 && w >= 2) {
                    return next;
                }
                if (car->wheels[w].suspLength == 0) {
                    return -1;
                }
                if (car->wheels[w].unk44 <
                    (short)-(car->wheels[w].slipGrip * -car->tyres[w >> 1].slipLimitUp >> 12)) {
                    return -1;
                }
            }
            return next;
        }
        return -1;
    }
    if (mode == 1) {
        gear = car->gear;
        if (input[0] != 0) {
            return gear != 0 ? 0 : -1;
        }
        if (gear == 0 && car->throttle != 0) {
            next = Gearbox_GearForSpeed(car, car->speedFwd);
            return next != 0 ? next : 1;
        }
        if (car->startDelay != 0) {
            car->startDelay--;
            if (car->startDelay == 0 && input[1] != 0) {
                car->startDelay = 1;
            }
            return -1;
        }
        if (input[1] > 0 && gear < car->gearCount) {
            car->startDelay = 1;
            return car->gear + 1;
        }
        if (input[1] < 0 && gear >= 2) {
            car->startDelay = 1;
            return car->gear - 1;
        }
        return -1;
    }
    return -1;
}

// Body axes from pitch, roll and yaw.
void Matrix_FromAngles(short *row0, short *row1, short *row2, int pitch, int roll, int yaw)
{
    int sinRoll = g_SinTable[roll & 0xfff];
    int cosYaw = g_CosTable[yaw & 0xfff];
    int sinPitch = g_SinTable[pitch & 0xfff];
    int cosPitch = g_CosTable[pitch & 0xfff];
    int cosRoll = g_CosTable[roll & 0xfff];
    int sinYaw = g_SinTable[yaw & 0xfff];
    int a = Fixed_Mul32(cosYaw, sinRoll);
    int b = Fixed_Mul32(sinYaw, sinRoll);

    row1[0] = Fixed_Mul32(cosYaw, cosRoll);
    row1[1] = Fixed_Mul32(sinYaw, cosRoll);
    row1[2] = -sinRoll;
    row0[0] = (a * sinPitch + -sinYaw * cosPitch) / 4096;
    row0[1] = (b * sinPitch + cosYaw * cosPitch) / 4096;
    row0[2] = Fixed_Mul32(cosRoll, sinPitch);
    row2[0] = (a * cosPitch + sinYaw * sinPitch) / 4096;
    row2[1] = (b * cosPitch + -cosYaw * sinPitch) / 4096;
    row2[2] = Fixed_Mul32(cosRoll, cosPitch);
}

// Wraps an angle to -0x800..0x7ff.
int Angle_Wrap(int a)
{
    if (a < -0x800) {
        do {
            a += 0x1000;
        } while (a < -0x800);
        return a;
    }
    while (a >= 0x800) {
        a -= 0x1000;
    }
    return a;
}

// a - b, wrapped.
int Angle_Diff(int a, int b)
{
    int diff = Angle_Wrap(a) - Angle_Wrap(b);

    if (diff >= 0x800) {
        diff -= 0x1000;
    } else if (diff < -0x800) {
        diff += 0x1000;
    }
    return diff;
}

// Engine torque through the torque converter of an automatic (gearCount 2): the ratio
// follows the speed ratio, slowly when it has to rise.
// Registers and the ratio step differ.
int Drivetrain_ClutchTorque(CarPhys *car)
{
    CarDrive *d = (CarDrive *)&car->driveLayout;
    int ratio = 0x1000;
    int throttle;

    if (*(unsigned short *)&car->gear == 0x101) {
        int limit;
        int target;
        int wheels;

        ratio = ((long long)car->engineSpeed << 12) / SCRATCH[car->carIndex].wheelSpeed;
        if (ratio > 0x1000) {
            ratio = 0x1000;
        } else if (ratio < d->gearRatio[2]) {
            ratio = d->gearRatio[2];
        }
        limit = (d->shiftRpm[0] << 12) / 60;
        wheels = SCRATCH[car->carIndex].wheelSpeed;
        target = 0x1000;
        if (limit < wheels) {
            target = ((long long)limit << 12) / wheels;
            if (target < d->gearRatio[2]) {
                target = d->gearRatio[2];
            }
        }
        if (target != ratio) {
            int step = (g_FrameDt << 9 >> 16) * car->throttleTarget >> 12;

            if (ratio < target) {
                ratio += step;
                if (target < ratio) {
                    ratio = target;
                }
            } else {
                ratio -= step;
                if (ratio < target) {
                    ratio = target;
                }
            }
        }
    }
    SCRATCH[car->carIndex].torqueRatio = ratio;
    throttle = car->throttleTarget;
    ratio = Fixed_Mul(ratio, Engine_ApplyRevLimiter(car, throttle));
    car->engineLoad = throttle;
    return ratio;
}

// The drive model of gearCount 1: creep from the wheel speed, and the idle engine.
// Draft from the assembly, not tuned.
int Drivetrain_IdleTorque(CarPhys *car)
{
    CarDrive *d = (CarDrive *)&car->driveLayout;
    int wheels = Fixed_Mul(car->axleSpeed[d->driveAxle], d->gearRatio[car->gear] * 652 >> 12);
    int torque;
    int drag;
    int input;
    int target;
    int load;
    int limited;
    int volume;
    int pitch;

    if (car->gear == 0) {
        wheels = -wheels;
    }
    torque = Fixed_MulFloor(car->throttleTarget, 0x498);
    drag = wheels * 784 / 341333;
    input = 0;
    if (car->gear == 1) {
        input = car->throttleTarget;
    }
    target = 0;
    if (car->speedFwd >= 0xd56) {
        if (car->speedFwd < 0x2c74) {
            target = 0x10aaa;
        } else if (car->speedFwd > 0x241e4) {
            target = (d->launchRpm << 12) / 60;
        } else {
            target = (long long)(car->speedFwd - 0x2c74) * (((d->launchRpm - 1000) << 12) / 60) / 0x21570 + 0x10aaa;
        }
    }
    target = input * target >> 12;
    if (target <= 0x10a66) {
        target = 0;
    }
    load = Fixed_MulFloor(target - car->engineSpeed, 0xf5);
    if (load > 0x1000) {
        load = 0x1000;
    } else if (load < 0) {
        load = 0;
    }
    limited = Engine_ApplyRevLimiter(car, load);
    car->engineLoad = load;
    car->engineSpeed += Fixed_Mul16(g_FrameDt, Fixed_Mul(limited, d->inertia));
    if (target == 0 && car->engineSpeed <= 0x8554) {
        car->engineSpeed /= 2;
    }
    if (car->engineSpeed > 0x10aaa && wheels > 0x8555) {
        torque += Fixed_MulFloor(input, car->engineFriction - limited) * car->engineSpeed / wheels;
    }
    volume = 0;
    pitch = 0;
    if (car->engineSpeed >= 0x1aab) {
        volume = (car->engineLoad * 3 + 0x1000) >> 7;
        pitch = (car->engineSpeed << 6) / 273066;
    }
    car->unk758 = volume;
    car->unk757 = pitch;
    car->blowOffVolume = 0;
    car->boostDisplay = 0;
    if (car->clutch != 0 && car->brake != 0) {
        car->blowOffVolume = 0x40;
        if (wheels < 0) {
            wheels = -wheels;
        }
        car->boostDisplay = Fixed_MulFloor(wheels, 0x3c);
    }
    return torque - drag;
}

// Each wheel's torque from its tyre force, less the brakes against its rotation.
// The game walks a pointer at the scratchpad entry from s0 with no rebase; the same issue as Tyre_ComputeSlipRatio.
void RaceCars_ComputeBrakeTorque(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        CarDrive *d = (CarDrive *)&car->driveLayout;
        int w;

        g_FrameDt = car->timeStep;
        for (w = 0; w < 4; w++) {
            WheelScratch *s = WHEEL_SCRATCH(i, w);
            int torque = -Fixed_Mul(d->tyreRadius[w >> 1], s->forceFwd + s->forceSide);
            int roll;

            s->torque = torque;
            roll = car->wheels[w].rollSpeed;
            if (roll != 0) {
                if (roll > 0) {
                    s->torque = torque - s->brakeForce;
                } else {
                    s->torque = torque + s->brakeForce;
                }
            }
        }
    }
}

// Active differentials (types 6, 7): moves torque across the axle at speed, more while
// steering.
// Registers differ.
void RaceCars_ApplyLsdBias(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        CarDrive *d = (CarDrive *)&car->driveLayout;
        int axle;

        g_FrameDt = car->timeStep;
        for (axle = 0; axle < 2; axle++) {
            int type = d->diffType[axle];

            if (type >= 6 && type < 8 && car->speedFwd >= 0x2c74) {
                int bias = SCRATCH[i + 1].bias * d->diffPreload[axle] >> 12;

                if (bias != 0) {
                    CarPhysWheel *left = &car->wheels[axle * 2];
                    CarPhysWheel *right = left + 1;
                    int average = (left->rollSpeed + right->rollSpeed) / 2;
                    int less;
                    int more;

                    if (car->steer > 0) {
                        average += average / 8;
                    } else if (car->steer < 0) {
                        average -= average / 8;
                    }
                    less = bias * 7 / 16;
                    more = bias * 9 / 16;
                    if (right->rollSpeed < average) {
                        WHEEL_SCRATCH(i, axle * 2 + 1)->torque += less;
                        WHEEL_SCRATCH(i, axle * 2)->torque -= more;
                    } else if (average < right->rollSpeed) {
                        WHEEL_SCRATCH(i, axle * 2 + 1)->torque -= more;
                        WHEEL_SCRATCH(i, axle * 2)->torque += less;
                    }
                }
            }
        }
    }
}

// The share a viscous centre coupling sends to the front, 0x7fffffff when fully locked.
// Branch order differs.
int Diff_GetCouplingTorque(CarPhys *car, int torque, int front, int rear)
{
    CarDrive *d = (CarDrive *)&car->driveLayout;
    int yaw = CARPHYS_FIELD(car, short, 0x64a);
    int factor = 0x1000;
    int slip;
    int result;

    if (yaw < 0) {
        yaw = -yaw;
    }
    if (yaw >= 0x401) {
        if (yaw < 0x1000) {
            factor = 0x1400 - yaw;
        } else {
            factor = 0x400;
        }
    }
    if (d->lockLimit != 0) {
        if (*(unsigned short *)&car->gear == 0x100 || car->clutchIn == 0) {
            return 0x7fffffff;
        }
        if (factor == 0x1000) {
            if (car->clutchIn == 2 || car->brake >= 0xc01) {
                return 0x7fffffff;
            }
            if (torque > 0 && torque + rear > 0) {
                return 0x7fffffff;
            }
            if (torque < 0 && torque + rear < 0) {
                return 0x7fffffff;
            }
        }
    }
    slip = car->axleSpeed[1] - car->axleSpeed[0];
    if (slip < 0) {
        slip = -slip;
    }
    result = Fixed_MulFloor(factor, Fixed_MulFloor(car->centerSplit, slip));
    if (result >= 0x1000) {
        result = 0x1000;
        if (d->lockLimit != 0) {
            result = 0x7fffffff;
        }
    }
    return result;
}

// The drive torque from the engine through the gearbox and centre differential to each
// axle's speed.
// Draft from the assembly, not tuned.
void RaceCars_UpdateDrivetrain(RaceVehicle *cars, int count)
{
    int sum[2];
    int inertia[2];
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        CarDrive *d = (CarDrive *)&car->driveLayout;
        PhysCarScratch *s = &SCRATCH[i];
        int out = 0;
        int lock = 0;
        int center = 0;
        int engine = 0;
        int gap = 0;
        int k;

        g_FrameDt = car->timeStep;
        if (car->clutchIn != 0) {
            switch (d->layout) {
            case 0:
            case 4:
            case 5:
                car->wheels[2].unk63 |= 8;
                car->wheels[3].unk63 |= 8;
                break;
            case 1:
                car->wheels[0].unk63 |= 8;
                car->wheels[1].unk63 |= 8;
                break;
            case 2:
                car->wheels[0].unk63 |= 8;
                car->wheels[1].unk63 |= 8;
                car->wheels[2].unk63 |= 4;
                car->wheels[3].unk63 |= 4;
                break;
            case 3:
            case 6:
                car->wheels[0].unk63 |= 4;
                car->wheels[1].unk63 |= 4;
                car->wheels[2].unk63 |= 8;
                car->wheels[3].unk63 |= 8;
                break;
            }
        }
        if (d->gearCount == 1) {
            out = Drivetrain_IdleTorque(car);
            if (car->clutchIn == 0) {
                out = 0;
            }
        } else if (car->clutchIn != 0) {
            int torque;

            if (d->gearCount == 2) {
                torque = Drivetrain_ClutchTorque(car);
            } else {
                torque = Engine_UpdateTurbo(car, car->throttleTarget);
            }
            if (car->clutchIn == 1) {
                out = torque;
            } else {
                if (car->clutchIn == 3) {
                    out = d->slipTorque;
                } else if (torque > 0) {
                    out = Fixed_MulFloor(Speed_FactorAbove30Kmh(s->engineSpeed - s->wheelSpeed), torque);
                } else {
                    out = d->dragTorque;
                }
                if (s->engineSpeed < s->wheelSpeed) {
                    out = -out;
                }
                car->engineSpeed += Fixed_Mul16(g_FrameDt, Fixed_Mul(torque - out, d->inertia));
            }
        }
        if (car->gear == 0) {
            out = -out;
        }
        out = Fixed_Mul(out, d->gearRatio[car->gear]);
        sum[0] = WHEEL_SCRATCH(i, 0)->torque + WHEEL_SCRATCH(i, 1)->torque;
        sum[1] = WHEEL_SCRATCH(i, 2)->torque + WHEEL_SCRATCH(i, 3)->torque;
        switch (d->layout) {
        case 2:
            lock = 1;
            break;
        case 3:
            lock = 2;
            break;
        case 4:
        case 5:
        case 6: {
            int split;

            if (d->layout == 6) {
                split = Fixed_Mul(d->centerSplit, out);
            } else if (d->layout == 4) {
                if (d->centerSplit == 0x1000 || out <= 0 || *(unsigned short *)&car->gear == 0x100) {
                    split = 0x7fffffff;
                } else {
                    split = Fixed_MulFloor(d->centerSplit, out);
                }
            } else {
                split = Diff_GetCouplingTorque(car, out, sum[0], sum[1]);
                if (split != 0x7fffffff) {
                    split = Fixed_MulFloor(split, out) / 2;
                }
            }
            if (car->unk63c != 0) {
                if (split == 0x7fffffff) {
                    lock = 3;
                } else {
                    int diff = sum[0] - sum[1] - out;

                    if (diff < 0) {
                        diff = -diff;
                    }
                    if (d->lockLimit < diff) {
                        car->unk63c = 0;
                    } else {
                        lock = 3;
                    }
                }
            }
            if (car->unk63c == 0) {
                center = split;
                if (split == 0x7fffffff) {
                    center = d->lockLimit;
                    if (car->axleSpeed[1] - car->axleSpeed[0] < 0) {
                        center = -center;
                    }
                }
                if (center != 0) {
                    lock = 1;
                }
            }
            if (lock != 0) {
                car->wheels[0].unk63 |= 4;
                car->wheels[1].unk63 |= 4;
            }
            break;
        }
        }
        if (d->gearCount != 1 && car->clutchIn == 1) {
            engine = d->engineInertia * d->gearRatio[car->gear] >> 12;
            if (d->gearCount == 2) {
                engine = s->torqueRatio * engine >> 12;
            }
        }
        if (lock == 3) {
            int accel = Fixed_Mul16(g_FrameDt, ((long long)(out + sum[0] + sum[1]) << 16) /
                                                   (d->axleInertia[0] + d->axleInertia[1] + engine));
            int old = car->axleSpeed[0];

            if (g_RaceSetup.raceType == 3 && d->layout == 5 && car->brake >= 0xc01) {
                car->axleSpeed[0] = old + accel;
            } else {
                car->axleSpeed[0] += (car->wheels[1].unk16 + accel) / 2;
            }
            if (WHEEL_SCRATCH(i, 0)->brakeForce + WHEEL_SCRATCH(i, 1)->brakeForce + WHEEL_SCRATCH(i, 2)->brakeForce +
                    WHEEL_SCRATCH(i, 3)->brakeForce !=
                0) {
                if (Fixed_MulFloor(car->axleSpeed[0], old) < 0 || old == 0) {
                    car->axleSpeed[0] = 0;
                    accel = 0;
                }
            }
            car->wheels[3].unk16 = accel;
            car->wheels[1].unk16 = accel;
            car->axleSpeed[1] = car->axleSpeed[0];
            s->axleTorque[0] = s->axleTorque[1] = out / 2;
        } else if (lock == 2) {
            int average = (car->axleSpeed[0] + car->axleSpeed[1]) / 2;
            int accel = Fixed_Mul16(g_FrameDt, ((long long)(out + sum[0] + sum[1]) << 16) /
                                                   (d->axleInertia[0] + d->axleInertia[1] + engine));
            int old = average;
            int spread;
            int half;
            int twist;

            average += (car->wheels[1].unk16 + accel) / 2;
            if (WHEEL_SCRATCH(i, 0)->brakeForce + WHEEL_SCRATCH(i, 1)->brakeForce + WHEEL_SCRATCH(i, 2)->brakeForce +
                    WHEEL_SCRATCH(i, 3)->brakeForce !=
                0) {
                if (Fixed_MulFloor(average, old) < 0 || old == 0) {
                    average = 0;
                    accel = 0;
                }
            }
            car->wheels[1].unk16 = accel;
            spread = car->axleSpeed[0] - car->axleSpeed[1];
            twist = Fixed_Mul16(Fixed_MulShiftTrunc(sum[0] - sum[1] - Fixed_Mul(car->centerSplit, spread),
                                                    d->centerGain, -4),
                                g_FrameDt);
            half = spread / 2;
            spread = half + (car->wheels[3].unk16 + twist) / 2;
            if (half * spread < 0) {
                spread = 0;
                twist = -half;
            }
            car->axleSpeed[0] = average + spread;
            car->axleSpeed[1] = average - spread;
            s->axleTorque[0] = s->axleTorque[1] = out / 2;
            car->wheels[3].unk16 = twist;
        } else {
            switch (d->layout) {
            case 0:
                inertia[0] = 0;
                inertia[1] = d->axleInertia[1] + engine;
                s->axleTorque[0] = 0;
                s->axleTorque[1] = out;
                break;
            case 1:
                inertia[0] = d->axleInertia[0] + engine;
                inertia[1] = 0;
                s->axleTorque[0] = out;
                s->axleTorque[1] = 0;
                break;
            case 2:
                inertia[0] = d->axleInertia[0] + engine;
                inertia[1] = d->axleInertia[1];
                {
                    int shared = Fixed_Mul(car->centerSplit, car->axleSpeed[0] - car->axleSpeed[1]);

                    s->axleTorque[0] = out - shared;
                    s->axleTorque[1] = shared;
                }
                break;
            case 4:
            case 5:
            case 6:
                inertia[0] = d->axleInertia[0];
                inertia[1] = d->axleInertia[1] + engine;
                s->axleTorque[0] = center;
                s->axleTorque[1] = out - center;
                break;
            }
            if (lock == 1) {
                gap = car->axleSpeed[0] - car->axleSpeed[1];
            }
            for (k = 0; k < 2; k++) {
                if (d->diffType[k] != 0) {
                    CarPhysWheel *wheel = &car->wheels[k * 2 + 1];
                    int accel = 0;
                    int old = car->axleSpeed[k];

                    if (inertia[k] != 0) {
                        accel = Fixed_Mul16(g_FrameDt, ((long long)(s->axleTorque[k] + sum[k]) << 16) / inertia[k]);
                    }
                    car->axleSpeed[k] += (wheel->unk16 + accel) / 2;
                    if (WHEEL_SCRATCH(i, k * 2)->brakeForce + WHEEL_SCRATCH(i, k * 2 + 1)->brakeForce != 0) {
                        if (Fixed_MulFloor(car->axleSpeed[k], old) < 0 || old == 0) {
                            car->axleSpeed[k] = 0;
                            accel = 0;
                        }
                    }
                    wheel->unk16 = accel;
                }
            }
            if (lock == 1) {
                int now = car->axleSpeed[0] - car->axleSpeed[1];

                if ((gap > 0 && now <= 0) || (gap < 0 && now >= 0)) {
                    car->axleSpeed[1 - d->driveAxle] = car->axleSpeed[d->driveAxle];
                    if ((unsigned int)(d->layout - 4) < 2) {
                        car->unk63c = 1;
                    }
                }
            }
        }
    }
}

// Each axle's differential: the limited slip torque, then the wheel speeds.
// Draft from the assembly, not tuned.
void RaceCars_UpdateDifferentials(RaceVehicle *cars, int count)
{
    int *torque[2];
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        CarDrive *d = (CarDrive *)&car->driveLayout;
        int axle;

        g_FrameDt = car->timeStep;
        for (axle = 0; axle < 2; axle++) {
            CarPhysWheel *left = &car->wheels[axle * 2];
            CarPhysWheel *right = left + 1;
            int slip = left->rollSpeed - right->rollSpeed;
            int locked = 0;
            int type;
            int k;

            torque[0] = &WHEEL_SCRATCH(i, axle * 2)->torque;
            torque[1] = &WHEEL_SCRATCH(i, axle * 2 + 1)->torque;
            switch (d->diffType[axle]) {
            case 1:
                locked = 1;
                break;
            case 3:
            case 4: {
                int input = SCRATCH[i].axleTorque[axle];
                int limit;

                if (input >= 0) {
                    limit = Fixed_MulFloor(d->diffAccel[axle], input);
                } else {
                    limit = Fixed_MulFloor(d->diffDecel[axle], -input);
                }
                if (limit < d->diffPreload[axle]) {
                    limit = d->diffPreload[axle];
                }
                if (car->diffLocked[axle] != 0) {
                    int gap = *torque[0] - *torque[1];

                    if (gap > limit * 2 || gap < -(limit * 2)) {
                        car->diffLocked[axle] = 0;
                    } else {
                        locked = 1;
                        break;
                    }
                }
                if (slip > 0) {
                    *torque[0] -= limit;
                    *torque[1] += limit;
                } else if (slip < 0) {
                    *torque[0] += limit;
                    *torque[1] -= limit;
                }
                break;
            }
            case 5: {
                int limit = Fixed_MulFloor(slip, slip);

                if (d->diffPreload[axle] < limit) {
                    limit = d->diffPreload[axle];
                }
                limit = Fixed_MulFloor(d->diffAccel[axle], limit);
                if (slip > 0) {
                    *torque[0] -= limit;
                    *torque[1] += limit;
                } else if (slip < 0) {
                    *torque[0] += limit;
                    *torque[1] -= limit;
                }
                break;
            }
            }
            type = d->diffType[axle];
            if (locked != 0) {
                type = 1;
            }
            if (type == 0) {
                for (k = 0; k < 2; k++) {
                    CarPhysWheel *wheel = &car->wheels[axle * 2 + k];
                    int accel = Fixed_Mul16(g_FrameDt, Fixed_MulShiftTrunc(*torque[k], d->diffInertia[axle], -4));
                    int roll = wheel->rollSpeed;
                    int next = roll + (wheel->unk16 + accel) / 2;

                    if (torque[k][-3] != 0) {
                        if (roll > 0) {
                            if (next < 0) {
                                next = 0;
                                accel = 0;
                            }
                        } else if (roll < 0) {
                            if (next > 0) {
                                next = 0;
                                accel = 0;
                            }
                        } else {
                            next = 0;
                            accel = 0;
                        }
                    }
                    wheel->unk16 = accel;
                    wheel->rollSpeed = next;
                }
            } else if (type == 1) {
                int speed = Fixed_Mul(d->tyreRadius[axle], car->axleSpeed[axle]);

                right->rollSpeed = speed;
                left->rollSpeed = speed;
                left->unk16 = 0;
            } else {
                int accel = Fixed_Mul16(Fixed_MulShiftTrunc(*torque[0] - *torque[1], d->diffInertia[axle], -3), g_FrameDt);
                int half = slip / 2 + (left->unk16 + accel) / 2;
                int speed;

                if (slip * half < 0) {
                    half = 0;
                    if ((unsigned int)(type - 3) < 2) {
                        car->diffLocked[axle] = 1;
                    }
                    accel = 0;
                }
                left->unk16 = accel;
                speed = Fixed_Mul(d->tyreRadius[axle], car->axleSpeed[axle]);
                left->rollSpeed = speed + half;
                right->rollSpeed = speed - half;
            }
        }
    }
}

// RaceCars_ComputeBrakeTorque, RaceCars_ApplyLsdBias, RaceCars_UpdateDrivetrain,
// RaceCars_UpdateDifferentials.
void RaceCars_StepDrivetrain(RaceVehicle *cars, int count)
{
    RaceCars_ComputeBrakeTorque(cars, count);
    RaceCars_ApplyLsdBias(cars, count);
    RaceCars_UpdateDrivetrain(cars, count);
    RaceCars_UpdateDifferentials(cars, count);
}
}
