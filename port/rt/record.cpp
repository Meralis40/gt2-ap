// SPDX-License-Identifier: GPL-2.0-or-later
// Input recordings. Vblanks fall on a fixed count of pumps (rt/irq.cpp), so a run depends on
// nothing but the memory card it starts from and the controls held at each vblank: the
// game's random numbers all come from the vblank count and the counters that follow it.
//
// The file is the header, the card, then one Frame per vblank, flushed as it goes so a crash
// keeps everything up to it. The random seed in each frame is only a check. Files of version 1
// stop each frame before the sticks, and were all played on a digital controller.
#include "hw.h"
#include "rt.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

extern "C" unsigned int g_RandomSeed;

namespace {

constexpr char kMagic[8] = {'G', 'T', '2', 'R', 'E', 'C', 0, 2};
constexpr std::size_t kVersionByte = 7;
constexpr char kOldVersion = 1;
constexpr std::size_t kOldFrameSize = 8;

struct Frame {
    std::uint16_t buttons;
    std::uint8_t analog;
    std::uint8_t reserved;
    std::uint32_t seed;
    std::uint8_t sticks[4];
};

FILE *g_Recording;
std::vector<Frame> g_Replay;
std::size_t g_ReplayPos;
bool g_Replaying;
bool g_StopAtEnd;
bool g_SeedDiffered;

void playFrame()
{
    if (g_ReplayPos == g_Replay.size()) {
        g_Replaying = false;
        Rt_Log("replay finished after %u vblanks", static_cast<unsigned int>(g_ReplayPos));
        if (g_StopAtEnd) {
            std::exit(0);
        }
        return;
    }
    const Frame &frame = g_Replay[g_ReplayPos++];
    Hw_PadSetButtons(frame.buttons);
    Hw_PadSetSticks(frame.analog, frame.sticks);
    if (frame.seed != g_RandomSeed && !g_SeedDiffered) {
        g_SeedDiffered = true;
        Rt_Log("replay: at vblank %u the random seed is %08x, it was %08x", static_cast<unsigned int>(g_ReplayPos),
               g_RandomSeed, frame.seed);
    }
}

}  // namespace

void Rt_ReplayOpen(const char *path, bool stopAtEnd)
{
    FILE *in = std::fopen(path, "rb");
    if (in == nullptr) {
        Rt_Fatal("cannot read %s", path);
    }
    char magic[sizeof(kMagic)];
    std::vector<std::uint8_t> card(kHwCardSize);
    if (std::fread(magic, 1, sizeof(magic), in) != sizeof(magic) || std::memcmp(magic, kMagic, kVersionByte) != 0 ||
        (magic[kVersionByte] != kMagic[kVersionByte] && magic[kVersionByte] != kOldVersion) ||
        std::fread(card.data(), 1, card.size(), in) != card.size()) {
        Rt_Fatal("%s isn't a recording", path);
    }
    std::size_t frameSize = magic[kVersionByte] == kOldVersion ? kOldFrameSize : sizeof(Frame);
    Frame frame{};
    while (std::fread(&frame, frameSize, 1, in) == 1) {
        g_Replay.push_back(frame);
    }
    std::fclose(in);
    Hw_CardUseImage(card.data());
    g_Replaying = true;
    g_StopAtEnd = stopAtEnd;
    Rt_Log("replaying %u vblanks from %s", static_cast<unsigned int>(g_Replay.size()), path);
}

void Rt_RecordOpen(const char *path)
{
    g_Recording = std::fopen(path, "wb");
    if (g_Recording == nullptr) {
        Rt_Fatal("cannot write %s", path);
    }
    std::fwrite(kMagic, 1, sizeof(kMagic), g_Recording);
    std::fwrite(Hw_CardImage(), 1, kHwCardSize, g_Recording);
    std::fflush(g_Recording);
}

void Rt_RecordVblank()
{
    if (g_Replaying) {
        playFrame();
    }
    if (g_Recording != nullptr) {
        Frame frame{Hw_PadButtons(), 0, 0, g_RandomSeed, {}};
        frame.analog = static_cast<std::uint8_t>(Hw_PadSticks(frame.sticks));
        std::fwrite(&frame, sizeof(frame), 1, g_Recording);
        std::fflush(g_Recording);
    }
}
