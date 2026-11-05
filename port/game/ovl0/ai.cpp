// OVL0 80035714-800392d8 (ai): CPU drivers, racing lines and pits.
// The functions that don't match yet are the original assembly.
#define FIXED_MULFLOOR_INT
#include "common.h"
#include "gamestate.h"

// A point of a line, up to the next one. Its corner's arc is centred off the point,
// at radius across the heading.
struct AiSegment {
    int kind; /* 0 straight, 1/2 corner, 3/4 braking */
    int x;
    int z;
    int y;
    short section;
    signed char surface;
    unsigned char flags;
    int dist; /* course distance */
    int radius; /* signed: the side of the corner */
    int slope;
    int bank;
    int heading;
};

struct AiLine {
    int count;
    AiSegment segs[1];
};

// The course's AI data: count goes negative once the offsets of lines are relocated.
struct AiLines {
    int count;
    int unk04;
    AiLine *lines[7]; /* 3 pit entry, 4 pit lane, 6 the racing line */
};

// How a drive class is driven.
struct AiDriveParams {
    short brakeScale;
    short cornerScale;
    int cornerBase;
    short steerGain;
    short throttleCut; /* steering past the lock */
    short brakeAdd; /* steering past the lock */
    short lookRate;
    int lookMin;
    int lookMax;
    unsigned char unk18[14];
    short throttleScale;
};

// A car's controls for the frame, in the scratchpad.
struct AiInput {
    signed char shift;
    signed char b;
    unsigned char pad[2];
};
#define AI_INPUT ((AiInput *)0x1f800364)

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

#define AI_COURSE ((CourseData *)((char *)&g_Race_IsDemo + 0xb534))

extern "C" {
extern unsigned char g_Race_IsDemo;
extern GameState g_GameState;
extern RaceWorld g_RaceWorld;
extern AiLines *g_AiLines;
extern AiDriveParams g_AiDriveParams[];
extern signed char g_AiLineChoices[7][5];
extern unsigned char g_AiLineOrder[];
extern short g_SinTable[];
extern short g_CosTable[];
extern int g_FrameRate;
extern int g_TyreWearEnabled;
extern int g_TyreWearInit;
extern unsigned int g_RaceClock;
extern unsigned char g_RaceCourseId;
extern unsigned char g_RaceCarCount;

int Math_ApproxDist3(int x, int y, int z);
void World_GetGroundInfo(void *world, GroundInfo *info);
int World_GetGroundHeight(void *world, int x, int z, int y, int *section);
int World_GetCourseDistance(void *world, int hint, int *pos);
int CourseSect_FindNearest(CourseSectionTable *table, int *pos);
void CarColl_ResetCar(int carIdx, int count);
void Car_UpdateClutchAuto(CarPhys *phys);
void CarPhys_ResetState(CarPhys *phys);
void CarPhys_ServiceTyres(CarPhys *phys);
int AiLine_FindSegmentXZ(int x, int z, int hint);
int AiLine_CornerSpeed(CarPhys *phys, int index);
int AiLine_PointAt(int line, int hint, int pos, int *x, int *z);
int PitLine_PointAt(int line, int hint, int pos, int *x, int *z);
int Ai_SpeedControl(CarPhys *phys, int index, int *state);
void Ai_UpdateNextCorner(CarPhys *phys);
int Ai_SetMode(CarPhys *phys, int mode);
void Ai_EnterDefaultMode(CarPhys *phys);
int Ai_FindLookaheadPoint(CarPhys *phys, int dist, int *outX, int *outZ);
void Ai_Drive(CarPhys *phys, int index);

// The course, as the game reaches it: the race plus 0xb534, added at run time.
static inline CourseData *Ai_Course(void)
{
    char *base = (char *)&g_Race_IsDemo;
    int offset = 0xb534;

    return (CourseData *)(base + offset);
}

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *Ai_RaceSetup(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// Whether a car with laps to run has finished them: past half the lap counts as the
// lap's end, and an endurance race ends on time.
int Car_HasCompletedLaps(CarPhys *phys)
{
    RaceSetup *setup = Ai_RaceSetup();
    int laps;

    if (setup->lapCount == 0 || (phys->flags & 2) != 0) {
        return 0;
    }
    if (Race_IsEnduranceTimeUp() != 0) {
        return 1;
    }
    laps = setup->lapCount;
    if ((unsigned int)phys->trackPos < Ai_Course()->sections->unk00 >> 1) {
        laps++;
    }
    return phys->lap >= laps;
}

// How far the jacks are up at a pit stop's time left: 0 down, then the lift, 1 going down.
int PitStop_GetLift(int time, int *state)
{
    if (time >= g_FrameRate * 5 - g_FrameRate / 2) {
        *state = 1;
        return (g_FrameRate * 5 - time) * 0x4cc / (g_FrameRate / 2);
    }
    if (time >= g_FrameRate / 2) {
        *state = 2;
        return 0x4cc;
    }
    *state = 0;
    return 0;
}

// How far ahead a driver looks at a speed.
int AiParam_LookaheadForSpeed(AiDriveParams *params, int speed)
{
    int t;
    int result;

    if (speed <= 0) {
        return params->lookMin;
    }
    t = Fixed_MulFloor(params->lookRate, speed);
    if (t < 0x1000) {
        result = params->lookMin + Fixed_MulFloor(t, params->lookMax - params->lookMin);
    } else {
        result = params->lookMax;
    }
    return result;
}

// The first line of a slot's choices that the course has, or -1.
int AiLine_PickForSlot(unsigned int slot)
{
    int i = 0;

    if (slot >= 7) {
        slot = 6;
    }
    while (1) {
        int line;

        if (i >= 5) {
            return -1;
        }
        line = g_AiLineChoices[slot][i];
        i++;
        if (g_AiLines->lines[line] != 0) {
            return line;
        }
    }
}

// The speed a car can take a segment's corner at: its radius and grip, over the banking.
int AiLine_CornerSpeed(CarPhys *phys, int index)
{
    AiSegment *segs = g_AiLines->lines[phys->aiLine]->segs;
    int grip = phys->cornerGrip;
    int radius;
    AiDriveParams *params;
    int speed;
    int top;

    grip = phys->surfaceGrip[segs[index].surface] * grip >> 8;
    radius = segs[index].radius;
    if (g_TyreWearEnabled != 0 && (phys->flags & 0x10) == 0) {
        int i = 0;
        short sum = 0;

        for (; i < 4; i++) {
            sum += phys->wheels[i].tyreGrip;
        }
        grip = (sum >> 2) * grip >> 12;
    }
    if (radius >= 0) {
        int bank;

        grip = Fixed_MulFloor(radius, grip);
        bank = segs[index].bank;
        if (bank < 0) {
            grip = ((long long)grip << 12) / (g_SinTable[bank & 0xfff] + 0x1000);
        }
    } else {
        int bank;

        grip = Fixed_MulFloor(-radius, grip);
        bank = segs[index].bank;
        if (bank > 0) {
            grip = ((long long)grip << 12) / (0x1000 - g_SinTable[bank & 0xfff]);
        }
    }
    params = &g_AiDriveParams[phys->driveClass];
    speed = (int)Fixed_MulFloor(params->cornerScale, Math_ISqrt(grip, 6)) + params->cornerBase;
    top = phys->topSpeed;
    if (speed > top) {
        speed = top;
    } else if (speed < 0x58e8) {
        speed = 0x58e8;
    }
    return speed;
}

// Moves on to the next segment's corner and its target speed.
void Ai_UpdateNextCorner(CarPhys *phys)
{
    CarPhys *car = phys;
    AiLine *line = g_AiLines->lines[phys->aiLine];
    AiSegment *segs = line->segs;
    int count = line->count;
    int index;

    phys->aiCornerPrev = phys->aiCornerNext;
    phys->aiTargetSpeedPrev = phys->aiTargetSpeedNext;
    if (phys->aiSegment != count) {
        phys->aiCornerNext = segs[phys->aiSegment].kind;
    } else {
        phys->aiCornerNext = segs[count - 1].kind;
    }
    switch (phys->aiCornerNext) {
    case 0:
        phys->aiTargetSpeedNext = car->topSpeed;
        break;
    case 1:
    case 2:
        index = phys->aiSegment;
        if (index == count) {
            index--;
        }
        phys->aiTargetSpeedNext = AiLine_CornerSpeed(phys, index);
        break;
    }
}

// The segments around a course position, searching out from hint: 1 before the first,
// 2 past the last, 3 between two, 0 not found.
int AiLine_FindSegment(int line, int hint, int pos, int *next, int *prev)
{
    AiLine *data = g_AiLines->lines[line];
    AiSegment *segs = data->segs;
    int count = data->count;
    int result;
    int index;
    int before;
    int i;

    if (pos < segs[0].dist) {
        result = 1;
wrap:
        *prev = count - 1;
        *next = 0;
        return result;
    }
    i = 0;
    if (pos < segs[count - 1].dist) {
        goto search;
    }
    result = 2;
    goto wrap;
found:
    *prev = before;
    *next = index;
    return 3;
search:
    while (1) {
        if (i > count) {
            break;
        }
        if ((i & 1) == 0) {
            hint -= i;
        } else {
            hint += i;
        }
        index = hint;
        if (index < 0) {
            index += count;
        } else if (index >= count) {
            index -= count;
        }
        before = index - 1;
        if (index != 0) {
            if (before < 0) {
                before += count;
            }
            if (pos >= segs[before].dist && pos < segs[index].dist) {
                goto found;
            }
        }
        i++;
    }
    return 0;
}

// The point of a line at a course position: along a straight, or round a corner's arc.
int AiLine_PointAt(int line, int hint, int pos, int *x, int *z)
{
    int length = Ai_Course()->sections->unk00;
    AiSegment *segs = g_AiLines->lines[line]->segs;
    int next = 0;
    int prev = 0;
    int along;
    int span;
    long long frac;
    int fraction;
    int x0, z0, x1, z1;

    if (pos < 0) {
        pos += length;
    } else if (pos >= length) {
        pos -= length;
    }
    switch (AiLine_FindSegment(line, hint, pos, &next, &prev)) {
    case 1:
        along = pos + length - segs[prev].dist;
        span = segs[next].dist + length - segs[prev].dist;
        break;
    case 2:
        along = pos - segs[prev].dist;
        span = segs[next].dist + length - segs[prev].dist;
        break;
    case 3:
        along = pos - segs[prev].dist;
        span = segs[next].dist - segs[prev].dist;
        break;
    default:
        return 0;
    }
    frac = ((long long)along << 12) / span;
    z0 = segs[prev].z;
    x0 = segs[prev].x;
    x1 = segs[next].x;
    z1 = segs[next].z;
    fraction = frac;
    if (segs[prev].kind == 0) {
        *x = Fixed_Mul(fraction, x1 - x0) + x0;
        *z = Fixed_Mul(fraction, z1 - z0) + z0;
    } else {
        int radius = segs[prev].radius;
        int s = -g_SinTable[segs[prev].heading & 0xfff];
        int cx = x0 - Fixed_Mul(g_CosTable[segs[prev].heading & 0xfff], radius);
        int cz = z0 + Fixed_Mul(s, radius);
        int start = Math_Atan2(cx - x0, z0 - cz);
        int end = Math_Atan2(cx - x1, z1 - cz);
        int angle;

        if (radius > 0) {
            if (end < start) {
                end += 0x1000;
            }
        } else {
            radius = -radius;
            if (start < end) {
                start += 0x1000;
            }
        }
        angle = start + Fixed_Mul32(fraction, end - start);
        *x = cx - Fixed_Mul(g_SinTable[angle & 0xfff], radius);
        *z = cz + Fixed_Mul(g_CosTable[angle & 0xfff], radius);
    }
    return 1;
}

// Whether line 5's segment at a position is a corner.
int AiLine5_TestAt(int hint, int pos)
{
    int next;
    int prev;

    if (g_AiLines->lines[5] != 0) {
        if (hint > g_AiLines->lines[5]->count) {
            hint = 0;
        }
        if (AiLine_FindSegment(5, hint, pos, &next, &prev) != 0) {
            return g_AiLines->lines[5]->segs[prev].kind != 0;
        }
    }
    return 0;
}

// The point of the pit lane at a course position; positions past half the lap are the
// next lap's.
int PitLine_PointAt(int line, int hint, int pos, int *x, int *z)
{
    int next = 0;
    int prev = 0;
    CourseData *course = Ai_Course();
    AiLine *data = g_AiLines->lines[line];
    AiSegment *segs = data->segs;
    int length = course->sections->unk00;
    int count = data->count;
    int frac = 0;
    int i;
    int last;
    int x0, z0, x1, z1;

    if (pos < 0) {
        pos += length;
    } else if (pos >= length) {
        pos -= length;
    }
    if (pos < length / 2) {
        pos += length;
    }
    if (pos < segs[0].dist) {
        return 0;
    }
    if (pos < segs[count - 1].dist) {
        goto search;
    }
    return 0;
found:
    frac = ((long long)(pos - segs[i].dist) << 12) / (segs[i + 1].dist - segs[i].dist);
    goto done;
search:
    last = count - 1;
    for (i = 0; i < last; i++) {
        prev = i;
        next = i + 1;
        if (pos >= segs[i].dist && pos < segs[i + 1].dist) {
            goto found;
        }
    }
done:
    x0 = segs[prev].x;
    x1 = segs[next].x;
    z0 = segs[prev].z;
    z1 = segs[next].z;
    *x = Fixed_Mul(frac, x1 - x0) + x0;
    *z = Fixed_Mul(frac, z1 - z0) + z0;
    return 1;
}

// Finds the car's segment on its line, and the corner it's in.
void Ai_SyncToLine(CarPhys *phys)
{
    CarPhys *car = phys;
    AiLine *line = g_AiLines->lines[phys->aiLine];
    int pos = phys->trackPos;
    int count = line->count;
    AiSegment *segs = line->segs;
    int i;
    int index;

    phys->aiSegment = count;
    for (i = 0; i < count; i++) {
        if (pos < segs[i].dist) {
            phys->aiSegment = i;
            break;
        }
    }
    index = phys->aiSegment + count - 1;
    while (index >= count) {
        index -= count;
    }
    phys->aiCornerNext = segs[index].kind;
    if (phys->aiCornerNext == 0) {
        phys->aiTargetSpeedNext = car->topSpeed;
    } else {
        phys->aiTargetSpeedNext = AiLine_CornerSpeed(phys, index);
    }
    Ai_UpdateNextCorner(phys);
}

// Tracks the car's segment, and returns the throttle limit for the next corner: state
// 1 braking soon, 2 braking.
int Ai_SpeedControl(CarPhys *phys, int index, int *state)
{
    int throttle = 0x1000;

    *state = 0;
    if (phys->aiLine != 4) {
        AiLine *line = g_AiLines->lines[phys->aiLine];
        AiSegment *segs = line->segs;
        int count = line->count;
        int length = Ai_Course()->sections->unk00;
        CarPhys *car = phys;
        AiDriveParams *params;
        int speed;
        int target;
        int v;

        if (phys->aiSegment == count && phys->trackPos < length / 2) {
            phys->aiSegment = 0;
            Ai_UpdateNextCorner(phys);
        } else if (phys->aiSegment < count && phys->trackPos >= segs[phys->aiSegment].dist) {
            phys->aiSegment = phys->aiSegment + 1;
            Ai_UpdateNextCorner(phys);
        }
        params = &g_AiDriveParams[car->driveClass];
        if ((unsigned int)(phys->aiCornerPrev - 3) < 2) {
            goto brake;
        }
        if (phys->aiCornerNext != 0) {
          speed = phys->speedFwd;
          target = phys->aiTargetSpeedNext;
          if (target < speed) {
            int decel;
            int minimum;
            int dist;
            int gap;

            v = Fixed_MulFloor(speed + target, speed - target);
            if (g_TyreWearEnabled == 0 || (phys->flags & 0x10) != 0) {
                decel = car->brakeDecel >> 1;
            } else {
                int i = 0;
                short sum = 0;

                for (; i < 4; i++) {
                    sum += phys->wheels[i].tyreGrip;
                }
                decel = sum * car->brakeDecel >> 15;
            }
            decel = decel * car->surfaceBrake[segs[phys->aiSegment].surface] >> 8;
            minimum = car->brakeMin;
            if (minimum < decel) {
                minimum = decel;
            }
            dist = Fixed_MulFloor(v, (0x1000 - g_SinTable[segs[phys->aiSegment].slope & 0xfff]) * minimum >> 12);
            if (phys->followSlowdown != 0) {
                dist += (phys->followSlowdown * 5 << 14) >> 12;
            }
            gap = Math_ApproxDist2(segs[phys->aiSegment].x - phys->pos[0],
                                   segs[phys->aiSegment].z - phys->pos[1]);
            if (dist < gap) {
                int scale = params->brakeScale;

                if (scale > 0x1000) {
                    int margin = Fixed_MulFloor(dist, scale - 0x1000);
                    int left = gap - dist;

                    if (left < margin + 0x32000) {
                        *state = 1;
                    }
                    if (left < margin) {
                        throttle = Fixed_Div(left, margin, 0);
                    }
                }
            } else {
                phys->aiCornerPrev = phys->aiCornerPrev == 0 ? 3 : 4;
brake:
                *state = 2;
            }
          }
        }
        if (phys->aiLine != 3) {
            throttle = throttle * params->throttleScale >> 12;
        }
    }
    return throttle;
}

// aiLine = line, Ai_SyncToLine.
void Ai_SetLine(CarPhys *phys, int line)
{
    phys->aiLine = line;
    Ai_SyncToLine(phys);
}

// The line to the left of the line, or -1.
int AiLine_SwapToLeft(int line)
{
    if ((line == 1 || line == 6) && g_AiLines->lines[2] != 0) {
        return 2;
    }
    return -1;
}

// The line to the right of the line, or -1.
int AiLine_SwapToRight(int line)
{
    if ((line == 2 || line == 6) && g_AiLines->lines[1] != 0) {
        return 1;
    }
    return -1;
}

// The line to pass on, by the side line 5 turns to.
int Ai_PickPassLineA(int line, int hint, int pos)
{
    int result;

    if (AiLine5_TestAt(hint, pos) != 0) {
        result = AiLine_SwapToRight(line);
    } else {
        result = AiLine_SwapToLeft(line);
    }
    return result;
}

int Ai_PickPassLineB(int line, int hint, int pos)
{
    int result;

    if (AiLine5_TestAt(hint, pos) != 0) {
        result = AiLine_SwapToLeft(line);
    } else {
        result = AiLine_SwapToRight(line);
    }
    return result;
}

// The racing line, or -1 on line 6 already.
int AiLine_DefaultUnless6(int line)
{
    int slot = 6;
    int result;

    if (line != slot) {
        result = AiLine_PickForSlot(slot);
    } else {
        result = -1;
    }
    return result;
}

// The pit entry line, or -1 on it already or if the course has none.
int AiLine_PitEntry(int line)
{
    int entry = 3;
    int result;

    if (g_AiLines->lines[3] != 0) {
        result = -1;
        if (line != entry) {
            result = entry;
        }
    } else {
        result = -1;
    }
    return result;
}

// The pit lane, or -1 in it already or if the course has none.
int AiLine_PitLane(int line)
{
    int lane = 4;
    int result;

    if (g_AiLines->lines[4] != 0) {
        result = -1;
        if (line != lane) {
            result = lane;
        }
    } else {
        result = -1;
    }
    return result;
}

// Enters a mode and its line: returns 0 if the course has no such line.
int Ai_SetMode(CarPhys *phys, int mode)
{
    int count = g_RaceCarCount;
    int line;

    switch (mode) {
    case 2:
        phys->flags |= 8;
    case 4:
        if (phys->aiLine == 3) {
            break;
        }
        line = AiLine_PitEntry(phys->aiLine);
        if (line == -1) {
            return 0;
        }
        Ai_SetLine(phys, line);
        break;
    case 3:
        line = AiLine_PitLane(phys->aiLine);
        if (line == -1) {
            return 0;
        }
        Ai_SetLine(phys, line);
        CarColl_ResetCar(phys->carIndex, count);
        break;
    case 0:
        phys->aiPassState = 0;
    default:
        if (phys->aiMode == 2 || phys->aiMode == 4 || phys->aiMode == 3) {
    case 5:
            Ai_SetLine(phys, AiLine_PickForSlot(6));
            break;
        }
        if ((signed char)phys->control == 0 && phys->aiMode == 0) {
            Ai_SyncToLine(phys);
        }
        break;
    case 6:
    case 7:
        break;
    }
    phys->aiMode = mode;
    return 1;
}

// The mode a car drives in by default: 5 stops it at the end of a one-way course or of
// the races that don't go on.
void Ai_EnterDefaultMode(CarPhys *phys)
{
    if (phys->aiStarted == 0) {
        int mode = phys->aiMode;

        phys->aiStarted = 1;
        if ((unsigned int)(mode - 2) < 3) {
            return;
        }
    }
    if ((Course_Get(g_RaceCourseId)->flags & 0x20) != 0) {
        Ai_SetMode(phys, 5);
    } else {
        switch (Ai_RaceSetup()->raceType) {
        case 3:
        case 7:
        case 8:
        case 11:
            Ai_SetMode(phys, 5);
            break;
        default:
            Ai_SetMode(phys, 1);
            break;
        }
    }
}

// Whether a race has anything to pit for.
int Pit_TyreWearActive(void)
{
    if (g_TyreWearEnabled != 0 || Race_GetCollisionRule() == 2) {
        return 1;
    }
    return 0;
}

// Whether a course position is in the pit lane, past index segments at either end.
int Pit_IsInPitZone(int pos, int index)
{
    AiLine *line = g_AiLines->lines[4];
    AiSegment *segs;
    int length;

    if (line == 0 || line->count < 6) {
        return 0;
    }
    length = Ai_Course()->sections->unk00;
    segs = line->segs;
    if (pos < length / 2) {
        pos += length;
    }
    if (segs[index + 1].dist < pos && pos < segs[line->count - 2 - index].dist) {
        return 1;
    }
    return 0;
}

// The car's way through the pits, as its position goes from prev to pos.
void Pit_Update(CarPhys *phys, int pos, int prev)
{
    AiLine *line = g_AiLines->lines[4];
    int length;
    int count;
    AiSegment *segs;
    int half;
    int need;
    int done;
    int i;
    int stop;
    int warn;

    if (line == 0) {
        return;
    }
    length = Ai_Course()->sections->unk00;
    count = line->count;
    half = length / 2;
    segs = line->segs;
    if (pos < half) {
        pos += length;
    }
    if (prev < half) {
        prev += length;
    }
    if (phys->pitTimer != 0) {
        CarAi_SetMessage(phys, 0xb, 0);
    }
    if ((phys->flags & 1) != 0) {
        if ((signed char)phys->control == 2) {
            return;
        }
        if (phys->aiMode != 0) {
            return;
        }
        if ((phys->flags & 0x10) == 0) {
            return;
        }
        if (pos > segs[3].dist && pos < segs[count - 2].dist) {
            int mark = segs[count - 3].dist;

            if (mark < prev && mark >= pos) {
                Ai_SetMode(phys, 4);
                CarAi_SetMessage(phys, 0xc, g_FrameRate * 2);
                return;
            }
            if ((unsigned int)(phys->speedFwd + 0x163a) < 0x2c75 && phys->speedLat <= 0x163a
                && phys->speedLat >= -0x163a) {
                return;
            }
            if (phys->wallContact == 0) {
                CarAi_SetMessage(phys, 2, 0);
            }
        }
        return;
    }
    need = 0;
    Pit_TyreWearActive();
    for (i = 0; i < 4; i++) {
        if (phys->wheels[i].tyreWear != g_TyreWearInit) {
            need |= 1;
        }
        if (phys->wheels[i].dirt != 0) {
            need |= 2;
        }
    }
    done = Car_HasCompletedLaps(phys);
    switch (phys->aiMode) {
    case 2:
        if (done != 0) {
            Ai_SetMode(phys, 4);
            CarAi_SetMessage(phys, 8, g_FrameRate * 2);
            return;
        }
        if (need == 0) {
            Ai_SetMode(phys, 4);
            CarAi_SetMessage(phys, 9, g_FrameRate * 2);
            return;
        }
        stop = segs[(signed char)phys->pitBox].dist - 0x190000;
        if (prev < stop && pos >= stop) {
            if (phys->speedFwd > 0x1bc87) {
                Ai_SetMode(phys, 4);
                CarAi_SetMessage(phys, 7, g_FrameRate * 2);
                return;
            }
            Ai_SetMode(phys, 3);
            return;
        }
        CarAi_SetMessage(phys, 0xa, 0);
        if ((signed char)phys->control == 0 && (phys->flags & 0x10) == 0) {
            Ai_SetMode(phys, 0);
            CarAi_SetMessage(phys, 0, 0);
        }
        return;
    case 3:
        if (phys->pitTimer == 0) {
            stop = segs[(signed char)phys->pitBox].dist;
            if (need != 0 && prev < stop && pos >= stop) {
                CarPhys_ResetState(phys);
                phys->pitTimer = g_FrameRate * 5;
                CarAi_SetMessage(phys, 0xb, 0);
                return;
            }
            if (pos >= stop + 0x8000) {
                Ai_SetMode(phys, 4);
                phys->flags &= 0xfb;
            }
        } else if (phys->pitTimer == g_FrameRate) {
            CarPhys_ServiceTyres(phys);
        }
        return;
    case 4:
        if ((signed char)phys->control == 0 && (phys->flags & 0x10) == 0 && pos < length) {
            Ai_SetMode(phys, 0);
            CarAi_SetMessage(phys, 0, 0);
        }
        if ((signed char)phys->control != 0 || phys->aiStarted != 0) {
            stop = segs[count - 1].dist;
            warn = segs[count - 2].dist;
        } else {
            stop = segs[count - 2].dist;
            warn = segs[count - 3].dist;
        }
        if (prev < stop && pos >= stop) {
            if (phys->aiStarted == 0) {
                Ai_SetMode(phys, 0);
                if (done == 0) {
                    CarAi_SetMessage(phys, 6, g_FrameRate * 2);
                }
            } else {
                Ai_EnterDefaultMode(phys);
            }
            phys->flags &= 0xf7;
            return;
        }
        if (pos >= warn) {
            if (done == 0) {
                if (phys->unk6fd != 0) {
                    CarAi_SetMessage(phys, 5, 0);
                } else {
                    CarAi_SetMessage(phys, 4, 0);
                }
            }
        } else if (phys->aiStarted == 0) {
            CarAi_SetMessage(phys, 0xa, 0);
        }
        return;
    default:
        if ((signed char)phys->control == 0) {
            if ((phys->flags & 0x10) == 0) {
                return;
            }
            stop = segs[1].dist;
        } else {
            if ((phys->flags & 0x10) != 0 && phys->aiBlockMask != 0 && pos < length) {
                Ai_SetMode(phys, 2);
                phys->aiPassState = 4;
                return;
            }
            if ((phys->flags & 8) == 0 || done != 0) {
                return;
            }
            stop = segs[0].dist;
        }
        if (prev < stop && pos >= stop) {
            Ai_SetMode(phys, 2);
        }
        return;
    }
}

// Revs up on the start line: blips the throttle around the launch rpm, and holds it at
// the launch rpm in the last moment.
void Ai_StartLineRev(CarPhys *phys, int index, int time)
{
    unsigned int i;

    phys->clutch = 0;
    if (time > g_FrameRate * 2 / 5) {
        if (phys->launchHold == 0) {
            if (phys->rpm < phys->launchRpm * 2 - phys->engine.redline) {
                phys->launchHold = 1;
            }
        } else if (phys->rpm >= (phys->engine.redline + phys->launchRpm) >> 1) {
            phys->launchHold = 0;
        }
        if (phys->launchHold != 0) {
            phys->throttle = 0x1000;
        } else {
            phys->throttle = 0;
        }
    } else {
        phys->throttle = 0xa00 - ((phys->rpm - phys->launchRpm) << 13) / phys->engine.redline;
        if (phys->throttle > 0x1000) {
            phys->throttle = 0x1000;
        } else if (phys->throttle < 0) {
            phys->throttle = 0;
        }
    }
    phys->brake = 0;
    phys->throttleTarget = phys->throttle;
    for (i = 0; i < 4; i++) {
        phys->wheels[i].brake = 0;
    }
    phys->steer = 0;
}

// A throttle from base, gain over the speed error.
int Ai_ThrottleFromSpeedError(int speed, int target, int base, int gain)
{
    int error = speed - target;
    int throttle;

    if (error < -0x2c74) {
        error = -0x2c74;
    } else if (error > 0x2c74) {
        error = 0x2c74;
    }
    throttle = base + Fixed_Mul32(gain, error);
    if (throttle > 0x1000) {
        throttle = 0x1000;
    } else if (throttle < 0) {
        throttle = 0;
    }
    return throttle;
}

// Throttle to a target speed, braking above it.
void Ai_BrakeForTarget(CarPhys *phys, int target, int gain)
{
    int throttle = Ai_ThrottleFromSpeedError(phys->speedFwd, target - 0x8e4, 0xc00, gain);

    phys->throttleTarget = throttle;
    if (throttle == 0 && target < phys->speedFwd) {
        unsigned int i;

        for (i = 0; i < 4; i++) {
            phys->wheels[i].brake = 0x1000;
        }
    }
    if (throttle != 0x1000 && throttle != 0) {
        phys->startDelay++;
    }
}

// The vector to the line's point dist ahead, further on when it's too close; returns
// the distance.
int Ai_FindLookaheadPoint(CarPhys *phys, int dist, int *outX, int *outZ)
{
    int x;
    int z;
    int dz = 0;
    int dx = 0;
    int i = 0;
    int line = phys->aiLine;
    int segment = phys->aiSegment;
    int step = 0x10000;

    while (1) {
        int lane;
        int target;

        if (i >= 10) {
            break;
        }
        lane = 4;
        target = phys->trackPos + dist;
        if (line != lane) {
            AiLine_PointAt(line, segment, target, &x, &z);
        } else {
            PitLine_PointAt(4, segment, target, &x, &z);
        }
        dx = x - phys->pos[0];
        dz = z - phys->pos[1];
        if (Math_ApproxDist2(dx, dz) >= 0x5000) {
            break;
        }
        if (i < 9) {
            dist += step;
        }
        i++;
    }
    *outX = dx;
    *outZ = dz;
    return dist;
}

// Steers at the car's rate towards an angle.
void Ai_SteerTowards(CarPhys *phys, int target)
{
    int step = phys->steerSpeed * *(int *)0x1f800000 >> 16;

    if (phys->steer < target) {
        phys->steer += step;
        if (phys->steer > target) {
            phys->steer = target;
        }
    } else if (phys->steer > target) {
        phys->steer -= step;
        if (phys->steer < target) {
            phys->steer = target;
        }
    }
}

// Brakes the car to a stop, then enters mode 7.
void Ai_StopCar(CarPhys *phys, int steer)
{
    int yaw, a, b;
    int speed;
    int spin;
    int stopped;

    Ai_SteerTowards(phys, steer);
    phys->throttle = 0;
    phys->throttleTarget = 0;
    yaw = phys->yawRate / 128;
    a = phys->unk652 / 16;
    b = phys->unk650 / 16;
    speed = Math_ApproxDist3(phys->vel[0], phys->vel[1], phys->vel[2]);
    spin = Math_ApproxDist3(yaw, a, b);
    stopped = speed < 0x11c && spin < 0x22;
    if (stopped) {
        unsigned int i;

        phys->brake = 0;
        for (i = 0; i < 4; i++) {
            phys->wheels[i].brake = 0;
        }
        Ai_SetMode(phys, 7);
    } else {
        unsigned int i;

        phys->brake = 0x1000;
        for (i = 0; i < 4; i++) {
            phys->wheels[i].brake = 0x1000;
        }
    }
}

// Lets go of everything, wheel straight.
void Ai_ReleaseControls(CarPhys *phys)
{
    unsigned int i;

    Ai_SteerTowards(phys, 0);
    phys->throttle = 0;
    phys->brake = 0;
    phys->throttleTarget = 0;
    for (i = 0; i < 4; i++) {
        phys->wheels[i].brake = 0;
    }
}

// A driver's frame: steers at the line's lookahead point, picks the throttle and the
// line.
void Ai_Drive(CarPhys *phys, int index)
{
    CarPhys *car = phys;
    int moving;
    int action;
    int reverse;
    int steer;
    int limit;
    int state;
    int over;
    AiDriveParams *params;
    int lookX;
    int lookZ;
    int error;
    int angle;
    int look;
    int s, c;
    unsigned int i;

    if (phys->aiMode == 6) {
        Ai_StopCar(phys, 0);
        return;
    }
    moving = 0;
    action = 0;
    reverse = 0;
    params = &g_AiDriveParams[phys->driveClass];
    limit = Ai_SpeedControl(phys, index, &state);
    over = 0;
    if (phys->aiCornerPrev == 0 && g_RaceClock < 9000) {
        steer = 0;
        action = 1;
        goto steered;
    }
    s = -g_SinTable[(unsigned short)phys->yaw & 0xfff];
    c = g_CosTable[(unsigned short)phys->yaw & 0xfff];
    if (phys->aiMode == 3) {
        look = 0x50000;
    } else if (phys->aiMode == 4 && phys->aiPitLimit != 0) {
        look = 0x140000;
    } else if (phys->aiPassState < 3) {
        look = AiParam_LookaheadForSpeed(params, phys->speedFwd);
    } else if (phys->aiPassState == 3) {
        look = 0xa0000;
    } else {
        look = 0x280000;
    }
    Ai_FindLookaheadPoint(phys, look, &lookX, &lookZ);
    error = Angle_Diff(Math_Atan2(-lookX, lookZ), phys->yaw);
    angle = error;
    if (angle < 0) {
        angle = -error;
    }
    switch (phys->aiPassState) {
    case 3:
        if (angle < 0x71) {
            if (phys->speedFwd > 0x855c) {
                phys->aiPassState = 0;
            } else {
                phys->aiPassState = 4;
            }
        } else if ((phys->aiBlockMask & 0xc) != 0) {
            phys->aiPassState = 4;
        }
        break;
    case 4:
        if (angle < 0x1c7 && phys->speedFwd > 0x855c) {
            phys->aiPassState = 0;
        } else if ((phys->aiBlockMask & 3) != 0 && (phys->aiBlockMask == 3 || angle >= 0x2ab)) {
            phys->aiPassState = 3;
        }
        break;
    default:
        if (angle >= 0x38f) {
            phys->aiPassState = 4;
        } else if (phys->aiBlockMask == 3) {
            phys->aiPassState = 3;
        }
        break;
    }
    if (phys->speedFwd > 0x8e4) {
        moving = 1;
    } else if (phys->speedFwd < -0x8e4) {
        moving = -1;
    }
    {
        int cone = 0x2aa;

        if (phys->aiPassState == 3) {
            if (phys->speedFwd >= 0) {
                cone = 0x71;
            } else if (phys->speedFwd > -0x855c) {
                cone = 0x71 - phys->speedFwd * 0x238 / 0x855c;
            }
        }
        if (angle < cone) {
            if (phys->speedFwd < -0x8e3) {
                reverse = 1;
            } else {
                action = 1;
            }
        } else if (phys->speedFwd < 0x8e5) {
            action = -1;
        } else {
            reverse = 1;
        }
    }
    if (moving == 0) {
        steer = 0;
        goto steered;
    }
    if (action == 1) {
        long long vx = (long long)phys->vel[0] * phys->vel[0];
        long long vz = (long long)phys->vel[1] * phys->vel[1];
        long long dx = (long long)lookX * lookX;
        long long dz = (long long)lookZ * lookZ;
        int gapSq = (dx + dz) >> 12;
        int speed = Math_ISqrt((vx + vz) >> 12, 6);
        int cross;
        int want;
        int rate;

        if (speed < 0x163a) {
            cross = (c * lookX + -s * lookZ) / 0x1000;
        } else {
            int heading = Math_Atan2(-phys->vel[0], phys->vel[1]) & 0xfff;

            cross = (g_CosTable[heading] * lookX + g_SinTable[heading] * lookZ) / 0x1000;
        }
        want = Fixed_Mul(((long long)speed * cross) / gapSq, 0x518);
        if (phys->speedFwd >= 0) {
            want = -want;
        }
        rate = phys->yawRate;
        if (rate >= 0) {
            rate >>= 7;
        } else {
            rate = -(-rate >> 7);
        }
        steer = 0;
        if (want > 0) {
            if (rate < want) {
                steer = Fixed_Mul32(want - rate, params->steerGain);
                if (steer > car->maxSteer) {
                    over = (steer << 12) / car->maxSteer;
                    if ((Course_Get(g_RaceCourseId)->flags & 4) == 0) {
                        steer = car->maxSteer;
                    }
                }
            }
        } else if (want < 0 && want < rate) {
            int amount = (rate - want) * params->steerGain >> 12;

            steer = -amount;
            if (steer < -car->maxSteer) {
                over = (amount << 12) / car->maxSteer;
                if ((Course_Get(g_RaceCourseId)->flags & 4) == 0) {
                    steer = -car->maxSteer;
                }
            }
        }
        steer += (phys->wheels[0].unk50 + phys->wheels[1].unk50) / 2;
        if (steer > 0x200) {
            steer = 0x200;
        } else if (steer < -0x200) {
            steer = -0x200;
        }
    } else if (error < -0xe3) {
        steer = -0x155;
    } else if (error < 0xe4) {
        steer = Fixed_Mul32(error, 0x1809);
    } else {
        steer = 0x155;
    }
    if (moving == -1) {
        steer = -steer;
    }
steered:
    if (phys->aiMode == 5) {
        Ai_StopCar(phys, steer);
        return;
    }
    if ((signed char)phys->control == 2 || phys->aiMode == 0) {
        Ai_SteerTowards(phys, steer);
    }
    phys->throttleTarget = 0;
    for (i = 0; i < 4; i++) {
        phys->wheels[i].brake = 0;
    }
    phys->throttle = 0;
    phys->brake = 0;
    if (action == 1) {
        int canPass = 0;
        int kind;
        int prev;
        int next;

        phys->aiPitLimit = 0;
        if (phys->aiLine == 4) {
            prev = 0x58e8;
            next = 0x58e8;
            kind = 3;
            phys->aiPitLimit = 1;
        } else if (phys->aiLine == 3) {
            AiLine *pit = g_AiLines->lines[4];
            int length = Ai_Course()->sections->unk00;
            int pos = phys->trackPos;

            if (pos < length / 2) {
                pos += length;
            }
            if (pos < pit->segs[1].dist) {
                prev = phys->aiTargetSpeedPrev;
                if (prev >= 0x1c56c) {
                    prev = 0x1c56c;
                }
                next = phys->aiTargetSpeedNext;
                if (next >= 0x1c56c) {
                    next = 0x1c56c;
                }
                kind = phys->aiCornerPrev;
            } else if (pos < pit->segs[2].dist) {
                prev = 0x15f2e;
                next = 0x15f2e;
                kind = 3;
            } else if (pos < pit->segs[pit->count - 3].dist) {
                prev = 0x15f2e;
                next = 0x15f2e;
                kind = 1;
                phys->aiPitLimit = 1;
            } else {
                prev = phys->aiTargetSpeedPrev;
                next = phys->aiTargetSpeedNext;
                kind = phys->aiCornerPrev;
            }
        } else {
            canPass = 1;
            prev = phys->aiTargetSpeedPrev;
            next = phys->aiTargetSpeedNext;
            kind = phys->aiCornerPrev;
        }
        switch (kind) {
        case 3:
            Ai_BrakeForTarget(phys, next, -0x2000);
            break;
        case 4:
            Ai_BrakeForTarget(phys, next, -0x1000);
            break;
        case 1:
        case 2:
            Ai_BrakeForTarget(phys, prev, -0x800);
            if (state == 0) {
                if (phys->throttleTarget == 0x1000) {
                    state = kind == 1 ? 4 : 3;
                } else {
                    state = 5;
                }
            }
            break;
        case 0:
            phys->throttleTarget = 0x1000;
            break;
        }
        phys->throttleTarget = limit * phys->throttleTarget >> 12;
        if (phys->aiLine != 3) {
            if (state > 0 && (state < 3 || state == 5)) {
                canPass = 0;
            }
            if (canPass != 0) {
                int line = -1;

                if (phys->followSlowdown != 0) {
                    int right = -1;
                    int left = -1;
                    int current = phys->aiLine;

                    if ((signed char)phys->proximity[1] == 0) {
                        left = Ai_PickPassLineA(current, phys->aiSegment, phys->trackPos);
                    }
                    if ((signed char)phys->proximity[2] == 0) {
                        right = Ai_PickPassLineB(current, phys->aiSegment, phys->trackPos);
                    }
                    if (phys->aiCornerNext == 1) {
                        if (left >= 0) {
                            Ai_SetLine(phys, left);
                            goto done;
                        }
                    } else if (phys->aiCornerNext == 2 && right >= 0) {
                        Ai_SetLine(phys, right);
                        goto done;
                    } else if (left >= 0 && state != 3) {
                        Ai_SetLine(phys, left);
                        goto done;
                    }
                    if (right >= 0 && state != 4) {
                        Ai_SetLine(phys, right);
                        goto done;
                    }
                }
                phys->throttleTarget = (0x1000 - phys->followSlowdown) * phys->throttleTarget >> 12;
                if ((signed char)phys->proximity[0] + (signed char)phys->proximity[1]
                        + (signed char)phys->proximity[2] + (signed char)phys->proximity[3]
                        + (signed char)phys->proximity[4] == 0) {
                    line = AiLine_DefaultUnless6(phys->aiLine);
                } else {
                    int a = (signed char)phys->proximity[3];
                    int b = (signed char)phys->proximity[4];
                    int both = 0;

                    if (a != 0) {
                        both = 0 < (unsigned int)b;
                    }
                    if ((a != 0 || b != 0) && both == 0) {
                        int current = phys->aiLine;

                        if (a != 0 && state != 4) {
                            line = Ai_PickPassLineB(current, phys->aiSegment, phys->trackPos);
                            if (line != -1) {
                                Ai_SetLine(phys, line);
                                goto done;
                            }
                        }
                        if (b != 0 && state != 3) {
                            line = Ai_PickPassLineA(current, phys->aiSegment, phys->trackPos);
                        }
                    }
                }
                if (line != -1) {
                    Ai_SetLine(phys, line);
                }
            }
        } else if (((signed char)phys->proximity[3] != 0 && (signed char)phys->proximity[1] != 0)
                   || ((signed char)phys->proximity[4] != 0 && (signed char)phys->proximity[2] != 0)) {
            phys->throttleTarget = phys->throttleTarget / 2;
        }
done:
        phys->brake = phys->wheels[0].brake;
        phys->throttle = phys->throttleTarget;
        if ((signed char)phys->control == 2 || phys->aiMode != 0) {
            AI_INPUT[index].shift = 0;
        }
    } else {
        if (action == -1) {
            AI_INPUT[index].shift = 1;
            phys->throttleTarget = 0x1000;
        } else {
            AI_INPUT[index].shift = 0;
            phys->throttleTarget = 0;
        }
        for (i = 0; i < 4; i++) {
            phys->wheels[i].brake = reverse << 12;
        }
        phys->throttle = phys->throttleTarget;
        phys->brake = phys->wheels[0].brake;
    }
    if (over != 0 && phys->gear >= 2) {
        int extra = over - 0x1000;
        int keep = 0x1000 - (params->throttleCut * extra >> 12);
        short brake;

        if (keep < 0) {
            keep = 0;
        }
        phys->throttleTarget = keep * phys->throttleTarget >> 12;
        phys->throttle = phys->throttleTarget;
        brake = phys->wheels[0].brake + (params->brakeAdd * extra >> 12);
        if (brake > 0x1000) {
            brake = 0x1000;
        }
        for (i = 0; i < 4; i++) {
            phys->wheels[i].brake = brake;
        }
        phys->brake = brake;
    }
}

// A CPU car's frame: clutch at the start, then Ai_Drive.
void CarAi_Drive(CarPhys *phys, short *input, int index)
{
    AiInput *in = &AI_INPUT[index];

    phys->unk614 = 0;
    in->shift = 0;
    in->b = 0;
    if (phys->gearCount == 1) {
        Car_UpdateClutchAuto(phys);
        if (phys->clutch != 0) {
            Ai_Drive(phys, index);
        } else if (phys->aiMode == 7) {
            Ai_ReleaseControls(phys);
        }
        return;
    }
    if (phys->unk718 != 0) {
        Ai_StartLineRev(phys, index, g_RaceWorld.introTimer - g_FrameRate);
        return;
    }
    if (phys->pitTimer != 0) {
        Ai_StartLineRev(phys, index, phys->pitTimer);
        return;
    }
    if (phys->aiMode == 7) {
        phys->clutch = 0;
        Ai_ReleaseControls(phys);
        return;
    }
    if (phys->clutchIn == 0) {
        phys->clutch = 0;
        if (phys->rpm > phys->launchRpm * 3 / 4) {
            phys->clutch = 0x1000;
        }
    } else {
        phys->clutch = 0x1000;
        if (phys->rpm < phys->engine.unk8e) {
            phys->clutch = 0;
        }
    }
    Ai_Drive(phys, index);
}

// Track segment containing (x, 0, z) (World_LocateSection on g_Course), searching from
// hint.
int AiLine_FindSegmentXZ(int x, int z, int hint)
{
    int iVar1;
    int local_18 [4];

    local_18[0] = x << 4;
    local_18[2] = z << 4;
    local_18[1] = 0;
    iVar1 = World_LocateSection(&g_Race_IsDemo, hint, local_18);
    return iVar1;
}

// Follows a segment to its end through the course's sections, from the section it
// starts in; a corner's kind is its direction.
int AiLine_TraceSegment(AiSegment *segs, int index, int section, int line)
{
    int previous = index - 1;
    int x0 = segs[previous].x;
    int z0 = segs[previous].z;
    int x1 = segs[index].x;
    int z1 = segs[index].z;
    int steps;
    int i;

    if (segs[previous].kind == 0) {
        steps = Math_ApproxDist2(x0 - x1, z0 - z1) / 0x14000;
        if (steps == 0) {
            steps = 1;
        }
        i = 1;
        while (i <= steps) {
            int t = (i << 12) / steps;
            int x;

            i++;
            x = Fixed_Mul(t, x1 - x0) + x0;
            section = AiLine_FindSegmentXZ(x, Fixed_Mul(t, z1 - z0) + z0, section);
        }
    } else {
        int radius = segs[previous].radius;
        int heading = segs[previous].heading & 0xfff;
        int s = -g_SinTable[heading];
        int cx = x0 - Fixed_Mul(g_CosTable[heading], radius);
        int cz = z0 + Fixed_Mul(s, radius);
        int start = Math_Atan2(cx - x0, z0 - cz);
        int end = Math_Atan2(cx - x1, z1 - cz);

        if (radius > 0) {
            segs[previous].kind = 1;
            if (end < start) {
                end += 0x1000;
            }
            steps = (end - start) / 0xaa;
        } else {
            segs[previous].kind = 2;
            radius = -radius;
            if (start < end) {
                start += 0x1000;
            }
            steps = (start - end) / 0xaa;
        }
        if (steps == 0) {
            steps = 1;
        }
        if (line == 5) {
            segs[previous].kind = 1;
        }
        i = 1;
        while (i <= steps) {
            int angle = (i << 12) / steps;
            int x;

            angle = (start + Fixed_Mul(angle, end - start)) & 0xfff;
            i++;
            x = cx - Fixed_Mul(g_SinTable[angle], radius);
            section = AiLine_FindSegmentXZ(x, cz + Fixed_Mul(g_CosTable[angle], radius), section);
        }
    }
    return section;
}

// The ground at a point of a line, and its slope along the heading and across it, from
// probes on either side.
int AiLine_ProbeGround(int *height, int *slope, int *bank, int *surface, int *side, int *flags,
                       int x, int z, int heading, int sectionIn)
{
    int section;
    int dx;
    int dz;
    GroundInfo info;
    int centre;
    int a, b, c, d;

    heading &= 0xfff;
    dx = Fixed_Mul(g_SinTable[heading], 0x800);
    dz = Fixed_Mul(g_CosTable[heading], 0x800);
    section = sectionIn;
    info.pos[0] = x << 4;
    info.pos[1] = 0x640000;
    info.pos[2] = z << 4;
    info.section = section;
    World_GetGroundInfo(&g_Race_IsDemo, &info);
    if (info.height != 0x7fffffff) {
        centre = info.height >> 4;
        *flags = info.kind & 2;
        *surface = info.surface;
        *side = info.side;
    } else {
        centre = 0x7fffffff;
        *flags = 0;
        *surface = 0;
        *side = 0;
    }
    a = World_GetGroundHeight(&g_Race_IsDemo, x - dx, z + dz, 0x64000, &section);
    b = World_GetGroundHeight(&g_Race_IsDemo, x + dx, z - dz, 0x64000, &section);
    c = World_GetGroundHeight(&g_Race_IsDemo, x - dz, z - dx, 0x64000, &section);
    d = World_GetGroundHeight(&g_Race_IsDemo, x + dz, z + dx, 0x64000, &section);
    if (centre == 0x7fffffff) {
        return 0;
    }
    if (a == 0x7fffffff && b == a) {
        return 0;
    }
    if (c == 0x7fffffff && d == c) {
        return 0;
    }
    *height = centre;
    if (a == 0x7fffffff) {
        *slope = Math_Atan2(centre - b, 0x800);
    } else if (b == 0x7fffffff) {
        *slope = Math_Atan2(a - centre, 0x800);
    } else {
        *slope = Math_Atan2(a - b, 0x1000);
    }
    if (c == 0x7fffffff) {
        *bank = Math_Atan2(centre - d, 0x800);
    } else if (d == 0x7fffffff) {
        *bank = Math_Atan2(c - centre, 0x800);
    } else {
        *bank = Math_Atan2(c - d, 0x1000);
    }
    return 1;
}

// The racing line's section with ground under a point, and its height.
int AiLine_FindSegmentFromLine6(int *section, int x, int z, int *height)
{
    int line = AiLine_PickForSlot(6);
    AiLine *data;
    AiSegment *segs;
    int count;
    int i;
    int found;
    int ground;

    if (line == -1) {
        return 0;
    }
    data = g_AiLines->lines[line];
    count = data->count;
    segs = data->segs;
    for (i = 0; i < count; i++) {
        found = segs[i].section;
        ground = World_GetGroundHeight(&g_Race_IsDemo, x, z, 0x64000, &found);
        if (ground != 0x7fffffff) {
            *height = ground;
            *section = found;
            return 1;
        }
    }
    return 0;
}

// Relocates the course's AI lines, and fills their segments in from the course.
int AiLines_Init(AiLines *lines)
{
    int length = Ai_Course()->sections->unk00;
    int half;
    int i;

    g_AiLines = lines;
    half = length / 2;
    for (i = 0; i < 7; i++) {
        int line = g_AiLineOrder[i];
        int offset = (int)g_AiLines->lines[line];

        if (offset != 0) {
            AiLine *data;
            AiSegment *segs;
            AiSegment *seg;
            int count;
            int j;
            int section;
            int pos[3];
            int height;
            int y, slope, bank, surface, flags, side;

            if (g_AiLines->count > 0) {
                g_AiLines->lines[line] = (AiLine *)((char *)g_AiLines + offset);
            }
            data = g_AiLines->lines[line];
            count = data->count;
            segs = data->segs;
            section = CourseSect_FindNearest(AI_COURSE->sections, (int *)((char *)AI_COURSE + 0x58));
            seg = segs;
            for (j = 0; j < count; j++) {
                pos[1] = 0;
                pos[0] = seg->x << 4;
                pos[2] = seg->z << 4;
                if (line != 4) {
                    if (j == 0) {
                        section = World_LocateSection(&g_Race_IsDemo, section, pos);
                    } else {
                        section = AiLine_TraceSegment(segs, j, section, line);
                    }
                } else {
                    AiLine_FindSegmentFromLine6(&section, seg->x, seg->z, &height);
                }
                seg->section = section;
                seg->dist = World_GetCourseDistance(&g_Race_IsDemo, section, pos);
                seg->y = 0;
                seg->slope = 0;
                seg->bank = 0;
                seg->surface = 0;
                seg->flags = 0;
                if (line != 5 && AiLine_ProbeGround(&y, &slope, &bank, &surface, &side, &flags, seg->x,
                                                    seg->z, seg->heading, section) != 0) {
                    seg->surface = surface;
                    seg->y = y;
                    seg->slope = slope;
                    seg->bank = bank;
                    seg->flags = side;
                }
                if (line == 4 && seg->dist < half) {
                    seg->dist += length;
                }
                seg++;
            }
        }
    }
    if (g_AiLines->count > 0) {
        g_AiLines->count = -g_AiLines->count;
    }
    return 1;
}

// A section, point and direction of the racing line, dist from the start.
int AiLine_GetPointAtDistance(int *section, int *x, int *z, int *dx, int *dz, int dist)
{
    RaceMem *race = (RaceMem *)&g_Race_IsDemo;
    int offset = 0xb534;
    CourseData *course = (CourseData *)((char *)race + offset);
    int length = course->sections->unk00;
    int line = AiLine_PickForSlot(6);
    int pos;
    AiLine *data;
    AiSegment *segs;
    int count;
    int limit;
    int found;
    int hint;
    int i;
    AiSegment *seg;
    int ax, az, bx, bz;

    if (line == -1) {
        return 0;
    }
    pos = dist << 16;
    data = g_AiLines->lines[line];
    count = data->count;
    segs = data->segs;
    if ((Course_Get(race->world.course)->flags & 0x20) != 0) {
        limit = segs[count - 1].dist - length;
        if (pos < limit) {
            pos = limit;
        }
        limit = course->checkpoints[course->checkpointCount - 1] - 0x140000;
    } else {
        limit = length - 0x140000;
        if (pos < -limit) {
            pos = -limit;
            goto wrap;
        }
    }
    if (pos > limit) {
        pos = limit;
    }
wrap:
    while (pos < 0) {
        pos += length;
    }
    while (pos >= length) {
        pos -= length;
    }
    found = segs[count - 1].section;
    hint = count - 1;
    seg = segs;
    for (i = 0; i < count; i++, seg++) {
        if (seg->dist >= pos) {
            found = seg->section;
            hint = i;
            break;
        }
    }
    AiLine_PointAt(line, hint, pos, &ax, &az);
    AiLine_PointAt(line, hint, pos + 0xa0000, &bx, &bz);
    *section = found;
    *x = ax;
    *z = az;
    *dx = bx - ax;
    *dz = bz - az;
    return dist;
}

// A car's segment of the pit lane; 0 with no pit lane.
int AiLine_GetPitBox(int index)
{
    if (g_AiLines->lines[4] == 0) {
        return 0;
    }
    return 8 - index;
}
}
