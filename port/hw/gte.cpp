// SPDX-License-Identifier: GPL-2.0-or-later
// The geometry coprocessor, written from the public psx-spx documentation of its registers
// and commands. The game calls one function per register move and per command (game/include/gte.h).
#include "gptr.h"
#include "hw.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

namespace {

using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

// Data registers.
enum { VXY0, VZ0, VXY1, VZ1, VXY2, VZ2, RGBC, OTZ, IR0, IR1, IR2, IR3, SXY0, SXY1, SXY2, SXYP,
       SZ0, SZ1, SZ2, SZ3, RGB0, RGB1, RGB2, RES1, MAC0, MAC1, MAC2, MAC3, IRGB, ORGB, LZCS, LZCR };
// Control registers.
enum { R11R12, R13R21, R22R23, R31R32, R33, TRX, TRY, TRZ, L11L12, L13L21, L22L23, L31L32, L33, RBK, GBK, BBK,
       LR1LR2, LR3LG1, LG2LG3, LB1LB2, LB3, RFC, GFC, BFC, OFX, OFY, H, DQA, DQB, ZSF3, ZSF4, FLAG };

enum FlagBits : uint32_t {
    kMac1Pos = 1u << 30, kMac2Pos = 1u << 29, kMac3Pos = 1u << 28, kMac1Neg = 1u << 27, kMac2Neg = 1u << 26,
    kMac3Neg = 1u << 25, kIr1 = 1u << 24, kIr2 = 1u << 23, kIr3 = 1u << 22, kColorR = 1u << 21, kColorG = 1u << 20,
    kColorB = 1u << 19, kSz3 = 1u << 18, kDivide = 1u << 17, kMac0Pos = 1u << 16, kMac0Neg = 1u << 15,
    kSx2 = 1u << 14, kSy2 = 1u << 13, kIr0 = 1u << 12,
};

int32_t g_Data[32];
int32_t g_Ctrl[32];
uint32_t g_Flag;

int16_t low16(int32_t value)
{
    return static_cast<int16_t>(value);
}

int16_t high16(int32_t value)
{
    return static_cast<int16_t>(value >> 16);
}

int16_t vectorX(int v)
{
    return low16(g_Data[VXY0 + v * 2]);
}

int16_t vectorY(int v)
{
    return high16(g_Data[VXY0 + v * 2]);
}

int16_t vectorZ(int v)
{
    return low16(g_Data[VZ0 + v * 2]);
}

struct Matrix {
    int32_t m[3][3];
};

Matrix rotation()
{
    return {{{low16(g_Ctrl[R11R12]), high16(g_Ctrl[R11R12]), low16(g_Ctrl[R13R21])},
             {high16(g_Ctrl[R13R21]), low16(g_Ctrl[R22R23]), high16(g_Ctrl[R22R23])},
             {low16(g_Ctrl[R31R32]), high16(g_Ctrl[R31R32]), low16(g_Ctrl[R33])}}};
}

Matrix lightMatrix()
{
    return {{{low16(g_Ctrl[L11L12]), high16(g_Ctrl[L11L12]), low16(g_Ctrl[L13L21])},
             {high16(g_Ctrl[L13L21]), low16(g_Ctrl[L22L23]), high16(g_Ctrl[L22L23])},
             {low16(g_Ctrl[L31L32]), high16(g_Ctrl[L31L32]), low16(g_Ctrl[L33])}}};
}

Matrix colorMatrix()
{
    return {{{low16(g_Ctrl[LR1LR2]), high16(g_Ctrl[LR1LR2]), low16(g_Ctrl[LR3LG1])},
             {high16(g_Ctrl[LR3LG1]), low16(g_Ctrl[LG2LG3]), high16(g_Ctrl[LG2LG3])},
             {low16(g_Ctrl[LB1LB2]), high16(g_Ctrl[LB1LB2]), low16(g_Ctrl[LB3])}}};
}

// A MAC value: flags its overflow of 44 bits, gives its low 32 bits.
int32_t setMac(int index, int64_t value, int shift)
{
    static const uint32_t kPositive[4] = {0, kMac1Pos, kMac2Pos, kMac3Pos};
    static const uint32_t kNegative[4] = {0, kMac1Neg, kMac2Neg, kMac3Neg};

    if (value >= (int64_t{1} << 43)) {
        g_Flag |= kPositive[index];
    } else if (value < -(int64_t{1} << 43)) {
        g_Flag |= kNegative[index];
    }
    int32_t result = static_cast<int32_t>(value >> shift);
    g_Data[MAC0 + index] = result;
    return result;
}

void setIr(int index, int32_t value, bool unsignedLimit)
{
    static const uint32_t kBits[4] = {0, kIr1, kIr2, kIr3};
    int32_t low = unsignedLimit ? 0 : -0x8000;

    if (value < low || value > 0x7fff) {
        g_Flag |= kBits[index];
        value = std::clamp(value, low, 0x7fff);
    }
    g_Data[IR0 + index] = value;
}

// MAC1-3 and IR1-3 from three 44-bit values.
void setMacIr(int64_t x, int64_t y, int64_t z, int shift, bool unsignedLimit)
{
    setIr(1, setMac(1, x, shift), unsignedLimit);
    setIr(2, setMac(2, y, shift), unsignedLimit);
    setIr(3, setMac(3, z, shift), unsignedLimit);
}

int32_t limitColor(int32_t value, uint32_t bit)
{
    if (value < 0 || value > 0xff) {
        g_Flag |= bit;
        value = std::clamp(value, 0, 0xff);
    }
    return value;
}

// Pushes MAC1-3 / 16 to the colour FIFO, with RGBC's code.
void pushColor()
{
    uint32_t r = limitColor(g_Data[MAC1] >> 4, kColorR);
    uint32_t g = limitColor(g_Data[MAC2] >> 4, kColorG);
    uint32_t b = limitColor(g_Data[MAC3] >> 4, kColorB);

    g_Data[RGB0] = g_Data[RGB1];
    g_Data[RGB1] = g_Data[RGB2];
    g_Data[RGB2] = static_cast<int32_t>(r | g << 8 | b << 16 | (static_cast<uint32_t>(g_Data[RGBC]) & 0xff000000));
}

void pushSz(int32_t value)
{
    if (value < 0 || value > 0xffff) {
        g_Flag |= kSz3;
        value = std::clamp(value, 0, 0xffff);
    }
    g_Data[SZ0] = g_Data[SZ1];
    g_Data[SZ1] = g_Data[SZ2];
    g_Data[SZ2] = g_Data[SZ3];
    g_Data[SZ3] = value;
}

void pushSxy(int32_t x, int32_t y)
{
    if (x < -0x400 || x > 0x3ff) {
        g_Flag |= kSx2;
        x = std::clamp(x, -0x400, 0x3ff);
    }
    if (y < -0x400 || y > 0x3ff) {
        g_Flag |= kSy2;
        y = std::clamp(y, -0x400, 0x3ff);
    }
    g_Data[SXY0] = g_Data[SXY1];
    g_Data[SXY1] = g_Data[SXY2];
    g_Data[SXY2] = (x & 0xffff) | static_cast<int32_t>(static_cast<uint32_t>(y) << 16);
}

// The hardware's division H * 0x10000 / SZ3, by Newton's method from a table.
uint32_t divide(uint32_t numerator, uint32_t denominator)
{
    static const auto table = [] {
        std::array<uint8_t, 257> values{};
        for (int i = 0; i < 257; i++) {
            values[i] = static_cast<uint8_t>(std::max(0, (0x40000 / (i + 0x100) + 1) / 2 - 0x101));
        }
        return values;
    }();

    if (denominator == 0 || numerator >= denominator * 2) {
        g_Flag |= kDivide;
        return 0x1ffff;
    }
    int shift = 0;
    while (((denominator << shift) & 0x8000) == 0) {
        shift++;
    }
    uint64_t n = static_cast<uint64_t>(numerator) << shift;
    int64_t d = static_cast<int64_t>(denominator) << shift;
    int64_t u = table[(d - 0x7fc0) >> 7] + 0x101;
    d = (0x2000080 - d * u) >> 8;
    d = (0x0000080 + d * u) >> 8;
    return static_cast<uint32_t>(std::min<uint64_t>(0x1ffff, (n * static_cast<uint64_t>(d) + 0x8000) >> 16));
}

int64_t setMac0(int64_t value)
{
    if (value > 0x7fffffffLL) {
        g_Flag |= kMac0Pos;
    } else if (value < -0x80000000LL) {
        g_Flag |= kMac0Neg;
    }
    g_Data[MAC0] = static_cast<int32_t>(value);
    return value;
}

void perspective(int v, bool lastVertex)
{
    const Matrix r = rotation();
    constexpr int kShift = 12;
    int32_t x = vectorX(v), y = vectorY(v), z = vectorZ(v);

    setMacIr((int64_t{g_Ctrl[TRX]} << 12) + r.m[0][0] * x + r.m[0][1] * y + r.m[0][2] * z,
             (int64_t{g_Ctrl[TRY]} << 12) + r.m[1][0] * x + r.m[1][1] * y + r.m[1][2] * z,
             (int64_t{g_Ctrl[TRZ]} << 12) + r.m[2][0] * x + r.m[2][1] * y + r.m[2][2] * z, kShift, false);
    pushSz(g_Data[MAC3]);

    uint32_t height = static_cast<uint16_t>(g_Ctrl[H]);
    int64_t n = divide(height, static_cast<uint32_t>(g_Data[SZ3]));
    int64_t sx = setMac0(n * static_cast<int16_t>(g_Data[IR1]) + g_Ctrl[OFX]);
    int64_t sy = setMac0(n * static_cast<int16_t>(g_Data[IR2]) + g_Ctrl[OFY]);
    pushSxy(static_cast<int32_t>(sx >> 16), static_cast<int32_t>(sy >> 16));
    if (lastVertex) {
        int64_t depth = setMac0(n * low16(g_Ctrl[DQA]) + g_Ctrl[DQB]) >> 12;
        if (depth < 0 || depth > 0x1000) {
            g_Flag |= kIr0;
            depth = std::clamp<int64_t>(depth, 0, 0x1000);
        }
        g_Data[IR0] = static_cast<int32_t>(depth);
    }
}

// MAC1-3 towards the far colour by IR0.
void interpolate(int64_t x, int64_t y, int64_t z, int shift)
{
    int32_t ir0 = g_Data[IR0];

    setIr(1, setMac(1, (int64_t{g_Ctrl[RFC]} << 12) - x, shift), false);
    setIr(2, setMac(2, (int64_t{g_Ctrl[GFC]} << 12) - y, shift), false);
    setIr(3, setMac(3, (int64_t{g_Ctrl[BFC]} << 12) - z, shift), false);
    setMacIr(g_Data[IR1] * int64_t{ir0} + x, g_Data[IR2] * int64_t{ir0} + y, g_Data[IR3] * int64_t{ir0} + z, shift,
             false);
    pushColor();
}

void depthCue(uint32_t rgb, int shift)
{
    interpolate(int64_t{rgb & 0xff} << 16, int64_t{(rgb >> 8) & 0xff} << 16, int64_t{(rgb >> 16) & 0xff} << 16, shift);
}

}  // namespace

extern "C" {

void gte_SetDataReg(int reg, int value)
{
    switch (reg) {
    case VZ0:
    case VZ1:
    case VZ2:
    case IR0:
    case IR1:
    case IR2:
    case IR3:
        g_Data[reg] = low16(value);
        break;
    case OTZ:
    case SZ0:
    case SZ1:
    case SZ2:
    case SZ3:
        g_Data[reg] = value & 0xffff;
        break;
    case SXYP:
        g_Data[SXY0] = g_Data[SXY1];
        g_Data[SXY1] = g_Data[SXY2];
        g_Data[SXY2] = value;
        break;
    case IRGB:
        g_Data[IRGB] = value & 0x7fff;
        g_Data[IR1] = (value & 0x1f) << 7;
        g_Data[IR2] = ((value >> 5) & 0x1f) << 7;
        g_Data[IR3] = ((value >> 10) & 0x1f) << 7;
        break;
    case ORGB:
    case LZCR:
        break;
    case LZCS: {
        g_Data[LZCS] = value;
        uint32_t bits = static_cast<uint32_t>(value < 0 ? ~value : value);
        int count = 0;
        while (count < 32 && (bits & 0x80000000u) == 0) {
            bits <<= 1;
            count++;
        }
        g_Data[LZCR] = count;
        break;
    }
    default:
        g_Data[reg] = value;
        break;
    }
}

int gte_GetDataReg(int reg)
{
    switch (reg) {
    case SXYP:
        return g_Data[SXY2];
    case IRGB:
    case ORGB: {
        int r = std::clamp(g_Data[IR1] >> 7, 0, 0x1f);
        int g = std::clamp(g_Data[IR2] >> 7, 0, 0x1f);
        int b = std::clamp(g_Data[IR3] >> 7, 0, 0x1f);
        return r | g << 5 | b << 10;
    }
    default:
        return g_Data[reg];
    }
}

void gte_SetCtrlReg(int reg, int value)
{
    switch (reg) {
    case R33:
    case L33:
    case LB3:
    case H:
    case DQA:
    case ZSF3:
    case ZSF4:
        g_Ctrl[reg] = low16(value);
        break;
    case FLAG:
        g_Flag = static_cast<uint32_t>(value) & 0x7ffff000;
        break;
    default:
        g_Ctrl[reg] = value;
        break;
    }
}

int gte_GetCtrlReg(int reg)
{
    if (reg == FLAG) {
        uint32_t error = g_Flag & 0x7f87e000;
        return static_cast<int>(g_Flag | (error != 0 ? 0x80000000u : 0));
    }
    return g_Ctrl[reg];
}

void gte_LoadDataReg(int reg, const void *ptr)
{
    int value;

    std::memcpy(&value, ptr, 4);
    gte_SetDataReg(reg, value);
}

void gte_LoadDataRegAt(int reg, const void *ptr, int offset)
{
    gte_LoadDataReg(reg, static_cast<const char *>(ptr) + offset);
}

void gte_StoreDataReg(int reg, void *ptr)
{
    int value = gte_GetDataReg(reg);

    std::memcpy(ptr, &value, 4);
}

int gte_Lzc(int value)
{
    gte_SetDataReg(LZCS, value);
    return g_Data[LZCR];
}

void InitGeom(void)
{
    g_Ctrl[ZSF3] = 0x155;
    g_Ctrl[ZSF4] = 0x100;
    g_Ctrl[H] = 1000;
    g_Ctrl[DQA] = -0x1062;
    g_Ctrl[DQB] = 0x1400000;
    g_Ctrl[OFX] = 0;
    g_Ctrl[OFY] = 0;
}

void gte_rtps(void)
{
    g_Flag = 0;
    perspective(0, true);
}

void gte_rtpt(void)
{
    g_Flag = 0;
    perspective(0, false);
    perspective(1, false);
    perspective(2, true);
}

void gte_nclip(void)
{
    int64_t x0 = low16(g_Data[SXY0]), y0 = high16(g_Data[SXY0]);
    int64_t x1 = low16(g_Data[SXY1]), y1 = high16(g_Data[SXY1]);
    int64_t x2 = low16(g_Data[SXY2]), y2 = high16(g_Data[SXY2]);

    g_Flag = 0;
    setMac0(x0 * y1 + x1 * y2 + x2 * y0 - x0 * y2 - x1 * y0 - x2 * y1);
}

void gte_dpcs(void)
{
    g_Flag = 0;
    depthCue(static_cast<uint32_t>(g_Data[RGBC]), 12);
}

void gte_dpct(void)
{
    g_Flag = 0;
    for (int i = 0; i < 3; i++) {
        depthCue(static_cast<uint32_t>(g_Data[RGB0]), 12);
    }
}

void gte_intpl(void)
{
    g_Flag = 0;
    interpolate(int64_t{g_Data[IR1]} << 12, int64_t{g_Data[IR2]} << 12, int64_t{g_Data[IR3]} << 12, 12);
}

void gte_mvmva(int sf, int mx, int v, int cv, int lm)
{
    int shift = sf != 0 ? 12 : 0;
    Matrix m = mx == 0 ? rotation() : mx == 1 ? lightMatrix() : colorMatrix();
    int64_t x = v == 3 ? g_Data[IR1] : vectorX(v);
    int64_t y = v == 3 ? g_Data[IR2] : vectorY(v);
    int64_t z = v == 3 ? g_Data[IR3] : vectorZ(v);
    int64_t t[3] = {};

    g_Flag = 0;
    if (cv == 0) {
        t[0] = g_Ctrl[TRX];
        t[1] = g_Ctrl[TRY];
        t[2] = g_Ctrl[TRZ];
    } else if (cv == 1) {
        t[0] = g_Ctrl[RBK];
        t[1] = g_Ctrl[GBK];
        t[2] = g_Ctrl[BBK];
    }
    setMacIr((t[0] << 12) + m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z,
             (t[1] << 12) + m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z,
             (t[2] << 12) + m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z, shift, lm != 0);
}

void gte_sqr(int sf)
{
    int shift = sf != 0 ? 12 : 0;
    int64_t x = g_Data[IR1], y = g_Data[IR2], z = g_Data[IR3];

    g_Flag = 0;
    setMacIr(x * x, y * y, z * z, shift, true);
}

void gte_gpf(int sf)
{
    int shift = sf != 0 ? 12 : 0;
    int64_t ir0 = g_Data[IR0];

    g_Flag = 0;
    setMacIr(g_Data[IR1] * ir0, g_Data[IR2] * ir0, g_Data[IR3] * ir0, shift, false);
    pushColor();
}

void gte_gpl(int sf)
{
    int shift = sf != 0 ? 12 : 0;
    int64_t ir0 = g_Data[IR0];

    g_Flag = 0;
    setMacIr((int64_t{g_Data[MAC1]} << shift) + g_Data[IR1] * ir0, (int64_t{g_Data[MAC2]} << shift) + g_Data[IR2] * ir0,
             (int64_t{g_Data[MAC3]} << shift) + g_Data[IR3] * ir0, shift, false);
    pushColor();
}

// Links a primitive, or a chain, into an OT entry: first takes the entry's place in the
// list, and last links to what the entry linked to.
void ot_AddPrims(unsigned int *ot, void *first, void *last)
{
    auto *tail = static_cast<unsigned int *>(last);
    unsigned int next = *ot & 0xffffff;

    *ot = (*ot & 0xff000000) | (rt::to_guest(first) & 0xffffff);
    *tail = (*tail & 0xff000000) | next;
}

void ot_AddPrim(unsigned int *ot, void *prim)
{
    ot_AddPrims(ot, prim, prim);
}

}  // extern "C"
