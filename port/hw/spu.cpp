// SPDX-License-Identifier: GPL-2.0-or-later
// The sound chip, written from the psx-spx reference. The game writes its registers into
// g_HwRegs as plain memory, so this reads them once a vblank, runs the 44100 Hz clock for that
// vblank's samples and writes back what the game reads. Everything here runs on the vblank
// count and never on the host's clock: the envelopes the game reads have to be the same in
// every run, or recordings would drift.
#include "hw.h"
#include "game/include/hw.h"
#include "host.h"
#include "rt.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

constexpr int kVoices = 24;
constexpr int kRamSize = 0x80000;
constexpr int kSampleRate = 44100;
constexpr int kBlockSamples = 28;
constexpr int kBlockBytes = 16;
constexpr int kHistory = 3;  // the samples the interpolation reaches back for

// Register offsets from the start of the I/O page.
enum Reg {
    kVoiceRegs = 0xc00, kVoiceVolL = 0, kVoiceVolR = 2, kVoicePitch = 4, kVoiceStart = 6, kVoiceAdsr1 = 8,
    kVoiceAdsr2 = 0xa, kVoiceEnvx = 0xc, kVoiceRepeat = 0xe, kVoiceStep = 0x10,
    kMainVolL = 0xd80, kMainVolR = 0xd82, kReverbVolL = 0xd84, kReverbVolR = 0xd86, kKeyOn = 0xd88, kKeyOff = 0xd8c,
    kPitchMod = 0xd90, kNoiseOn = 0xd94, kReverbOn = 0xd98, kVoiceEnd = 0xd9c, kReverbBase = 0xda2, kTransferAddr = 0xda6,
    kControl = 0xdaa, kStatus = 0xdae, kCdVolL = 0xdb0, kCdVolR = 0xdb2, kReverbConfig = 0xdc0,
};
enum ControlBits {
    kEnable = 0x8000, kUnmute = 0x4000, kReverbMaster = 0x80, kCdReverb = 0x4, kCdInput = 0x1, kStatusMirror = 0x3f,
};
enum BlockFlags { kLoopEnd = 1, kLoopRepeat = 2, kLoopStart = 4 };

// The reverb registers, in register order.
struct ReverbConfig {
    std::uint16_t dAPF1, dAPF2;
    std::int16_t vIIR, vCOMB1, vCOMB2, vCOMB3, vCOMB4, vWALL, vAPF1, vAPF2;
    std::uint16_t mLSAME, mRSAME, mLCOMB1, mRCOMB1, mLCOMB2, mRCOMB2, dLSAME, dRSAME, mLDIFF, mRDIFF, mLCOMB3, mRCOMB3,
        mLCOMB4, mRCOMB4, dLDIFF, dRDIFF, mLAPF1, mRAPF1, mLAPF2, mRAPF2;
    std::int16_t vLIN, vRIN;
};
static_assert(sizeof(ReverbConfig) == 0x40);

constexpr std::int16_t kGauss[512] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5,
    5, 6, 7, 7, 8, 9, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 21, 22, 24, 25, 27, 28, 30,
    32, 33, 35, 37, 39, 41, 44, 46, 48, 51, 53, 56,
    58, 61, 64, 67, 70, 73, 77, 80, 84, 87, 91, 95,
    99, 103, 107, 111, 116, 120, 125, 130, 135, 140, 145, 150,
    156, 161, 167, 173, 179, 186, 192, 199, 205, 212, 219, 227,
    234, 242, 250, 257, 266, 274, 283, 291, 300, 309, 319, 328,
    338, 348, 358, 369, 379, 390, 401, 412, 424, 436, 448, 460,
    473, 485, 498, 512, 525, 539, 553, 567, 582, 597, 612, 627,
    643, 659, 675, 692, 708, 726, 743, 761, 779, 797, 816, 835,
    854, 874, 894, 914, 935, 956, 977, 999, 1020, 1043, 1066, 1089,
    1112, 1136, 1160, 1184, 1209, 1234, 1260, 1286, 1312, 1339, 1366, 1394,
    1422, 1450, 1479, 1508, 1537, 1567, 1598, 1628, 1660, 1691, 1723, 1756,
    1789, 1822, 1856, 1890, 1924, 1959, 1995, 2031, 2067, 2104, 2141, 2179,
    2217, 2256, 2295, 2334, 2374, 2415, 2456, 2497, 2539, 2582, 2624, 2668,
    2712, 2756, 2801, 2846, 2892, 2938, 2985, 3032, 3079, 3128, 3176, 3225,
    3275, 3325, 3376, 3427, 3479, 3531, 3584, 3637, 3691, 3745, 3799, 3855,
    3910, 3967, 4023, 4081, 4138, 4197, 4255, 4315, 4374, 4435, 4495, 4557,
    4619, 4681, 4744, 4807, 4871, 4935, 5000, 5065, 5131, 5197, 5264, 5332,
    5399, 5468, 5536, 5606, 5676, 5746, 5817, 5888, 5959, 6032, 6104, 6177,
    6251, 6325, 6400, 6475, 6550, 6626, 6702, 6779, 6856, 6934, 7012, 7091,
    7170, 7249, 7329, 7409, 7490, 7571, 7653, 7735, 7817, 7900, 7983, 8066,
    8150, 8234, 8319, 8404, 8489, 8575, 8661, 8748, 8834, 8922, 9009, 9097,
    9185, 9273, 9362, 9451, 9541, 9630, 9720, 9811, 9901, 9992, 10083, 10174,
    10266, 10358, 10450, 10542, 10635, 10727, 10820, 10913, 11007, 11100, 11194, 11288,
    11382, 11476, 11571, 11665, 11760, 11855, 11950, 12045, 12140, 12236, 12331, 12427,
    12522, 12618, 12714, 12809, 12905, 13001, 13097, 13193, 13289, 13385, 13481, 13577,
    13673, 13769, 13865, 13961, 14056, 14152, 14248, 14343, 14439, 14534, 14630, 14725,
    14820, 14915, 15010, 15104, 15199, 15293, 15387, 15481, 15575, 15669, 15762, 15855,
    15948, 16041, 16133, 16226, 16317, 16409, 16500, 16592, 16682, 16773, 16863, 16953,
    17042, 17131, 17220, 17308, 17396, 17484, 17571, 17658, 17744, 17830, 17916, 18001,
    18086, 18170, 18254, 18337, 18420, 18502, 18584, 18665, 18746, 18826, 18905, 18985,
    19063, 19141, 19219, 19295, 19372, 19447, 19522, 19597, 19671, 19744, 19816, 19888,
    19959, 20030, 20100, 20169, 20238, 20306, 20373, 20439, 20505, 20570, 20634, 20698,
    20760, 20822, 20884, 20944, 21004, 21063, 21121, 21178, 21235, 21290, 21345, 21399,
    21452, 21505, 21556, 21607, 21657, 21706, 21754, 21801, 21848, 21893, 21938, 21982,
    22025, 22066, 22107, 22148, 22187, 22225, 22262, 22299, 22334, 22369, 22402, 22435,
    22467, 22498, 22527, 22556, 22584, 22611, 22637, 22662, 22686, 22709, 22731, 22752,
    22772, 22791, 22809, 22826, 22842, 22857, 22872, 22885, 22897, 22908, 22918, 22927,
    22935, 22942, 22948, 22953, 22957, 22960, 22962, 22963,
};

// The reverb's resampling filter, on both sides of it.
constexpr int kFirTaps = 39;
constexpr std::int16_t kFir[kFirTaps] = {
    -0x0001, 0, 0x0002, 0, -0x000a, 0, 0x0023, 0, -0x0067, 0, 0x010a, 0, -0x0268, 0, 0x0534, 0, -0x0b90, 0, 0x2806,
    0x4000, 0x2806, 0, -0x0b90, 0, 0x0534, 0, -0x0268, 0, 0x010a, 0, -0x0067, 0, 0x0023, 0, -0x000a, 0, 0x0002, 0, -0x0001,
};

// The prediction filters, shared by SPU and XA ADPCM.
constexpr int kFilterPos[5] = {0, 60, 115, 98, 122};
constexpr int kFilterNeg[5] = {0, 0, -52, -55, -60};

struct Level {
    int value = 0;
    unsigned int counter = 0;
};

enum Phase { kOff, kAttack, kDecay, kSustain, kRelease };

struct Voice {
    Phase phase = kOff;
    std::uint32_t addr = 0;
    std::uint32_t counter = 0;
    std::int16_t samples[kHistory + kBlockSamples] = {};
    int old = 0;
    int older = 0;
    int out = 0;  // the sample after its envelope: the next voice's pitch modulation
    std::uint8_t flags = 0;
    Level envelope;
    Level left, right;
};

// A stereo ring of decoded XA, ahead of the mixer by up to a few sectors.
constexpr unsigned int kXaRingFrames = 1 << 15;
constexpr int kXaUnitsPerGroup4 = 8;
constexpr int kXaUnitsPerGroup8 = 4;
constexpr int kXaGroups = 18;
constexpr int kXaGroupBytes = 128;
constexpr int kXaDataOffset = 24;
constexpr int kXaCodingOffset = 19;

struct XaChannel {
    int old = 0;
    int older = 0;
};

std::array<std::uint8_t, kRamSize> g_Ram;
Voice g_Voices[kVoices];
Level g_MainLeft, g_MainRight;
std::uint32_t g_EndFlags;
std::uint32_t g_SampleCarry;
int g_NoiseLevel = 1;
int g_NoiseTimer;

int g_ReverbAddr;
int g_ReverbBase;
bool g_ReverbTick;
int g_ReverbIn[2][kFirTaps];
int g_ReverbOut[2][kFirTaps];
int g_ReverbPos;

std::int16_t g_Xa[kXaRingFrames][2];
unsigned int g_XaRead, g_XaWrite;
int g_XaRate = 37800;
unsigned int g_XaStart = 1;  // frames to hold before playing, so that a late sector doesn't click
bool g_XaPlaying;
XaChannel g_XaChannels[2];
std::int16_t g_XaBlend[4][2];
std::uint32_t g_XaPhase;

FILE *g_Dump;

std::uint16_t reg(int offset)
{
    std::uint16_t value;

    std::memcpy(&value, g_HwRegs + offset, sizeof(value));
    return value;
}

void setReg(int offset, std::uint16_t value)
{
    std::memcpy(g_HwRegs + offset, &value, sizeof(value));
}

std::uint32_t reg32(int offset)
{
    return reg(offset) | static_cast<std::uint32_t>(reg(offset + 2)) << 16;
}

int clamp16(int value)
{
    return std::clamp(value, -0x8000, 0x7fff);
}

int scale(int value, int volume)
{
    return (value * volume) >> 15;
}

// A step of an envelope or a volume sweep.
void stepLevel(Level &level, int shift, int stepValue, bool exponential, bool decreasing)
{
    int step = 7 - stepValue;
    unsigned int increment = 0x8000u >> std::max(0, shift - 11);

    if (decreasing) {
        step = ~step;
    }
    step *= 1 << std::max(0, 11 - shift);
    if (exponential && !decreasing && level.value > 0x6000) {
        if (shift < 10) {
            step /= 4;
        } else if (shift >= 11) {
            increment /= 4;
        } else {
            step /= 2;
            increment /= 2;
        }
    } else if (exponential && decreasing) {
        step = (step * level.value) >> 15;
    }
    if ((stepValue | shift << 2) != 0x7f) {
        increment = std::max(increment, 1u);
    }
    level.counter += increment;
    if (!(level.counter & 0x8000)) {
        return;
    }
    level.counter = 0;
    level.value += step;
    level.value = decreasing ? std::max(level.value, 0) : clamp16(level.value);
}

void stepVolume(Level &level, std::uint16_t value)
{
    if (value & 0x8000) {
        stepLevel(level, (value >> 2) & 0x1f, value & 3, (value & 0x4000) != 0, (value & 0x2000) != 0);
    } else {
        level.value = static_cast<std::int16_t>(value << 1);
    }
}

void decodeBlock(Voice &voice, int index)
{
    const std::uint8_t *block = &g_Ram[voice.addr & (kRamSize - kBlockBytes)];
    int shift = block[0] & 0xf;
    int filter = std::min((block[0] >> 4) & 7, 4);

    if (shift > 12) {
        shift = 9;
    }
    voice.flags = block[1];
    if (voice.flags & kLoopStart) {
        setReg(kVoiceRegs + index * kVoiceStep + kVoiceRepeat, static_cast<std::uint16_t>(voice.addr >> 3));
    }
    std::memcpy(voice.samples, voice.samples + kBlockSamples, kHistory * sizeof(voice.samples[0]));
    for (int i = 0; i < kBlockSamples; i++) {
        int nibble = (block[2 + i / 2] >> ((i & 1) * 4)) & 0xf;
        int sample = static_cast<std::int16_t>(nibble << 12) >> shift;

        sample += (voice.old * kFilterPos[filter] + voice.older * kFilterNeg[filter] + 32) >> 6;
        sample = clamp16(sample);
        voice.older = voice.old;
        voice.old = sample;
        voice.samples[kHistory + i] = static_cast<std::int16_t>(sample);
    }
}

void keyOn(int index)
{
    Voice &voice = g_Voices[index];

    voice.addr = static_cast<std::uint32_t>(reg(kVoiceRegs + index * kVoiceStep + kVoiceStart)) << 3;
    voice.counter = 0;
    voice.old = voice.older = 0;
    std::memset(voice.samples, 0, sizeof(voice.samples));
    voice.envelope = {};
    voice.phase = kAttack;
    g_EndFlags &= ~(1u << index);
    decodeBlock(voice, index);
}

void nextBlock(Voice &voice, int index)
{
    if (voice.flags & kLoopEnd) {
        g_EndFlags |= 1u << index;
        voice.addr = static_cast<std::uint32_t>(reg(kVoiceRegs + index * kVoiceStep + kVoiceRepeat)) << 3;
        if (!(voice.flags & kLoopRepeat)) {
            voice.phase = kOff;
            voice.envelope.value = 0;
        }
    } else {
        voice.addr = (voice.addr + kBlockBytes) & (kRamSize - 1);
    }
    decodeBlock(voice, index);
}

void stepEnvelope(Voice &voice, std::uint32_t adsr)
{
    Level &level = voice.envelope;

    switch (voice.phase) {
    case kAttack:
        stepLevel(level, (adsr >> 10) & 0x1f, (adsr >> 8) & 3, (adsr & 0x8000) != 0, false);
        if (level.value >= 0x7fff) {
            voice.phase = kDecay;
            level.counter = 0;
        }
        break;
    case kDecay:
        stepLevel(level, (adsr >> 4) & 0xf, 0, true, true);
        if (level.value <= static_cast<int>((adsr & 0xf) + 1) * 0x800) {
            voice.phase = kSustain;
            level.counter = 0;
        }
        break;
    case kSustain:
        stepLevel(level, (adsr >> 24) & 0x1f, (adsr >> 22) & 3, (adsr & 0x80000000u) != 0, (adsr & 0x40000000u) != 0);
        break;
    case kRelease:
        stepLevel(level, (adsr >> 16) & 0x1f, 0, (adsr & 0x200000u) != 0, true);
        if (level.value <= 0) {
            voice.phase = kOff;
        }
        break;
    case kOff:
        break;
    }
}

int interpolate(const Voice &voice)
{
    int i = (voice.counter >> 4) & 0xff;
    const std::int16_t *s = voice.samples + (voice.counter >> 12);
    int out = (kGauss[0x0ff - i] * s[0]) >> 15;

    out += (kGauss[0x1ff - i] * s[1]) >> 15;
    out += (kGauss[0x100 + i] * s[2]) >> 15;
    out += (kGauss[0x000 + i] * s[3]) >> 15;
    return out;
}

void stepNoise(std::uint16_t control)
{
    int shift = (control >> 10) & 0xf;
    int step = ((control >> 8) & 3) + 4;
    int reload = 0x20000 >> shift;
    int parity = ((g_NoiseLevel >> 15) ^ (g_NoiseLevel >> 12) ^ (g_NoiseLevel >> 11) ^ (g_NoiseLevel >> 10) ^ 1) & 1;

    g_NoiseTimer -= step;
    if (g_NoiseTimer < 0) {
        g_NoiseLevel = static_cast<std::uint16_t>(g_NoiseLevel * 2 + parity);
        g_NoiseTimer += reload;
        if (g_NoiseTimer < 0) {
            g_NoiseTimer += reload;
        }
    }
}

// The reverb work area, by byte offset from its current position. It wraps inside its region.
int reverbAddr(int offset)
{
    int size = kRamSize - g_ReverbBase;
    int position = (g_ReverbAddr - g_ReverbBase + offset) % size;

    return (g_ReverbBase + (position < 0 ? position + size : position)) & (kRamSize - 2);
}

int reverbRead(int offset)
{
    std::int16_t value;

    std::memcpy(&value, &g_Ram[reverbAddr(offset)], sizeof(value));
    return value;
}

void reverbWrite(int offset, int sample)
{
    auto value = static_cast<std::int16_t>(clamp16(sample));

    std::memcpy(&g_Ram[reverbAddr(offset)], &value, sizeof(value));
}

// The addresses of one side of the reverb, in bytes.
struct ReverbSide {
    int same, sameIn, diff, diffIn, comb1, comb2, comb3, comb4, apf1, apf2;
};

// One 22050 Hz step for one side. Takes the input and gives the output.
int reverbStep(const ReverbConfig &c, const ReverbSide &side, bool write, int in)
{
    constexpr int kPrevious = -2;

    if (write) {
        int lastSame = reverbRead(side.same + kPrevious);
        int lastDiff = reverbRead(side.diff + kPrevious);

        reverbWrite(side.same, scale(in + scale(reverbRead(side.sameIn), c.vWALL) - lastSame, c.vIIR) + lastSame);
        reverbWrite(side.diff, scale(in + scale(reverbRead(side.diffIn), c.vWALL) - lastDiff, c.vIIR) + lastDiff);
    }

    int out = scale(reverbRead(side.comb1), c.vCOMB1) + scale(reverbRead(side.comb2), c.vCOMB2) +
              scale(reverbRead(side.comb3), c.vCOMB3) + scale(reverbRead(side.comb4), c.vCOMB4);
    int back1 = reverbRead(side.apf1 - c.dAPF1 * 8);
    int back2 = reverbRead(side.apf2 - c.dAPF2 * 8);

    out = clamp16(out - scale(back1, c.vAPF1));
    if (write) {
        reverbWrite(side.apf1, out);
    }
    out = clamp16(scale(out, c.vAPF1) + back1);
    out = clamp16(out - scale(back2, c.vAPF2));
    if (write) {
        reverbWrite(side.apf2, out);
    }
    return clamp16(scale(out, c.vAPF2) + back2);
}

int firAt(const int *history, int newest)
{
    int sum = 0;

    for (int tap = 0; tap < kFirTaps; tap++) {
        if (kFir[tap] != 0) {
            sum += kFir[tap] * history[(newest + tap) % kFirTaps];
        }
    }
    return sum;
}

// Takes one 44100 Hz stereo sample and gives the reverb's next one. The reverb itself runs at
// half that rate; hardware takes turns between its sides, which this doesn't do.
void reverbTick(const ReverbConfig &c, bool write, int inLeft, int inRight, int *outLeft, int *outRight)
{
    g_ReverbPos = (g_ReverbPos + kFirTaps - 1) % kFirTaps;
    g_ReverbIn[0][g_ReverbPos] = inLeft;
    g_ReverbIn[1][g_ReverbPos] = inRight;
    g_ReverbOut[0][g_ReverbPos] = 0;
    g_ReverbOut[1][g_ReverbPos] = 0;
    g_ReverbTick = !g_ReverbTick;
    if (g_ReverbTick) {
        const ReverbSide left = {c.mLSAME * 8, c.dLSAME * 8, c.mLDIFF * 8, c.dRDIFF * 8, c.mLCOMB1 * 8,
                                 c.mLCOMB2 * 8, c.mLCOMB3 * 8, c.mLCOMB4 * 8, c.mLAPF1 * 8, c.mLAPF2 * 8};
        const ReverbSide right = {c.mRSAME * 8, c.dRSAME * 8, c.mRDIFF * 8, c.dLDIFF * 8, c.mRCOMB1 * 8,
                                  c.mRCOMB2 * 8, c.mRCOMB3 * 8, c.mRCOMB4 * 8, c.mRAPF1 * 8, c.mRAPF2 * 8};

        g_ReverbOut[0][g_ReverbPos] = reverbStep(c, left, write, scale(firAt(g_ReverbIn[0], g_ReverbPos) >> 15, c.vLIN));
        g_ReverbOut[1][g_ReverbPos] = reverbStep(c, right, write, scale(firAt(g_ReverbIn[1], g_ReverbPos) >> 15, c.vRIN));
        g_ReverbAddr += 2;
        if (g_ReverbAddr >= kRamSize) {
            g_ReverbAddr = g_ReverbBase;
        }
    }
    *outLeft = clamp16(firAt(g_ReverbOut[0], g_ReverbPos) >> 14);
    *outRight = clamp16(firAt(g_ReverbOut[1], g_ReverbPos) >> 14);
}

// The next CD sample at 44100 Hz, blended between the XA samples around it. Hardware has its
// own filter for this, which isn't reproduced.
void xaTick(int *left, int *right)
{
    constexpr std::uint32_t kPhaseOne = 1 << 16;

    g_XaPhase += static_cast<std::uint32_t>((static_cast<long long>(g_XaRate) << 16) / kSampleRate);
    while (g_XaPhase >= kPhaseOne) {
        g_XaPhase -= kPhaseOne;
        std::memmove(g_XaBlend[0], g_XaBlend[1], sizeof(g_XaBlend) - sizeof(g_XaBlend[0]));
        unsigned int held = g_XaWrite - g_XaRead;
        if (g_XaPlaying && held == 0) {
            g_XaPlaying = false;
        } else if (!g_XaPlaying && held >= g_XaStart) {
            g_XaPlaying = true;
        }
        if (g_XaPlaying) {
            std::memcpy(g_XaBlend[3], g_Xa[g_XaRead % kXaRingFrames], sizeof(g_XaBlend[3]));
            g_XaRead++;
        } else {
            g_XaBlend[3][0] = g_XaBlend[3][1] = 0;
        }
    }
    // Catmull-Rom between the middle two.
    float t = static_cast<float>(g_XaPhase) / kPhaseOne;
    for (int side = 0; side < 2; side++) {
        float a = g_XaBlend[0][side], b = g_XaBlend[1][side], c = g_XaBlend[2][side], d = g_XaBlend[3][side];
        float value = b + 0.5f * t * (c - a + t * (2 * a - 5 * b + 4 * c - d + t * (3 * (b - c) + d - a)));
        (side == 0 ? *left : *right) = clamp16(static_cast<int>(value));
    }
}

// What stays the same for a vblank's samples.
struct MixState {
    std::uint32_t adsr[kVoices];
    std::uint32_t pitchMod, noiseOn, reverbOn;
    std::uint16_t control;
    ReverbConfig reverb;
    bool reverbWrites;
};

void mixSample(const MixState &mix, std::int16_t *out)
{
    int dryLeft = 0, dryRight = 0, wetLeft = 0, wetRight = 0;
    int previousOut = 0;

    stepNoise(mix.control);
    for (int i = 0; i < kVoices; i++) {
        Voice &voice = g_Voices[i];
        int base = kVoiceRegs + i * kVoiceStep;

        stepVolume(voice.left, reg(base + kVoiceVolL));
        stepVolume(voice.right, reg(base + kVoiceVolR));
        if (voice.phase == kOff) {
            voice.out = previousOut = 0;
            continue;
        }
        int sample = mix.noiseOn >> i & 1 ? static_cast<std::int16_t>(g_NoiseLevel) : interpolate(voice);
        stepEnvelope(voice, mix.adsr[i]);
        voice.out = scale(sample, voice.envelope.value);

        int left = scale(voice.out, voice.left.value);
        int right = scale(voice.out, voice.right.value);
        dryLeft += left;
        dryRight += right;
        if (mix.reverbOn >> i & 1) {
            wetLeft += left;
            wetRight += right;
        }

        std::uint32_t step = reg(base + kVoicePitch);
        if ((mix.pitchMod >> i & 1) && i > 0) {
            int factor = previousOut + 0x8000;
            step = static_cast<std::uint32_t>((static_cast<std::int16_t>(step) * factor) >> 15) & 0xffff;
        }
        voice.counter += std::min<std::uint32_t>(step, 0x4000);
        while (voice.counter >> 12 >= kBlockSamples) {
            voice.counter -= kBlockSamples << 12;
            nextBlock(voice, i);
        }
        previousOut = voice.out;
    }

    int cdLeft, cdRight;
    xaTick(&cdLeft, &cdRight);
    if (!(mix.control & kEnable) || !(mix.control & kUnmute)) {
        dryLeft = dryRight = wetLeft = wetRight = 0;
    }
    if (mix.control & kCdInput) {
        cdLeft = scale(cdLeft, static_cast<std::int16_t>(reg(kCdVolL)));
        cdRight = scale(cdRight, static_cast<std::int16_t>(reg(kCdVolR)));
    } else {
        cdLeft = cdRight = 0;
    }
    if (mix.control & kCdReverb) {
        wetLeft += cdLeft;
        wetRight += cdRight;
    }
    dryLeft += cdLeft;
    dryRight += cdRight;

    int reverbLeft, reverbRight;
    reverbTick(mix.reverb, mix.reverbWrites, clamp16(wetLeft), clamp16(wetRight), &reverbLeft, &reverbRight);
    dryLeft += scale(reverbLeft, static_cast<std::int16_t>(reg(kReverbVolL)));
    dryRight += scale(reverbRight, static_cast<std::int16_t>(reg(kReverbVolR)));

    stepVolume(g_MainLeft, reg(kMainVolL));
    stepVolume(g_MainRight, reg(kMainVolR));
    out[0] = static_cast<std::int16_t>(clamp16(scale(clamp16(dryLeft), g_MainLeft.value)));
    out[1] = static_cast<std::int16_t>(clamp16(scale(clamp16(dryRight), g_MainRight.value)));
}

// Off keys first: the sound driver writes both for a voice it restarts.
void applyKeys()
{
    std::uint32_t off = reg32(kKeyOff);
    std::uint32_t on = reg32(kKeyOn);

    for (int i = 0; i < kVoices; i++) {
        if ((off >> i & 1) && g_Voices[i].phase != kOff) {
            g_Voices[i].phase = kRelease;
            g_Voices[i].envelope.counter = 0;
        }
        if (on >> i & 1) {
            keyOn(i);
        }
    }
    setReg(kKeyOn, 0);
    setReg(kKeyOn + 2, 0);
    setReg(kKeyOff, 0);
    setReg(kKeyOff + 2, 0);
}

void writeBack()
{
    for (int i = 0; i < kVoices; i++) {
        setReg(kVoiceRegs + i * kVoiceStep + kVoiceEnvx, static_cast<std::uint16_t>(g_Voices[i].envelope.value));
    }
    setReg(kVoiceEnd, static_cast<std::uint16_t>(g_EndFlags));
    setReg(kVoiceEnd + 2, static_cast<std::uint16_t>(g_EndFlags >> 16));
    setReg(kStatus, reg(kControl) & kStatusMirror);
}

}  // namespace

void Hw_SpuDumpTo(const char *path)
{
    g_Dump = std::fopen(path, "wb");
    if (g_Dump == nullptr) {
        Rt_Fatal("cannot write %s", path);
    }
}

extern "C" {

void Hw_SpuDma(void *guest, unsigned int bytes, int toGuest)
{
    std::uint32_t addr = static_cast<std::uint32_t>(reg(kTransferAddr)) << 3;
    auto *data = static_cast<std::uint8_t *>(guest);

    for (unsigned int i = 0; i < bytes; i++) {
        std::uint8_t &byte = g_Ram[(addr + i) & (kRamSize - 1)];

        if (toGuest) {
            data[i] = byte;
        } else {
            byte = data[i];
        }
    }
}

void Hw_SpuXaClear(void)
{
    g_XaRead = g_XaWrite = 0;
    g_XaPlaying = false;
}

void Hw_SpuXaSector(const unsigned char *sector)
{
    int coding = sector[kXaCodingOffset];
    bool stereo = (coding & 1) != 0;
    bool eightBit = ((coding >> 4) & 3) == 1;
    int units = eightBit ? kXaUnitsPerGroup8 : kXaUnitsPerGroup4;
    unsigned int frames = kXaGroups * units * kBlockSamples / (stereo ? 2 : 1);
    bool full = g_XaWrite - g_XaRead + frames > kXaRingFrames;  // nobody is taking them
    std::int16_t decoded[kBlockSamples];

    g_XaRate = ((coding >> 2) & 3) == 1 ? 18900 : 37800;
    g_XaStart = 2 * frames;
    for (int group = 0; group < kXaGroups; group++) {
        const unsigned char *data = sector + kXaDataOffset + group * kXaGroupBytes;

        for (int unit = 0; unit < units; unit++) {
            XaChannel &channel = g_XaChannels[stereo ? unit & 1 : 0];
            int header = data[4 + unit];
            int shift = header & 0xf;
            int filter = (header >> 4) & 3;

            if (shift > 12) {
                shift = 9;
            }
            for (int i = 0; i < kBlockSamples; i++) {
                int sample;

                if (eightBit) {
                    sample = static_cast<std::int16_t>(data[16 + i * 4 + unit] << 8) >> shift;
                } else {
                    int nibble = (data[16 + i * 4 + unit / 2] >> ((unit & 1) * 4)) & 0xf;
                    sample = static_cast<std::int16_t>(nibble << 12) >> shift;
                }
                sample += (channel.old * kFilterPos[filter] + channel.older * kFilterNeg[filter] + 32) >> 6;
                sample = clamp16(sample);
                channel.older = channel.old;
                channel.old = sample;
                decoded[i] = static_cast<std::int16_t>(sample);
            }
            if (full) {
                continue;
            }
            bool rightOfStereo = stereo && (unit & 1) != 0;
            // The right channel fills in the frames the left one has just started.
            unsigned int first = g_XaWrite - (rightOfStereo ? kBlockSamples : 0);
            for (int i = 0; i < kBlockSamples; i++) {
                std::int16_t *frame = g_Xa[(first + i) % kXaRingFrames];

                if (stereo) {
                    frame[unit & 1] = decoded[i];
                } else {
                    frame[0] = frame[1] = decoded[i];
                }
            }
            if (!rightOfStereo) {
                g_XaWrite += kBlockSamples;
            }
        }
    }
}

void Hw_SpuVblank(void)
{
    constexpr int kMaxSamples = 1024;
    // 59.94 vblanks a second: the game's clock, in rt/irq.cpp.
    constexpr std::uint32_t kPerVblank = kSampleRate * 1001u;
    constexpr std::uint32_t kPerSecond = 60000;

    static std::int16_t buffer[kMaxSamples][2];
    static MixState mix;

    g_SampleCarry += kPerVblank;
    int samples = static_cast<int>(g_SampleCarry / kPerSecond);
    g_SampleCarry %= kPerSecond;

    applyKeys();
    for (int i = 0; i < kVoices; i++) {
        int base = kVoiceRegs + i * kVoiceStep;

        mix.adsr[i] = reg(base + kVoiceAdsr1) | static_cast<std::uint32_t>(reg(base + kVoiceAdsr2)) << 16;
    }
    std::memcpy(&mix.reverb, g_HwRegs + kReverbConfig, sizeof(mix.reverb));
    int base = reg(kReverbBase) * 8;
    if (base != g_ReverbBase) {
        g_ReverbBase = base;
        g_ReverbAddr = base;
    }
    mix.pitchMod = reg32(kPitchMod);
    mix.noiseOn = reg32(kNoiseOn);
    mix.reverbOn = reg32(kReverbOn);
    mix.control = reg(kControl);
    mix.reverbWrites = (mix.control & kReverbMaster) != 0 && base != 0;

    for (int i = 0; i < samples; i++) {
        mixSample(mix, buffer[i]);
    }
    writeBack();
    Host_Audio(&buffer[0][0], samples);
    if (g_Dump != nullptr) {
        std::fwrite(buffer, sizeof(buffer[0]), static_cast<std::size_t>(samples), g_Dump);
        std::fflush(g_Dump);
    }
}

}  // extern "C"
