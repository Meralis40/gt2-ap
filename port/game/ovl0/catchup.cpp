// OVL0 80041e4c-80043388 (catchup): Catch-up, race order and car bodies.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

// The drivetrain block of CarPhys, from driveLayout on.
struct CarDrive {
    unsigned char unk00[0x34];
    int ratio[8];
};

// The course, as the game reaches it: the race plus 0xb534, added at run time.
static inline CourseData *World_Course(void *world)
{
    int offset = 0xb534;

    return (CourseData *)((char *)world + offset);
}

extern "C" {
extern CatchUpParams g_CatchUp;
extern unsigned char g_RaceEventParams[];
extern int g_FrameRate;
extern int g_FrameDt;
extern unsigned char g_Race_IsDemo;
extern signed char g_RaceOrder[];
extern unsigned char g_PhysConstBytes[];
extern RaceWorld g_RaceWorld;
extern short g_SinTable[];
extern short g_CosTable[];
extern unsigned char D_801C9995;
// The rest of g_CatchUp, which the game reaches one word at a time.
extern int D_80046F70; // aheadStart
extern int D_80046F74; // aheadEnd
extern int D_80046F78; // behindPct
extern int D_80046F7C; // behindStart
extern int D_80046F80; // behindEnd
extern int D_80046F84; // scale

// Takes the catch-up settings from the event, or turns them off.
void CatchUp_Init(int enabled)
{
    if (enabled != 0) {
        unsigned char *event = g_RaceEventParams;
        int scale = event[0x14];

        D_80046F70 = event[0x18] << 16;
        D_80046F74 = (event[0x19] * 5) << 17;
        D_80046F7C = event[0x1b] << 16;
        D_80046F80 = (event[0x1c] * 5) << 17;
        g_CatchUp.aheadPct = (event[0x17] << 12) / 100;
        D_80046F78 = (event[0x1a] << 12) / 100;
        if (scale == 0) {
            scale = 100;
        }
        D_80046F84 = (scale << 12) / 100;
    } else {
        D_80046F70 = 0;
        D_80046F74 = 0;
        g_CatchUp.aheadPct = 0;
        D_80046F7C = 0;
        D_80046F80 = 0;
        D_80046F78 = 0;
        D_80046F84 = 0x1000;
    }
}

// The factor of the car behind in a two player race: full past the end of the range,
// growing along it.
void CatchUp_Apply2PSide(CarPhys *other, CarPhys *body, int lapGap, int posGap)
{
    int factor;

    if ((body->flags & 0x10) != 0 || lapGap < 0) {
        factor = 0x1000;
    } else if (lapGap > 0 || posGap >= D_80046F80) {
        factor = D_80046F78 + 0x1000;
    } else if (D_80046F7C < posGap) {
        int t = ((long long)(posGap - D_80046F7C) << 12) /
                (D_80046F80 - D_80046F7C);

        factor = (t * D_80046F78 >> 12) + 0x1000;
    } else {
        factor = 0x1000;
    }
    body->catchUp = factor;
}

// Catch-up between the two cars of raceType 0: the one behind gets the behind factor
// (only when a behind % is set).
void CatchUp_Apply2P(CarPhys *bodyA, CarPhys *bodyB)
{
    int local_18;
    int local_14;

    if (g_CatchUp.behindPct != 0) {
        Body_GetGap(bodyA, bodyB, &local_18, &local_14);
        CatchUp_Apply2PSide(bodyA, bodyB, local_18, local_14);
        CatchUp_Apply2PSide(bodyB, bodyA, -local_18, -local_14);
    }
}

// The same for a CPU car behind the player.
void CatchUp_BehindFactor(CarPhys *body, int lapGap, int posGap)
{
    int factor;

    if ((body->flags & 0x10) != 0) {
        factor = 0x1000;
    } else if (lapGap > 0 || posGap >= D_80046F80) {
        factor = D_80046F78 + 0x1000;
    } else if (D_80046F7C < posGap) {
        int t = ((long long)(posGap - D_80046F7C) << 12) /
                (D_80046F80 - D_80046F7C);

        factor = (t * D_80046F78 >> 12) + 0x1000;
    } else {
        factor = 0x1000;
    }
    body->catchUp = factor;
}

// A CPU car ahead of the player slows down along its range.
void CatchUp_AheadFactor(CarPhys *body, int lapGap, int posGap)
{
    int factor;

    if (lapGap > 0 || posGap >= D_80046F74) {
        factor = 0x1000 - g_CatchUp.aheadPct;
    } else if (D_80046F70 < posGap) {
        int t = ((long long)(posGap - D_80046F70) << 12) /
                (D_80046F74 - D_80046F70);

        factor = 0x1000 - (t * g_CatchUp.aheadPct >> 12);
    } else {
        factor = 0x1000;
    }
    body->catchUp = factor;
}

// A CPU car against the player, once it has started.
void CatchUp_ApplyAI(CarPhys *player, CarPhys *body)
{
    int lapGap;
    int posGap;
    int behind;

    if (body->aiStarted != 0) {
        return;
    }
    Body_GetGap(player, body, &lapGap, &posGap);
    if (lapGap > 0) {
        behind = 1;
    } else if (lapGap < 0) {
        behind = 0;
    } else {
        behind = posGap > 0;
    }
    if (behind) {
        if (D_80046F78 == 0) {
            body->catchUp = D_80046F84;
            return;
        }
        CatchUp_BehindFactor(body, lapGap, posGap);
    } else {
        if (g_CatchUp.aheadPct == 0) {
            body->catchUp = D_80046F84;
            return;
        }
        CatchUp_AheadFactor(body, -lapGap, -posGap);
    }
    body->catchUp = D_80046F84 * body->catchUp >> 12;
}

// Scales the car's frame step by its catch-up factor.
void Body_ApplyTimeScale(CarPhys *body)
{
    int factor = body->catchUp;

    if (factor == 0x1000) {
        body->timeStep = g_FrameDt;
        body->rateScale = g_FrameRate << 12;
    } else {
        body->timeStep = g_FrameDt * factor >> 12;
        body->rateScale = (g_FrameRate << 24) / body->catchUp;
    }
}

// Lap counter (body+0x608), minus 1 while body+0x78d bit 1 is set (start line not
// crossed yet).
int Body_GetLap(CarPhys *body)
{
    int iVar1;

    iVar1 = (int)body->lap;
    if ((body->flags & 2) != 0) {
        iVar1 = iVar1 + -1;
    }
    return iVar1;
}

// How far ahead a is of b, in laps and distance. A lap ahead but behind on the course
// is less than a lap.
void Body_GetGap(CarPhys *a, CarPhys *b, int *lapOut, int *posOut)
{
    int lapGap = Body_GetLap(a) - Body_GetLap(b);
    int posGap = a->trackPos - b->trackPos;

    if (lapGap > 0 && posGap < 0) {
        lapGap--;
        posGap += World_Course(&g_Race_IsDemo)->sections->unk00;
    } else if (lapGap < 0 && posGap > 0) {
        lapGap++;
        posGap -= World_Course(&g_Race_IsDemo)->sections->unk00;
    }
    *lapOut = lapGap;
    *posOut = posGap;
}

// Which of two cars leads: 1 for a, -1 for b. Finished cars keep their order.
int RaceOrder_Compare(RaceVehicle *cars, int a, int b)
{
    CarPhys *bodyB = &cars[b].phys;
    CarPhys *bodyA = &cars[a].phys;
    int lapA;
    int lapB;
    int posA;
    int posB;

    if (bodyB->aiStarted == 2 || bodyA->aiStarted == 2) {
        return 0;
    }
    lapA = Body_GetLap(bodyA);
    lapB = Body_GetLap(bodyB);
    if (lapB < lapA) {
        return 1;
    }
    if (lapA < lapB) {
        return -1;
    }
    posA = bodyA->trackPos;
    posB = bodyB->trackPos;
    if (posB < posA) {
        return 1;
    }
    if (posA < posB) {
        return -1;
    }
    return 0;
}

// Insertion sort of g_RaceOrder, then each car's place. A car that has crossed the line
// (1) keeps its place from now on (2).
void RaceOrder_Update(RaceVehicle *cars, int count)
{
    int i;

    for (i = 1; i < count; i++) {
        int j = i - 1;
        int car = g_RaceOrder[i];

        for (; j >= 0; j--) {
            if (RaceOrder_Compare(cars, g_RaceOrder[j], car) >= 0) {
                break;
            }
            g_RaceOrder[j + 1] = g_RaceOrder[j];
        }
        g_RaceOrder[j + 1] = car;
    }
    for (i = 0; i < count; i++) {
        CarPhys *body = &cars[g_RaceOrder[i]].phys;

        body->racePos = i + 1;
        if (body->aiStarted == 1) {
            body->aiStarted = 2;
        }
    }
}

// The grid order, first to last.
void RaceOrder_Init(RaceVehicle *cars, int count)
{
    int i;

    for (i = 5; i >= 0; i--) {
        g_RaceOrder[i] = -1;
    }
    for (i = 0; i < count; i++) {
        int next = i + 1;

        cars[i].phys.racePos = next;
        g_RaceOrder[i] = i;
    }
}

// Smoke, squeal and gravel of each wheel, the body rattle and the turbo sound.
void Body_UpdateTyreFx(CarPhys *body)
{
    unsigned char *params;
    int level;
    int gravel = 0;
    int rattle;
    int load = 0;
    int surface = 0;
    int hit = 0;
    int a;
    int b;
    int i;

    a = body->wheels[0].unk44 * body->wheels[0].unk44 >> 12;
    b = body->wheels[1].unk44 * body->wheels[1].unk44 >> 12;
    level = Fixed_MulFloor(body->wheels[0].unk58 + body->wheels[1].unk58, (a + b) / 2 + 0x199);
    if (level < 0) {
        level = -level;
    }
    level = level * 2 / 3;
    if (level >= 0x556) {
        level = 0x555;
    }
    a = (body->wheels[0].unk5c + body->wheels[1].unk5c) / 12;
    a = Fixed_MulFloor(a, a);
    if (a >= 0x556) {
        a = 0x555;
    }
    rattle = level + a;
    if (body->unk73e == g_FrameRate || body->unk738 < body->wallHit) {
        body->unk738 = body->wallHit;
    } else if (body->unk73e < g_FrameRate * 3 / 5) {
        body->unk738 = 0;
    }
    if (body->unk738 != 0) {
        hit = body->unk738 * 8 + 0x400;
    }
    params = g_PhysConstBytes;
    for (i = 0; i < 4; i++) {
        CarPhysWheel *wheel = &body->wheels[i];
        CarPhysWheel *w = &body->wheels[i];
        int amount;
        int soft;
        short slip;
        unsigned char *p;

        if (body->unk718 == 0) {
            if (wheel->unk4c > 0) {
                amount = wheel->unk4c / 12;
                if (amount >= 0x1001) {
                    amount = 0x1000;
                }
                load += amount;
            }
            if (i < 2 && wheel->surface == 1) {
                amount = Fixed_Mul(wheel->suspLength / 8, body->speedFwd / 64);
                if (amount < 0) {
                    amount = -amount;
                }
                if (amount >= 0x401) {
                    amount = 0x400;
                }
                surface += amount;
            }
        }
        if (wheel->suspLength == 0) {
            wheel->fxSqueal = 0;
            wheel->fxSmoke = 0;
            wheel->fxGravel = 0;
            wheel->fxTimer = 0xff;
            continue;
        }
        soft = 0;
        if (wheel->surface < 6) {
            soft = wheel->surface >= 2;
        }
        slip = wheel->unk44;
        if (slip >= 0) {
            slip >>= 5;
            if (slip >= 0x80) {
                slip = 0x7f;
            }
        } else {
            slip = -slip >> 5;
            if (slip < -0x80) {
                slip = -0x80;
            }
        }
        level = Fixed_Mul(g_CosTable[w->heading & 0xfff], w->velLat);
        level += Fixed_Mul(g_SinTable[w->heading & 0xfff], w->velFwd);
        if (wheel->surface != 1) {
            level = params[0x8c] * level >> 12;
            level = level * level >> 12;
            if (params[0x8d] < level) {
                level = params[0x8d];
            }
            amount = 0;
            if (slip > 0) {
                if ((unsigned int)(w->velFwd + 0x8e4) >= 0x11c9) {
                    if (slip == 0x7f) {
                        level += params[0x90];
                        goto capped;
                    }
                    amount = slip * params[0x8e] / 100 + params[0x91];
                    if (params[0x90] < amount) {
                        amount = params[0x90];
                    }
                }
            } else {
                amount = -slip * params[0x8f] / 100 + params[0x91];
                if (params[0x90] < amount) {
                    amount = params[0x90];
                }
            }
            level += amount;
capped:
            if (soft != 0) {
                level += (wheel->unk40 >> 4) * params[0x94] / 100;
            }
            if (params[0x92] < level) {
                level = params[0x92];
            } else if (level < params[0x93]) {
                level = 0;
            }
            wheel->fxSqueal = level;
            wheel->fxGravel = 0;
            if (soft != 0) {
                amount = (wheel->unk40 >> 5) + 0x40;
                if (amount >= 0x100) {
                    amount = 0xff;
                }
                wheel->fxGravel = amount;
            }
        } else {
            amount = (wheel->suspLength >> 5) - 0x40;
            if (amount < 0) {
                amount = 0;
            }
            if (amount >= 0x100) {
                amount = 0xff;
            }
            wheel->fxSqueal = amount;
            amount = wheel->velFwd;
            if (amount < 0) {
                amount = -amount;
            }
            amount = (amount >> 10) - 0x20;
            if (amount < 0) {
                amount = 0;
            } else if (amount >= 0x100) {
                amount = 0xff;
            }
            wheel->fxGravel = amount;
        }
        level = wheel->unk50;
        if (level < -0x400) {
            level += 0x800;
        } else if (level < 0) {
            level = -level;
        } else if (level >= 0x401) {
            level = 0x800 - level;
        }
        if (level >= 0x2ab) {
            level = 0x2aa;
        }
        p = params + soft;
        level = p[0x98] * level / 16;
        level = level * level >> 12;
        if (p[0x9a] < level) {
            level = p[0x9a];
        }
        if (slip == 0x7f) {
            amount = p[0xa0];
        } else {
            if (slip > 0) {
                amount = slip * p[0x9c];
            } else {
                amount = -slip * p[0x9e];
            }
            amount = amount / 100 + p[0xa2];
            if (p[0xa0] < amount) {
                amount = p[0xa0];
            }
        }
        level += amount;
        if (p[0xa4] < level) {
            level = p[0xa4];
        }
        if (soft != 0) {
            amount = (wheel->unk40 >> 4) * params[0xaa] / 100;
            if (params[0xab] < amount) {
                amount = params[0xab];
            }
            level += amount;
            if (level >= 0x100) {
                level = 0xff;
            }
            if ((Course_Get(g_RaceWorld.course)->flags & 4) != 0) {
                gravel = (unsigned int)level >> 1;
            }
        }
        if (level < p[0xa6]) {
            level = 0;
        }
        if (level != 0) {
            amount = wheel->fxTimer + (*(int *)0x1f800000 * wheel->unk40 >> 16);
            if (amount < p[0xa8]) {
                level = 0;
                wheel->fxTimer = amount;
            } else {
                wheel->fxTimer = 0;
            }
        } else {
            wheel->fxTimer = 0xff;
        }
        wheel->fxSmoke = level;
    }
    body->dirt += (unsigned int)(*(int *)0x1f800000 * gravel) >> 16;
    if (body->dirt > 0x75300) {
        body->dirt = 0x75300;
    }
    a = load + surface;
    if (a < rattle) {
        a = rattle;
    }
    if (a < hit) {
        a = hit;
    }
    a >>= 4;
    if (a >= 0x100) {
        a = 0xff;
    }
    body->unk762 = a;
    body->unk763 = 0;
    if (g_FrameRate * 4 / 5 < body->unk73e) {
        body->unk763 = 1;
    }
    if (body->gearCount != 1) {
        int tone;
        int scale;

        level = body->engineLoad;
        if (level >= 0x1001) {
            level = 0x1000;
        }
        tone = (level * 3 + 0x1000) >> 7;
        level -= 0x600;
        if (level < 0) {
            level = -level;
        }
        level += 0x600;
        scale = 0x800;
        if (body->clutchIn == 1) {
            CarDrive *drive = (CarDrive *)&body->driveLayout;
            int ratio = ((long long)drive->ratio[body->gear] << 12) / drive->ratio[1];

            scale = ratio * 7 / 8 + 0x200;
            if (scale >= 0x1001) {
                scale = 0x1000;
            }
        }
        body->unk758 = (body->unk758 + tone) / 2;
        body->unk757 = (body->unk757 + (level * scale >> 17)) / 2;
    }
    {
        int quiet = 0;

        if (body->control == 2 && (&g_Race_IsDemo + 4)[0x18] == 0 &&
            (D_801C9995 == 0 || (&g_Race_IsDemo + 4)[0x5d2e] == 0)) {
            quiet = 1;
        }
        if (quiet != 0) {
            for (i = 0; i < 4; i++) {
                body->wheels[i].fxSqueal = 0;
                body->wheels[i].fxSmoke = 0;
                body->wheels[i].fxGravel = 0;
            }
            body->unk758 = 0;
            body->blowOffVolume = 0;
            body->unk757 = 0;
        }
    }
}

// A colour from a heat value: red, through yellow and green, to blue.
void Color_HeatRamp(signed char heat, int *r, int *g, int *b)
{
    if (heat >= 0x40) {
        *r = 0x7e;
        *g = 0x7f - heat;
        *b = 0;
    } else if (heat >= 0x20) {
        *r = 0x7e;
        *g = (0x5f - heat) * 2;
        *b = 0;
    } else if (heat >= 0) {
        *r = heat << 2;
        *g = 0x7e;
        *b = 0;
    } else if (heat >= -0x3f) {
        int cool = -heat;

        cool *= 2;
        *r = 0;
        *g = 0x7e - cool;
        *b = cool;
    } else {
        *b = 0;
        *g = 0;
        *r = 0;
    }
}

// Each wheel's height against the body, from its pitch and roll.
void Body_UpdateWheelHeights(CarPhys *body)
{
    int front = Fixed_Mul32(body->fwdDir[2], body->distFrontAxle);
    int rear = Fixed_Mul32(body->fwdDir[2], body->distRearAxle);
    int sideF = Fixed_Mul32(body->rightDir[2], body->halfTrackF);
    int sideR = Fixed_Mul32(body->rightDir[2], body->halfTrackR);

    body->wheels[0].offZ = front - sideF;
    body->wheels[1].offZ = sideF + front;
    body->wheels[2].offZ = -sideR - rear;
    body->wheels[3].offZ = sideR - rear;
}

// Each wheel's position in the body's plane.
void Body_UpdateWheelOffsets(CarPhys *body)
{
    int frontX = Fixed_Mul32(body->fwdDir[0], body->distFrontAxle);
    int frontY = Fixed_Mul32(body->fwdDir[1], body->distFrontAxle);
    int rearX = Fixed_Mul32(body->fwdDir[0], body->distRearAxle);
    int rearY = Fixed_Mul32(body->fwdDir[1], body->distRearAxle);
    int sideFX = Fixed_Mul32(body->rightDir[0], body->halfTrackF);
    int sideFY = Fixed_Mul32(body->rightDir[1], body->halfTrackF);
    int sideRX = Fixed_Mul32(body->rightDir[0], body->halfTrackR);
    int sideRY = Fixed_Mul32(body->rightDir[1], body->halfTrackR);

    body->wheels[0].offX = frontX - sideFX;
    body->wheels[0].offY = frontY - sideFY;
    body->wheels[1].offX = sideFX + frontX;
    body->wheels[1].offY = sideFY + frontY;
    body->wheels[2].offX = -sideRX - rearX;
    body->wheels[2].offY = -sideRY - rearY;
    body->wheels[3].offX = sideRX - rearX;
    body->wheels[3].offY = sideRY - rearY;
}

// Body_UpdateWheelHeights then Body_UpdateWheelOffsets.
void Body_UpdateWheelPositions(CarPhys *body)
{
    Body_UpdateWheelHeights(body);
    Body_UpdateWheelOffsets(body);
}
}
