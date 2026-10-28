// Resident code 80074844-80075fac (enginecurve). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int EngineCurve_BoostFactor(EngineCurve *curve, short rpm1, short rpm2);
void EngineCurve_Build(CarParams *params, EngineCurve *curve, int unused);
void EngineCurve_GetPerf(EngineCurve *curve, EnginePerf *out);

// The car name file: a count, then 0x48-byte entries starting with the car id.
struct CarNameEntry {
    int id;
    char name[0x44];
};

struct CarNameTable {
    short count;
    CarNameEntry entries[1];
};

extern char D_8008FB3C[];
extern unsigned char D_800923E2[][6]; // gear spacing per gear count: 3 pairs of 0xc8000 / value at 150 and 400 km/h

unsigned char CarParams_GetDriveLayout(CarParams *params, unsigned char *layout, unsigned char *flag, short *front, short *rear);
short CarParams_GetTyreRadius(CarParams *params, int rear, short *radius, short *width, short *sidewall);
int Fixed_Div(int a, int b, int shift);
int Fixed_Mul(int a, int b);
int Lerp_Clamped(int x, int x0, int x1, int y0, int y1);
int EngineCurve_BoostFactorClamped(EngineCurve *curve, short rpm1, short rpm2);
void EngineCurve_SetupTurbo(CarParams *params, EngineCurve *curve);
int EngineCurve_TorqueAt(EngineCurve *curve, int rpm);

// y0 + (y1 - y0) * (x - x0) / (x1 - x0), clamped to y0 and y1 outside [x0, x1];
// -1 when x0 is negative (no point).
int Lerp_Clamped(int x, int x0, int x1, int y0, int y1)
{
    if (x0 < 0) {
        return -1;
    }
    if (x <= x0) {
        return y0;
    }
    if (x >= x1) {
        return y1;
    }
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

// Resamples an engine curve to 80 points every 250 rpm (-1 past its end).
#ifdef NON_MATCHING
// The game keeps the three arrays' addresses on the stack (a 0x350-byte frame) and
// its registers differ throughout.
void EngineCurve_Resample(int count, short *rpm, short *power, short *torque, short *outPower, short *outTorque)
{
    int r[64];
    int p[64];
    int t[64];
    int i;
    int n;
    int x;

    t[0] = -1;
    p[0] = -1;
    r[0] = -1;
    for (i = 0; i < count; i++) {
        r[i + 1] = rpm[i];
        p[i + 1] = power[i];
        t[i + 1] = torque[i];
    }
    r[i + 1] = -1;
    p[i + 1] = -1;
    t[i + 1] = -1;
    i = 1;
    n = 0;
    x = 0;
    do {
        if (r[i] < x) {
            i++;
            if (i >= count + 1) {
                break;
            }
        } else {
            *outPower++ = Lerp_Clamped(x, r[i - 1], r[i], p[i - 1], p[i]);
            *outTorque++ = Lerp_Clamped(x, r[i - 1], r[i], t[i - 1], t[i]);
            n++;
            x += 250;
        }
    } while (n < 79);
    for (; n < 80; n++) {
        *outPower++ = -1;
        *outTorque++ = -1;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_Resample);
#endif

// Car id's name, or D_8008FB3C if the table has none.
char *CarNameTable_Find(CarNameTable *table, int id)
{
    int i;

    for (i = 0; i < table->count; i++) {
        if (table->entries[i].id == id) {
            return table->entries[i].name;
        }
    }
    return D_8008FB3C;
}

// Gear ratios x1000 for params->gearCount gears from the top speed (150..400 km/h),
// the tyre radius and the gear spacing table; ratios[0] = (3 * ratios[1] + ratios[2]) / 4.
#ifdef NON_MATCHING
// The game computes the ratio table's constants in another order and keeps the
// gear count in another register.
void Gearbox_ComputeRatios(CarParams *params, int topSpeedKmh, short *ratios)
{
    unsigned char layout;
    unsigned char rear;
    short front;
    short back;
    short radius;
    short width;
    short sidewall;
    int count;
    int row;
    int final;
    int t;
    int r;
    unsigned int a;
    int step;
    unsigned int b;
    int stepMul;
    unsigned int c;
    int firstMul;
    short *out;

    CarParams_GetDriveLayout(params, &layout, &rear, &front, &back);
    CarParams_GetTyreRadius(params, rear, &radius, &width, &sidewall);
    count = params->gearCount;
    row = count;
    if (row < 5) {
        row = 5;
    }
    final = params->gear28;
    t = ((topSpeedKmh - 150) << 12) / 250;
    r = Fixed_Div(params->redlineDiv100 * (radius * 0x6488 >> 12) * 100, topSpeedKmh * 0x472, 0);
    r = Fixed_Div(r / 60, (final << 12) / 1000, 0);
    a = 0xc8000 / D_800923E2[row][1];
    step = a + Fixed_Mul(t, 0xc8000 / D_800923E2[row][0] - a);
    b = 0xc8000 / D_800923E2[row][3];
    stepMul = Fixed_Mul(t, 0xc8000 / D_800923E2[row][2] - b);
    c = 0xc8000 / D_800923E2[row][5];
    firstMul = Fixed_Mul(t, 0xc8000 / D_800923E2[row][4] - c);
    out = ratios + count;
    for (; count > 0; count--) {
        if (count == 1) {
            r = Fixed_Mul(r, c + firstMul);
        }
        *out-- = Fixed_Mul(r, 1000);
        r = Fixed_Mul(r, step);
        step = Fixed_Mul(step, b + stepMul);
    }
    ratios[0] = (ratios[1] * 3 + ratios[2]) / 4;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Gearbox_ComputeRatios);
#endif

// The range each gear ratio may be set to: a third of the way to the neighbouring
// gears; first gear up to 9/8, the last down to 7/8.
void Gearbox_ComputeLimits(int gearCount, short *ratios, short *minRatios, short *maxRatios)
{
    int i;

    maxRatios[1] = ratios[1] * 9 / 8;
    for (i = 1; i < gearCount; i++) {
        minRatios[i] = (ratios[i] * 2 + ratios[i + 1]) / 3;
        maxRatios[i + 1] = (ratios[i] + ratios[i + 1] * 2) / 3;
    }
    minRatios[gearCount] = ratios[gearCount] * 7 / 8;
    minRatios[0] = ratios[0] * 7 / 8;
    maxRatios[0] = ratios[0] * 9 / 8;
}

// 1.0 (0x1000) plus each turbo's boost at its rpm: shapes 0 and 1 are quadratic up to
// the boost rpm, 2 linear; past it the boost falls off up to 3 times that rpm.
#ifdef NON_MATCHING
// Matches but for the switch's case values 1 and 2, which the game loads into $t7
// and $t6 before the loop.
int EngineCurve_BoostFactor(EngineCurve *curve, short rpm1, short rpm2)
{
    short rpm[2];
    int sum;
    int i;
    int boost;
    int r;
    int peak;
    int b;
    int d;

    rpm[0] = rpm1;
    rpm[1] = rpm2;
    sum = 0;
    i = 0;
    for (;;) {
        if (i >= 2) {
            return sum + 0x1000;
        }
        boost = curve->boost[i];
        if (boost != 0) {
            r = rpm[i];
            peak = (short)curve->boostRpm[i];
            b = 0;
            if (r < peak) {
                switch (curve->boostType[i]) {
                case 0:
                    d = r - peak;
                    b = boost - (curve->boostK[i] * (d * d >> 12) >> 12);
                    break;
                case 1:
                    b = curve->boostK[i] * (r * r >> 12) >> 12;
                    break;
                case 2:
                    b = boost * r / peak;
                    break;
                }
            } else if (r < peak * 3) {
                d = r - peak;
                b = boost - (curve->boostK[i] / 4 * (d * d >> 12) >> 12);
            }
            sum += b;
        }
        i++;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_BoostFactor);
#endif

int EngineCurve_BoostFactorClamped(EngineCurve *curve, short rpm1, short rpm2)
{
    int iVar1;
    int iVar2;

    iVar1 = EngineCurve_BoostFactor(curve, rpm1, rpm2);
    iVar2 = curve->maxBoost + 0x1000;
    if (iVar2 < iVar1) {
        iVar1 = iVar2;
    }
    return iVar1;
}

// The boost curve of up to 2 turbos from params; all 0 without a turbo.
void EngineCurve_SetupTurbo(CarParams *params, EngineCurve *curve)
{
    short rpm;

    if (params->turboBoost == 0) {
        curve->maxBoost = 0;
    } else {
        curve->maxBoost = (params->turboBoost << 12) / 10;
        if (params->turbo1RpmDiv100 == 0 || params->turbo1Boost == 0 || params->turbo1K == 0) {
            curve->maxBoost = 0;
        }
    }
    if (curve->maxBoost == 0) {
        curve->boostType[0] = 0;
        curve->boost[0] = 0;
        curve->boostA[0] = 0;
        curve->boostK[0] = 0;
        curve->boostRpm[0] = 0;
        curve->boostType[1] = 0;
        curve->boost[1] = 0;
        curve->boostA[1] = 0;
        curve->boostK[1] = 0;
        curve->boostRpm[1] = 0;
        return;
    }
    curve->boostType[0] = params->turbo1Type;
    if (curve->boostType[0] > 2) {
        curve->boostType[0] = 0;
    }
    curve->boost[0] = (params->turbo1Boost << 12) / 10;
    curve->boostA[0] = (params->turbo1K << 12) / 10;
    rpm = params->turbo1RpmDiv100 * 100;
    curve->boostRpm[0] = rpm;
    curve->boostK[0] = Fixed_Div(curve->boost[0], rpm * rpm >> 12, 0);
    curve->boostType[1] = 0;
    curve->boost[1] = 0;
    curve->boostA[1] = 0;
    curve->boostRpm[1] = 0;
    curve->boostK[1] = 0;
    if (params->turbo2Boost + params->turbo2K + params->turbo2RpmDiv100 != 0 && params->turbo2Boost != 0 &&
        params->turbo2RpmDiv100 != 0 && params->turbo2K != 0) {
        curve->boostType[1] = params->turbo2Type;
        if (curve->boostType[1] > 2) {
            curve->boostType[1] = 0;
        }
        curve->boost[1] = (params->turbo2Boost << 12) / 10;
        curve->boostA[1] = (params->turbo2K << 12) / 10;
        rpm = params->turbo2RpmDiv100 * 100;
        curve->boostRpm[1] = rpm;
        curve->boostK[1] = Fixed_Div(curve->boost[1], rpm * rpm >> 12, 0);
    }
}

// The torque curve: rpm points, torque scaled by powerFactor and by a factor going
// from lowRpmFactorPct at the first point to highRpmFactor at the last, the redline,
// and the turbo boost.
#ifdef NON_MATCHING
// The game's statement order and registers differ (the redline and torque setup).
void EngineCurve_Build(CarParams *params, EngineCurve *curve, int unused)
{
    unsigned int redline;
    unsigned short floor;
    unsigned int low;
    unsigned int high;
    unsigned int factor;
    unsigned int rpm0;
    unsigned int rpmN;
    unsigned int rpm;
    int torque;
    int i;
    short r;

    curve->unk8e = params->unk13 * 10;
    redline = params->redlineDiv100;
    if (redline == 0) {
        redline = params->curveRpmDiv100[params->curvePoints - 1];
    }
    curve->redline = redline * 100;
    floor = params->minRedlineDiv100 * 100 + 500;
    if (curve->redline < floor) {
        curve->redline = floor;
    }
    if (params->curveTorque[params->curvePoints - 1] == 0) {
        params->curveTorque[params->curvePoints - 1] = params->curveTorque[params->curvePoints - 2];
    }
    low = params->lowRpmFactorPct;
    if (low == 0) {
        low = 100;
    }
    high = params->highRpmFactor;
    if (high == 0) {
        high = 1000;
    }
    rpm0 = params->curveRpmDiv100[0];
    rpmN = params->curveRpmDiv100[params->curvePoints - 1];
    for (i = 0; i < params->curvePoints; i++) {
        factor = params->powerFactor;
        if (factor == 0) {
            factor = 1000;
        } else if (factor < 0x100) {
            factor *= 10;
        }
        rpm = params->curveRpmDiv100[i];
        curve->rpmFx[i] = rpm * 0x64000 / 60;
        torque = (int)((unsigned short)params->curveTorque[i] * 0x27334) / 4096;
        curve->torqueFx[i] = (long long)torque *
                             (int)(factor * (low * 10 + (int)((high - low * 10) * (rpm - rpm0)) / (int)(rpmN - rpm0))) /
                             100000000;
    }
    curve->rpm = curve->rpmFx;
    curve->torque = curve->torqueFx;
    curve->count = params->curvePoints;
    curve->unk90 = Fixed_Div(params->unk2c * 0x27334 / 4096, curve->rpmFx[params->curvePoints - 1], 0);
    EngineCurve_SetupTurbo(params, curve);
    for (i = 0; i < curve->count; i++) {
        r = params->curveRpmDiv100[i] * 100;
        curve->torqueFx[i] = Fixed_Mul(
            Fixed_Div(EngineCurve_BoostFactorClamped(curve, r, r), EngineCurve_BoostFactor(curve, r, r), 0),
            curve->torqueFx[i]);
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_Build);
#endif

// Samples power and torque every 100 rpm up to the redline, keeping the maximum power
// and torque with their rpm, and the values at the curve's rpm points.
#ifdef NON_MATCHING
// The game's frame is 8 bytes larger and its registers differ.
void EngineCurve_GetPerf(EngineCurve *curve, EnginePerf *out)
{
    int maxPower;
    int maxTorque;
    int n;
    int i;
    int maxPowerRpm;
    int maxTorqueRpm;
    int rpm;
    int top;
    int t;
    int torque;
    int power;
    int j;

    maxPower = 0;
    maxTorque = 0;
    n = 0;
    maxPowerRpm = 0;
    maxTorqueRpm = 0;
    for (i = 0; i < curve->count; i++) {
        rpm = (curve->rpmFx[i] * 60 / 4096 + 50) / 100 * 100;
        if (n <= 0 || out->rpm[n - 1] < rpm) {
            out->rpm[n] = rpm;
            n++;
        }
    }
    out->pointCount = n;
    out->redline = curve->redline;
    top = out->redline;
    if (top < out->rpm[n - 1]) {
        top = out->rpm[n - 1];
    }
    j = 0;
    for (rpm = out->rpm[0]; rpm <= top; rpm += 100) {
        t = EngineCurve_TorqueAt(curve, (rpm << 12) / 60);
        torque = t * 10 / 39;
        power = Fixed_Div(Fixed_Mul(Fixed_Div(t, 0x9ccd, 0), rpm), 0xb30, 0);
        if (out->rpm[j] == rpm) {
            out->power[j] = power;
            out->torque[j] = torque;
            j++;
        }
        if (rpm <= out->redline) {
            if (maxTorque < torque) {
                maxTorque = torque;
                maxTorqueRpm = rpm;
            }
            if (maxPower < power) {
                maxPower = power;
                maxPowerRpm = rpm;
            }
        }
    }
    if (out != 0) {
        out->maxPower = maxPower;
        out->maxTorque = maxTorque;
        out->maxTorqueRpm = maxTorqueRpm;
        out->maxPowerRpm = maxPowerRpm;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_GetPerf);
#endif

int CarParams_GetPerformance(CarParams *params, EnginePerf *out)
{
    EngineCurve EStack_b8;

    EngineCurve_Build(params, &EStack_b8, 0);
    EngineCurve_GetPerf(&EStack_b8, out);
    return (unsigned int)(unsigned short)out->maxPower;
}

// The Fixed_ functions multiply fixed-point numbers: 12 fractional bits, or 16 or 8
// for the variants after them. The Mul ones round toward zero, the Floor and
// Shift ones down; a shift argument adds to the fraction bits.

#ifdef NON_MATCHING
static inline int Fixed_MulRound32(int a, int b, int shift)
{
    return (a * b + ((a ^ b) >> 31 & ((1 << shift) - 1))) >> shift;
}

static inline int Fixed_MulRound64(int a, int b, int shift)
{
    long long p = (long long)a * b;
    unsigned int mask = (1 << shift) - 1;

    return (int)(p >> shift) + ((int)(((unsigned int)p & mask) + ((a ^ b) >> 31 & mask)) >> shift);
}
#endif

// a * b in 32 bits.
#ifdef NON_MATCHING
// The game keeps a and b in $a0 and $a1 and the product and rounding term in $v0
// and $v1; GCC reuses the argument registers (permuter didn't find a form).
int Fixed_Mul32(int a, int b)
{
    return Fixed_MulRound32(a, b, 12);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul32);
#endif

#ifdef NON_MATCHING
// The game keeps a and b in $a0 and $a1 and the product and rounding term in $v0
// and $v1; GCC reuses the argument registers (permuter didn't find a form).
int Fixed_Mul32_16(int a, int b)
{
    return Fixed_MulRound32(a, b, 16);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul32_16);
#endif

#ifdef NON_MATCHING
// The game keeps a and b in $a0 and $a1 and the product and rounding term in $v0
// and $v1; GCC reuses the argument registers (permuter didn't find a form).
int FUN_800759ac(int a, int b)
{
    return Fixed_MulRound32(a, b, 8);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_800759ac);
#endif

#ifdef NON_MATCHING
// The game keeps a and b in $a0 and $a1 and the product and rounding term in $v0
// and $v1; GCC reuses the argument registers (permuter didn't find a form).
int Fixed_Mul32Shift(int a, int b, int shift)
{
    return Fixed_MulRound32(a, b, shift + 12);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul32Shift);
#endif

#ifdef NON_MATCHING
// The game keeps a and b in $a0 and $a1 and the product and rounding term in $v0
// and $v1; GCC reuses the argument registers (permuter didn't find a form).
int FUN_800759fc(int a, int b, int shift)
{
    return Fixed_MulRound32(a, b, shift + 16);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_800759fc);
#endif

#ifdef NON_MATCHING
// The game keeps a and b in $a0 and $a1 and the product and rounding term in $v0
// and $v1; GCC reuses the argument registers (permuter didn't find a form).
int FUN_80075a2c(int a, int b, int shift)
{
    return Fixed_MulRound32(a, b, shift + 8);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_80075a2c);
#endif

// a * b with a 64-bit product.
#ifdef NON_MATCHING
// Register choice and order differ from the game's (permuter didn't find a form).
int Fixed_Mul(int a, int b)
{
    return Fixed_MulRound64(a, b, 12);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul);
#endif

#ifdef NON_MATCHING
// Register choice and order differ from the game's (permuter didn't find a form).
int Fixed_Mul16(int a, int b)
{
    return Fixed_MulRound64(a, b, 16);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul16);
#endif

#ifdef NON_MATCHING
// Register choice and order differ from the game's (permuter didn't find a form).
int FUN_80075acc(int a, int b)
{
    return Fixed_MulRound64(a, b, 8);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_80075acc);
#endif

#ifdef NON_MATCHING
// Register choice and order differ from the game's (permuter didn't find a form).
int Fixed_MulShiftTrunc(int a, int b, int shift)
{
    long long p = (long long)a * b;
    unsigned int lo = (unsigned int)p;
    int hi = (int)(p >> 32);
    unsigned int mask;

    shift += 12;
    mask = (1 << shift) - 1;
    return (lo >> shift | hi << (32 - shift)) + ((lo & mask) + ((a ^ b) >> 31 & mask) >> shift);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_MulShiftTrunc);
#endif

#ifdef NON_MATCHING
// Register choice and order differ from the game's (permuter didn't find a form).
int Fixed_Mul16Shift(int a, int b, int shift)
{
    long long p = (long long)a * b;
    unsigned int lo = (unsigned int)p;
    int hi = (int)(p >> 32);
    unsigned int mask;

    shift += 16;
    mask = (1 << shift) - 1;
    return (lo >> shift | hi << (32 - shift)) + ((lo & mask) + ((a ^ b) >> 31 & mask) >> shift);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", Fixed_Mul16Shift);
#endif

#ifdef NON_MATCHING
// Register choice and order differ from the game's (permuter didn't find a form).
int FUN_80075ba4(int a, int b, int shift)
{
    long long p = (long long)a * b;
    unsigned int lo = (unsigned int)p;
    int hi = (int)(p >> 32);
    unsigned int mask;

    shift += 8;
    mask = (1 << shift) - 1;
    return (lo >> shift | hi << (32 - shift)) + ((lo & mask) + ((a ^ b) >> 31 & mask) >> shift);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", FUN_80075ba4);
#endif

long long Fixed_MulFloor(int a, int b)
{
    return (long long)a * b >> 12;
}

long long Fixed_MulFloor16(int a, int b)
{
    return (long long)a * b >> 16;
}

long long FUN_80075c34(int param_1, int param_2)
{
    return (long long)param_1 * (long long)param_2 >> 8;
}

long long Fixed_MulShift(int a, int b, int shift)
{
    return (long long)a * b >> (shift + 12);
}

long long FUN_80075c9c(int a, int b, int shift)
{
    return (long long)a * b >> (shift + 16);
}

long long FUN_80075ce4(int a, int b, int shift)
{
    return (long long)a * b >> (shift + 8);
}

// The torque at rpm, interpolated in the torque table (a binary search, then a
// linear one over the last 3 points).
#ifdef NON_MATCHING
// The game loads both ends of the table before its first test and keeps the
// search bounds in other registers.
int EngineCurve_TorqueAt(EngineCurve *curve, int rpm)
{
    int *x = curve->rpm;
    int *y = curve->torque;
    unsigned short lo;
    unsigned short hi;
    unsigned short mid;

    hi = curve->count - 1;
    if (rpm <= x[0]) {
        return y[0];
    }
    if (rpm >= x[hi]) {
        return y[hi];
    }
    lo = 0;
    while (hi - lo > 3) {
        mid = (lo + hi) >> 1;
        if (rpm < x[mid]) {
            hi = mid;
        } else if (rpm == x[mid]) {
            return y[mid];
        } else {
            lo = mid;
        }
    }
    for (; lo < hi; lo++) {
        if (rpm < x[lo]) {
            break;
        }
    }
    return y[lo - 1] + (long long)(y[lo] - y[lo - 1]) * (rpm - x[lo - 1]) / (x[lo] - x[lo - 1]);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/enginecurve", EngineCurve_TorqueAt);
#endif

// (a << (shift + 12)) / b, in 64 bits.
int Fixed_Div(int a, int b, int shift)
{
    return ((long long)a << (shift + 12)) / b;
}

// (a * b + c * d) >> (shift + 12) in 64 bits, rounding toward zero.
long long Fixed_Dot2Shift(int a, int b, int c, int d, int shift)
{
    long long sum = (long long)a * b + (long long)c * d;

    if (sum < 0) {
        sum += (1 << (shift + 12)) - 1;
    }
    return sum >> (shift + 12);
}
}
