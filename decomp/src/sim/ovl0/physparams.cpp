// OVL0 8003b69c-8003c360 (physparams): Physics constants and parameters, car state and input.
// The functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned char g_RaceEventParams[];
extern unsigned char g_Race_IsDemo;

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
    int *x;
    int *y;
};

extern PhysConstBytes g_PhysConstBytes;
extern int g_PhysConst_SlipDrag;
extern int g_PhysConst_SlipDragMin;
extern int g_DragCurveCount;
extern int g_DragCurveX[8];
extern int g_DragCurveY[8];
extern short g_SurfaceGrip[8];
extern int g_SurfaceRollResist[8];
extern short g_SurfaceTable3[8];
extern unsigned char g_SurfaceTable4[8];
extern IntCurve g_DragCurve;
extern int D_80046F3C;
extern int D_80046F40;
extern int D_80046F44;

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

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/physparams", PhysParams_Init);

// Uses a jump table on g_RaceSetup.raceType.
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/physparams", Race_InitPhysicsParams);

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
