// OVL0 8003c360-8003d848 (lapprogress): Lap progress, race clock and license tests.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

// A time in two bytes.
struct PackedTime {
    unsigned char seconds;
    unsigned char hundredths;
};

// No time yet: past any race's end.
#define TIME_NONE 0x157529ff

extern "C" {
extern unsigned char g_Race_IsEndurance;
extern int g_RaceClock;
extern RaceMem g_Race_IsDemo;
extern GameState g_GameState;
extern unsigned char g_RaceEventParams[];
extern signed char g_RaceOrder[];

// Per sector line: the order the cars crossed it on the lap the leader is on.
extern unsigned int g_SplitTimes[4][6];
extern signed char g_SplitCarIdx[4][6];
extern short g_SplitLeaderLap[4];
extern signed char g_SplitCount[4];
extern signed char g_SplitReported[4];
extern unsigned char g_Split_IsTimeTrial;

void Ai_SetMode(CarPhys *car, int mode);
void Ai_EnterDefaultMode(CarPhys *car);
void Pit_Update(CarPhys *car, int pos, int prev);
void Race_OnLapCb(int index, int lap, int time, int invalid);
void Race_OnLapDoneCb(int index, int lap, int time, int best, unsigned short extra, int invalid);
void Race_OnFinish(int index, int unused, int place, int time, unsigned short extra);
void Race_ShowGap(int index, int gap);
void TimeTrial_OnLap(CarPhys *car, unsigned int time, int lap, int a3, int noGhost, int a5);
void Ghost_CommitLap(CarPhys *car);
int World_GetCourseDistance(void *world, int hint, int *pos);
int Math_ApproxDist3(int x, int y, int z);
int Car_UpdateLapProgress(CarPhys *car, int prev, int index);
void SplitTimes_Record(CarPhys *car, int lap, int sector, unsigned int time, int lapTime, int index);
void LicenseTest_Fail(CarPhys *car);
void LicenseTest_Succeed(CarPhys *car);
void LicenseTest_OnSector(CarPhys *car, int index, int lap, int sector, int time);
void LicenseTest_OnLap(CarPhys *car, int index, int lap, int time, int lapTime, int maxSpeed);
int CarPhys_CountWheelsOffRoad(CarPhys *car);
void CarPhys_GetTrackDistRange(CarPhys *car, int *min, int *max);

// The test ends here: its time, with the frame fraction in the clock.
static inline void LicenseTest_Clear(CarPhys *car, int clock)
{
    car->licenseResult = 1;
    car->licenseTime = (unsigned int)clock / 3 - car->raceTime;
    LicenseTest_Succeed(car);
}

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// The course, as the game reaches it: the race plus 0xb534, added at run time.
static inline CourseData *Race_Course(void)
{
    char *base = (char *)&g_Race_IsDemo;
    int offset = 0xb534;

    return (CourseData *)(base + offset);
}

// How far into the frame a line was crossed, on the race clock.
static inline int Clock_FrameFraction(int dist, int step)
{
    int scaled;

    if (RaceSetup_Get()->frameStep == 1) {
        scaled = dist * 50;
    } else {
        scaled = dist * 100;
    }
    return scaled / step;
}

int Math_ApproxDist2(int x, int y)
{
    int small;
    int sum;

    if (x < 0) {
        x = -x;
    }
    if (y >= 0) {
        small = y;
    } else {
        y = -y;
        small = y;
    }
    sum = x + y;
    if (x < small) {
        small = x;
    }
    return sum - (small >> 1);
}

int Math_ApproxDist3(int x, int y, int z)
{
    int swap;

    if (x < 0) {
        x = -x;
    }
    if (y < 0) {
        y = -y;
    }
    if (z < 0) {
        z = -z;
    }
    if (x < y) {
        swap = x;
        x = y;
        y = swap;
    }
    if (x < z) {
        swap = x;
        x = z;
        z = swap;
    }
    return x + ((y + z) >> 2);
}

void SplitTimes_Reset(void)
{
    int i;
    int j;

    g_RaceClock = 0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 6; j++) {
            g_SplitTimes[i][j] = TIME_NONE;
        }
        g_SplitLeaderLap[i] = -1;
        g_SplitCount[i] = 0;
        g_SplitReported[i] = 0;
    }
    g_Split_IsTimeTrial = 0;
    if (Race_IsTimeTrial()) {
        g_Split_IsTimeTrial = 1;
    }
    g_Race_IsEndurance = 0;
    switch (RaceSetup_Get()->raceType) {
    case 2:
    case 4:
    case 12:
        if (RaceSetup_Get()->lapCount == 99) {
            g_Race_IsEndurance = 1;
        }
        break;
    }
}

// Lists the cars in the order they cross, for the gaps. The car list isn't shifted with
// the times, so a car placed between two others overwrites the one after it.
void SplitTimes_Record(CarPhys *car, int lap, int sector, unsigned int time, int lapTime, int index)
{
    short *lead = &g_SplitLeaderLap[sector];
    int best = *lead;

    if (best < lap) {
        *lead = lap;
        g_SplitCount[sector] = 1;
        g_SplitReported[sector] = 0;
        g_SplitTimes[sector][0] = time;
        g_SplitCarIdx[sector][0] = index;
    } else if (lap == best) {
        int count = g_SplitCount[sector];
        int reported = g_SplitReported[sector];
        int slot;

        if (reported == count) {
            slot = reported;
        } else {
            int i;
            int above;

            slot = count;
            for (i = slot; reported < i; i = slot) {
                above = i - 1;
                if (time >= g_SplitTimes[sector][above]) {
                    break;
                }
                slot = above;
            }
            for (i = g_SplitCount[sector]; slot < i; i--) {
                g_SplitTimes[sector][i] = g_SplitTimes[sector][i - 1];
            }
        }
        g_SplitTimes[sector][slot] = time;
        g_SplitCarIdx[sector][slot] = index;
        g_SplitCount[sector]++;
    } else {
        Race_Nop(index, best - lap);
    }
}

// Returns 1 when the car went backwards along the course.
int Car_UpdateLapProgress(CarPhys *car, int prev, int index)
{
    int wrongWay = 0;
    int pos = car->trackPos;
    int len = Race_Course()->sections->unk00;

    if (pos >= prev) {
        if (prev + len / 2 < pos) {
            wrongWay = 1;
            car->flags |= 2;
        } else {
            int line;

            if (car->aiStarted == 0) {
                int finish = 0;

                switch (RaceSetup_Get()->raceType) {
                case 7:
                    finish = 0x1900000;
                    break;
                case 8:
                    finish = 0x3e80000;
                    break;
                }
                if (finish != 0 && (car->flags & 2) == 0 && prev < finish && pos >= finish) {
                    int fraction = Clock_FrameFraction(finish - prev, pos - prev);
                    unsigned int time = (unsigned int)(g_RaceClock + fraction) / 3;

                    Race_OnLapDoneCb(index, 1, time, time - car->raceTime, car->maxSpeedKmh,
                                     car->hitFeedback);
                }
            }
            {
                char *base = (char *)&g_Race_IsDemo;
                int offset = 0xb558;

                int *lines = (int *)(base + offset);

                line = lines[car->checkpoint + 1];
            }
            if ((car->flags & 2) == 0 && car->lap != 0 && prev < line && pos >= line) {
                if (car->aiStarted == 0) {
                    int fraction = Clock_FrameFraction(line - prev, pos - prev);
                    int clock = g_RaceClock + fraction;
                    unsigned int time = (unsigned int)clock / 3;
                    int lapTime;

                    if (Race_IsTimeTrial() && car->ghost == 2) {
                        char *ghost = (char *)TimeTrial_GetGhostState();

                        lapTime = (unsigned int)(clock + *(int *)(ghost + 0xc0)) / 3 - car->raceTime;
                    } else {
                        lapTime = time - car->raceTime;
                    }
                    SplitTimes_Record(car, car->lap - 1, car->checkpoint, time, lapTime, index);
                    if (RaceSetup_Get()->raceType == 3) {
                        LicenseTest_OnSector(car, index, car->lap, car->checkpoint, lapTime);
                    } else {
                        int last = 0;

                        if (Course_Get(g_Race_IsDemo.world.course)->flags & 0x20) {
                            last = (car->checkpoint ^ (Race_Course()->checkpointCount - 1)) == 0;
                        }
                        if (last) {
                            Race_OnLapDoneCb(index, 1, time, lapTime, car->maxSpeedKmh,
                                             car->hitFeedback);
                            car->lap = 2;
                            Ai_EnterDefaultMode(car);
                            if (Race_IsTimeTrial()) {
                                TimeTrial_OnLap(car, clock, 0, 0, car->hitFeedback, 0);
                            }
                            car->raceTime = time;
                        } else {
                            Race_OnLapCb(index, car->checkpoint, lapTime, car->hitFeedback);
                        }
                    }
                }
                car->checkpoint++;
            }
        }
    } else if (pos + len / 2 < prev) {
        if (car->flags & 2) {
            car->flags &= 0xfd;
        } else if (car->lap == 0 && g_Split_IsTimeTrial == 0) {
            car->lap++;
        } else {
            int past = len - prev;
            int step = past + pos;
            int fraction;
            int sector;

            if (RaceSetup_Get()->frameStep == 1) {
                fraction = past * 50;
            } else {
                fraction = past * 100;
            }
            fraction /= step;
            sector = car->checkpoint;

            car->checkpoint = 0;
            if (car->aiStarted == 0) {
                int clock = g_RaceClock + fraction;
                unsigned int time = (unsigned int)clock / 3;
                int lapTime;
                int left;
                int lap;
                int ratio;

                if (Race_IsTimeTrial() && car->ghost == 2) {
                    char *ghost = (char *)TimeTrial_GetGhostState();

                    lapTime = (unsigned int)(clock + *(int *)(ghost + 0xc0)) / 3 - car->raceTime;
                } else {
                    lapTime = time - car->raceTime;
                }
                left = (RaceSetup_Get()->frameStep == 1 ? 50 : 100) - fraction;
                lap = car->lap;
                car->lap++;
                ratio = (past << 12) / step;
                if (lap == 0) {
                    if (Race_IsTimeTrial()) {
                        TimeTrial_OnLap(car, 0x405f7dfd, ratio, left, 0, 1);
                    }
                } else {
                    RaceSetup *setup;

                    SplitTimes_Record(car, lap - 1, sector, time, lapTime, index);
                    setup = RaceSetup_Get();
                    if (setup->raceType == 3) {
                        LicenseTest_OnLap(car, index, lap, time, lapTime, car->maxSpeedKmh);
                    } else {
                        Race_OnLapDoneCb(index, lap, time, lapTime, car->maxSpeedKmh,
                                         car->hitFeedback);
                        if (Race_IsTimeTrial()) {
                            TimeTrial_OnLap(car, clock, ratio, left, car->hitFeedback, 1);
                        }
                        if (setup->raceType != 9) {
                            car->hitFeedback = 0;
                            car->unk6fb = 0;
                            car->maxSpeedKmh = car->speedKmh;
                        }
                    }
                }
                if (RaceSetup_Get()->raceType != 3) {
                    car->raceTime = time;
                }
            }
        }
    } else {
        wrongWay = 1;
    }
    {
        int live = 0;

        if (Race_IsTimeTrial()) {
            live = g_Race_IsDemo.world.isReplay == 0;
        }
        if (live) {
            Ghost_CommitLap(car);
        }
    }
    return wrongWay;
}

void Car_UpdateTrackPosition(CarPhys *car, int index)
{
    int pos[3];
    RaceMem *race = &g_Race_IsDemo;
    int prev = car->trackPos;

    pos[0] = car->pos[0] << 4;
    pos[1] = car->pos[2] << 4;
    pos[2] = car->pos[1] << 4;
    car->trackPos = World_GetCourseDistance(race, car->section, pos);
    if (race->world.introTimer == 0) {
        if (Car_UpdateLapProgress(car, prev, index) == 1) {
            int speed;

            car->flags |= 1;
            speed = Fixed_MulFloor(car->speedFwd, car->speedFwd);
            speed += (int)Fixed_MulFloor(car->speedLat, car->speedLat);
            if (speed > 0x1ee0) {
                if (car->wallContact == 0 && (signed char)car->control != 2 && car->aiMode == 0) {
                    CarAi_SetMessage(car, 1, 0);
                }
            } else if (speed < 0xb1d) {
                CarAi_SetMessage(car, 0, 0);
            }
        } else {
            car->flags &= 0xfe;
            CarAi_SetMessage(car, 0, 0);
        }
        Pit_Update(car, car->trackPos, prev);
    }
}

// Moves the cars in race order, then shows the gap of every car that crossed a line.
void Cars_UpdateTrackPositions(RaceVehicle *cars, int count)
{
    int i = 0;

    while (i < count) {
        int car = g_RaceOrder[i];

        i++;
        Car_UpdateTrackPosition(&cars[car].phys, car);
    }
    for (i = 0; i < Race_Course()->checkpointCount + 1; i++) {
        int j = g_SplitReported[i];
        unsigned int *first = g_SplitTimes[i];

        if (j != g_SplitCount[i]) {
            while (j < g_SplitCount[i]) {
                int car = g_SplitCarIdx[i][j];

                if (j != 0) {
                    Race_ShowGap(car, g_SplitTimes[i][j] - *first);
                }
                j++;
            }
            g_SplitReported[i] = g_SplitCount[i];
        }
    }
}

// Endurance race (g_Race_IsEndurance) and g_RaceClock past 21600000 (2 hours).
int Race_IsEnduranceTimeUp(void)
{
    return g_Race_IsEndurance != '\0' && 21600000 < (unsigned int)g_RaceClock;
}

void RaceClock_Tick(void)
{
    RaceMem *race = &g_Race_IsDemo;

    if (race->world.introTimer == 0) {
        char *base = (char *)&g_GameState;
        int offset = 0xbf7c;
        RaceSetup *setup = (RaceSetup *)(base + offset);
        int waiting = 0;

        if (setup->raceType == 6) {
            waiting = race->world.vehicles[0].phys.lap == 0;
        }
        if (waiting == 0) {
            int *clock = &g_RaceClock;
            int next;

            if (setup->frameStep == 1) {
                next = g_RaceClock + 50;
            } else {
                next = g_RaceClock + 100;
            }
            *clock = next;
        }
    }
}

int Car_GetDisplayLap(CarPhys *car)
{
    int lap = car->lap;

    if (lap <= 0 || (RaceSetup_Get()->raceType == 3 && g_RaceEventParams[2] == 5)) {
        lap = 1;
    }
    return lap;
}

void LicenseTest_ResetResult(CarPhys *car)
{
    car->licenseState = 0;
    car->licenseResult = 0;
    car->licenseTime = TIME_NONE;
}

void LicenseTest_Fail(CarPhys *car)
{
    car->licenseState = 2;
    Ai_SetMode(car, 6);
    car->aiStarted = 1;
    Race_OnFinish(car->carIndex, g_RaceEventParams[2], car->licenseResult, car->licenseTime,
                  car->maxSpeedKmh);
}

void LicenseTest_Succeed(CarPhys *car)
{
    car->licenseState = 1;
    if (g_RaceEventParams[2] == 5) {
        Ai_SetMode(car, 5);
    } else {
        Ai_SetMode(car, 5);
    }
    car->aiStarted = 1;
    Race_OnFinish(car->carIndex, g_RaceEventParams[2], car->licenseResult, car->licenseTime,
                  car->maxSpeedKmh);
}

void LicenseTest_OnSector(CarPhys *car, int index, int lap, int sector, int time)
{
    if (g_RaceEventParams[2] == 3 && sector == Race_Course()->checkpointCount - 1) {
        car->licenseResult = 1;
        car->licenseTime = time;
        LicenseTest_Succeed(car);
    } else {
        Race_OnLapCb(index, (lap - 1) * (Race_Course()->checkpointCount + 1) + sector, time,
                     car->hitFeedback);
    }
}

void LicenseTest_OnLap(CarPhys *car, int index, int lap, int time, int lapTime, int maxSpeed)
{
    unsigned char *event = g_RaceEventParams;

    if (event[2] == 5 && event[1] == lap) {
        car->licenseResult = 1;
        car->licenseTime = time;
        LicenseTest_Succeed(car);
    } else {
        Race_OnLapCb(index, lap * (Race_Course()->checkpointCount + 1) - 1, time, car->hitFeedback);
    }
}

int CarPhys_CountWheelsOffRoad(CarPhys *car)
{
    int i = 0;
    int count = i;

    for (; i < 4; i++) {
        unsigned int surface = car->wheels[i].surface;

        if (surface >= 2) {
            count++;
        }
    }
    return count;
}

// The lowest and highest course distance of the four wheels, past the line when the car
// sits across it.
void CarPhys_GetTrackDistRange(CarPhys *car, int *min, int *max)
{
    int pos[3];
    int low = 0x7fffffff;
    int high = 0x80000001;
    int len = Race_Course()->sections->unk00;
    int wrap = len / 8 * 7;
    int i = 0;

    while (i < 4) {
        int dist;

        pos[0] = (car->pos[0] + car->wheels[i].offX) << 4;
        pos[1] = (car->pos[2] + car->wheels[i].offZ) << 4;
        pos[2] = (car->pos[1] + car->wheels[i].offY) << 4;
        dist = World_GetCourseDistance(&g_Race_IsDemo, car->section, pos);
        if (wrap < dist) {
            dist -= len;
        }
        if (dist < low) {
            low = dist;
        }
        if (high < dist) {
            high = dist;
        }
        i++;
    }
    *min = low;
    *max = high;
}

void LicenseTest_CheckEnd(CarPhys *car)
{
    int state = car->licenseState;
    int result;
    unsigned char *event;
    int low;
    int high;
    int stop;
    int speed;
    int fraction;

    if (state == 2) {
        return;
    }
    if (state == 1) {
        return;
    }
    if (car->hitFeedback & 2) {
        result = 4;
        goto fail;
    }
    if (car->hitFeedback & 1) {
        result = 5;
        goto fail;
    }
    event = g_RaceEventParams;
    if (event[2] != 2) {
        return;
    }
    CarPhys_GetTrackDistRange(car, &low, &high);
    stop = event[0x24] * 5;
    if ((stop * 2 + event[0x25]) << 16 < high) {
        result = 3;
        goto fail;
    }
    if (low < stop << 17) {
        return;
    }
    speed = Math_ApproxDist3(car->vel[0], car->vel[1], car->vel[2]);
    if (speed >= 0x472) {
        return;
    }
    if (CarPhys_CountWheelsOffRoad(car) != 0) {
        result = 4;
    fail:
        car->licenseResult = result;
        LicenseTest_Fail(car);
        return;
    }
    if (RaceSetup_Get()->frameStep == 1) {
        fraction = speed * 50 / 0x472;
    } else {
        fraction = speed * 100 / 0x472;
    }
    LicenseTest_Clear(car, g_RaceClock + fraction);
}

// Minutes, seconds and hundredths: the first byte holds minutes * 100 + seconds.
unsigned int PackedTime_ToMs(void *time, int index)
{
    char *base = (char *)time + 0x26;
    PackedTime *packed = (PackedTime *)(index * 2 + base);
    unsigned char minutes = packed->seconds / 100;
    unsigned char seconds = packed->seconds - minutes * 100;

    return minutes * 60000 + seconds * 1000 + packed->hundredths * 10;
}
}
