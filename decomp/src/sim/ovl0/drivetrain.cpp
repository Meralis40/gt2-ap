// OVL0 800392d8-8003b69c (drivetrain): Engine, gearbox and tyre slip.
// The functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
struct Curve16;

// Per-wheel values in the scratchpad, 4 per 0x90 bytes of car, from 0x1f800004.
struct WheelScratch {
    int brakeForce;
    int forceFwd;
    int forceSide;
    unsigned char unkc[4];
    signed char state; /* 0 not set, 1 grip, -1 slipping */
    unsigned char unk11;
    signed char dir;
    unsigned char unk13;
    short slip; /* 4.12 slip ratio */
    short slipOut; /* Tyres_LookupSlipCurve */
    short sin; /* steering */
    short cos;
};
struct CarWheelScratch {
    WheelScratch wheels[4];
    unsigned char unk70[0x20];
};
#define WHEEL_SCRATCH(car, wheel) ((WheelScratch *)(0x1f800004 + (car) * 0x90 + (wheel) * sizeof(WheelScratch)))

// One player's shift input in the scratchpad, per car.
struct ShiftInput {
    signed char button;
    unsigned char unk1[3];
};
#define SHIFT_INPUT ((ShiftInput *)0x1f800364)

// Per-car values in the scratchpad (0x1f800000 + carIndex * 0x90).
struct PhysCarScratch {
    unsigned char unk0[0x7c];
    int engineSpeed; /* before the clutch started to slip */
    int wheelSpeed;
    int invMass;
    int rollDrag;
    int torqueRatio; /* RaceCars_StepDrivetrain */
};

#define g_FrameDt (*(int *)0x1f800000)
#define CARPHYS_FIELD(car, type, offset) (*(type *)((char *)(car) + (offset)))

extern int g_TyreWearEnabled;
extern int g_TyreWear_MaxGripLoss;
extern int g_TyreWearInit;
extern int g_TyreGripLossInit;
extern int g_TyreWear_Knee;
extern int g_TyreWear_KneeGripLoss;
extern int g_TyreWear_WornGrip;
extern int g_SurfaceRollResist[];
extern EngineCurve g_DragCurve;
extern int g_FrameRate;
extern short g_SinTable[];
extern short g_CosTable[];

int Curve16_Eval(Curve16 *curve, short x);
int Curve16_Eval2(Curve16 *curve, Curve16 *curve2, short x, short *outY, short *outY2);
int Gearbox_SelectGear(CarPhys *car, ShiftInput *input);
int TyreCurve_EvalAngle(TyreAxle *tyre, int angle);
int Engine_UpdateTurbo(CarPhys *car, int throttle);
int Race_AllowsOffCourseFlag(void);
int Body_GetCollisionRule(CarPhys *body);

// 1.0 + (|speed| - 30 km/h) x 30 (4.12, speed in m/s 4.12), at least 1.0, at most 1.125.
int Speed_FactorAbove30Kmh(int speed)
{
    int iVar1;
    longlong lVar2;

    if (speed < 0) {
        speed = -speed;
    }
    iVar1 = speed + -0x8555;
    if (iVar1 < 0) {
        iVar1 = 0;
    }
    lVar2 = Fixed_MulFloor(iVar1, 0x1e);
    iVar1 = (int)lVar2 + 0x1000;
    if (0x11ff < iVar1) {
        iVar1 = 0x1200;
    }
    return iVar1;
}

// The engine speed the driven wheels ask for in the current gear, 0 when they roll
// backwards.
int Drivetrain_EngineSpeedFromWheels(CarPhys *car)
{
    int speed = 0;
    int result;

    switch (car->driveLayout) {
    case 0:
    case 4:
    case 5:
        speed = car->axleSpeed[1];
        break;
    case 1:
    case 2:
        speed = car->axleSpeed[0];
        break;
    case 3:
        speed = (car->axleSpeed[0] + car->axleSpeed[1]) / 2;
        break;
    case 6:
        speed = car->axleSpeed[1] + Fixed_Mul(car->centerSplit, car->axleSpeed[0] - car->axleSpeed[1]);
        break;
    }
    if (car->gear == 0) {
        speed = -speed;
    }
    result = 0;
    if (speed > 0) {
        result = Fixed_MulFloor(car->gearRatio[car->gear], speed);
        result = Fixed_MulFloor(0x28c, result);
    }
    return result;
}

// Rpm display = engine speed x 60, at least idle - 1.
void Engine_UpdateRpmDisplay(CarPhys *car)
{
    int idle = car->engine.unk8e - 1;
    int rpm = Fixed_MulFloor(car->engineSpeed, 0x3c);

    if (rpm < idle) {
        rpm = idle;
    }
    car->rpm = rpm;
}

// When +0x372 is 1: clutchIn +0x619 = clutch +0x60a != 0.
void Clutch_UpdateShiftState(CarPhys *car)
{
    if (car->clutch == 0) {
        car->clutchIn = 0;
        return;
    }
    car->clutchIn = 1;
}

// The slip ratio of each wheel of one car, between -0x1000 and 0x1000, faded out near
// a standstill.
#ifdef NON_MATCHING
// Same code, but the pointer register is based on s->slip (+0x14) here and on s in
// the game, so every field offset differs.
void Tyre_ComputeSlipRatio(CarPhys *car, int index)
{
    unsigned int i;
    WheelScratch *s = ((CarWheelScratch *)0x1f800004)[index].wheels;

    for (i = 0; i < 4; i++, s++) {
        CarPhysWheel *wheel = &car->wheels[i];
        int speed = Fixed_Mul(s->cos, wheel->velFwd) - Fixed_Mul(s->sin, wheel->velLat);
        int roll = wheel->rollSpeed;

        s->state = 0;
        if ((unsigned int)(roll + 0x46) < 0x8d) {
            if (roll < speed) {
                s->slip = 0x1000;
                s->dir = -1;
            } else if (speed < roll) {
                s->slip = 0x1000;
                s->dir = 1;
            } else {
                s->slip = 0;
                s->dir = 0;
            }
            s->state = -1;
        } else if (roll > 0) {
            if (roll >= speed) {
                s->slip = ((long long)Fixed_Mul(wheel->slipGrip, speed - roll) << 12) / roll;
                s->dir = 1;
                s->state = 1;
            } else {
                if ((unsigned int)(speed + 0x46) < 0x8d) {
                    s->slip = 0;
                } else {
                    s->slip = ((long long)Fixed_Mul(wheel->slipGrip, speed - roll) << 12) / speed;
                }
                s->dir = -1;
                s->state = -1;
            }
        } else {
            if (speed >= roll) {
                s->slip = ((long long)Fixed_Mul(wheel->slipGrip, speed - roll) << 12) / roll;
                s->dir = -1;
                s->state = 1;
            } else {
                if ((unsigned int)(speed + 0x46) < 0x8d) {
                    s->slip = 0;
                } else {
                    s->slip = ((long long)Fixed_Mul(wheel->slipGrip, speed - roll) << 12) / speed;
                }
                s->dir = 1;
                s->state = -1;
            }
        }
        if (s->slip > 0x1000) {
            s->slip = 0x1000;
        } else if (s->slip < -0x1000) {
            s->slip = -0x1000;
        }
        if (s->state == 0) {
            s->slip = 0;
        } else {
            int ref = speed;

            if (s->state > 0) {
                ref = roll;
            }
            if (ref < 0) {
                ref = -ref;
            }
            if (ref < 0x58e8) {
                s->slip = Fixed_Mul32(ref * 737 >> 12, s->slip);
            }
        }
        wheel->unk44 = s->slip;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Tyre_ComputeSlipRatio);
#endif

// Tyre_ComputeSlipRatio for every car.
void Tyres_ComputeSlipRatios(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        Tyre_ComputeSlipRatio(car, i);
    }
}

// Slip ratio to force, through each axle's two slip curves.
#ifdef NON_MATCHING
// The game keeps its counters in t registers saved around the call (frame 0x50); here
// they are in s registers (frame 0x40).
void Tyres_LookupSlipCurve(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        unsigned int w;

        for (w = 0; w < 4; w++) {
            WheelScratch *s = WHEEL_SCRATCH(i, w);
            TyreAxle *tyre = &car->tyres[w >> 1];
            int slip;

            Curve16_Eval2((Curve16 *)tyre->slipCurve, (Curve16 *)tyre->slipCurve2, s->slip, &s->slipOut,
                          &car->wheels[w].unk46);
            slip = s->slip;
            if (slip < 0) {
                slip = -slip;
            }
            car->wheels[w].slipAbs = slip;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Tyres_LookupSlipCurve);
#endif

// Applies the gear Gearbox_SelectGear picks. A running clutch restarts: mode 3 from
// one forward gear to another, else mode 2.
void Gearbox_ApplyShiftRequest(CarPhys *car, int index)
{
    int gear = Gearbox_SelectGear(car, SHIFT_INPUT + index);

    if (gear != -1) {
        if (car->clutchIn != 0) {
            car->clutchIn = (gear == 0 || car->gear == 0) ? 2 : 3;
        }
        car->gear = gear;
    }
}

// Rpm display +0x6ac = engine speed +0x624 x 60.
void Engine_SetRpmDisplay(CarPhys *car)
{
    longlong lVar1;

    lVar1 = Fixed_MulFloor(car->engineSpeed, 0x3c);
    car->rpm = (short)lVar1;
}

// Automatic gearbox: picks reverse (0) or first (1) from the shift input, the speed
// and the throttle.
#ifdef NON_MATCHING
// The game reads the input at lo(0x1f800364) off the indexed register; here the full
// address is built first, which also shifts the registers.
void Gearbox_AutoSelectDirection(CarPhys *car, int index)
{
    int want = *(signed char *)((int *)0x1f800364 + index);
    int gear = car->gear;
    int next = -1;

    if (want == 0) {
        int speed = car->speedFwd;

        if (speed >= 0x473 || car->throttleTarget != 0) {
            if (gear == 0) {
                next = 1;
            }
            goto done;
        }
        if (speed >= -0x472) {
            goto done;
        }
    }
    if (gear != 0) {
        next = 0;
    }
done:
    if (next != -1) {
        int mode = car->gearCount;

        car->gear = next;
        if (mode == 2) {
            car->clutchIn = mode;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Gearbox_AutoSelectDirection);
#endif

// Tyre grip from wear, then each wheel's grip limit from its load, camber and surface.
#ifdef NON_MATCHING
// Draft: wear lives on the stack in the game, and more values stay in memory there
// (frame 0x58, here 0x50).
void Tyres_ComputeGripLimit(RaceVehicle *cars, int count)
{
    int camber[4];
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        int wear = 0;
        int w;
        unsigned int k;
        int roll;

        if (g_TyreWearEnabled != 0) {
            wear = !(car->flags & 0x10);
        }
        if (wear != 0) {
            for (w = 0; w < 4; w++) {
                CarPhysWheel *wheel = &car->wheels[w];
                int tyreWear = wheel->tyreWear;
                int limit = g_TyreWearEnabled;
                int level;
                int t;

                if (tyreWear >= limit) {
                    wheel->tyreWear = limit;
                    wheel->wearLevel = 0x7f;
                    wheel->tyreGrip = 0x1000 - (unsigned short)g_TyreWear_MaxGripLoss;
                    continue;
                }
                if (tyreWear >= g_TyreWear_Knee) {
                    t = ((long long)(tyreWear - g_TyreWear_Knee) << 12) / (limit - g_TyreWear_Knee);
                    wheel->tyreGrip = 0x1000 - Fixed_MulFloor(t, g_TyreWear_MaxGripLoss - g_TyreWear_KneeGripLoss) -
                                      (unsigned short)g_TyreWear_KneeGripLoss;
                    if (t >= 0x1000) {
                        wheel->wearLevel = 0x7f;
                        continue;
                    }
                    level = (t >> 6) + 0x40;
                } else if (tyreWear >= 0) {
                    t = ((long long)tyreWear << 12) / g_TyreWear_Knee;
                    wheel->tyreGrip = 0x1000 - Fixed_MulFloor(g_TyreWear_KneeGripLoss, t);
                    level = t >> 6;
                    if (t >= 0x1000) {
                        level = 0x3f;
                    }
                } else if (tyreWear >= g_TyreWearInit) {
                    t = ((long long)-tyreWear << 12) / -g_TyreWearInit;
                    wheel->tyreGrip = 0x1000 - Fixed_MulFloor(g_TyreGripLossInit, t);
                    if (t < 0x1000) {
                        level = -(t >> 6);
                    } else {
                        level = -0x3f;
                    }
                } else {
                    wheel->tyreGrip = 0x1000;
                    level = -0x80;
                }
                wheel->wearLevel = level;
            }
        }
        roll = car->roll - *(short *)(car->unk6f4 + 2);
        camber[2] = -roll;
        camber[0] = -roll;
        camber[3] = roll;
        camber[1] = roll;
        for (k = 0; k < 4; k++) {
            CarPhysWheel *wheel = &car->wheels[k];
            int load = wheel->suspLength;
            int axle = k >> 1;
            TyreAxle *tyre;
            int grip;
            int angle;

            if (wear != 0) {
                load = wheel->tyreGrip * load >> 12;
            }
            tyre = &car->tyres[axle];
            grip = EngineCurve_TorqueAt((EngineCurve *)tyre->loadCurve, load);
            angle = camber[k] + car->camber[axle];
            if (angle < 0) {
                angle = -angle;
            }
            grip = (short)Curve16_Eval((Curve16 *)tyre->camberCurve, angle) * grip >> 12;
            wheel->gripLimit = car->surfaceGrip[wheel->surface] * grip >> 8;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", Tyres_ComputeGripLimit);
#endif

// The slip angle of each wheel, and how much of it applies at this speed.
void Tyres_ComputeSlipAngle(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        unsigned int w;

        for (w = 0; w < 4; w++) {
            int fwd;
            int lat;
            CarPhysWheel *wheel = &car->wheels[w];
            short *fade;
            int factor;
            int angle;
            int speed;

            fwd = wheel->velFwd;
            lat = wheel->velLat;
            fade = &wheel->slipFade;

            if ((unsigned int)(fwd + 0x2c74) >= 0x58e9 || lat >= 0x2c75 || lat < -0x2c74 ||
                (speed = Math_ISqrt((lat * lat >> 12) + (fwd * fwd >> 12), 6)) >= 0x2c74) {
                factor = 0x1000;
            } else if (speed < 0x473) {
                factor = 0;
            } else {
                factor = (speed - 0x472) * 1638 >> 12;
            }
            *fade = factor;
            if (factor == 0) {
                angle = 0;
            } else {
                angle = Math_Atan2(-lat, fwd);
            }
            wheel->unk50 = angle;
        }
    }
}

// The tyre angle curve, mirrored around +-0x400 and odd around 0.
#ifdef NON_MATCHING
// Only x's register differs (v0 in the game, v1 here).
int TyreCurve_EvalAngle(TyreAxle *tyre, int angle)
{
    short x;
    short sign;
    short y;

    if (angle < -0x400) {
        x = angle + 0x800;
        sign = -1;
    } else if (angle < 0) {
        x = -angle;
        sign = -1;
    } else {
        x = angle;
        if (angle >= 0x400) {
            x = 0x800 - angle;
        }
        sign = 1;
    }
    y = Curve16_Eval((Curve16 *)tyre->angleCurve, x);
    return sign * y;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", TyreCurve_EvalAngle);
#endif

// One physics step of the drivetrain of every car: clutch, brakes, tyre forces, speeds,
// engine and gearbox, yaw, world velocities, tyre wear.
#ifdef NON_MATCHING
// Draft from the assembly, not tuned: the game splits the same steps but the registers
// and most loop shapes differ. The engine step is expanded twice, as in the game.
// A car's engine speed for this frame, for gearbox modes 0 and 2.
static inline void Engine_Step(CarPhys *car, PhysCarScratch *scratch)
{
    if (car->clutchIn == 0) {
        int idle = (car->engine.unk8e << 12) / 60;
        int torque = Engine_UpdateTurbo(car, car->throttleTarget);

        torque = Fixed_Mul(torque, CARPHYS_FIELD(car, int, 0x40c));
        car->engineSpeed += Fixed_Mul16(torque, g_FrameDt);
        if (car->engineSpeed < idle) {
            car->engineSpeed = idle;
        }
    } else {
        int wheels = Drivetrain_EngineSpeedFromWheels(car);

        if (car->clutchIn == 1) {
            if (car->gearCount == 2) {
                car->engineSpeed = scratch->torqueRatio * wheels >> 12;
            } else {
                car->engineSpeed = wheels;
            }
        } else if ((unsigned char)(car->clutchIn - 2) < 2) {
            int engage = 0;

            if ((scratch->wheelSpeed >= scratch->engineSpeed && car->engineSpeed >= wheels) ||
                (scratch->engineSpeed >= scratch->wheelSpeed && wheels >= car->engineSpeed)) {
                engage = 1;
            }
            if (engage) {
                car->clutchIn = 1;
                car->engineSpeed = wheels;
                car->startDelay = g_FrameRate / 5;
            }
            if (car->clutchIn == 3 && (car->engine.unk8e << 12) / 60 >= scratch->wheelSpeed) {
                car->clutchIn = 0;
            }
            if (car->clutchIn == 2 && car->brake == 0x1000 &&
                car->engineSpeed < ((car->engine.unk8e + 500) << 12) / 60) {
                car->clutchIn = 0;
            }
        }
        if (car->engineSpeed < 0) {
            car->engineSpeed = 0;
        } else {
            int limit = ((car->engine.redline + 10) << 12) / 60;

            if (car->engineSpeed >= limit) {
                car->engineSpeed = limit;
            }
        }
    }
}

void CarPhys_UpdateDrivetrain(RaceVehicle *cars, int count)
{
    int i;
    unsigned int w;

    // The clutch and the brake torque of each wheel.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        PhysCarScratch *scratch = &((PhysCarScratch *)0x1f800000)[i];
        int hand;

        g_FrameDt = car->timeStep;
        scratch->wheelSpeed = 0;
        scratch->engineSpeed = 0;
        if (car->gearCount == 1) {
            ((void (*)(CarPhys *, int))Clutch_UpdateShiftState)(car, i);
        } else {
            if (car->clutchIn == 0) {
                if (car->clutch != 0) {
                    int speed = car->speedFwd;

                    if ((speed >= 0x2c75 && car->gear != 0) || (speed < -0x2c74 && car->gear == 0)) {
                        if (car->throttleTarget >= 0x401) {
                            car->clutchIn = 3;
                        }
                    } else if (car->launchLimit < car->rpm) {
                        car->clutchIn = 2;
                    } else {
                        car->clutch = 0;
                    }
                }
            } else if (car->clutchIn == 1) {
                if (car->clutch == 0) {
                    car->clutchIn = 0;
                } else if (car->gearCount == 2) {
                    scratch->wheelSpeed = Drivetrain_EngineSpeedFromWheels(car);
                }
            }
            if ((unsigned int)(car->clutchIn - 2) < 2) {
                int speed;

                scratch->engineSpeed = car->engineSpeed;
                speed = Drivetrain_EngineSpeedFromWheels(car);
                scratch->wheelSpeed = speed;
                if (car->clutchIn == 3) {
                    if (speed < scratch->engineSpeed) {
                        car->throttleTarget = 0;
                    } else {
                        car->throttleTarget = 0x1000;
                    }
                }
            }
        }
        for (w = 0; w < 4; w++) {
            WHEEL_SCRATCH(i, w)->brakeForce = car->wheels[w].brake * CARPHYS_FIELD(car, int, 0x414 + (w >> 1) * 4) >> 12;
        }
        hand = (CARPHYS_FIELD(car, short, 0x614) * CARPHYS_FIELD(car, int, 0x41c)) << 4 >> 16;
        for (w = 2; w < 4; w++) {
            WHEEL_SCRATCH(i, w)->brakeForce += hand;
        }
        for (w = 0; w < 4; w++) {
            int state = (WHEEL_SCRATCH(i, w)->brakeForce != 0) * 2;

            if (car->wheels[w].suspLength == 0) {
                state |= 1;
            }
            car->wheels[w].unk63 = state;
        }
    }

    // The rolling resistance.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        PhysCarScratch *scratch = &((PhysCarScratch *)0x1f800000)[i];
        int sum = 0;
        int k;

        CARPHYS_FIELD(car, int, 0x72c) = 0;
        CARPHYS_FIELD(car, int, 0x734) = 0;
        CARPHYS_FIELD(car, int, 0x730) = 0;
        CARPHYS_FIELD(car, int, 0x724) = 0;
        for (k = 0; k < 4; k++) {
            sum += Fixed_MulFloor(car->wheels[k].suspLength, g_SurfaceRollResist[car->wheels[k].surface]);
        }
        if (car->speedFwd > 0) {
            scratch->rollDrag = -Fixed_MulFloor(EngineCurve_TorqueAt(&g_DragCurve, car->speedFwd), sum);
        } else if (car->speedFwd < 0) {
            scratch->rollDrag = Fixed_MulFloor(EngineCurve_TorqueAt(&g_DragCurve, -car->speedFwd), sum);
        } else {
            scratch->rollDrag = 0;
        }
    }

    // The tyre forces along and across the slip.
    Tyres_LookupSlipCurve(cars, count);
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        for (w = 0; w < 4; w++) {
            WheelScratch *s = WHEEL_SCRATCH(i, w);
            CarPhysWheel *wheel = &car->wheels[w];
            int force = Fixed_MulFloor(s->slipOut, wheel->gripLimit);
            int grip;

            if (s->dir < 0) {
                force = -force;
            }
            grip = Fixed_Mul(wheel->slipGrip, force);
            if (s->state >= 0) {
                s->forceFwd = grip;
                s->forceSide = 0;
                wheel->unk58 = force;
                wheel->unk5c = 0;
            } else {
                s->forceFwd = 0;
                s->forceSide = grip;
                wheel->unk58 = 0;
                wheel->unk5c = force;
            }
        }
    }

    // The cornering force of each wheel from its slip angle.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        for (w = 0; w < 4; w++) {
            CarPhysWheel *wheel = &car->wheels[w];
            int roll;
            int diff;

            if (wheel->slipFade == 0) {
                wheel->lateral = 0;
                wheel->lateralAbs = 0;
            } else {
                int forward = wheel->rollSpeed > 0;
                int angle = (short)wheel->heading;
                int y;
                int cap;

                if (!forward) {
                    angle += 0x800;
                }
                y = TyreCurve_EvalAngle(&car->tyres[w >> 1], Angle_Diff(angle, wheel->unk50));
                y = Fixed_Mul(wheel->slipFade, y);
                cap = wheel->unk46;
                if (cap < y) {
                    y = cap;
                } else if (y < -cap) {
                    y = -cap;
                }
                wheel->lateralAbs = y < 0 ? -y : y;
                y = Fixed_Mul32(wheel->gripLimit, y);
                wheel->lateral = forward ? y : -y;
            }
            roll = wheel->rollSpeed;
            diff = car->speedFwd - roll;
            if ((unsigned int)(roll + 0x1639) < 0x2c73 && diff >= -0x471 && diff < 0x472) {
                int side = Fixed_Mul(g_CosTable[wheel->heading & 0xfff], wheel->velLat) +
                           Fixed_Mul(g_SinTable[wheel->heading & 0xfff], wheel->velFwd);
                int push = 0;

                if (side != 0) {
                    if (side >= 0x8e4) {
                        side = 0x2000;
                    } else if (side < -0x8e3) {
                        side = -0x2000;
                    } else {
                        side = Fixed_Mul32(0x3996, side);
                    }
                    if (roll < 0) {
                        roll = -roll;
                    }
                    push = Fixed_Mul32((0x163a - roll) * 737 * 4 >> 12, side);
                }
                wheel->lateral += (short)push;
            }
        }
    }

    // The forward acceleration.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        PhysCarScratch *scratch = &((PhysCarScratch *)0x1f800000)[i];
        int sum = 0;
        int weight = 0;
        int accel;

        g_FrameDt = car->timeStep;
        for (w = 0; w < 4; w++) {
            sum += Fixed_Mul32(WHEEL_SCRATCH(i, w)->cos, car->wheels[w].unk58);
        }
        CARPHYS_FIELD(car, int, 0x734) += sum;
        CARPHYS_FIELD(car, int, 0x734) += scratch->rollDrag;
        for (w = 0; w < 4; w++) {
            if ((car->wheels[w].unk63 & 0xd) == 0) {
                weight += CARPHYS_FIELD(car, unsigned short, 0x410 + (w >> 1) * 2);
            }
        }
        if ((short)weight != 0) {
            scratch->invMass = 0x1000000 / ((short)weight + car->mass);
        } else {
            scratch->invMass = car->invMass;
        }
        accel = Fixed_Mul(CARPHYS_FIELD(car, int, 0x734) + CARPHYS_FIELD(car, int, 0x72c) +
                              CARPHYS_FIELD(car, short, 0x71a),
                          scratch->invMass);
        CARPHYS_FIELD(car, int, 0x724) += accel;
        car->speedFwd += Fixed_Mul16(CARPHYS_FIELD(car, int, 0x724), g_FrameDt);
        if (car->speedFwd != 0) {
            int speed;
            int next;

            sum = 0;
            for (w = 0; w < 4; w++) {
                sum += Fixed_Mul32(WHEEL_SCRATCH(i, w)->cos, car->wheels[w].unk5c);
                // Every wheel reads the front left wheel's force here.
                sum -= Fixed_Mul32(WHEEL_SCRATCH(i, w)->sin, car->wheels[0].lateral);
            }
            accel = Fixed_Mul32(scratch->invMass, sum);
            next = Fixed_Mul16(accel, g_FrameDt);
            speed = car->speedFwd;
            next += speed;
            if ((speed ^ next) >= 0) {
                CARPHYS_FIELD(car, int, 0x734) += sum;
                CARPHYS_FIELD(car, int, 0x724) += accel;
            } else {
                CARPHYS_FIELD(car, int, 0x724) += Fixed_Mul(car->timeStep, -speed);
                next = 0;
            }
            car->speedFwd = next;
        }
    }

    // The engines and gearboxes.
    RaceCars_StepDrivetrain(cars, count);
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        g_FrameDt = car->timeStep;
        if (car->gearCount == 1) {
            ((void (*)(CarPhys *, int))Engine_SetRpmDisplay)(car, i);
            Gearbox_AutoSelectDirection(car, i);
        } else if (car->gearCount == 2) {
            Engine_Step(car, &((PhysCarScratch *)0x1f800000)[i]);
            Engine_UpdateRpmDisplay(car);
            Gearbox_AutoSelectDirection(car, i);
        } else {
            Engine_Step(car, &((PhysCarScratch *)0x1f800000)[i]);
            Engine_UpdateRpmDisplay(car);
            Gearbox_ApplyShiftRequest(car, i);
        }
    }

    // The side force and the yaw.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        g_FrameDt = car->timeStep;
        CARPHYS_FIELD(car, int, 0x728) = 0;
        if (car->airborne != 0) {
            CARPHYS_FIELD(car, short, 0x64a) = 0;
        } else {
            short armX[4];
            short armZ[4];
            int yaw = 0;
            int side = 0;
            int rate;

            armX[0] = -car->halfTrackF;
            armZ[0] = car->distFrontAxle;
            armX[1] = car->halfTrackF;
            armZ[1] = car->distFrontAxle;
            armX[2] = -car->halfTrackR;
            armZ[2] = -car->distRearAxle;
            armX[3] = car->halfTrackR;
            armZ[3] = -car->distRearAxle;
            for (w = 0; w < 4; w++) {
                WheelScratch *s = WHEEL_SCRATCH(i, w);
                CarPhysWheel *wheel = &car->wheels[w];
                int lateral = wheel->lateral;
                int along = wheel->unk58 + wheel->unk5c;
                int fx = -Fixed_Mul(s->cos, lateral) - Fixed_Mul(s->sin, along);
                int fz;

                side += fx;
                fz = Fixed_Mul(s->cos, along) - Fixed_Mul(s->sin, lateral);
                yaw += Fixed_Mul(armX[w], fz) - Fixed_Mul(armZ[w], fx);
            }
            rate = Fixed_Mul16(g_FrameDt, Fixed_Mul(CARPHYS_FIELD(car, int, 0x68), yaw));
            car->yawRate += rate;
            if (car->yawRate > 0x80000) {
                car->yawRate = 0x80000;
            } else if (car->yawRate < -0x80000) {
                car->yawRate = -0x80000;
            }
            CARPHYS_FIELD(car, int, 0x728) = Fixed_Mul(side - CARPHYS_FIELD(car, int, 0x730), car->invMass);
            CARPHYS_FIELD(car, short, 0x64a) =
                Fixed_Mul(CARPHYS_FIELD(car, int, 0x728), CARPHYS_FIELD(car, short, 0x34));
        }
    }

    // The world velocities.
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        g_FrameDt = car->timeStep;
        if (car->unk718 != 0 || car->pitTimer != 0 || car->aiMode == 7) {
            CARPHYS_FIELD(car, int, 0x734) = 0;
            car->yawRate = 0;
            car->vel[0] = 0;
            car->vel[1] = 0;
            if (car->pitTimer == 0) {
                CARPHYS_FIELD(car, short, 0x650) = 0;
                CARPHYS_FIELD(car, short, 0x652) = 0;
                car->vel[2] = 0;
            }
        } else {
            if (car->airborne == 0) {
                if ((unsigned int)(car->speedFwd + 0x471) < 0x8e3 && car->wheels[0].slipFade == 0 &&
                    car->wheels[1].slipFade == 0 && car->wheels[2].slipFade == 0 &&
                    car->wheels[3].slipFade == 0 &&
                    ((car->wheels[0].unk63 | car->wheels[1].unk63 | car->wheels[2].unk63 |
                      car->wheels[3].unk63) &
                     2) != 0) {
                    car->yawRate = 0;
                    car->vel[0] = 0;
                    car->vel[1] = 0;
                    CARPHYS_FIELD(car, int, 0x734) = 0;
                    CARPHYS_FIELD(car, int, 0x724) = 0;
                    CARPHYS_FIELD(car, int, 0x728) = 0;
                    CARPHYS_FIELD(car, short, 0x64a) = 0;
                    car->speedFwd = 0;
                    car->speedLat = 0;
                } else {
                    int fwd = Fixed_Mul16(CARPHYS_FIELD(car, int, 0x724), g_FrameDt);
                    int side = Fixed_Mul16(CARPHYS_FIELD(car, int, 0x728), g_FrameDt);
                    int k;

                    for (k = 0; k < 2; k++) {
                        car->vel[k] += Fixed_Mul32(fwd, car->fwdDir[k]) + Fixed_Mul32(side, car->rightDir[k]);
                    }
                }
            }
            if (car->pushVec[0] != 0 || car->pushVec[1] != 0) {
                int k;

                for (k = 0; k < 2; k++) {
                    car->vel[k] += Fixed_Mul16(Fixed_Mul(car->pushVec[k], car->invMass), g_FrameDt);
                }
            }
        }
    }

    // The tyre wear.
    if (g_TyreWearEnabled != 0) {
        for (i = 0; i < count; i++) {
            CarPhys *car = &cars[i].phys;

            if ((car->flags & 0x10) == 0) {
                int k;

                g_FrameDt = car->timeStep;
                for (k = 0; k < 4; k++) {
                    CarPhysWheel *wheel = &car->wheels[k];
                    int axle = k >> 1;

                    wheel->wearRate = 0;
                    if (wheel->surface == 0 && wheel->suspLength != 0) {
                        int load = (wheel->suspLength + CARPHYS_FIELD(car, int, 0x434 + axle * 4)) / 3;
                        int work = (wheel->slipAbs * CARPHYS_FIELD(car, short, 0x44c + axle * 2) >> 12) +
                                   (wheel->lateralAbs * CARPHYS_FIELD(car, short, 0x448 + axle * 2) >> 12);
                        int wear = (work * load >> 12) * g_FrameDt >> 16;

                        wheel->tyreWear += wear;
                        if (wear >= 0x100) {
                            wear = 0xff;
                        }
                        wheel->wearRate = wear;
                    }
                }
            }
        }
    }

    // Sets flags bit 2 (and 3 for a CPU car) for a car with worn out or dirty tyres.
    if (Race_AllowsOffCourseFlag() != 0) {
        for (i = 0; i < count; i++) {
            CarPhys *car = &cars[i].phys;

            if ((CARPHYS_FIELD(car, int, 0x78c) & 0xc00) == 0) {
                int off = 0;
                int k;

                if (g_TyreWearEnabled != 0) {
                    for (k = 0; k < 4; k++) {
                        if (car->wheels[k].tyreWear >= 0 && car->wheels[k].tyreGrip < g_TyreWear_WornGrip) {
                            off = 1;
                            break;
                        }
                    }
                }
                if (off == 0 && Body_GetCollisionRule(car) == 2) {
                    for (k = 0; k < 4; k++) {
                        if (car->wheels[k].dirt != 0) {
                            off = 1;
                            break;
                        }
                    }
                }
                if (off == 1) {
                    int flags = car->flags;

                    car->flags = flags | 4;
                    if ((signed char)car->control != 0) {
                        car->flags = flags | 0xc;
                    }
                }
            }
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", CarPhys_UpdateDrivetrain);
#endif

// The angle from -45 to 45 degrees with the largest curve value x cos, in 4096ths of a
// turn. 90 degrees when none is positive.
#ifdef NON_MATCHING
// The game keeps the division constant in s4 across the loop; here it is rebuilt in
// the loop, one saved register fewer.
int TyreCurve_FindPeakAngle(CarPhys *car)
{
    int offset = 0;
    int best = 0;
    int peak = 90;
    int degrees;

    for (degrees = -45; degrees < 46; degrees++) {
        int angle = (degrees << 12) / 360;
        int y = TyreCurve_EvalAngle(&car->tyres[0], angle - offset);

        y = Fixed_Mul(g_CosTable[angle & 0xfff], y);
        if (best < y) {
            best = y;
            peak = degrees;
        }
    }
    return (peak << 12) / 360;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/drivetrain", TyreCurve_FindPeakAngle);
#endif
}
