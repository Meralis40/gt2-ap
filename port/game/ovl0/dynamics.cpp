// OVL0 8003d848-8003ef18 (dynamics): Car dynamics and the world step.
// The functions that don't match yet are the original assembly.
#include "gptr.h"
#include "common.h"

extern "C" {
// A piecewise linear curve: count points, x ascending.
struct Curve16 {
    unsigned short count;
    game_pointer<short> x;
    game_pointer<short> y;
};

// Per-car values in the scratchpad (0x1f800090 + carIndex * 0x90).
struct CarPhysScratch {
    short absSlip;   // ABS: wheel slip of the car
    short handbrake; // handbrake target value
    char unk4[0x8c];
};

struct PhysScratch {
    int frameDt; // frame step of the car being updated
    char unk4[0x8c];
    CarPhysScratch cars[6];
};

// CarPhys per-axle values missing from ovl0.h, accessed as array members.
struct CarPhysAxleView {
    char unk0[0x424];
    int axleSlip[2];   // 0x424
    short axleGrip[2]; // 0x42c
};

// A wheel's steering sine and cosine in the scratchpad, from 0x1f80001c, four per
// 0x90 bytes of car.
struct PhysWheelTrig {
    short sin;
    short cos;
    char unk4[0x18];
};

// A driving class's parameters (PhysParams_Init).
struct AiDriveParams {
    char unk0[0x18];
    short unk18;
    short unk1a;
    short unk1c;
    short unk1e;
    int unk20;
    char unk24[4];
};

extern RaceMem (&g_Race_IsDemo);
extern RaceWorld (&g_RaceWorld);
extern AiDriveParams (&g_AiDriveParams)[];
extern short (&g_SinTable)[];
extern unsigned short (&g_CosTable)[];
int Ghost_ShouldHold(void);
void Body_ApplyTimeScale(CarPhys *car);
void Tyres_ComputeGripLimit(RaceVehicle *cars, int count);
void Tyres_ComputeSlipAngle(RaceVehicle *cars, int count);
void Tyres_ComputeSlipRatios(RaceVehicle *cars, int count);
void CarInput_Get(void *input, RaceVehicle *car);
void Car_ApplyControls(CarPhys *car, short *input, int *out, int index);
void CarAi_Drive(CarPhys *car, short *input, int index);
int Body_GetCollisionRule(CarPhys *car);
void CarPhys_UpdateDrivetrain(RaceVehicle *cars, int count);
void Cars_ComputeFrameDelta(RaceVehicle *cars, int count);
void Cars_UpdateDynamics(RaceVehicle *cars, int count);
extern GameState (&g_GameState);
extern unsigned char (&g_RaceEventParams)[];
extern signed char (&g_RaceOrder)[];
extern int (&g_PhysConst_SlipDragMin);
extern unsigned char (&g_PhysConst_AbsPct);
extern int (&g_BodySway_YawStiffness)[];
extern int (&g_BodySway_YawDamping)[];

#define g_PhysScratch ((PhysScratch *)rt::guest(0x1f800000))
#define g_FrameDt (*(int *)rt::guest(0x1f800000))
#define AXLES(car) ((CarPhysAxleView *)(car))
// CarPhys fields that ovl0.h doesn't name yet (in its unkXXX runs).
#define CARPHYS_FIELD(car, type, off) (*(type *)((char *)(car) + (off)))

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// Piecewise linear curve lookup, clamped to the first and last points.
// Registers of xs and last are swapped.
int Curve16_Eval(Curve16 *curve, short x)
{
    short *xs = curve->x;
    short *ys = curve->y;
    unsigned short last = curve->count - 1;
    short xLast = xs[last];
    unsigned short i;
    short *px;
    short *py;
    int x0;
    int y0;

    if (x <= xs[0]) {
        return ys[0];
    }
    if (x >= xLast) {
        return ys[last];
    }
    for (i = 1; i < last; i++) {
        if (x < xs[i]) {
            break;
        }
    }
    px = &xs[i];
    py = &ys[i];
    y0 = py[-1];
    x0 = px[-1];
    return y0 + (x - x0) * (py[0] - y0) / (px[0] - x0);
}

// Curve16_Eval returning two outputs: y from curve and y2 from curve2's y table at the same x.
// The registers of the last two sums differ.
int Curve16_Eval2(Curve16 *curve, Curve16 *curve2, short x, short *outY, short *outY2)
{
    short *xs = curve->x;
    short *ys = curve->y;
    short *ys2 = curve2->y;
    unsigned short last = curve->count - 1;
    unsigned short i;
    short *px;
    short *py;
    short *py2;
    int dx;
    int w;

    if (x <= xs[0]) {
        *outY = ys[0];
        *outY2 = ys2[0];
        return 1;
    }
    if (x >= xs[last]) {
        *outY = ys[last];
        *outY2 = ys2[last];
        return 1;
    }
    for (i = 1; i < last; i++) {
        if (x < xs[i]) {
            break;
        }
    }
    w = xs[i] - xs[i - 1];
    dx = x - xs[i - 1];
    *outY = ys[i - 1] + (short)((dx * (ys[i] - ys[i - 1])) / w);
    *outY2 = ys2[i - 1] + (short)((dx * (ys2[i] - ys2[i - 1])) / w);
    return 1;
}

// Aerodynamics: drag from the forward speed squared, lowered by the slipstream amount (which
// builds up at the pushed rate while set and decays otherwise), and the front/rear downforce.
void CarPhys_UpdateAero(CarPhys *car)
{
    CarPhys *p = car; // the original works with two copies of car
    int speed;
    int drag;
    int force;
    int i;

    speed = car->speedFwd;
    if (speed < 0) {
        speed = -speed;
    }
    drag = (speed >> 5) * (speed >> 5) >> 12;
    if (car->pushed != 0) {
        CARPHYS_FIELD(car, short, 0x776) += car->pushed * g_FrameDt >> 16;
        if (CARPHYS_FIELD(car, short, 0x776) > 0x1000) {
            CARPHYS_FIELD(car, short, 0x776) = 0x1000;
        }
    } else {
        CARPHYS_FIELD(car, short, 0x776) -= g_FrameDt / 64;
        if (CARPHYS_FIELD(car, short, 0x776) < 0) {
            CARPHYS_FIELD(car, short, 0x776) = 0;
        }
    }
    drag = (g_PhysConst_SlipDragMin + ((0x1000 - g_PhysConst_SlipDragMin) * (0x1000 - CARPHYS_FIELD(p, short, 0x776)) >> 12)) * drag >> 12;
    force = CARPHYS_FIELD(car, int, 0x43c) * drag >> 12;
    if (p->speedFwd >= 0) {
        force = -force;
    }
    CARPHYS_FIELD(p, short, 0x71a) = force;
    for (i = 0; i < 2; i++) {
        int f = (&CARPHYS_FIELD(car, int, 0x440))[i];
        (&CARPHYS_FIELD(p, int, 0x71c))[i] = f * drag >> 12;
    }
}

// Brake force per wheel: front/rear balance lowered by ABS, the handbrake on one wheel, then
// each wheel's brake limited by its grip so that it does not lock.
// Registers differ in the handbrake part and the 0x1000 isn't hoisted out of the wheel loop.
void CarPhys_ApplyBrakes(CarPhys *car, int handbrake, int handbrakeTarget)
{
    CarPhys *c = car; // the original works with two copies of car
    int pct;
    int bal[4];
    int front;
    int rear;
    int k;
    int i;
    unsigned int w;

    bal[0] = bal[1] = front = CARPHYS_FIELD(c, short, 0x1fe);
    pct = g_PhysConst_AbsPct;
    bal[2] = bal[3] = rear = CARPHYS_FIELD(c, short, 0x2d6);
    if (pct != 0 && g_PhysScratch->cars[c->carIndex].absSlip != 0) {
        k = 0x1000 - g_PhysScratch->cars[c->carIndex].absSlip * pct / 100;
        if (k < 0) {
            k = 0;
        }
        if (c->yawRate > 0) {
            bal[1] = k * front >> 12;
            bal[3] = k * rear >> 12;
        } else {
            bal[0] = k * front >> 12;
            bal[2] = k * rear >> 12;
        }
        for (i = 0; i < 4; i++) {
            if (bal[i] > 0x1000) {
                bal[i] = 0x1000;
            }
        }
    }
    if (handbrake != 0) {
        int d = g_PhysScratch->cars[car->carIndex].handbrake - handbrakeTarget;

        if (d > 0) {
            CarPhysWheel *wheel = (car->yawRate > 0) ? &car->wheels[1] : &car->wheels[0];

            d = wheel->brake + (handbrake * d >> 9);
            if (d > 0x1000) {
                d = 0x1000;
            }
            wheel->brake = d;
        }
    }
    for (w = 0; w < 4; w++) {
        CarPhysWheel *wheel = &car->wheels[w];
        int slip = AXLES(c)->axleSlip[w >> 1] * wheel->slipGrip >> 12;
        int grip = CARPHYS_FIELD(wheel, int, 0x34) * AXLES(c)->axleGrip[w >> 1] >> 12;

        if (wheel->brake != 0) {
            k = grip * (0x1000 - ((CARPHYS_FIELD(wheel, short, 0x44) - bal[w]) * slip >> 12)) >> 12;
            if (k < 0) {
                k = 0;
            } else if (k > 0x1000) {
                k = 0x1000;
            }
            k = k * wheel->brake >> 12;
            if (k == 0) {
                k = 1;
            }
            wheel->brake = k;
        }
    }
}

// Traction control: scales the throttle down by how far the lowest grip ratio of the driven
// wheels is below the target. Registers of minRatio/target differ.
void CarPhys_ApplyTractionControl(CarPhys *car, int strength, int steerFactor, int brakeFactor)
{
    int axle;
    int side;
    int minRatio;
    int target;
    int scale;
    unsigned char *layout;

    if (car->clutchIn == 1 && car->engineFriction > 0 && car->throttleTarget != 0) {
        minRatio = 0x1000;
        target = 0x1000;
        layout = &car->driveLayout;
        for (axle = 0; axle < 2; axle++) {
            switch (*layout) {
            case 0:
            case 5:
                if (axle == 0) {
                    continue;
                }
                break;
            case 1:
                if (axle == 1) {
                    continue;
                }
                break;
            }
            for (side = 0; side < 2; side++) {
                int idx = axle * 2 + side;
                int ratio = CARPHYS_FIELD(&car->wheels[idx], short, 0x44);

                if (car->wheels[idx].suspLength != 0 && ratio < minRatio) {
                    minRatio = ratio;
                    target = CARPHYS_FIELD(car, short, 0x1fc + axle * 0xd8);
                }
            }
        }
        scale = Fixed_Mul32(minRatio - (target * steerFactor >> 12), strength * (0x1000 - brakeFactor) >> 12) + 0x1000;
        if (scale < 0) {
            scale = 0;
        } else if (scale > 0x1000) {
            scale = 0x1000;
        }
        car->throttleTarget = scale * car->throttleTarget >> 12;
    }
}

// 1.0 - (|value| - deadzone) * rate, clamped to 0..1.0 (4.12); 1.0 inside the dead zone.
int Input_Attenuate(int value, int rate, int deadzone)
{
    if (value < 0) {
        value = -value;
    }
    value -= deadzone;
    if (value < 0) {
        return 0x1000;
    }
    value = value * rate >> 12;
    if (value > 0x1000) {
        return 0;
    }
    return 0x1000 - value;
}

// From the input frame: the steering wheel display (analog: value * 25 / 1024, digital: +-100),
// and clears unk6fd when the second analog channel is released.
void CarInput_ApplyFlags(CarPhys *car, short *input)
{
    if (!(input[0] & 1)) {
        if (input[1] == 0) {
            CARPHYS_FIELD(car, signed char, 0x768) = 0;
        } else if (input[1] > 0) {
            CARPHYS_FIELD(car, signed char, 0x768) = 100;
        } else {
            CARPHYS_FIELD(car, signed char, 0x768) = -100;
        }
    } else {
        CARPHYS_FIELD(car, signed char, 0x768) = input[1] * 25 >> 10;
    }
    if (!(input[0] & 2)) {
        if (input[3] == 0) {
            car->unk6fd = 0;
        }
    } else if (input[3] != 0x1000) {
        car->unk6fd = 0;
    }
}

void Cars_UpdateDynamics(RaceVehicle *cars, int count)
{
    unsigned int intro = g_RaceWorld.introTimer;
    int i;
    int frozen;
    int recorded;
    int forever;

    i = 0;
    recorded = 2;
    forever = 0xff;
    frozen = i < intro;
    for (; i < count; i++) {
        RaceVehicle *car = &cars[i];
        CarPhys *p = &car->phys;
        int hold;

        p->unk718 = frozen;
        if (Race_IsTimeTrial() && p->ghost == recorded) {
            p->unk718 |= Ghost_ShouldHold();
        }
        if (CARPHYS_FIELD(p, short, 0x73e) != 0) {
            CARPHYS_FIELD(p, short, 0x73e)--;
        }
        if ((unsigned short)p->pitTimer != 0) {
            p->pitTimer--;
        }
        hold = p->aiMessageHold;
        if ((unsigned char)hold != 0 && (unsigned char)hold != forever) {
            p->aiMessageHold = hold - 1;
        }
        Body_ApplyTimeScale(p);
    }
    for (i = 0; i < count; i++) {
        CarPhys *p = &cars[i].phys;

        g_FrameDt = p->timeStep;
        CarPhys_UpdateAero(p);
    }
    for (i = 0; i < count; i++) {
        RaceVehicle *car = &cars[i];
        CarPhys *p = &car->phys;

        p->engineFriction = EngineCurve_TorqueAt(&p->engine, p->engineSpeed);
        p->engineBraking = Fixed_MulFloor(p->engine.unk90, p->engineSpeed);
    }
    Tyres_ComputeGripLimit(cars, count);
    Tyres_ComputeSlipAngle(cars, count);
    {
    char *pad;

    i = 0;
    pad = (char *)rt::from_guest(0x1f800000);
    for (; i < count; i++) {
        RaceVehicle *car = &cars[i];
        int offset = i * 4 + 0x364;
        CarPhys *p;
        short input[6];

        CarInput_Get(input, car);
        p = &car->phys;
        *(int *)pad = p->timeStep;
        p->unk6fd = 1;
        switch ((signed char)p->control) {
        case 0:
            CarInput_ApplyFlags(p, input);
            if (p->aiMode == 0 || (unsigned short)p->pitTimer != 0) {
                unsigned int w;

                Car_ApplyControls(p, input, (int *)rt::from_guest((offset + (int)rt::to_guest(pad))), i);
                p->throttleTarget = p->throttle;
                for (w = 0; w < 4; w++) {
                    p->wheels[w].brake = p->brake;
                }
                break;
            }
            CarAi_Drive(p, input, i);
            break;
        case 2:
            CarAi_Drive(p, 0, i);
            break;
        }
    }
    }
    {
    char *pad;
    unsigned short *cosTable;

    i = 0;
    pad = (char *)rt::from_guest(0x1f800000);
    cosTable = g_CosTable;
    for (; i < count; i++) {
        CarPhys *p = &cars[i].phys;
        int steer = p->steer;
        CarPhys *c = p;
        int axle;
        unsigned int w;
        int rule;
        unsigned short *sinTable;

        if (steer >= 0) {
            p->wheels[0].heading = steer * CARPHYS_FIELD(p, short, 0x64) >> 12;
            p->wheels[1].heading = steer * CARPHYS_FIELD(p, short, 0x66) >> 12;
        } else {
            steer = -steer;
            p->wheels[0].heading = -(steer * CARPHYS_FIELD(p, short, 0x66) >> 12);
            p->wheels[1].heading = -(steer * CARPHYS_FIELD(p, short, 0x64) >> 12);
        }
        p->wheels[2].heading = 0;
        p->wheels[3].heading = 0;
        for (axle = 0; axle < 2; axle++) {
            unsigned short toe = (&CARPHYS_FIELD(c, unsigned short, 0x36c))[axle];

            p->wheels[axle * 2].heading -= toe;
            p->wheels[axle * 2 + 1].heading += toe;
        }
        rule = Body_GetCollisionRule(p);
        w = 0;
        if (rule == 2) {
            p->wheels[0].heading -= p->wheels[0].dirt;
            p->wheels[1].heading += p->wheels[1].dirt;
            p->wheels[2].heading += p->wheels[2].dirt;
            p->wheels[3].heading -= p->wheels[3].dirt;
        }
        sinTable = (unsigned short *)g_SinTable;
        for (; w < 4; w++) {
            int offset = i * 0x90 + w * 0x1c;
            PhysWheelTrig *trig = (PhysWheelTrig *)rt::from_guest((offset + (int)rt::to_guest(pad) + 0x1c));

            trig->cos = cosTable[p->wheels[w].heading & 0xfff];
            trig->sin = sinTable[p->wheels[w].heading & 0xfff];
        }
    }
    }
    Tyres_ComputeSlipRatios(cars, count);
    {
    short *sinTable;
    PhysScratch *scratch;
    int n;

    n = 0;
    sinTable = g_SinTable;
    scratch = g_PhysScratch;
    for (; n < count; n++) {
        CarPhys *p = &cars[n].phys;

        scratch->cars[n].absSlip = 0;
        scratch->cars[n].handbrake = 0;
        if (p->speedFwd > 0) {
            int steer = p->steer;
            int yaw = p->yawRate;
            int angle;

            if (steer * yaw >= 0) {
                int turn;

                if (steer < 0) {
                    steer = -steer;
                    yaw = -yaw;
                }
                turn = (int)Fixed_MulFloor(p->speedFwd, sinTable[steer & 0xfff]);
                turn = ((long long)turn << 12) / ((int)(p->wheelbase * 25736) >> 12);
                if (turn != 0) {
                    int limit = turn << 7;

                    if (yaw < limit) {
                        scratch->cars[n].absSlip = 0x1000 - ((long long)yaw << 12) / limit;
                    }
                }
            }
            angle = Math_Atan2(-p->speedLat, p->speedFwd);
            if (angle != 0) {
                if (angle > 0) {
                    if (p->yawRate > 0) {
                        continue;
                    }
                } else {
                    if (p->yawRate < 0) {
                        continue;
                    }
                    angle = -angle;
                }
                if (p->speedFwd >= 0) {
                    int value;

                    Math_ApproxDist2(p->speedFwd, p->speedLat);
                    value = Fixed_MulShift(angle, p->speedFwd, 2);
                    if (value >= 0) {
                        if (value > 0x1000) {
                            value = 0x1000;
                        }
                        scratch->cars[n].handbrake = value;
                    }
                }
            }
        }
    }
    }
    for (i = 0; i < count; i++) {
        CarPhys *p = &cars[i].phys;

        switch ((signed char)p->control) {
        case 0:
            if (p->aiMode == 0 || (unsigned short)p->pitTimer != 0) {
                CarPhys *c = p;
                int steer = CARPHYS_FIELD(p, short, 0x454);

                if (steer != 0) {
                    int factor = Input_Attenuate(CARPHYS_FIELD(p, short, 0x64a), steer, 0x400);
                    int brake;

                    if (p->gear == 0) {
                        brake = 0;
                    } else {
                        brake = CARPHYS_FIELD(p, short, 0x700) * CARPHYS_FIELD(p, short, 0x456) >> 12;
                    }
                    CarPhys_ApplyTractionControl(p, CARPHYS_FIELD(c, int, 0x450), factor, brake);
                }
                CarPhys_ApplyBrakes(p, CARPHYS_FIELD(c, short, 0x458), CARPHYS_FIELD(c, short, 0x45a));
                break;
            }
        case 2: {
            AiDriveParams *params = &g_AiDriveParams[p->driveClass];
            int factor = Input_Attenuate(CARPHYS_FIELD(p, short, 0x64a), params->unk1c, params->unk1e);

            CarPhys_ApplyTractionControl(p, params->unk20, factor, 0);
            CarPhys_ApplyBrakes(p, params->unk18, params->unk1a);
            break;
        }
        }
    }
    CarPhys_UpdateDrivetrain(cars, count);
    Cars_ComputeFrameDelta(cars, count);
}

// Body pitch and roll from the four wheel ground heights, and the body matrix from them.
void CarPhys_UpdatePitchRoll(CarPhys *car)
{
    int left;
    int right;
    short roll;

    left = car->wheels[0].groundY + Fixed_Mul32(car->wheels[2].groundY - car->wheels[0].groundY, car->rearWeight);
    right = car->wheels[1].groundY + Fixed_Mul32(car->wheels[3].groundY - car->wheels[1].groundY, car->rearWeight);
    CARPHYS_FIELD(car, int, 0x680) = car->pos[0];
    CARPHYS_FIELD(car, int, 0x684) = car->pos[1];
    CARPHYS_FIELD(car, int, 0x688) = (left + right) / 2;
    CARPHYS_FIELD(car, short, 0x6f4) = Math_Atan2(((car->wheels[0].groundY + car->wheels[1].groundY) - (car->wheels[2].groundY + car->wheels[3].groundY)) / 2, car->wheelbase);
    roll = Math_Atan2(left - right, CARPHYS_FIELD(car, short, 0x12));
    CARPHYS_FIELD(car, short, 0x6f6) = roll;
    Matrix_FromAngles(&CARPHYS_FIELD(car, short, 0x68c), &CARPHYS_FIELD(car, short, 0x694), &CARPHYS_FIELD(car, short, 0x69c),
                      CARPHYS_FIELD(car, short, 0x6f4), roll, car->yaw);
}

// After the dynamics: suspension, tyre effects, the visual body sway springs (heading and pitch
// lag), the wheel positions, the body matrices and pitch/roll, and the wheel rotation.
// Registers differ, and the g_GameState / table addresses are split differently.
void Cars_UpdateOrientation(RaceVehicle *cars, int count)
{
    int i;

    if (g_RaceWorld.introTimer == 0) {
        RaceCars_UpdateSuspension(cars, count);
    }
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        g_FrameDt = car->timeStep;
        Body_UpdateTyreFx(car);
    }
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;
        int sway = g_GameState.options2[2];
        int heading = car->yaw;
        int stiffness = g_BodySway_YawStiffness[sway];
        int damping = g_BodySway_YawDamping[sway];
        int force;

        if (sway == 1 && Math_ApproxDist2(car->vel[0], car->vel[1]) >= 0x8e4) {
            heading = Math_Atan2(-car->vel[0], car->vel[1]);
        }
        force = (Angle_Diff(heading, CARPHYS_FIELD(car, short, 0x748)) * stiffness + CARPHYS_FIELD(car, short, 0x74a) * -damping) / 4096;
        CARPHYS_FIELD(car, short, 0x74a) += Fixed_Mul32_16(g_FrameDt, (CARPHYS_FIELD(car, short, 0x752) + force) / 2);
        CARPHYS_FIELD(car, short, 0x752) = force;
        CARPHYS_FIELD(car, short, 0x748) += Fixed_Mul32_16(CARPHYS_FIELD(car, short, 0x74a), g_FrameDt);
        CARPHYS_FIELD(car, short, 0x748) = Angle_Wrap(CARPHYS_FIELD(car, short, 0x748));
        CARPHYS_FIELD(car, short, 0x73a) = Angle_Diff(car->yaw, CARPHYS_FIELD(car, short, 0x748));

        force = Fixed_Mul32(Angle_Diff(car->pitch, CARPHYS_FIELD(car, short, 0x74c)), 0x14000);
        CARPHYS_FIELD(car, short, 0x74e) += Fixed_Mul32_16(force - Fixed_Mul32(CARPHYS_FIELD(car, short, 0x74e), 0xa000), g_FrameDt);
        CARPHYS_FIELD(car, short, 0x74c) += Fixed_Mul32_16(CARPHYS_FIELD(car, short, 0x74e), g_FrameDt);
        CARPHYS_FIELD(car, short, 0x74c) = Angle_Wrap(CARPHYS_FIELD(car, short, 0x74c));
        CARPHYS_FIELD(car, short, 0x73c) = Angle_Diff(car->pitch, CARPHYS_FIELD(car, short, 0x74c));
    }
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        Body_UpdateWheelPositions(car);
    }
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        Matrix_FromAngles(&car->fwdDir[0], &CARPHYS_FIELD(car, short, 0x670), &CARPHYS_FIELD(car, short, 0x678), car->pitch, car->roll, car->yaw);
    }
    for (i = 0; i < count; i++) {
        CarPhys *car = &cars[i].phys;

        CarPhys_UpdatePitchRoll(car);
    }
    Cars_UpdateWheelRotation(cars, count);
}

// One simulation step of every car: dynamics, collisions, ground, orientation, track positions,
// the race clock, the license test end checks, the body transforms (and the ghost) and the
// catch-up between the cars. The original keeps a second copy of raceType for the switch.
void Race_StepWorld(void)
{
    RaceMem *mem = &g_Race_IsDemo;
    RaceWorld *world = &mem->world;
    RaceVehicle *cars = mem->world.vehicles;
    int count = world->carCount;
    int i;

    Cars_UpdateDynamics(cars, count);
    if (world->introTimer == 0) {
        Cars_MoveAndCollide(cars, count);
    }
    RaceCars_PrepareWheelProbes(cars, count);
    RaceCars_ProbeGround(cars, count);
    RaceCars_ApplyGroundProbes(cars, count);
    Cars_UpdateOrientation(cars, count);
    Cars_UpdateTrackPositions(cars, count);
    RaceClock_Tick();
    if (RaceSetup_Get()->raceType == 3) {
        for (i = 0; i < count; i++) {
            CarPhys *phys = &cars[i].phys;

            LicenseTest_CheckEnd(phys);
        }
    }
    if (!Race_IsTimeTrial()) {
        for (i = 0; i < count; i++) {
            RaceVehicle_UpdateTransform(&cars[i]);
        }
    } else {
        RaceVehicle_UpdateTransform(&cars[0]);
        if (count == 2) {
            GhostCar_Interpolate(1);
            RaceVehicle_UpdateTransform(&cars[1]);
        }
    }
    RaceOrder_Update(cars, count);
    if (Race_IsTimeTrial() && count == 2) {
        GhostCar_ApplyFrame(1);
    }
    switch (RaceSetup_Get()->raceType) {
    case 2:
    case 4:
    case 12:
        if (g_RaceEventParams[0x16] == 0) {
            for (i = 1; i < count; i++) {
                CatchUp_ApplyAI(&cars[0].phys, &cars[i].phys);
            }
        } else {
            for (i = 0; i < CARPHYS_FIELD(&cars[0].phys, signed char, 0x750) - 1; i++) {
                CatchUp_ApplyAI(&cars[g_RaceOrder[i + 1]].phys, &cars[g_RaceOrder[i]].phys);
            }
            for (i = CARPHYS_FIELD(&cars[0].phys, signed char, 0x750); i < count; i++) {
                CatchUp_ApplyAI(&cars[g_RaceOrder[i - 1]].phys, &cars[g_RaceOrder[i]].phys);
            }
        }
        break;
    case 0:
        CatchUp_Apply2P(&cars[0].phys, &cars[1].phys);
        break;
    }
}
}
