// OVL0 8003b69c-8003c360 (physparams): Physics constants and parameters, car state and input.
// The functions that don't match yet are the original assembly.
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

// One driving class's parameters as bytes, converted by PhysParams_Init.
struct PhysDriveBytes {
    unsigned char b[16];
};

struct PhysDriveTable {
    PhysDriveBytes classes[5];
};

// One driving class's parameters in fixed point (0x28 bytes). The fields are not named yet.
struct AiDriveParams {
    short unk00;
    short unk02;
    int unk04;
    short unk08;
    short unk0a;
    short unk0c;
    short unk0e;
    int unk10;
    int unk14;
    short unk18;
    short unk1a;
    short unk1c;
    short unk1e;
    int unk20;
    short unk24;
    short unk26;
};

extern "C" {
extern RaceWorld (&g_RaceWorld);
extern PhysDriveTable (&D_80046E5C);
extern AiDriveParams (&g_AiDriveParams)[];
extern int (&g_FrameRate);
extern unsigned char (&D_80046ED4)[];
extern short (&g_PhysFrameRates)[2];
extern short (&D_80046DB4)[2];
extern short (&D_80046DB8)[2];
extern short (&D_80046DBC)[2];
extern short (&D_80046DC0)[2];
extern short (&D_80046DC4)[2];
extern int (&g_TyreWearEnabled);
extern int (&g_TyreWear_MaxGripLoss);
extern int (&g_TyreWear_WornGrip);
extern int (&g_TyreWearInit);
extern int (&g_TyreGripLossInit);
extern int (&g_TyreWear_Knee);
extern int (&g_TyreWear_KneeGripLoss);
void CatchUp_Init(int on);
void SplitTimes_Reset(void);
void TimeTrial_InitBuffers(void);
void PhysParams_Init(int unused);
extern unsigned char (&g_RaceEventParams)[];
extern unsigned char (&g_Race_IsDemo);

// The physics constants as bytes (0x80046e20), converted to fixed point by PhysConsts_Init.
struct PhysConstBytes {
    unsigned char slipDrag;
    unsigned char slipDragMin;
    unsigned char unk2[9];
    unsigned char dragCount;    // 0xb
    unsigned char dragX[8];     // 0xc, speed in 2 km/h steps
    unsigned char dragY[8];     // 0x14, /1000
    unsigned char grip[8];      // 0x1c, per surface, /1000
    unsigned char rollResist[8]; // 0x24, per surface, /10
    unsigned char table3[8];    // 0x2c, per surface
    unsigned char table4[8];    // 0x34, per surface
    unsigned char unk3c[0xc5 - 0x3c];
    unsigned char unkC5;
    unsigned char unkC6;
    unsigned char unkC7;
};

// A piecewise linear curve over int tables.
struct IntCurve {
    short count;
    game_pointer<int> x;
    game_pointer<int> y;
};

extern PhysConstBytes (&g_PhysConstBytes);
extern int (&g_PhysConst_SlipDrag);
extern int (&g_PhysConst_SlipDragMin);
extern int (&g_DragCurveCount);
extern int (&g_DragCurveX)[8];
extern int (&g_DragCurveY)[8];
extern short (&g_SurfaceGrip)[8];
extern int (&g_SurfaceRollResist)[8];
extern short (&g_SurfaceTable3)[8];
extern unsigned char (&g_SurfaceTable4)[8];
extern IntCurve (&g_DragCurve);
extern int (&D_80046F3C);
extern int (&D_80046F40);
extern int (&D_80046F44);

// Tyre wear parameters g_RaceEventParams[0x1d..0x23]: level 1 strong, 2 light, anything else off.
void RaceParams_SetTyreWear(int level)
{
    switch (level) {
    case 1:
        g_RaceEventParams[0x20] = 2;
        g_RaceEventParams[0x22] = 0x10;
        g_RaceEventParams[0x1d] = 0x14;
        g_RaceEventParams[0x21] = 10;
        g_RaceEventParams[0x1f] = 13;
        g_RaceEventParams[0x23] = 15;
        g_RaceEventParams[0x1e] = 25;
        break;
    case 2:
        g_RaceEventParams[0x20] = 1;
        g_RaceEventParams[0x22] = 8;
        g_RaceEventParams[0x1d] = 10;
        g_RaceEventParams[0x21] = 10;
        g_RaceEventParams[0x1f] = 13;
        g_RaceEventParams[0x23] = 15;
        g_RaceEventParams[0x1e] = 25;
        break;
    default:
        g_RaceEventParams[0x20] = 0;
        g_RaceEventParams[0x22] = 0;
        g_RaceEventParams[0x1d] = 0;
        g_RaceEventParams[0x21] = 0;
        g_RaceEventParams[0x1f] = 100;
        g_RaceEventParams[0x23] = 0;
        g_RaceEventParams[0x1e] = 0;
        break;
    }
}

// Catch-up for the cars behind, g_RaceEventParams[0x1a..0x1c] (percent, start, end): level 1
// 15 % over 20..100, level 2 20 % over 10..20, off otherwise. Returns whether catch-up is on.
int RaceParams_SetCatchUp(int level)
{
    int on = 0;

    switch (level) {
    case 1:
        on = 1;
        g_RaceEventParams[0x1a] = 15;
        g_RaceEventParams[0x1b] = 20;
        g_RaceEventParams[0x1c] = 100;
        break;
    case 2:
        on = 1;
        g_RaceEventParams[0x1a] = 20;
        g_RaceEventParams[0x1b] = 10;
        g_RaceEventParams[0x1c] = 20;
        break;
    default:
        g_RaceEventParams[0x1a] = 0;
        g_RaceEventParams[0x1b] = 0;
        g_RaceEventParams[0x1c] = 0;
        break;
    }
    return on;
}

// Converts the byte table g_PhysConstBytes to fixed point: slip drag, the rolling resistance
// curve g_DragCurve and the per-surface tables.
void PhysConsts_Init(void)
{
    int i;

    g_PhysConst_SlipDrag = (g_PhysConstBytes.slipDrag << 12) / 10;
    if (g_PhysConstBytes.slipDrag != 0) {
        g_PhysConst_SlipDragMin = (g_PhysConstBytes.slipDragMin << 12) / g_PhysConstBytes.slipDrag;
    } else {
        g_PhysConst_SlipDragMin = 0;
    }
    g_DragCurveCount = g_PhysConstBytes.dragCount;
    for (i = 0; i < g_DragCurveCount; i++) {
        g_DragCurveX[i] = g_PhysConstBytes.dragX[i] * 2276;
        g_DragCurveY[i] = (g_PhysConstBytes.dragY[i] << 12) / 1000;
    }
    for (i = 0; i < 8; i++) {
        g_SurfaceGrip[i] = (g_PhysConstBytes.grip[i] << 12) / 1000;
    }
    for (i = 0; i < 8; i++) {
        g_SurfaceRollResist[i] = (g_PhysConstBytes.rollResist[i] << 12) / 10;
    }
    for (i = 0; i < 8; i++) {
        g_SurfaceTable3[i] = (g_PhysConstBytes.table3[i] << 8) / 10;
    }
    for (i = 0; i < 8; i++) {
        g_SurfaceTable4[i] = g_PhysConstBytes.table4[i];
    }
    g_DragCurve.x = g_DragCurveX;
    D_80046F3C = g_PhysConstBytes.unkC5 << 12;
    g_DragCurve.y = g_DragCurveY;
    D_80046F44 = g_PhysConstBytes.unkC7 << 12;
    g_DragCurve.count = g_DragCurveCount;
    D_80046F40 = (g_PhysConstBytes.unkC6 << 12) / 10;
}

void PhysParams_Init(int unused)
{
    int i;
    RaceWorld *world;
    PhysDriveTable *table;
    AiDriveParams *drive;

    PhysConsts_Init();
    i = 0;
    world = &g_RaceWorld;
    table = &D_80046E5C;
    drive = g_AiDriveParams;
    for (; i < 4; i++) {
        AiDriveParams *params = &drive[i];
        int idx = i;
        PhysDriveBytes *src;

        if (Course_Get(world->course)->flags & 4) {
            idx = 4;
        }
        src = (PhysDriveBytes *)rt::from_guest((idx * 16 + (int)rt::to_guest(table)));
        params->unk08 = (src->b[3] << 12) / 100;
        params->unk10 = src->b[7] << 16;
        params->unk14 = src->b[8] << 16;
        params->unk0e = Fixed_Div(0x1000, src->b[6] * 2276, 0);
        params->unk00 = 0x64000 / src->b[0];
        params->unk02 = (src->b[1] << 12) / 100;
        params->unk04 = (src->b[2] - 100) * 1138;
        params->unk0a = (src->b[4] << 12) / 100;
        params->unk0c = (src->b[5] << 12) / 100;
        params->unk24 = (src->b[14] << 12) / 100;
        params->unk18 = (src->b[9] << 12) / 100;
        params->unk1a = (src->b[10] << 12) / 100;
        params->unk1c = (src->b[11] << 12) / 10;
        params->unk1e = (src->b[12] << 12) / 100;
        params->unk20 = (src->b[13] << 12) / 10;
    }
    {
        unsigned char *event;

    i = 0;
    event = g_RaceEventParams;
    drive = g_AiDriveParams;
    for (; i < 4; i++) {
        AiDriveParams *params = &drive[i];
        unsigned char *p = (unsigned char *)rt::from_guest((i + (int)rt::to_guest(event)));
        int grip = p[0xc];
        int brake;

        if (grip == 0) {
            grip = 100;
        }
        if (grip > 100) {
            grip = 100;
        }
        params->unk26 = (grip << 12) / 100;
        brake = p[0x10];
        if (brake == 0) {
            brake = 100;
        }
        if (brake > 100) {
            brake = 100;
        }
        params->unk00 = params->unk00 * 100 / brake;
    }
    }
    g_TyreWearEnabled = 0;
    g_TyreWear_MaxGripLoss = 0;
    g_TyreWear_WornGrip = 0;
    g_TyreWearInit = 0;
    g_TyreGripLossInit = 0;
    g_TyreWear_Knee = 0;
    g_TyreWear_KneeGripLoss = 0;
    unsigned char *event = g_RaceEventParams;
    if (event[0x1d] != 0 && (Course_Get(g_RaceWorld.course)->flags & 4) == 0 &&
        event[0x22] < event[0x1d] && event[0x23] < event[0x1e] && event[0x1f] < event[0x1e]) {
        g_TyreWear_MaxGripLoss = (event[0x1e] << 12) / 100;
        g_TyreWearEnabled = event[0x1d] * 10000;
        g_TyreWear_WornGrip = ((100 - event[0x1f]) << 12) / 100;
        g_TyreWearInit = -event[0x20] * 10000;
        g_TyreGripLossInit = (event[0x21] << 12) / 100;
        g_TyreWear_Knee = event[0x22] * 10000;
        g_TyreWear_KneeGripLoss = (event[0x23] << 12) / 100;
    }
    {
        unsigned char *step;
        int rate;
        short *base0;
        short *base1;
        short *base2;
        short *base3;
        short *base4;
        short *base5;

        i = 0;
        step = D_80046ED4;
        rate = g_FrameRate;
        base0 = g_PhysFrameRates;
        base1 = D_80046DB4;
        base2 = D_80046DB8;
        base3 = D_80046DBC;
        base4 = D_80046DC0;
        base5 = D_80046DC4;
        for (; i < 2; i++) {
            short *r0 = &base0[i];
            short *r1 = &base1[i];
            short *r2 = &base2[i];
            short *r3 = &base3[i];
            short *r4 = &base4[i];
            short *r5 = &base5[i];

            *r0 = 0x64000 / (step[0] * rate);
            *r1 = 0x64000 / (step[1] * rate);
            *r2 = 0x64000 / (step[2] * rate);
            *r3 = 0x64000 / (step[3] * rate);
            *r4 = 0x64000 / (step[4] * rate);
            *r5 = 0x64000 / (step[5] * rate);
            if (*r0 > 0x1000) {
                *r0 = 0x1000;
            }
            if (*r1 > 0x1000) {
                *r1 = 0x1000;
            }
            if (*r2 > 0x1000) {
                *r2 = 0x1000;
            }
            if (*r3 > 0x1000) {
                *r3 = 0x1000;
            }
            if (*r4 > 0x1000) {
                *r4 = 0x1000;
            }
            if (*r5 > 0x1000) {
                *r5 = 0x1000;
            }
        }
    }
}

void Race_InitPhysicsParams(void)
{
    int on = 0;
    int wear = on;

    switch (RaceSetup_Get()->raceType) {
    case 2:
    case 4:
    case 12:
        on = 1;
    case 1:
        wear = -1;
        break;
    case 0: {
        RaceSetup *setup = RaceSetup_Get();

        on = RaceParams_SetCatchUp((signed char)setup->options[7]);
        wear = setup->options[3];
        break;
    }
    }
    CatchUp_Init(on);
    if (wear >= 0) {
        RaceParams_SetTyreWear(wear);
    }
    PhysParams_Init(1);
    SplitTimes_Reset();
    if (Race_IsTimeTrial()) {
        TimeTrial_InitBuffers();
    }
}

// Race start: resets the car collisions and the race order.
void Race_InitCarState(void)
{
    // The original addresses g_RaceWorld from 4 bytes before it (g_Race_IsDemo).
    unsigned char *base = &g_Race_IsDemo;
    RaceVehicle *cars = (RaceVehicle *)(base + 4 + 0x184); // RaceWorld.vehicles
    int count = base[4 + 0x5d2d];                          // RaceWorld.carCount

    CarColl_ResetAll();
    RaceOrder_Init(cars, count);
    RaceOrder_Update(cars, count);
}

// Fills the 12-byte input frame of a car from its controlType: the pads for players, the
// recorded log for a ghost (nothing once the ghost stops playing).
void CarInput_Get(void *input, RaceVehicle *car)
{
    void *in = input;
    RaceVehicle *c = car;
    unsigned char *log;

    memset(in, 0, 0xc);
    switch (c->controlType) {
    case 2:
        RaceVehicle_GetPlayer1Input(c, in);
        break;
    case 3:
        RaceVehicle_GetPlayer2Input(c, in);
        break;
    case 1:
        log = (unsigned char *)TimeTrial_GetBestLapLog();
        if (log[1] != 1 && (log[0] == 3 || TimeTrial_GhostEnded() != 0)) {
            memset(in, 0, 0xc);
        } else {
            (*(unsigned short *)(log + 2))++; // frames played
            RaceVehicle_GetLoggedInput(c, in);
        }
        break;
    }
}
}
