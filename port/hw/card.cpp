// SPDX-License-Identifier: GPL-2.0-or-later
// The memory card in port 1, as a 128 KB .mcd file; port 2 is empty. The game's driver sends
// BIOS _card_ commands and waits for their events, which arrive on the next pump.
#include "hw.h"
#include "rt.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr std::uint32_t kClassHwCard = 0xf0000011;
constexpr std::uint32_t kClassSwCard = 0xf4000001;
constexpr int kSpecDone = 0x0004;
constexpr int kSpecTimeout = 0x0100;
constexpr int kSpecNewCard = 0x2000;
constexpr int kFrameSize = 128;
constexpr int kFramesPerBlock = 64;
constexpr int kDirectoryFrames = 15;
constexpr int kBrokenFrames = 20;

struct Event {
    std::uint32_t cls;
    int spec;
    int (*handler)(int);
};

std::vector<Event> g_Events;
std::vector<std::uint8_t> g_Card;
FILE *g_File;
const char *g_Path = "gt2ap.mcd";
bool g_FromImage;
bool g_Announced;
std::uint32_t g_PendingClass;
int g_PendingSpec;

void setChecksum(std::uint8_t *frame)
{
    std::uint8_t sum = 0;

    for (int i = 0; i < kFrameSize - 1; i++) {
        sum ^= frame[i];
    }
    frame[kFrameSize - 1] = sum;
}

// A formatted card: the header frame, a free directory, an empty broken frame list.
void formatCard()
{
    g_Card.assign(kHwCardSize, 0);
    g_Card[0] = 'M';
    g_Card[1] = 'C';
    setChecksum(g_Card.data());
    for (int i = 1; i <= kDirectoryFrames; i++) {
        std::uint8_t *frame = &g_Card[i * kFrameSize];
        frame[0] = 0xa0;
        frame[8] = 0xff;
        frame[9] = 0xff;
        setChecksum(frame);
    }
    for (int i = 0; i < kBrokenFrames; i++) {
        std::uint8_t *frame = &g_Card[(kDirectoryFrames + 1 + i) * kFrameSize];
        std::memset(frame, 0xff, 4);
        setChecksum(frame);
    }
}

void openCard()
{
    if (g_File != nullptr || g_FromImage) {
        return;
    }
    g_File = std::fopen(g_Path, "r+b");
    if (g_File != nullptr) {
        g_Card.resize(kHwCardSize);
        if (std::fread(g_Card.data(), 1, kHwCardSize, g_File) != kHwCardSize) {
            Rt_Fatal("%s isn't a 128 KB memory card", g_Path);
        }
        return;
    }
    formatCard();
    g_File = std::fopen(g_Path, "w+b");
    if (g_File == nullptr) {
        Rt_Fatal("cannot create %s", g_Path);
    }
    std::fwrite(g_Card.data(), 1, kHwCardSize, g_File);
    std::fflush(g_File);
}

void finish(std::uint32_t cls, int spec)
{
    g_PendingClass = cls;
    g_PendingSpec = spec;
}

bool isCardPort(int address)
{
    return address == 0;
}

}  // namespace

const unsigned char *Hw_CardImage()
{
    openCard();
    return g_Card.data();
}

void Hw_CardUseImage(const unsigned char *image)
{
    g_Card.assign(image, image + kHwCardSize);
    g_FromImage = true;
}

extern "C" {

int OpenEvent(unsigned int cls, int spec, int, int (*handler)(int))
{
    g_Events.push_back({cls, spec, handler});
    return static_cast<int>(g_Events.size());
}

int _card_info(int address)
{
    if (!isCardPort(address)) {
        finish(kClassSwCard, kSpecTimeout);
    } else if (!g_Announced) {
        g_Announced = true;
        finish(kClassSwCard, kSpecNewCard);
    } else {
        finish(kClassSwCard, kSpecDone);
    }
    return 1;
}

int _card_clear(int address)
{
    finish(kClassHwCard, isCardPort(address) ? kSpecDone : kSpecTimeout);
    return 1;
}

int _card_read(int address, int sector, void *buffer)
{
    if (!isCardPort(address)) {
        finish(kClassHwCard, kSpecTimeout);
        return 1;
    }
    openCard();
    std::memcpy(buffer, &g_Card[sector * kFrameSize], kFrameSize);
    finish(kClassHwCard, kSpecDone);
    return 1;
}

int _card_write(int address, int sector, const void *buffer)
{
    if (!isCardPort(address)) {
        finish(kClassHwCard, kSpecTimeout);
        return 1;
    }
    openCard();
    std::memcpy(&g_Card[sector * kFrameSize], buffer, kFrameSize);
    if (g_File != nullptr) {
        std::fseek(g_File, sector * kFrameSize, SEEK_SET);
        std::fwrite(buffer, 1, kFrameSize, g_File);
        std::fflush(g_File);
    }
    finish(kClassHwCard, kSpecDone);
    return 1;
}

void Hw_CardPump(void)
{
    if (g_PendingSpec == 0) {
        return;
    }
    std::uint32_t cls = g_PendingClass;
    int spec = g_PendingSpec;
    g_PendingSpec = 0;
    for (const Event &event : g_Events) {
        if (event.cls == cls && event.spec == spec) {
            event.handler(0);
        }
    }
}

}  // extern "C"
