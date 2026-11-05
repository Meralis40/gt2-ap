// OVL0 8002f92c-80035714 (carphys): Car controls and physics setup.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define FIXED_MULFLOOR_INT
#include "common.h"

extern "C" {
extern unsigned char g_Damper_RateMin;
extern unsigned char g_Damper_RateMax;
extern signed char g_DiffTypeTable[];
extern short g_SinTable[];
extern short g_CosTable[];
extern int g_TyreWearEnabled;
extern int g_TyreWearInit;
extern int g_TyreGripLossInit;
extern int g_FrameRate;
extern int g_SurfaceRollResist;
extern unsigned char g_DragCurve[];
extern RaceMem g_Race_IsDemo;
extern unsigned char g_AiDriveParams[];
extern int g_PhysConst_SlipDrag;
extern unsigned char g_RaceCourseId;
void CarPhys_SetUnk1c(CarPhys *phys, unsigned char v);
void CarPhys_InitSpeedLimits(CarPhys *phys, int arg);
int Drivetrain_GetClass(int layout);
int TyreCurve_FindPeakAngle(CarPhys *phys);
int CarPhys_ClampMin12000A(int x);
int CarPhys_ClampMin12000B(int x);
short CarPhys_EffectiveMass(short weight);
int Gearbox_ApplyTopSpeedSetting(CarParams *params, int arg);
void Suspension_InitAxleTravel(CarPhys *phys, SuspAxle *susp, CarParams *params, int axle);
void Damper_Init(SuspAxle *susp, unsigned char *settings, unsigned char rate, int axle);
extern char D_80046BC4[];
extern char D_80046BD0[];
extern unsigned char D_80046DA4[];
extern unsigned short D_80046DB4[];
extern unsigned short D_80046DB8[];
extern unsigned short D_80046DBC[];
extern unsigned short D_80046DC0[];
extern unsigned short D_80046DC4[];
extern unsigned short g_PhysFrameRates[];
extern int D_80046F3C;
extern int D_80046F40;
extern int D_80046F44;
int Math_ApproxDist3(int x, int y, int z);
void Car_UpdateClutchAuto(CarPhys *phys);
void Car_UpdateClutchManual(CarPhys *phys, short *input, unsigned char *out, int assist);
void Car_LaunchAssist(CarPhys *phys, short *input, unsigned char *reverse);
void CarPhys_SetProximityFlags(CarPhys *phys, int x, int z, int front, int back, int width);
void Damper_SolveCurve(int *dead, int *base, int *gain, short speed0, int force0, short speed1, int force1, int axle,
                       char *name);
extern GameState g_GameState;
extern unsigned char g_PhysConstBytes[];
extern unsigned char g_RaceEventParams[];
extern unsigned short D_800A9520;
extern unsigned char D_801C98A2;
void Body_ComputeFootprint(CarPhys *body);
void Body_SaveFootprint(CarPhys *body);
unsigned int Body_SweepFootprint(CarPhys *body, int *outFraction, unsigned int *outCorner);
int Body_GetCollisionRule(CarPhys *body);
void Body_ApplyTimeScale(CarPhys *car);
void CarColl_ComputeCorners(RaceVehicle *cars, int count);
void CarColl_FindImpacts(RaceVehicle *cars, int count);
void CarColl_ResolveImpacts(RaceVehicle *cars, int count);
void CarColl_RecomputeCorners(RaceVehicle *cars, int count);
void CarColl_SeparateOverlaps(RaceVehicle *cars, int count);
int CarPhys_CarPairInteraction(CarPhys *a, CarPhys *b);
void CarPhys_ClearProximity(CarPhys *phys);
int CarPhys_MoveAndCollide(CarPhys *phys, int *move, int *rest, int mode);
int CarPhys_PlaceOnTrack(CarPhys *phys, int section, int x, int z, int dirX, int dirZ);
void CarPhys_ResetOrientationHistory(CarPhys *phys);
void CarPhys_ResetState(CarPhys *phys);
void CarPhys_ResetWheels(CarPhys *phys);
void CarPhys_ScaleFrameDelta(CarPhys *phys, int *delta, int scale);
void CarPhys_ServiceTyres(CarPhys *phys);
void CarPhys_SetRollingStart(CarPhys *phys, int mph);
void CarPhys_SetUnk642(CarPhys *phys, unsigned char value);
void CarPhys_SetControl(CarPhys *phys, unsigned char control);
void CarPhys_UpdateLocalVelocity(CarPhys *phys);
void CarPhys_ComputeTopSpeed(CarPhys *phys);
void CarPhys_WallDeflect(CarPhys *phys);
int CarPhys_Init(CarPhys *phys, CarParams *params, int control, int arg, unsigned char unk1c);
void Engine_UpdateRpmDisplay(CarPhys *car);
int Engine_ApplyRevLimiter(CarPhys *phys, int throttle);
int Gearbox_GearForSpeed(CarPhys *phys, int speed);
int GhostCar_Init(CarPhys *phys);
void LicenseTest_ResetResult(CarPhys *phys);
void Race_ShowMessage(int car, int message);
int Race_AllowsOffCourseFlag(void);
int AiLine_GetPitBox(int index);
int AiLine_GetPointAtDistance(int *section, int *x, int *z, int *dx, int *dz, int dist);
int AiLine_PickForSlot(int slot);
int World_GetGroundHeight(void *world, int x, int z, int y, int *section);
int World_GetCourseDistance(void *world, int hint, int *pos);

extern int g_FrameDt;
#define FRAME_DT (*(int *)0x1f800000)

// What World_GetGroundInfo fills.
struct GroundInfo {
    unsigned char surface;
    unsigned char side;
    unsigned char kind;
    unsigned char pad3;
    unsigned short section;
    unsigned short pad6;
    int pos[3];
    int height;
};

void World_GetGroundInfo(void *world, GroundInfo *info);

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// CarPhys fields that ovl0.h doesn't name yet (in its unkXXX runs).
#define CARPHYS_FIELD(car, type, off) (*(type *)((char *)(car) + (off)))

// A curve header: count points, at x and y.
struct CurveHeader {
    short count;
    short *x;
    short *y;
};

// Curve16_Eval's curve.
typedef struct CurveHeader Curve16;
int Curve16_Eval(Curve16 *curve, short x);

struct CurveHeaderInt {
    short count;
    int *x;
    int *y;
};

// TyreAxle, as its builders see it.
struct TyreCurves {
    CurveHeader angle;
    short angleX[8];
    short angleY[8];
    CurveHeader slip;
    short slipX[12];
    short slipY[12];
    short limits[2];
    CurveHeader slip2;
    short slipY2[12];
    CurveHeaderInt load;
    int loadX[4];
    int loadY[4];
    CurveHeader camber;
    short camberX[4];
    short camberY[4];
};

// CarPhys from driveLayout on.
struct CarDrive {
    unsigned char layout;
    unsigned char unk1;
    unsigned char gearCount;
    unsigned char driven;
    unsigned char unk4[2];
    unsigned char unk6[2];
    short unk8;
    unsigned char diff[2];
    int diffA[2];
    int diffB[2];
    int diffC[2];
    unsigned short launchLimit;
    unsigned short launchRpm;
    short shift[6];
    int gearRatio[8];
    int gearSpeed[8];
    short tyreRadius[2];
    short invRadius[2];
    short tyreWidth[2];
    short tyreSide[2];
    short unk84;
    short unk86;
    int unk88[2];
    int unk90[2];
    int unk98;
    int unk9c;
    short unka0[2];
};

#define PARAM_B(params, offset) (((unsigned char *)(params))[offset])
#define PARAM_S(params, offset) (*(short *)((char *)(params) + (offset)))

// What a replay keeps of a car.
struct CarSnapshotWheel {
    int unk[7];
};

struct CarSnapshotBody {
    int unk[0x1a];
};

struct CarSnapshot {
    CarSnapshotBody body;
    CarSnapshotWheel wheels[4];
};

// Manual gearbox's clutch: open while frozen, in the pits or while stopped.
void Car_UpdateClutchManual(CarPhys *phys, short *input, unsigned char *out, int assist)
{
    CarPhys *car = phys;

    if (phys->unk718 != 0 || phys->pitTimer != 0 || phys->aiMode == 7 ||
        (phys->unk373 == 1 && phys->unk614 == 0x1000)) {
        phys->clutch = 0;
    } else if (phys->clutchIn == 0) {
        phys->clutch = 0;
        if (phys->brake != 0x1000) {
            phys->clutch = 0x1000;
        }
    } else {
        phys->clutch = 0x1000;
        if (phys->rpm < car->engine.unk8e) {
            phys->clutch = 0;
        }
    }
}

// Automatic gearbox's clutch: 0x472 is 1 km/h.
void Car_UpdateClutchAuto(CarPhys *phys)
{
    if (phys->unk718 != 0 || phys->pitTimer != 0 || phys->aiMode == 7) {
        phys->clutch = 0;
    } else if (phys->clutchIn == 0) {
        phys->clutch = 0;
        if (phys->brake == 0) {
            phys->clutch = 0x1000;
        }
    } else {
        phys->clutch = 0x1000;
        if (phys->brake != 0 && phys->speedFwd < 0x472 && phys->speedFwd >= -0x471) {
            phys->clutch = 0;
        }
    }
}

// Opens the throttle at low speed, in the direction of the gear; rolling backward
// with nothing pressed sets *reverse.
void Car_LaunchAssist(CarPhys *phys, short *input, unsigned char *reverse)
{
    if (phys->clutch != 0) {
        int speed = phys->speedFwd;

        if (phys->gear == 0) {
            speed = -speed;
        }
        if (speed >= -0x472) {
            speed = 0x3558 - speed;
            if (speed > 0) {
                speed = phys->throttle + (speed << 12) / 0x1aac0;
                if (speed > 0x1000) {
                    speed = 0x1000;
                }
                phys->throttle = speed;
            }
        }
        if (input[3] == 0 && ((signed char *)input)[9] == 0 && phys->speedFwd < -0x472) {
            *reverse = 1;
        }
    }
}

// Pad to steering, throttle, brake, handbrake and clutch.
void Car_ApplyControls(CarPhys *phys, short *input, unsigned char *out, int index)
{
    CarPhys *car = phys;
    short slip = (phys->wheels[0].unk50 + phys->wheels[1].unk50) / 2;
    int speed = phys->speedFwd;
    int limit;
    short full;
    int assist;

    if (speed < 0) {
        speed = -speed;
    }
    speed = speed * 461 >> 12;
    if (speed > 0x7fff) {
        speed = 0x7fff;
    }
    limit = Curve16_Eval((Curve16 *)((char *)phys + 0x3c), speed);
    full = limit;
    if ((short)limit >= phys->maxSteer) {
        full = phys->maxSteer;
    }
    if (phys->speedFwd <= 0x855b || (slip >= -full && slip <= full)) {
        phys->steerLeft = limit;
        phys->steerRight = -limit;
        CARPHYS_FIELD(phys, short, 0x700) = 0;
    } else if (slip > 0) {
        phys->steerLeft = slip;
        if (CARPHYS_FIELD(car, short, 0x3a) < slip) {
            phys->steerLeft = CARPHYS_FIELD(car, short, 0x3a);
        }
        phys->steerRight = -limit;
        CARPHYS_FIELD(phys, short, 0x700) = phys->steerLeft;
    } else if (slip < 0) {
        phys->steerLeft = limit;
        phys->steerRight = slip;
        if (slip < -CARPHYS_FIELD(car, short, 0x3a)) {
            phys->steerRight = -CARPHYS_FIELD(car, short, 0x3a);
        }
        CARPHYS_FIELD(phys, short, 0x700) = phys->steerRight;
    }
    if ((*(unsigned short *)input & 1) == 0) {
        short target;
        int force;
        int rate;
        int next;

        if (input[1] > 0) {
            target = phys->steerLeft;
        } else {
            target = 0;
            if (input[1] < 0) {
                target = phys->steerRight;
            }
        }
        force = Fixed_Mul32(target - phys->steer, D_80046F3C);
        force -= Fixed_Mul32(*(short *)phys->unk60e, D_80046F40);
        if (target == 0) {
            force -= Fixed_Mul32(phys->steer, D_80046F44);
        }
        *(short *)phys->unk60e += Fixed_Mul32_16(force, FRAME_DT);
        rate = *(short *)phys->unk60e;
        if (rate > car->steerSpeed) {
            rate = car->steerSpeed;
        } else if (rate < -car->steerSpeed) {
            rate = -car->steerSpeed;
        }
        next = phys->steer + Fixed_Mul32_16(rate, FRAME_DT);
        if ((target > 0 && target < next) || (target < 0 && next < target) || (phys->steer < target && next >= target) ||
            (target < phys->steer && next <= target)) {
            phys->steer = target;
            *(short *)phys->unk60e = 0;
        } else {
            phys->steer = next;
        }
    } else {
        int amount;
        int target;
        int step;
        int steer;

        if ((*(unsigned short *)input & 8) != 0) {
            amount = input[1];
            if (amount < 0) {
                amount = -amount;
            }
            amount = (short)Curve16_Eval((Curve16 *)D_80046DA4, amount);
            if (input[1] < 0) {
                amount = -amount;
            }
        } else {
            amount = input[1] * input[1] >> 12;
            if (input[1] < 0) {
                amount = -amount;
            }
        }
        if (amount >= 0) {
            target = phys->steerLeft * amount >> 12;
        } else {
            target = -(phys->steerRight * amount >> 12);
        }
        step = FRAME_DT * 1479 >> 16;
        steer = phys->steer;
        if (steer < (short)target) {
            steer += step;
            if ((short)target < steer) {
                steer = (short)target;
            }
        } else if ((short)target < steer) {
            steer -= step;
            if (steer < (short)target) {
                steer = (short)target;
            }
        }
        phys->steer = steer;
    }
    if (CARPHYS_FIELD(phys, short, 0x700) != 0) {
        CARPHYS_FIELD(phys, short, 0x700) = (phys->steer << 12) / CARPHYS_FIELD(phys, short, 0x700);
        if (CARPHYS_FIELD(phys, short, 0x700) < 0) {
            CARPHYS_FIELD(phys, short, 0x700) = 0;
        } else if (CARPHYS_FIELD(phys, short, 0x700) > 0x1000) {
            CARPHYS_FIELD(phys, short, 0x700) = 0x1000;
        }
    }
    assist = 0;
    if (phys->aiMode == 0) {
        assist = phys->unk642;
    }
    if ((*(unsigned short *)input & 2) == 0) {
        int down = -D_80046DB4[index];
        int up = g_PhysFrameRates[index];
        short next;

        if (((signed char *)input)[9] != 0 && phys->pitTimer == 0) {
            out[0] = 1;
            down = up;
        } else {
            out[0] = 0;
            if (input[3] != 0) {
                down = up;
            }
        }
        next = phys->throttle + down;
        if (next > 0x1000) {
            next = 0x1000;
        } else if (next < 0) {
            next = 0;
        }
        phys->throttle = next;
    } else {
        if (((signed char *)input)[9] != 0) {
            out[0] = 1;
            phys->throttle = 0x1000;
        } else {
            out[0] = 0;
            phys->throttle = input[3];
        }
        if (phys->throttle != 0) {
            phys->throttle = (phys->throttle * 7 >> 3) + 0x200;
        }
    }
    if ((*(unsigned short *)input & 4) == 0) {
        if (input[2] != 0) {
            phys->brake += D_80046DB8[index];
            if (phys->brake > 0x1000) {
                phys->brake = 0x1000;
            }
        } else {
            phys->brake -= D_80046DBC[index];
            if (phys->brake < 0) {
                phys->brake = 0;
            }
        }
    } else {
        phys->brake = input[2];
        if (phys->brake != 0) {
            phys->brake = (phys->brake * 7 >> 3) + 0x200;
        }
    }
    if (phys->pitTimer == 0) {
        out[1] = ((unsigned char *)input)[8];
    } else {
        out[1] = 0;
    }
    if (((signed char *)input)[10] != 0) {
        phys->unk614 += D_80046DC0[index];
        if (phys->unk614 > 0x1000) {
            phys->unk614 = 0x1000;
        }
    } else {
        phys->unk614 -= D_80046DC4[index];
        if (phys->unk614 < 0) {
            phys->unk614 = 0;
        }
    }
    if (phys->gearCount == 1) {
        Car_UpdateClutchAuto(phys);
        Car_LaunchAssist(phys, input, out);
    } else {
        Car_UpdateClutchManual(phys, input, out, assist);
        if (phys->gearCount == 2) {
            int forward = phys->speedFwd;

            if (phys->gear == 0) {
                forward = -forward;
            }
            if (forward >= -0x472) {
                forward = 0x2c74 - forward;
                if (forward > 0) {
                    forward = phys->throttle + (forward << 12) / 0x58e8;
                    if (forward > 0x1000) {
                        forward = 0x1000;
                    }
                    phys->throttle = forward;
                }
                if (input[3] == 0 && ((signed char *)input)[9] == 0 && phys->gear == 0) {
                    out[0] = 1;
                }
            }
        }
    }
}

// Sets aiMessage unless a held message is showing; a non-zero hold makes this_ one stick.
void CarAi_SetMessage(CarPhys *phys, unsigned char msg, int hold)
{
    if ((hold != 0) || (phys->aiMessageHold == 0)) {
        phys->aiMessage = msg;
        phys->aiMessageHold = (unsigned char)hold;
    }
}

// Each car's move over this frame, in the scratchpad from 0x1f8000b4.
void Cars_ComputeFrameDelta(RaceVehicle *cars, int count)
{
    int i;

    i = 0;
    while (i < count) {
        int offset = i * 0x10 + 0xb4;
        int *delta = (int *)(0x1f800000 + offset);
        CarPhys *phys = &cars[i].phys;
        int k;

        FRAME_DT = phys->timeStep;
        for (k = 0; k < 3; k++) {
            int moved = Fixed_Mul32_16(phys->vel[k], FRAME_DT);

            delta[k] = moved;
        }
        i++;
        delta[3] = Fixed_Mul16Shift(phys->yawRate, FRAME_DT, 7);
    }
}

// The car's move over scale of this frame.
void CarPhys_ScaleFrameDelta(CarPhys *phys, int *delta, int scale)
{
    int i;

    for (i = 0; i < 3; i++) {
        int moved = Fixed_Mul32(scale, phys->vel[i]);

        delta[i] = Fixed_Mul32_16(moved, FRAME_DT);
    }
    delta[3] = Fixed_Mul16Shift(Fixed_Mul(scale, phys->yawRate), FRAME_DT, 7);
}

// World velocity to body speeds, km/h and each wheel's own velocity.
void CarPhys_UpdateLocalVelocity(CarPhys *phys)
{
    int fwd = 0;
    int lat = 0;
    int i;
    int speed;
    int spin;
    int front;
    int rear;
    int limit;
    unsigned int w;
    int *fwdSide;
    int *latSide;
    int offsetLat[4];
    int offsetFwd[4];
    short *dirs = phys->localDirs;

    for (i = 0; i < 3; i++) {
        int v = phys->vel[i];

        fwd += Fixed_MulFloor(dirs[i + 6], v);
        lat += dirs[i + 10] * v >> 12;
    }
    phys->speedFwd = fwd;
    phys->speedLat = lat;
    speed = Fixed_MulShift(fwd < 0 ? -fwd : fwd, 0xdfbdd, 12);
    phys->speedKmh = speed;
    if (phys->maxSpeedKmh < (unsigned short)speed) {
        phys->maxSpeedKmh = speed;
    }
    spin = Fixed_MulShiftTrunc(phys->yawRate, 0x6488, 7);
    offsetLat[0] = offsetLat[1] = -Fixed_Mul32(phys->distFrontAxle, spin);
    offsetLat[2] = offsetLat[3] = Fixed_Mul32(phys->distRearAxle, spin);
    front = Fixed_Mul32(phys->halfTrackF, spin);
    offsetFwd[0] = -front;
    offsetFwd[1] = front;
    rear = Fixed_Mul32(phys->halfTrackR, spin);
    offsetFwd[2] = -rear;
    offsetFwd[3] = rear;
    limit = 0x1bc87;
    latSide = offsetLat;
    fwdSide = offsetFwd;
    for (w = 0; w < 4; w++) {
        CarPhysWheel *wheel = &phys->wheels[w];
        int abs;
        int grip;

        wheel->velFwd = phys->speedFwd + *fwdSide;
        wheel->velLat = phys->speedLat + *latSide;
        abs = wheel->velFwd;
        if (abs < 0) {
            abs = -abs;
        }
        grip = 0x1000;
        if (abs <= limit) {
            grip = (Fixed_Mul(abs, 0x93) << 11 >> 12) + 0x800;
        }
        wheel->slipGrip = grip;
        latSide++;
        fwdSide++;
    }
}

// Free wheels follow the road, then every wheel's spin angle moves.
void Cars_UpdateWheelRotation(RaceVehicle *cars, int count)
{
    int rolling[2];
    int *flags = rolling;
    int i;

    for (i = 0; i < count; i++, cars++) {
        CarPhys *phys = &cars->phys;
        CarPhys *car = phys;
        int w;

        CarPhys_UpdateLocalVelocity(phys);
        flags[1] = 0;
        flags[0] = 0;
        for (w = 0; w < 4; w++) {
            CarPhysWheel *wheel = &car->wheels[w];

            if (wheel->unk63 == 0) {
                wheel->rollSpeed = Fixed_Mul(g_CosTable[wheel->heading & 0xfff], wheel->velFwd) -
                                   Fixed_Mul(g_SinTable[wheel->heading & 0xfff], wheel->velLat);
                flags[w >> 1] = 1;
            }
        }
        for (w = 0; w < 2; w++) {
            if (flags[w] != 0 || car->unk37a[w] == 0) {
                car->axleSpeed[w] = Fixed_Mul(car->invTyreRadius[w],
                                              (car->wheels[w * 2].rollSpeed + car->wheels[w * 2 + 1].rollSpeed) / 2);
            }
        }
        for (w = 0; w < 4; w++) {
            CarPhysWheel *wheel = &car->wheels[w];
            int distance = Fixed_MulFloor16(FRAME_DT, wheel->rollSpeed) * phys->invTyreRadius[w >> 1] >> 12;
            short angle = wheel->spinAngle + Fixed_Mul32(distance, 0x28c);

            while (angle >= 0x1000) {
                angle -= 0x1000;
            }
            while (angle < 0) {
                angle += 0x1000;
            }
            wheel->spinAngle = angle;
        }
    }
}

// Walks up the speeds and gears until drag beats the engine's force.
void CarPhys_ComputeTopSpeed(CarPhys *phys)
{
    CarDrive *drive = (CarDrive *)&phys->driveLayout;
    int limit;
    int step;
    int gear;
    int kmh;

    if (drive->gearCount == 1) {
        phys->topSpeed = 0x32028;
        return;
    }
    limit = Fixed_MulShift(phys->topSpeed, 0x3999, 12);
    gear = 1;
    step = limit / 500 + 1;
    for (kmh = 0; kmh < limit; kmh += step) {
        int speed = kmh * 0x472;
        int rate = Fixed_Mul(speed, drive->gearSpeed[gear]);
        int rpm = rate * 60 / 0x1000;
        int force = Fixed_Mul(EngineCurve_TorqueAt(&phys->engine, rate), Fixed_Mul(0x6488, drive->gearSpeed[gear]));
        int abs = speed;
        int sq;
        int a, b;
        int drag;

        if (abs < 0) {
            abs = -speed;
        }
        abs >>= 5;
        sq = abs * abs >> 12;
        a = phys->dragA * sq >> 12;
        b = phys->dragB * sq >> 12;
        drag = Fixed_Mul(g_SurfaceRollResist, EngineCurve_TorqueAt((EngineCurve *)g_DragCurve, speed));
        drag = Fixed_Mul(phys->rollResist + a + b, drag) + (phys->unk43c * sq >> 12);
        if (drag < force) {
            phys->topSpeed = speed;
        }
        if (gear < drive->gearCount && (int)drive->launchRpm < rpm) {
            gear++;
        }
    }
}

// Light cars count as heavier: under 900 kg the weight rises towards 810.
short CarPhys_EffectiveMass(short weight)
{
    if (weight >= 900) {
        return weight;
    }
    if (weight < 750) {
        weight = 750;
    }
    return weight > 400 ? (short)((weight - 400) * 300 / 500 + 600) : 600;
}

// max(x, 12000). Used for CarParams.unk57[0] * 400 (identical to CarPhys_ClampMin12000B).
int CarPhys_ClampMin12000A(int x)
{
    if (x < 12000) {
        x = 12000;
    }
    return x;
}

// max(x, 12000). Used for CarParams.unk57[2] * 400.
int CarPhys_ClampMin12000B(int x)
{
    if (x < 12000) {
        x = 12000;
    }
    return x;
}

// Redline speed in top gear, least brake force and the real top speed.
void CarPhys_InitSpeedLimits(CarPhys *phys, int arg)
{
    int mass;
    int force;

    phys->topSpeed = Fixed_Div((phys->engine.redline << 12) / 60, phys->gearSpeed[phys->gearCount], 0);
    mass = phys->mass;
    force = Fixed_Mul(phys->brakeForce[0], phys->invTyreRadius[0]);
    phys->brakeMin = Fixed_Div(mass, (force + Fixed_Mul(phys->brakeForce[1], phys->invTyreRadius[1])) << 2, 0);
    CarPhys_ComputeTopSpeed(phys);
}

// phys->unk1c = v.
void CarPhys_SetUnk1c(CarPhys *phys, unsigned char v)
{
    phys->unk1c = v;
}

// Layout to driving class.
int Drivetrain_GetClass(int layout)
{
    int result = 0;

    switch (layout) {
    case 1:
    case 2:
        result = 1;
        break;
    case 3:
    case 4:
    case 6:
        result = 2;
        break;
    case 5:
        result = 3;
        break;
    }
    return result;
}

// control = control (0 player, 2 CPU), aiMode = 0.
void CarPhys_SetControl(CarPhys *phys, unsigned char control)
{
    phys->control = control;
    phys->aiMode = 0;
}

// The player's value; 0 for CPU cars, unchanged for the rest.
void CarPhys_SetUnk642(CarPhys *phys, unsigned char value)
{
    switch (phys->control) {
    case 0:
        phys->unk642 = value;
        break;
    case 2:
        phys->unk642 = 0;
        break;
    }
}

// Fits the damper's sqrt curve through (speed0, force0) and (speed1, force1).
void Damper_SolveCurve(int *dead, int *base, int *gain, short speed0, int force0, short speed1, int force1, int axle,
                       char *name)
{
    int det;
    int square;
    int q;
    int p;
    int dz;

    *gain = 0;
    *base = 0;
    *dead = 0;
    if (force0 == 0 || force1 == 0 || speed0 == 0 || speed1 == 0 || speed0 >= speed1 || force0 >= force1) {
        return;
    }
    det = Fixed_Mul(force0, speed1) - Fixed_Mul(force1, speed0);
    if (det <= 0) {
        return;
    }
    square = Fixed_Mul(Fixed_Mul(force0, force0), speed1) - Fixed_Mul(Fixed_Mul(force1, force1), speed0);
    if (square > 0) {
        return;
    }
    p = Fixed_Div(Fixed_Mul(Fixed_Mul(force0, force1), force1 - force0), det, 0);
    q = Fixed_Div(square, det * 2, 0);
    *gain = Math_ISqrt(p, 6);
    dz = Fixed_Div(Fixed_Mul(q, q), p, 0);
    *dead = -dz;
    *base = Fixed_Mul(-*gain, Math_ISqrt(dz, 6));
}

// Linear map of level 10..255 between the two bytes at 0x80046dc8/9.
int Damper_LevelToRate(int level)
{
    return (unsigned int)g_Damper_RateMin +
           (int)((level + -10) * ((unsigned int)g_Damper_RateMax - (unsigned int)g_Damper_RateMin)) / 0xf5;
}

// The suspension's spring, dampers and anti-roll from the car's settings.
void Damper_Init(SuspAxle *susp, unsigned char *settings, unsigned char rate, int axle)
{
    int scale;
    int speed0, speed1;
    int force0, force1;
    short x0;

    susp->springRate = Damper_LevelToRate(settings[0]) * 3920;
    scale = 0x64000 / rate;
    speed0 = (settings[4] << 12) / 100;
    force0 = settings[5] * 802820 / 0x1000;
    speed1 = (settings[6] << 12) / 100;
    force1 = settings[7] * 802820 / 0x1000;
    x0 = Fixed_Mul(scale, (short)speed0);
    Damper_SolveCurve(&susp->bumpDead, &susp->bumpBase, &susp->bumpGain, x0, force0, Fixed_Mul(scale, (short)speed1),
                      force1, axle, D_80046BC4);
    {
        int speed2 = (settings[8] << 12) / 100;

        force0 = settings[9] * 802820 / 0x1000;
        speed1 = (settings[10] << 12) / 100;
        force1 = settings[11] * 802820 / 0x1000;
        x0 = Fixed_Mul(scale, (short)speed2);
    }
    Damper_SolveCurve(&susp->reboundDead, &susp->reboundBase, &susp->reboundGain, x0, force0,
                      Fixed_Mul(scale, (short)speed1), force1, axle, D_80046BD0);
    susp->antiRoll = settings[1] * 0x188008 / 0x1000;
    susp->bumpRate = settings[2] * 0xf50050 / 0x1000;
    *(short *)susp->unk30 = settings[3];
    susp->preload = settings[3] * 160564 / 0x4000;
}

// One axle's ride height and how far it can travel each way.
void Suspension_InitAxleTravel(CarPhys *phys, SuspAxle *susp, CarParams *params, int axle)
{
    int bump;
    int limit;
    unsigned short ride;
    unsigned short least;
    unsigned char *travel = &params->unk19e[axle];

    if (susp->springRate != 0) {
        *(short *)susp->unka = -Fixed_Div(phys->unk434[axle] / 2, susp->springRate, 0);
    }
    ride = *(short *)susp->unka + (phys->tyreRadius[axle] - phys->unk24[axle]);
    susp->rideHeight = ride;
    least = ride - phys->tyreRadius[axle];
    susp->travelMin = least;
    bump = (travel[0] << 12) / 1000;
    if (phys->unk24[axle] < bump) {
        bump = phys->unk24[axle];
    }
    susp->bumpStart = least + bump;
    susp->travelMax = susp->travelMin + (phys->unk24[axle] + (travel[2] << 12) / 1000);
    limit = Fixed_Div(susp->preload, susp->springRate, 0);
    if (limit < susp->travelMax) {
        susp->travelMax = limit;
    }
}

// Points every curve header at the arrays after it.
void CarPhys_LinkTables(CarPhys *phys)
{
    CarPhys *car = phys;
    EngineCurve *engine = &car->engine;
    int i;

    engine->rpm = engine->rpmFx;
    engine->torque = engine->torqueFx;
    CARPHYS_FIELD(car, void *, 0x40) = (char *)car + 0x48;
    CARPHYS_FIELD(car, void *, 0x44) = (char *)car + 0x54;
    for (i = 0; i < 2; i++) {
        TyreCurves *t = (TyreCurves *)&car->tyres[i];
        short *slipX;
        short *slipY;

        t->angle.x = t->angleX;
        t->angle.y = t->angleY;
        slipX = t->slipX;
        slipY = t->slipY;
        t->slip.y = slipY;
        t->slip.x = slipX;
        t->slip2.x = slipX;
        t->slip2.y = t->slipY2;
        t->load.x = t->loadX;
        t->load.y = t->loadY;
        t->camber.x = t->camberX;
        t->camber.y = t->camberY;
    }
}

// The slip curves run from the negative points, reversed, through the positive ones.
void Tyre_BuildSlipCurves(unsigned char *negX, unsigned char *negY, unsigned char *negY2, int negCount, unsigned char *posX,
                          unsigned char *posY, unsigned char *posY2, int posCount, TyreCurves *t)
{
    int peak = 0;
    int i;
    int next;
    int index;
    short y;

    negX[0] = 0;
    negY[0] = 0;
    posX[0] = 0;
    posY[0] = 0;
    negY2[0] = posY2[0];
    t->limits[0] = 0;
    for (i = 0; i < negCount; i = next) {
        next = i + 1;
        index = negCount - next;
        t->slipX[index] = (-negX[i] << 12) / 255;
        t->slipY[index] = (negY[i] << 12) / 200;
        y = t->slipY[index];
        if (peak < y) {
            peak = y;
            t->limits[0] = t->slipX[index];
        }
        t->slipY2[index] = (negY2[i] << 12) / 200;
    }
    peak = 0;
    t->limits[1] = 0;
    for (i = 0; i < posCount; i++) {
        next = i - 1;
        index = negCount + next;
        t->slipX[index] = (posX[i] << 12) / 255;
        t->slipY[index] = (posY[i] << 12) / 200;
        y = t->slipY[index];
        if (peak < y) {
            peak = y;
            t->limits[1] = t->slipX[index];
        }
        t->slipY2[index] = (posY2[i] << 12) / 200;
    }
    t->slip.count = negCount + posCount - 1;
    t->slip.x = t->slipX;
    t->slip.y = t->slipY;
    t->slip2.count = negCount + posCount - 1;
    t->slip2.x = t->slipX;
    t->slip2.y = t->slipY2;
}

void Tyre_BuildCurveB(unsigned char *xs, unsigned char *ys, unsigned char count, TyreCurves *t)
{
    int i;

    xs[0] = 0;
    ys[0] = 0;
    for (i = 0; i < count; i++) {
        t->angleX[i] = (xs[i] << 10) / 255;
        t->angleY[i] = (ys[i] << 12) / 200;
    }
    t->angle.count = count;
    t->angle.x = t->angleX;
    t->angle.y = t->angleY;
}

void Tyre_BuildLoadCurves(unsigned char *loads, unsigned char *grips, unsigned char count, unsigned char *cambers,
                          unsigned char *camberGrips, unsigned char camberCount, short scale, int factor, TyreCurves *t)
{
    int n = count;
    int i;

    loads[0] = 0;
    for (i = 0; i < n; i++) {
        int load = scale * (loads[i] * 0x310010 / 0x1000) >> 12;

        t->loadX[i] = load;
        t->loadY[i] = Fixed_Mul(factor, load) * grips[i] / 200;
    }
    t->load.count = count;
    t->load.x = t->loadX;
    t->load.y = t->loadY;
    cambers[0] = 0;
    for (i = 0; i < camberCount; i++) {
        t->camberX[i] = cambers[i] * 227 / 255;
        t->camberY[i] = (camberGrips[i] << 12) / 200;
    }
    t->camber.count = camberCount;
    t->camber.x = t->camberX;
    t->camber.y = t->camberY;
}

// The whole car, from its race parameters.
void Drivetrain_Init(CarParams *params, CarDrive *drive, EngineCurve *curve, int arg);

int CarPhys_Init(CarPhys *phys, CarParams *params, int control, int arg, unsigned char unk1c)
{
    int i;
    int front;
    int rear;
    int weight;
    short wheelbase;
    int grip[2];
    int factor[2];
    int force;
    int area;
    int ride;
    int sprung;

    CarPhys_SetUnk1c(phys, unk1c);
    if (params->drivetrain >= 5) {
        params->drivetrain = 0;
    }
    weight = PARAM_B(params, 0x12);
    phys->lenFront = (PARAM_S(params, 0x8) << 12) / 1000;
    phys->lenRear = (PARAM_S(params, 0xa) << 12) / 1000;
    wheelbase = (PARAM_S(params, 0x10) << 12) / 1000;
    phys->wheelbase = wheelbase;
    phys->frontWeight = (weight << 12) / 100;
    phys->distRearAxle = wheelbase * weight / 100;
    phys->distFrontAxle = wheelbase - phys->distRearAxle;
    phys->rearWeight = 0x1000 - phys->frontWeight;
    CARPHYS_FIELD(phys, short, 0x2e) = (phys->distRearAxle - phys->distFrontAxle) / 2;
    phys->lenFront -= CARPHYS_FIELD(phys, short, 0x2e);
    phys->lenRear += CARPHYS_FIELD(phys, short, 0x2e);
    phys->width = (PARAM_S(params, 0xc) << 12) / 1000;
    phys->halfTrackF = (PARAM_S(params, 0x14) << 12) / 2000;
    phys->halfTrackR = (PARAM_S(params, 0x16) << 12) / 2000;
    CARPHYS_FIELD(phys, short, 0x12) = phys->halfTrackF * weight / 50 + phys->halfTrackR * (100 - weight) / 50;
    phys->lenFront -= 0x80;
    phys->lenRear -= 0x80;
    CARPHYS_FIELD(phys, short, 0xa) = (PARAM_S(params, 0xe) << 12) / 1000;
    phys->width -= phys->width / 20;
    EngineCurve_Build(params, &phys->engine, arg);
    Drivetrain_Init(params, (CarDrive *)&phys->driveLayout, &phys->engine, arg);
    phys->driveClass = Drivetrain_GetClass(phys->driveLayout);
    Tyre_BuildSlipCurves((unsigned char *)params + 0xb1, (unsigned char *)params + 0xb7, (unsigned char *)params + 0xbd,
                         PARAM_B(params, 0xb0), (unsigned char *)params + 0xc4, (unsigned char *)params + 0xca,
                         (unsigned char *)params + 0xd0, PARAM_B(params, 0xc3), (TyreCurves *)&phys->tyres[0]);
    Tyre_BuildSlipCurves((unsigned char *)params + 0xd7, (unsigned char *)params + 0xdd, (unsigned char *)params + 0xe3,
                         PARAM_B(params, 0xd6), (unsigned char *)params + 0xea, (unsigned char *)params + 0xf0,
                         (unsigned char *)params + 0xf6, PARAM_B(params, 0xe9), (TyreCurves *)&phys->tyres[1]);
    Tyre_BuildCurveB((unsigned char *)params + 0x8f, (unsigned char *)params + 0x97, PARAM_B(params, 0x8e),
                     (TyreCurves *)&phys->tyres[0]);
    Tyre_BuildCurveB((unsigned char *)params + 0xa0, (unsigned char *)params + 0xa8, PARAM_B(params, 0x9f),
                     (TyreCurves *)&phys->tyres[1]);
    phys->maxSteer = TyreCurve_FindPeakAngle(phys);
    if (phys->maxSteer >= 0x156) {
        phys->maxSteer = 0x155;
    }
    for (i = 0; i < 8; i++) {
        int surface = PARAM_B(params, 0x192 + i);

        if (surface == 0) {
            phys->surfaceGrip[i] = 0x100;
            phys->surfaceBrake[i] = 0x100;
        } else {
            phys->surfaceGrip[i] = (surface << 8) / 100;
            phys->surfaceBrake[i] = 0x6400 / surface;
        }
    }
    grip[0] = PARAM_B(params, 0x86);
    grip[1] = PARAM_B(params, 0x87);
    for (i = 0; i < 2; i++) {
        if (grip[i] == 0) {
            grip[i] = 100;
        }
    }
    for (i = 0; i < 2; i++) {
        int rate;

        if (grip[i] < 0xaf) {
            rate = ((grip[i] - 0xaf) << 12) / 100;
        } else {
            rate = ((grip[i] - 0xaf) << 12) / 200;
        }
        rate += 0x1000;
        if (rate < 0x200) {
            rate = 0x200;
        }
        (&CARPHYS_FIELD(phys, short, 0x448))[i] = (rate * 8) / 100;
        (&CARPHYS_FIELD(phys, short, 0x44c))[i] = (rate * 35) / 100;
    }
    for (i = 0; i < 2; i++) {
        grip[i] = (grip[i] << 12) / 100;
    }
    factor[0] = PARAM_B(params, 0x17e);
    factor[1] = PARAM_B(params, 0x17f);
    for (i = 0; i < 2; i++) {
        if (factor[i] == 0) {
            factor[i] = 100;
        }
        grip[i] = factor[i] * grip[i] / 100;
    }
    if (control == 2) {
        factor[0] = g_RaceEventParams[phys->driveClass + 4];
        factor[1] = g_RaceEventParams[phys->driveClass + 8];
        for (i = 0; i < 2; i++) {
            if (factor[i] == 0) {
                factor[i] = 100;
            }
            grip[i] = factor[i] * grip[i] / 100;
        }
    }
    {
        short bias = ((short *)((char *)g_AiDriveParams + phys->driveClass * 0x28 + 0x24))[0];

        force = Fixed_Mul(Fixed_Dot2Shift(bias, grip[0], 0x1000 - bias, grip[1], 0), 0x9ccd);
    }
    phys->brakeDecel = Fixed_Div(0x1000, force, 0);
    {
        short a = Curve16_Eval((Curve16 *)((char *)phys + 0x200), 0);
        short b = Curve16_Eval((Curve16 *)((char *)phys + 0x2d8), 0);

        phys->cornerGrip = Fixed_Mul((a + b) / 2, force);
    }
    *(short *)phys->unk34 = Fixed_Div(0x1000, phys->cornerGrip, 0);
    Tyre_BuildLoadCurves((unsigned char *)params + 0x11b, (unsigned char *)params + 0x11f, PARAM_B(params, 0x11a),
                         (unsigned char *)params + 0x12d, (unsigned char *)params + 0x131, PARAM_B(params, 0x12c),
                         *(short *)phys->unk3ec, grip[0], (TyreCurves *)&phys->tyres[0]);
    Tyre_BuildLoadCurves((unsigned char *)params + 0x124, (unsigned char *)params + 0x128, PARAM_B(params, 0x123),
                         (unsigned char *)params + 0x136, (unsigned char *)params + 0x13a, PARAM_B(params, 0x135),
                         ((short *)phys->unk3ec)[1], grip[1], (TyreCurves *)&phys->tyres[1]);
    area = Fixed_Mul(phys->width, CARPHYS_FIELD(phys, short, 0xa));
    phys->unk43c = Fixed_Mul(area, Fixed_Mul(g_PhysConst_SlipDrag / 2, (PARAM_B(params, 0x62) << 12) / 100));
    phys->dragA = Fixed_Mul(area, Fixed_Mul(g_PhysConst_SlipDrag / 2, (PARAM_B(params, 0x88) << 12) / 100));
    phys->dragB = Fixed_Mul(area, Fixed_Mul(g_PhysConst_SlipDrag / 2, (PARAM_B(params, 0x89) << 12) / 100));
    phys->unk43c += (phys->dragA + phys->dragB) / 10;
    phys->brakeForce[0] = PARAM_B(params, 0x60) * 0x188008 / 0x1000;
    phys->brakeForce[1] = PARAM_B(params, 0x61) * 0x188008 / 0x1000;
    CARPHYS_FIELD(phys, int, 0x41c) = PARAM_B(params, 0x63) * 0x188008 / 0x1000;
    if (PARAM_B(params, 0x60) != 0 && PARAM_B(params, 0x61) != 0) {
        for (i = 0; i < 2; i++) {
            (&CARPHYS_FIELD(phys, short, 0x42c))[i] = Fixed_Div(phys->tyreRadius[i], phys->brakeForce[i], 0);
        }
    }
    for (i = 0; i < 2; i++) {
        (&CARPHYS_FIELD(phys, int, 0x424))[i] = ((i == 0 ? PARAM_B(params, 0x2e) : PARAM_B(params, 0x2f)) << 12) / 10;
    }
    CARPHYS_FIELD(phys, short, 0x458) = (PARAM_B(params, 0x1a5) << 12) / 100;
    CARPHYS_FIELD(phys, short, 0x45a) = (PARAM_B(params, 0x1a6) << 12) / 100;
    CARPHYS_FIELD(phys, short, 0x454) = (PARAM_B(params, 0x1a2) << 12) / 10;
    CARPHYS_FIELD(phys, short, 0x456) = (PARAM_B(params, 0x1a3) << 12) / 100;
    CARPHYS_FIELD(phys, int, 0x450) = (PARAM_B(params, 0x1a4) << 12) / 10;
    CARPHYS_FIELD(phys, short, 0x38) = (PARAM_B(params, 0x5e) << 12) / 360;
    CARPHYS_FIELD(phys, short, 0x3a) = (PARAM_B(params, 0x5f) << 12) / 360;
    phys->steerSpeed = (PARAM_B(params, 0x118) * 5 << 13) / 360;
    if ((Course_Get(g_RaceCourseId)->flags & 4) != 0) {
        phys->steerSpeed = 0x5c7;
    }
    for (i = 0; i < PARAM_B(params, 0x44); i++) {
        (&CARPHYS_FIELD(phys, short, 0x48))[i] = PARAM_B(params, 0x45 + i) << 7;
        (&CARPHYS_FIELD(phys, short, 0x54))[i] = (PARAM_B(params, 0x4b + i) << 12) / 1800;
    }
    CARPHYS_FIELD(phys, short, 0x3c) = PARAM_B(params, 0x44);
    CARPHYS_FIELD(phys, void *, 0x40) = (char *)phys + 0x48;
    CARPHYS_FIELD(phys, void *, 0x44) = (char *)phys + 0x54;
    front = phys->distFrontAxle + phys->distRearAxle;
    rear = phys->halfTrackF;
    CARPHYS_FIELD(phys, short, 0x64) = Math_Atan2(front, front - rear) << 3;
    CARPHYS_FIELD(phys, short, 0x66) = Math_Atan2(front, front + rear) << 3;
    phys->camber[0] = -((PARAM_B(params, 0x6c) << 12) / 360) / 10;
    phys->camber[1] = -((PARAM_B(params, 0x6d) << 12) / 360) / 10;
    for (i = 0; i < 2; i++) {
        (&CARPHYS_FIELD(phys, short, 0x36c))[i] = PARAM_B(params, 0x18e + i) - 0x80;
    }
    if (PARAM_B(params, 0x57) != 0) {
        CARPHYS_FIELD(phys, int, 0x68) = Fixed_Div(0x28c, CarPhys_ClampMin12000A(PARAM_B(params, 0x57) * 400), 7);
    }
    if (PARAM_B(params, 0x58) != 0) {
        int stiffness = PARAM_B(params, 0x58) * 25 << 4;

        CARPHYS_FIELD(phys, int, 0x6c) = Fixed_Div(phys->distFrontAxle, stiffness, 4);
        CARPHYS_FIELD(phys, int, 0x70) = Fixed_Div(phys->distRearAxle, stiffness, 4);
    }
    if (PARAM_B(params, 0x59) != 0) {
        int stiffness = CarPhys_ClampMin12000B(PARAM_B(params, 0x59) * 400);

        CARPHYS_FIELD(phys, int, 0x74) = Fixed_Div(phys->halfTrackF, stiffness, 4);
        CARPHYS_FIELD(phys, int, 0x78) = Fixed_Div(phys->halfTrackR, stiffness, 4);
    }
    Damper_Init(&phys->susp[0], (unsigned char *)params + 0x6e, PARAM_B(params, 0x18c), arg);
    Damper_Init(&phys->susp[1], (unsigned char *)params + 0x7a, PARAM_B(params, 0x18d), arg);
    phys->unk24[0] = (PARAM_B(params, 0x116) << 12) / 1000;
    phys->unk24[1] = (PARAM_B(params, 0x117) << 12) / 1000;
    CARPHYS_FIELD(phys, short, 0x22) = (phys->unk24[0] * phys->frontWeight >> 12) + (phys->unk24[1] * phys->rearWeight >> 12);
    ride = CARPHYS_FIELD(phys, short, 0x22);
    if (ride >= 0x334) {
        ride = 0x333;
    }
    ride = (ride - 0xe1) * 230 / 594 + 0x32;
    ride = (((unsigned short)ride - 0x96) << 12) / 1000;
    if ((short)ride < 0) {
        ride = 0;
    }
    ride = (short)(ride + CARPHYS_FIELD(phys, short, 0x22));
    CARPHYS_FIELD(phys, short, 0x1e) = Fixed_Div(ride, CARPHYS_FIELD(phys, short, 0x12), 0) / 2;
    CARPHYS_FIELD(phys, short, 0x20) = Fixed_Div(ride, phys->wheelbase, 0) / 2;
    {
        int mass = CarPhys_EffectiveMass(PARAM_S(params, 0x5a));

        phys->mass = mass * 4;
        phys->invMass = Fixed_Div(0x1000, (short)(mass << 2), 0);
    }
    phys->rollResist = Fixed_Mul(phys->mass, 0x9ccd);
    sprung = phys->mass - (*(short *)phys->susp[0].unk30 + *(short *)phys->susp[1].unk30) * 2;
    if (sprung > 0) {
        int ratio;

        *(short *)phys->unk2c = Fixed_Div(0x1000, sprung, 0);
        phys->unk434[0] = Fixed_Mul(Fixed_Mul(sprung, 0x9ccd), phys->frontWeight);
        phys->unk434[1] = Fixed_Mul(Fixed_Mul(sprung, 0x9ccd), phys->rearWeight);
        ratio = Fixed_Div(sprung, phys->mass, 0);
        CARPHYS_FIELD(phys, short, 0x1e) = sprung * CARPHYS_FIELD(phys, short, 0x1e) >> 12;
        CARPHYS_FIELD(phys, short, 0x20) = ratio * CARPHYS_FIELD(phys, short, 0x20) >> 12;
    }
    for (i = 0; i < 2; i++) {
        Suspension_InitAxleTravel(phys, &phys->susp[i], params, i);
    }
    CarPhys_InitSpeedLimits(phys, arg);
    return 0;
}

// New tyres, at the wear the race starts with.
void CarPhys_ServiceTyres(CarPhys *phys)
{
    int i;

    for (i = 0; i < 4; i++) {
        CarPhysWheel *wheel = &phys->wheels[i];

        wheel->slipAbs = 0;
        wheel->lateralAbs = 0;
        if (g_TyreWearEnabled != 0) {
            wheel->tyreWear = g_TyreWearInit;
            if (g_TyreWearInit < 0) {
                wheel->wearLevel = -0x3f;
            } else {
                wheel->wearLevel = 0;
            }
            wheel->tyreGrip = 0x1000 - g_TyreGripLossInit;
        } else {
            wheel->tyreWear = 0;
            wheel->wearLevel = 0;
            wheel->tyreGrip = 0x1000;
        }
        wheel->wearRate = 0;
        wheel->dirt = 0;
    }
}

void CarPhys_ResetWheels(CarPhys *phys)
{
    int i;

    for (i = 0; i < 4; i++) {
        CarPhysWheel *wheel = &phys->wheels[i];

        wheel->fxSmoke = 0;
        wheel->fxTimer = 0xff;
        wheel->fxSqueal = 0;
        wheel->spinAngle = 0;
        wheel->bumpFade = 0;
        wheel->unk44 = 0;
        wheel->unk46 = 0;
        wheel->bump = 0;
        wheel->suspSpeed = 0;
        wheel->spring = 0;
        wheel->damper = 0;
        wheel->fxGravel = 0;
        wheel->heading = 0;
    }
}

// Puts the car down at (x, z), facing (dirX, dirZ), with its wheels on the ground.
int CarPhys_PlaceOnTrack(CarPhys *phys, int section, int x, int z, int dirX, int dirZ)
{
    RaceMem *race = &g_Race_IsDemo;
    CarPhys *car = phys;
    int ground;
    int pos[3];
    GroundInfo info;
    int rough = 0;
    int i;

    phys->section = section;
    ground = World_GetGroundHeight(race, x, z, 0x64000, &phys->section);
    phys->pos[0] = x;
    phys->pos[1] = z;
    phys->pitch = 0;
    phys->roll = 0;
    phys->pos[2] = ground + CARPHYS_FIELD(phys, short, 0x22);
    phys->yaw = Math_Atan2(-dirX, dirZ);
    pos[0] = x << 4;
    pos[1] = ground << 4;
    pos[2] = z << 4;
    phys->trackPos = World_GetCourseDistance(race, phys->section, pos);
    Matrix_FromAngles(phys->fwdDir, phys->rightDir, (short *)((char *)phys + 0x678), phys->pitch, phys->roll, phys->yaw);
    Body_UpdateWheelPositions(phys);
    for (i = 0; i < 4; i++) {
        CarPhysWheel *wheel = &phys->wheels[i];

        info.pos[0] = (phys->pos[0] + wheel->offX) << 4;
        info.pos[1] = (phys->pos[2] + wheel->offZ) << 4;
        info.pos[2] = (phys->pos[1] + wheel->offY) << 4;
        info.section = phys->section;
        World_GetGroundInfo(&g_Race_IsDemo, &info);
        if (info.height != 0x7fffffff) {
            wheel->groundY = info.height >> 4;
            wheel->groundY16 = info.height;
            wheel->surface = info.surface;
            wheel->surface2 = info.side;
            rough += info.kind & 2;
        } else {
            wheel->surface = 0;
            wheel->surface2 = 0;
            wheel->groundY = ground + wheel->offZ;
            wheel->groundY16 = wheel->groundY << 4;
        }
    }
    if (rough != 0 && Race_AllowsOffCourseFlag() != 0) {
        phys->flags |= 0x10;
    } else {
        phys->flags &= ~0x10;
    }
    for (i = 0; i < 4; i++) {
        CarPhysWheel *wheel = &phys->wheels[i];
        int axle = i >> 1;
        int length = car->unk434[axle] / 2 + car->susp[axle].preload;

        wheel->suspLength = length;
        wheel->gripLimit = length;
        wheel->suspPos = car->susp[axle].rideHeight;
    }
    phys->bufIdx = 1;
    Body_ComputeFootprint(phys);
    Body_SaveFootprint(phys);
    CarPhys_UpdatePitchRoll(phys);
    phys->pitch = phys->unk6f4;
    phys->roll = phys->unk6f6;
    phys->pitch += Math_Atan2(CARPHYS_FIELD(car, short, 0x24) - CARPHYS_FIELD(car, short, 0x26), car->wheelbase);
    Matrix_FromAngles(phys->fwdDir, phys->rightDir, (short *)((char *)phys + 0x678), phys->pitch, phys->roll, phys->yaw);
    Body_UpdateWheelPositions(phys);
    return 0;
}

void CarPhys_ResetOrientationHistory(CarPhys *phys)
{
    short yaw = phys->yaw;
    short pitch = phys->pitch;

    phys->unk752 = 0;
    phys->swayYawRate = 0;
    phys->chaseSway[0] = 0;
    phys->swayPitchRate = 0;
    phys->chaseSway[1] = 0;
    phys->swayYaw = yaw;
    phys->swayPitch = pitch;
}

// Everything that moves goes back to a standstill.
void CarPhys_ResetState(CarPhys *phys)
{
    CarPhys *car = phys;
    unsigned int i;

    for (i = 0; i < 4; i++) {
        CarPhysWheel *wheel = &phys->wheels[i];

        wheel->unk50 = 0;
        wheel->slipFade = 0;
        wheel->lateral = 0;
        wheel->unk58 = 0;
        wheel->unk5c = 0;
        wheel->velFwd = 0;
        wheel->velLat = 0;
        wheel->rollSpeed = 0;
        wheel->unk16 = 0;
        wheel->unk63 = 0;
        wheel->slipGrip = 0x1000;
    }
    for (i = 0; i < 2; i++) {
        phys->diffLocked[i] = 0;
        if ((unsigned int)(car->unk37a[i] - 3) < 2) {
            phys->diffLocked[i] = 1;
        }
        phys->axleSpeed[i] = 0;
    }
    phys->unk63c = 0;
    if ((unsigned int)(car->driveLayout - 4) < 2) {
        phys->unk63c = 1;
    }
    phys->catchUp = 0x1000;
    phys->steer = 0;
    CARPHYS_FIELD(phys, short, 0x70a) = 0;
    CARPHYS_FIELD(phys, short, 0x70c) = 0;
    phys->yawRate = 0;
    CARPHYS_FIELD(phys, short, 0x60e) = 0;
    phys->followSlowdown = 0;
    phys->pushed = 0;
    CARPHYS_FIELD(phys, short, 0x776) = 0;
    for (i = 0; i < 8; i++) {
        phys->proximity[i] = 0;
    }
    phys->vel[0] = 0;
    phys->vel[1] = 0;
    phys->vel[2] = 0;
    phys->unk768 = 0;
    phys->throttle = 0;
    phys->throttleTarget = 0;
    phys->brake = 0;
    {
        unsigned int w;

        for (w = 0; w < 4; w++) {
            phys->wheels[w].brake = 0;
        }
    }
    phys->blowOffState = -1;
    phys->gear = 1;
    phys->unk6fd = 0;
    phys->aiPitLimit = 0;
    phys->launchHold = 0;
    phys->boost = 0;
    phys->engineLoad = 0;
    phys->turboSpool[1] = 0;
    phys->turboSpool[0] = 0;
    phys->boostDisplay = 0;
    phys->engineFriction = 0;
    phys->engineBraking = 0;
    phys->revLimiter = 0;
    phys->hitFeedback = 0;
    phys->unk6fb = 0;
    phys->speedKmh = 0;
    phys->maxSpeedKmh = 0;
    phys->rpm = 0;
    phys->aiBlockMask = 0;
    phys->unk73e = 0;
    phys->wallHit = 0;
    phys->unk738 = 0;
    phys->airborne = 0;
    phys->unk762 = 0;
    phys->unk763 = 0;
    CARPHYS_FIELD(phys, short, 0x760) = 0;
    phys->unk757 = 0x80;
    phys->unk758 = 0x80;
    phys->blowOffVolume = 0;
    CarPhys_ResetWheels(phys);
    phys->speedFwd = 0;
    phys->speedLat = 0;
    phys->clutchIn = 0;
    phys->clutch = 0;
    phys->engineSpeed = 0;
    phys->unk650 = 0;
    phys->unk652 = 0;
    phys->wheelContact = 0;
    phys->wallContact = 0;
    phys->startDelay = 0;
    if (phys->aiMode != 0 || phys->unk642 == 0) {
        phys->startDelay = g_FrameRate / 5;
    }
    CARPHYS_FIELD(phys, int, 0x71c) = 0;
    CARPHYS_FIELD(phys, int, 0x720) = 0;
    CARPHYS_FIELD(phys, int, 0x724) = 0;
    CARPHYS_FIELD(phys, int, 0x728) = 0;
    CARPHYS_FIELD(phys, short, 0x64a) = 0;
    CARPHYS_FIELD(phys, short, 0x700) = 0;
    CARPHYS_FIELD(phys, int, 0x72c) = 0;
    CARPHYS_FIELD(phys, int, 0x730) = 0;
    CARPHYS_FIELD(phys, int, 0x734) = 0;
    for (i = 0; i < 2; i++) {
        (&phys->wallNormalX)[i] = 0;
    }
    phys->wallSide = 0;
    phys->wallSteerKick = 0;
    phys->impact = 0;
    for (i = 0; i < 2; i++) {
        phys->pushVec[i] = 0;
    }
}

// Starts the car moving at mph, in its gear and at matching revs.
void CarPhys_SetRollingStart(CarPhys *phys, int mph)
{
    int speed = mph * 0x472;
    CarPhys *car = phys;
    unsigned int i;
    int rpm;

    phys->yawRate = 0;
    for (i = 0; i < 3; i++) {
        phys->vel[i] = Fixed_Mul(phys->fwdDir[i], speed);
    }
    phys->speedKmh = mph * 1000000 / 16093;
    phys->maxSpeedKmh = phys->speedKmh;
    phys->gear = Gearbox_GearForSpeed(phys, speed);
    phys->engineSpeed = Fixed_Mul(car->gearSpeed[phys->gear], speed);
    if (phys->engineSpeed < (car->engine.unk8e << 12) / 60) {
        phys->engineSpeed = (car->engine.unk8e << 12) / 60;
    } else if (phys->engineSpeed > (car->engine.redline << 12) / 60) {
        phys->engineSpeed = (car->engine.redline << 12) / 60;
    }
    rpm = phys->engineSpeed * 60 / 0x1000;
    phys->rpm = rpm;
    if (car->engine.maxBoost != 0) {
        phys->turboSpool[0] = rpm;
        if (car->engine.boost[1] != 0) {
            phys->turboSpool[1] = rpm;
        }
        phys->boost = EngineCurve_BoostFactorClamped(&car->engine, phys->rpm, phys->rpm);
        phys->engineLoad = 0x1000;
        phys->blowOffState = 0;
        phys->boostDisplay = phys->rpm;
    }
    phys->clutchIn = 1;
    phys->speedFwd = speed;
    phys->speedLat = 0;
    phys->clutch = 0x1000;
    for (i = 0; i < 4; i++) {
        CarPhysWheel *wheel = &phys->wheels[i];

        wheel->rollSpeed = speed;
        wheel->velFwd = speed;
    }
    for (i = 0; i < 2; i++) {
        phys->axleSpeed[i] = Fixed_Mul(car->invTyreRadius[i], speed);
    }
}

// Builds a car and puts it on the grid.
int CarPhys_Setup(CarPhys *phys, CarParams *params, int section, int x, int z, int dirX, int dirZ, int control,
                  unsigned char unk642, int ghost, int arg, int carIndex, int distance)
{
    CarPhys *car = phys;
    int result;
    int offLine = 0;
    int i;

    phys->carIndex = carIndex;
    phys->ghost = ghost;
    if (D_801C98A2 == 1 && carIndex == 0) {
        control = 1;
    }
    CarPhys_SetControl(phys, control);
    result = CarPhys_Init(car, params, phys->control, 0, arg);
    CarPhys_SetUnk642(phys, unk642);
    if (Race_IsTimeTrial() != 0 && phys->ghost == 2 && GhostCar_Init(phys) != 0) {
        return result;
    }
    phys->flags = 0;
    if (distance != 0) {
        offLine = (unsigned int)AiLine_GetPointAtDistance(&section, &x, &z, &dirX, &dirZ, distance) >> 31;
    }
    CarPhys_PlaceOnTrack(phys, section, x, z, dirX, dirZ);
    CarPhys_ResetOrientationHistory(phys);
    CarPhys_ResetState(phys);
    CarPhys_ServiceTyres(phys);
    Body_ApplyTimeScale(phys);
    if (offLine != 0) {
        phys->checkpoint = 0;
        phys->lap = 0;
    } else {
        int *checkpoints = (int *)((char *)&g_Race_IsDemo + 0xb534 + 0x24);
        int count = checkpoints[0];

        phys->checkpoint = count;
        for (i = 0; i < count; i++) {
            if (phys->trackPos < checkpoints[i + 1]) {
                phys->checkpoint = i;
                break;
            }
        }
        if (phys->checkpoint == count) {
            phys->checkpoint = 0;
            phys->lap = 0;
        } else {
            phys->lap = 1;
        }
    }
    phys->raceTime = 0;
    phys->aiPassState = 0;
    phys->unk718 = 0;
    phys->pitTimer = 0;
    phys->aiStarted = 0;
    phys->dirt = 0;
    phys->aiMessage = 0;
    phys->aiMessageHold = 0;
    phys->aiTargetSpeedPrev = car->topSpeed;
    phys->aiTargetSpeedNext = car->topSpeed;
    phys->pitBox = AiLine_GetPitBox(carIndex);
    phys->racePos = carIndex + 1;
    Ai_SetLine(phys, AiLine_PickForSlot(6));
    if (((unsigned char *)RaceSetup_Get())[0xd] == 0 && g_RaceEventParams[0] != 0) {
        CarPhys_SetRollingStart(phys, g_RaceEventParams[0]);
    }
    LicenseTest_ResetResult(phys);
    return result;
}

// Clears proximity[8], followSlowdown and pushed.
void CarPhys_ClearProximity(CarPhys *phys)
{
    unsigned char *puVar1;
    int iVar2;

    iVar2 = 7;
    puVar1 = (unsigned char *)((int)&phys->lenRear + 3);
    phys->pushed = 0;
    phys->followSlowdown = 0;
    do {
        puVar1[0x76c] = 0;
        iVar2 = iVar2 + -1;
        puVar1 = puVar1 + -1;
    } while (-1 < iVar2);
}

// Marks the zones around the car that (x, z) falls in: 2 contact, 1 near.
void CarPhys_SetProximityFlags(CarPhys *phys, int x, int z, int front, int back, int width)
{
    if (front < x) {
        if (z < -width / 2) {
            phys->proximity[1] = 2;
        } else if (width / 2 < z) {
            phys->proximity[2] = 2;
        } else {
            phys->proximity[0] = 2;
            if (z > 0) {
                phys->proximity[2] = 1;
            } else {
                phys->proximity[1] = 1;
            }
        }
    } else if (x < -back) {
        if (z < -width / 2) {
            phys->proximity[5] = 2;
        } else if (width / 2 < z) {
            phys->proximity[6] = 2;
        } else {
            phys->proximity[7] = 2;
            if (z > 0) {
                phys->proximity[6] = 1;
            } else {
                phys->proximity[5] = 1;
            }
        }
    } else if (z < 0) {
        phys->proximity[3] = 2;
        if (x > 0) {
            phys->proximity[1] = 1;
        } else {
            phys->proximity[5] = 1;
        }
    } else {
        phys->proximity[4] = 2;
        if (x > 0) {
            phys->proximity[2] = 1;
        } else {
            phys->proximity[6] = 1;
        }
    }
}

// Two cars near each other: the zones around them, what the CPU car ahead makes it
// slow down for, and the slipstream of the car behind.
int CarPhys_CarPairInteraction(CarPhys *a, CarPhys *b)
{
    CarPhys *carA = a;
    CarPhys *carB = b;
    int dist;
    int rel[3];
    int relVel[3];
    int widthA, widthB;
    int i;
    int forwardA = 0, sideA = 0, forwardB = 0, sideB = 0;
    int frontA, rearA, frontB, rearB;
    int halfA, halfB;

    if (a->ghost != 0 || b->ghost != 0) {
        return 0;
    }
    if (a->aiLine == 4 || b->aiLine == 4) {
        return 0;
    }
    if ((a->flags & 0x10) != (b->flags & 0x10)) {
        return 0;
    }
    dist = Math_ApproxDist3(a->pos[0] - b->pos[0], a->pos[1] - b->pos[1], a->pos[2] - b->pos[2]);
    if (dist > 0x64000) {
        return 0;
    }
    widthA = CARPHYS_FIELD(a, short, 0xa);
    widthB = CARPHYS_FIELD(b, short, 0xa);
    for (i = 0; i < 3; i++) {
        rel[i] = b->pos[i] - a->pos[i];
    }
    if (rel[2] < -(widthB + 0x5000) || rel[2] > widthA + 0x5000) {
        return 0;
    }
    for (i = 0; i < 3; i++) {
        forwardA += Fixed_Mul32(a->fwdDir[i], rel[i]);
        sideA += Fixed_Mul32(a->rightDir[i], rel[i]);
        forwardB -= Fixed_Mul32(b->fwdDir[i], rel[i]);
        sideB -= Fixed_Mul32(b->rightDir[i], rel[i]);
    }
    frontA = carA->lenFront;
    rearA = carA->lenRear;
    halfA = carA->width;
    frontB = carB->lenFront;
    rearB = carB->lenRear;
    halfB = carB->width;
    if (dist <= 0x14000) {
        CarPhys_SetProximityFlags(a, forwardA, sideA, frontA + rearB, rearA + frontB, halfA + halfB);
        CarPhys_SetProximityFlags(b, forwardB, sideB, rearA + frontB, frontA + rearB, halfA + halfB);
        for (i = 0; i < 3; i++) {
            relVel[i] = b->vel[i] - a->vel[i];
        }
        if (a->control != 0 || a->aiMode != 0) {
            int gap = forwardA - rearB - frontA;

            if (gap > 0 && sideA > -(halfA + halfB) && sideA < halfA + halfB) {
                int closing = 0;

                for (i = 0; i < 3; i++) {
                    closing += Fixed_Mul(a->fwdDir[i], relVel[i]);
                }
                if (closing < -0xa008) {
                    a->followSlowdown = 0x1000;
                } else if (closing < -0x472 && (gap << 12) / -closing < 0x3000) {
                    a->followSlowdown = 0x800;
                }
                if (a->followSlowdown == 0 && gap < 0x2000) {
                    a->followSlowdown = 0x400;
                }
            }
        }
        if (b->control != 0 || b->aiMode != 0) {
            int gap = forwardB - rearA - frontB;

            if (gap > 0 && sideB > -(halfA + halfB) && sideB < halfA + halfB) {
                int closing = 0;

                for (i = 0; i < 3; i++) {
                    closing -= Fixed_Mul(b->fwdDir[i], relVel[i]);
                }
                if (closing < -0xa008) {
                    b->followSlowdown = 0x1000;
                } else if (closing < -0x472 && -((gap << 12) / closing) < 0x3000) {
                    b->followSlowdown = 0x800;
                }
                if (b->followSlowdown == 0 && gap < 0x2000) {
                    b->followSlowdown = 0x400;
                }
            }
        }
    }
    if (a->pushed != 0x1000 && b->speedFwd > 0x1bc88 && forwardB < 0 && forwardB > -b->speedFwd && sideB > -halfB &&
        sideB < halfB) {
        a->pushed += ((long long)(b->speedFwd + forwardB) << 12) / b->speedFwd;
        if (a->pushed > 0x1000) {
            a->pushed = 0x1000;
        }
    }
    if (b->pushed != 0x1000 && a->speedFwd > 0x1bc88 && forwardA < 0 && forwardA > -a->speedFwd && sideA > -halfA &&
        sideA < halfA) {
        b->pushed += ((long long)(a->speedFwd + forwardA) << 12) / a->speedFwd;
        if (b->pushed > 0x1000) {
            b->pushed = 0x1000;
        }
    }
    return 0;
}

// Steering kick from how the touching corner slid along the wall.
void CarPhys_WallDeflect(CarPhys *phys)
{
    int normalZ = phys->wallNormalZ;
    int normalX = phys->wallNormalX;
    int other = 1;
    int corner;
    int buffer = phys->bufIdx;
    int dx;
    int dz;

    corner = phys->wallSide;
    other -= buffer;
    dx = phys->footprint[buffer][corner][0] - phys->footprint[other][corner][0];
    dz = phys->footprint[buffer][corner][1] - phys->footprint[other][corner][1];

    if (Fixed_Dot2Shift(-normalZ, dx, normalX, dz, 0) <= 0) {
        int along = Fixed_Dot2Shift(normalX, dx, normalZ, dz, 0);
        int threshold = 0x1000 / g_FrameRate;
        int kick = along * g_FrameRate / 0x800;

        if (kick > 0x40) {
            kick = 0x40;
        } else if (kick < -0x40) {
            kick = -0x40;
        }
        if (along > threshold || along < -threshold) {
            phys->wallSteerKick = kick;
        } else {
            phys->wallSteerKick = 0;
        }
    }
}

// Moves the car by move (x, z, y, yaw); if the footprint hits a wall, takes back
// what is left of the move and returns non-zero. *rest gets how much of it was left.
int CarPhys_MoveAndCollide(CarPhys *phys, int *move, int *rest, int mode)
{
    int i;
    int hit;
    int fraction;
    unsigned int corner = 0;

    for (i = 0; i < 3; i++) {
        phys->pos[i] += move[i];
    }
    phys->yaw += move[3];
    if (phys->yaw >= 0x1000) {
        phys->yaw -= 0x1000;
    }
    if (phys->yaw < -0xfff) {
        phys->yaw += 0x1000;
    }
    if (mode != 1) {
        phys->bufIdx = 1 - phys->bufIdx;
    }
    Body_ComputeFootprint(phys);
    *rest = 0;
    if (D_800A9520 != 0) {
        return 0;
    }
    hit = Body_SweepFootprint(phys, &fraction, &corner);
    if (hit == 0) {
        return 0;
    }
    phys->wallSide = corner;
    switch (mode) {
    case 0:
        phys->wallSteerKick = 0;
    case 1:
    case 2:
        CarPhys_WallDeflect(phys);
        break;
    }
    fraction = 0x1000 - fraction;
    *rest = fraction;
    for (i = 0; i < 3; i++) {
        phys->pos[i] -= Fixed_Mul32(move[i], fraction);
    }
    phys->yaw -= Fixed_Mul32(move[3], fraction);
    if (phys->yaw >= 0x1000) {
        phys->yaw -= 0x1000;
    }
    if (phys->yaw < -0xfff) {
        phys->yaw += 0x1000;
    }
    phys->pos[0] -= phys->wallNormalZ >> 4;
    phys->pos[1] += phys->wallNormalX >> 4;
    if (mode != 0) {
        Body_ComputeFootprint(phys);
    }
    return hit;
}

// Takes the velocity into the wall away, scrubs speed by the impact and finishes the move.
void CarPhys_WallBounce(CarPhys *phys, int *move, int scale, int mode)
{
    int velX = phys->vel[0];
    int velZ = phys->vel[1];
    int normalX = phys->wallNormalX;
    int normalZ = phys->wallNormalZ;
    int along = Fixed_Dot2Shift(velX, normalX, velZ, normalZ, 0);
    int speed;

    phys->vel[0] = Fixed_Mul(normalX, along);
    phys->vel[1] = Fixed_Mul(normalZ, along);
    speed = Math_ApproxDist2(velX, velZ);
    if (phys->vel[2] > 0x1000 && speed > 0) {
        phys->vel[2] = Fixed_Mul(phys->vel[2], Fixed_Div(along, speed, 0));
    }
    if (mode != 1) {
        int lost = Math_ApproxDist2(velX - phys->vel[0], velZ - phys->vel[1]);
        RaceWorld *world = &g_Race_IsDemo.world;
        int rate;
        int full;
        int limit;
        int keep;
        int impact;
        int rest;

        phys->wallHit = 0;
        rate = (Course_Get(world->course)->flags & 4) != 0 ? 0x1b : 0x29;
        if (speed >= 0x8e5) {
            phys->wallHit = lost * rate >> 12;
        }
        full = g_PhysConstBytes[2];
        keep = g_PhysConstBytes[3];
        FRAME_DT = phys->timeStep;
        if ((Course_Get(world->course)->flags & 4) != 0) {
            full = g_PhysConstBytes[4];
            keep = g_PhysConstBytes[5];
        }
        limit = (unsigned char)full * 0x472;
        impact = 0x1000;
        if (limit >= lost) {
            impact = Fixed_Div(lost, limit, 0);
        }
        if (phys->impact < impact) {
            int i;
            int factor;

            phys->impact = impact;
            factor = 0x1000 - impact * (100 - keep) / 100;
            for (i = 0; i < 3; i++) {
                phys->vel[i] = Fixed_Mul(factor, phys->vel[i]);
            }
        }
        CarPhys_ScaleFrameDelta(phys, move, scale);
        CarPhys_MoveAndCollide(phys, move, &rest, 1);
    }
}

// Moves every car by its frame delta, against the walls.
void Cars_MoveAll(RaceVehicle *cars, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        CarPhys *phys = &cars[i].phys;
        int offset = i * 0x10 + 0xb4;
        int *delta = (int *)(0x1f800000 + offset);
        int rest;

        phys->aiBlockMask = CarPhys_MoveAndCollide(phys, delta, &rest, 0);
        if (phys->aiBlockMask == 0) {
            phys->impact -= Fixed_Mul32_16(g_FrameDt, 0x555);
            if (phys->impact < 0) {
                phys->impact = 0;
            }
            phys->wallHit = 0;
        } else {
            phys->wallContact |= 1;
            CarPhys_WallBounce(phys, delta, rest, 0);
            if (Body_GetCollisionRule(phys) == 2) {
                int dirt = phys->wallHit;

                dirt -= 0x155;
                if (dirt >= 0) {
                    dirt = (dirt >> 3) + phys->wheels[phys->wallSide].dirt;
                    if (dirt >= 0x100) {
                        dirt = 0xff;
                    }
                    phys->wheels[phys->wallSide].dirt = dirt;
                }
            }
        }
    }
}

// The whole move of a frame: walls, then car against car, then the hit feedback.
void Cars_MoveAndCollide(RaceVehicle *cars, int count)
{
    int i;
    int full;
    int one;

    for (i = 0; i < count; i++) {
        CarPhys *phys = &cars[i].phys;

        phys->wallContact = 0;
        CarPhys_ClearProximity(phys);
    }
    Cars_MoveAll(cars, count);
    CarColl_ComputeCorners(cars, count);
    CarColl_FindImpacts(cars, count);
    CarColl_ResolveImpacts(cars, count);
    CarColl_RecomputeCorners(cars, count);
    CarColl_SeparateOverlaps(cars, count);
    for (i = 0; i < count; i++) {
        int j;

        for (j = i + 1; j < count; j++) {
            CarPhys_CarPairInteraction(&cars[i].phys, &cars[j].phys);
        }
    }
    i = 0;
    full = 0x1000;
    one = 1;
    for (; i < count; i++) {
        CarPhys *phys = &cars[i].phys;

        if (phys->wallHit != 0) {
            int rule;

            if (phys->wallHit > 0x1000) {
                phys->wallHit = full;
            }
            rule = Body_GetCollisionRule(phys);
            if (rule == one && phys->hitFeedback == 0 && phys->wallHit >= 0x2ab) {
                Race_ShowMessage(phys->carIndex, 1);
                phys->hitFeedback = rule;
            }
            if (phys->unk73e < g_FrameRate * 3 / 5) {
                phys->unk73e = g_FrameRate;
            }
        }
    }
}

// Both differential types, from the values to their indices in g_DiffTypeTable.
void FUN_OVL0__80034688(CarParams *params)
{
    unsigned int side;

    for (side = 0; side < 2; side++) {
        int result;
        int value = ((signed char *)params->diffType)[side];
        int i;

        result = value;
        for (i = 0; i < 8; i++) {
            if (g_DiffTypeTable[i] == value) {
                result = i;
                break;
            }
        }
        params->diffType[side] = result;
    }
}

// And back from the indices to the values.
void FUN_OVL0__800346ec(CarParams *params)
{
    unsigned int i;

    for (i = 0; i < 2; i++) {
        params->diffType[i] = params->diffType[i] < 8 ? (unsigned char)g_DiffTypeTable[params->diffType[i]] : params->diffType[i];
    }
}

// A full-custom gearbox's ratios, from the chosen top speed.
int Gearbox_ApplyTopSpeedSetting(CarParams *params, int arg)
{
    int topSpeed;
    int i;

    if (params->unk1aa == 0 || params->gearTopSpeedDiv10 == 0) {
        return 0;
    }
    topSpeed = 0xff;
    if (params->gearTopSpeedDiv10 != 0xff) {
        topSpeed = params->gearTopSpeedDiv10 * 10;
    }
    for (i = 7; i >= 0; i--) {
        params->customGears[i] = -1;
    }
    Gearbox_ComputeRatios(params, topSpeed, params->customGears);
    return 1;
}

// Gears, tyres, differentials and shift points from the car's parts.
void Drivetrain_Init(CarParams *params, CarDrive *drive, EngineCurve *curve, int arg)
{
    unsigned int side;
    int i;
    int inertia[2];
    int mass[2];
    int peak;

    CarParams_GetDriveLayout(params, (unsigned char *)drive, &drive->driven, (short *)drive->unk4, (short *)drive->unk6);
    for (side = 0; side < 2; side++) {
        int value = ((signed char *)params->diffType)[side];
        int result = value;

        for (i = 0; i < 8; i++) {
            if (g_DiffTypeTable[i] == value) {
                result = i;
                break;
            }
        }
        drive->diff[side] = result;
    }
    switch (drive->layout) {
    case 0:
        if (drive->diff[0] != 0) {
            drive->diff[0] = 0;
        }
        if (drive->diff[1] == 0) {
            drive->diff[1] = 2;
        }
        break;
    case 1:
        if (drive->diff[0] == 0) {
            drive->diff[0] = 2;
        }
        if (drive->diff[1] != 0) {
            drive->diff[1] = 0;
        }
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        if (drive->diff[0] == 0) {
            drive->diff[0] = 2;
        }
        if (drive->diff[1] == 0) {
            drive->diff[1] = 2;
        }
        break;
    }
    for (side = 0; side < 2; side++) {
        switch (drive->diff[side]) {
        case 0:
        case 1:
        case 2:
            drive->diffA[side] = 0;
            drive->diffB[side] = 0;
            drive->diffC[side] = 0;
            break;
        case 3:
        case 4:
            drive->diffA[side] = params->unk186[side] * 160564 / 0x1000;
            goto rest;
        case 5:
            drive->diffA[side] = Fixed_Mul(params->unk186[side] << 12, params->unk186[side] << 12);
            drive->diffB[side] = params->unk186[side + 2] * 160564 / 0xa000;
            drive->diffC[side] = 0;
            break;
        case 6:
            drive->diffA[side] = params->unk186[side] * 160564 / 0x1000;
            goto rest;
        case 7:
            drive->diffA[side] = (params->unk186[side] << 12) / 100;
        rest:
            drive->diffB[side] = (params->unk186[side + 2] << 12) / 100;
            drive->diffC[side] = (params->unk186[side + 4] << 12) / 100;
            break;
        }
    }
    Gearbox_ApplyTopSpeedSetting(params, arg);
    drive->gearCount = params->gearCount;
    for (i = 0; i <= drive->gearCount; i++) {
    }
    for (i = 0; i <= drive->gearCount; i++) {
        drive->gearRatio[i] = (params->customGears[i] * ((long long)params->gear28 << 12)) / 1000000;
    }
    for (i = drive->gearCount + 1; i < 8; i++) {
    }
    CarParams_GetTyreRadius(params, 0, &drive->tyreRadius[0], &drive->tyreWidth[0], &drive->tyreSide[0]);
    CarParams_GetTyreRadius(params, 1, &drive->tyreRadius[1], &drive->tyreWidth[1], &drive->tyreSide[1]);
    if (drive->tyreRadius[0] != 0) {
        drive->invRadius[0] = 0x1000000 / drive->tyreRadius[0];
    }
    if (drive->tyreRadius[1] != 0) {
        drive->invRadius[1] = 0x1000000 / drive->tyreRadius[1];
    }
    inertia[0] = params->unk5c[8];
    inertia[1] = params->unk5c[9];
    for (i = 0; i < 2; i++) {
        if (inertia[i] != 0) {
            drive->unk88[i] = Fixed_Div(drive->tyreRadius[i], inertia[i] << 4, 0);
            drive->unka0[i] = Fixed_Mul(inertia[i], Fixed_Mul(drive->invRadius[i], drive->invRadius[i]));
        }
    }
    if (params->unk2b != 0) {
        drive->unk8 = (params->unk2b << 6) / 100;
        drive->unk9c = 0xfeb0000 / (params->unk2b * 4);
    }
    for (side = 0; side < 2; side++) {
        mass[side] = params->unk17e[side + 4] << 4;
        mass[side] = mass[side] * params->gear28 / 1000;
        mass[side] += inertia[side] << 5;
        drive->unk90[side] = mass[side];
    }
    drive->unk98 = Fixed_Div(0x1000, mass[0] + mass[1], 0);
    drive->launchLimit = params->unk5c[1] * 10;
    drive->launchRpm = params->minRedlineDiv100 * 100;
    peak = 0;
    for (i = 0; i < curve->count; i++) {
        int torque = EngineCurve_TorqueAt(curve, curve->rpmFx[i]);

        if (peak < torque) {
            peak = torque;
        }
    }
    drive->unk86 = peak * 15 / 10;
    drive->unk84 = params->unk8b[0] * 160564 / 0x1000;
    for (i = 0; i <= drive->gearCount; i++) {
        drive->gearSpeed[i] = Fixed_Div(drive->gearRatio[i], drive->tyreRadius[drive->driven] * 0x6488 >> 12, 0);
    }
    if (drive->gearCount == 2) {
        drive->gearRatio[2] = (params->customGears[2] << 12) / params->customGears[1];
    }
    if (drive->gearCount == 2) {
        drive->launchLimit = curve->unk8e;
    } else if (drive->launchLimit - 500 < curve->unk8e) {
        drive->launchLimit = curve->unk8e + 500;
    }
    if (drive->gearCount == 2) {
        int best = 0;
        int index = curve->count - 1;

        for (i = 0; i < curve->count; i++) {
            if (best < curve->torqueFx[i]) {
                best = curve->torqueFx[i];
                index = i;
            }
        }
        drive->shift[0] = params->curveRpmDiv100[index] * 100;
    } else {
        for (i = 1; i < drive->gearCount; i++) {
            int rpm = Fixed_Mul((drive->launchRpm << 12) / 60, Fixed_Div(drive->gearRatio[i + 1], drive->gearRatio[i], 0));

            rpm = rpm * 60 / 0x1000;
            if (drive->launchLimit + 1000 < rpm) {
                drive->shift[i - 1] = rpm - 500;
            } else {
                drive->shift[i - 1] = drive->launchLimit + 500;
            }
        }
    }
}

// The moving parts of a car, for a replay.
void CarPhys_SaveState(int *state, CarPhys *phys)
{
    CarSnapshot *snap = (CarSnapshot *)state;
    int i;

    for (i = 0; i < 4; i++) {
        snap->wheels[i] = *(CarSnapshotWheel *)&phys->wheels[i];
    }
    snap->body = *(CarSnapshotBody *)&phys->section;
}

// A replay's saved car, and everything that follows from it.
void CarPhys_RestoreState(CarPhys *phys, void *state)
{
    CarSnapshot *snap = (CarSnapshot *)state;
    int i;

    memset(&phys->carIndex, 0, 0x33c);
    for (i = 0; i < 4; i++) {
        *(CarSnapshotWheel *)&phys->wheels[i] = snap->wheels[i];
    }
    *(CarSnapshotBody *)&phys->section = snap->body;
    phys->catchUp = 0x1000;
    phys->airborne = 0;
    if (phys->wheels[0].suspLength + phys->wheels[1].suspLength + phys->wheels[2].suspLength +
            phys->wheels[3].suspLength == 0) {
        phys->airborne = 1;
    }
    phys->pitBox = AiLine_GetPitBox(0);
    phys->flags = 0;
    Matrix_FromAngles(phys->fwdDir, phys->rightDir, (short *)((char *)phys + 0x678), phys->pitch, phys->roll, phys->yaw);
    CarPhys_UpdatePitchRoll(phys);
    Body_UpdateWheelPositions(phys);
    CarPhys_UpdateLocalVelocity(phys);
    Engine_UpdateRpmDisplay(phys);
    CarPhys_ResetOrientationHistory(phys);
    phys->bufIdx = 0;
    Body_ComputeFootprint(phys);
    Body_SaveFootprint(phys);
    for (i = 7; i >= 0; i--) {
        phys->proximity[i] = 0;
    }
    phys->followSlowdown = 0;
}

// Cuts the torque at the redline, until 500 rpm below it.
int Engine_ApplyRevLimiter(CarPhys *phys, int throttle)
{
    int torque = 0;
    EngineCurve *engine = &phys->engine;

    if (phys->revLimiter == 0) {
        if (phys->rpm >= engine->redline) {
            phys->revLimiter = 1;
        }
    } else if (phys->rpm < engine->redline - 500) {
        phys->revLimiter = 0;
    }
    if (phys->revLimiter == 0) {
        torque = Fixed_MulFloor(throttle, phys->engineFriction + phys->engineBraking);
    }
    return torque - phys->engineBraking;
}

// Throttle through the turbos: spools them up, tracks boost and the blow-off valve.
int Engine_UpdateTurbo(CarPhys *phys, int throttle)
{
    EngineCurve *engine = &phys->engine;
    int input = throttle;
    int torque;

    if (engine->maxBoost == 0) {
        phys->engineLoad = input;
        phys->blowOffVolume = 0;
    } else {
        int full = EngineCurve_BoostFactorClamped(engine, phys->rpm, phys->rpm);
        int load = ((long long)(phys->engineLoad * phys->rpm >> 12) << 12) / full;
        int i;
        int fall;

        for (i = 0; i < 2; i++) {
            if (engine->boost[i] != 0) {
                phys->turboSpool[i] += Fixed_MulFloor16(Fixed_MulFloor(load - phys->turboSpool[i], engine->boostA[i]), FRAME_DT);
                if (phys->turboSpool[i] < 0) {
                    phys->turboSpool[i] = 0;
                }
            }
        }
        load = input * EngineCurve_BoostFactorClamped(engine, phys->turboSpool[0], phys->turboSpool[1]) >> 12;
        fall = 0x800 - phys->rpm / 4;
        if (fall < 0x100) {
            fall = 0x100;
        }
        load += (0x1000 - input) * fall >> 12;
        phys->boost = (phys->boost + load) / 2;
        if (phys->blowOffState == 0) {
            if (throttle < 0xe00) {
                phys->blowOffState = (phys->engineLoad - 0x1800) / 2;
                if (phys->blowOffState > 0x1000) {
                    phys->blowOffState = 0x1000;
                } else if (phys->blowOffState <= 0) {
                    phys->blowOffState = 1;
                }
            }
        } else if (phys->blowOffState == -1 && throttle > 0xf00) {
            phys->blowOffState = 0;
        }
        phys->engineLoad = load;
        if (load < full) {
            input = (load << 12) / full;
        } else {
            input = 0x1000;
        }
        phys->boostDisplay = phys->turboSpool[0];
        if (phys->boostDisplay < phys->turboSpool[1]) {
            phys->boostDisplay = phys->turboSpool[1];
        }
    }
    torque = Engine_ApplyRevLimiter(phys, input);
    if (engine->maxBoost != 0) {
        int over = phys->engineLoad - 0xc00;

        if (over <= 0 || torque <= 0) {
            phys->blowOffVolume = 0;
        } else {
            int level = over;

            if (torque < 0x930) {
                level = over * (torque * 0x1bdd >> 12) >> 12;
            }
            level = level * level >> 17;
            if (level >= 0x100) {
                level = 0xff;
            }
            phys->blowOffVolume = level;
        }
    }
    return torque;
}
}
