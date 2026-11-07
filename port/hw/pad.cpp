// SPDX-License-Identifier: GPL-2.0-or-later
// libpad: a digital controller or a DualShock in port 1, port 2 empty. The game reads the
// buffers it gave PadInitDirect each vblank.
#include "hw.h"

#include <cstdint>
#include <cstring>

namespace {

constexpr int kStateStable = 6;
constexpr std::uint8_t kDigitalPad = 0x41;
constexpr std::uint8_t kAnalogPad = 0x73;
constexpr int kStickBytes = 4;
constexpr std::uint8_t kNoController = 0xff;

std::uint8_t *g_Buffers[2];
std::uint16_t g_Held;
bool g_Analog;
std::uint8_t g_Sticks[kStickBytes];

}  // namespace

extern "C" {

void PadInitDirect(void *port1, void *port2)
{
    g_Buffers[0] = static_cast<std::uint8_t *>(port1);
    g_Buffers[1] = static_cast<std::uint8_t *>(port2);
}

int PadGetState(int port)
{
    return port == 0 ? kStateStable : 0;
}

int PadInfoAct(int, int, int)
{
    return 0;
}

int PadStartCom(void)
{
    return 1;
}

int PadSetAct(int, void *, int)
{
    return 0;
}

int PadSetActAlign(int, void *)
{
    return 1;
}

void Hw_PadSetButtons(unsigned short held)
{
    g_Held = held;
}

unsigned short Hw_PadButtons(void)
{
    return g_Held;
}

void Hw_PadSetSticks(int analog, const unsigned char *sticks)
{
    g_Analog = analog != 0;
    if (g_Analog) {
        std::memcpy(g_Sticks, sticks, kStickBytes);
    }
}

int Hw_PadSticks(unsigned char *sticks)
{
    std::memcpy(sticks, g_Sticks, kStickBytes);
    return g_Analog;
}

void Hw_PadVblank(void)
{
    if (g_Buffers[0] == nullptr) {
        return;
    }
    std::uint16_t raw = static_cast<std::uint16_t>(~g_Held);

    g_Buffers[0][0] = 0;
    g_Buffers[0][1] = g_Analog ? kAnalogPad : kDigitalPad;
    g_Buffers[0][2] = raw & 0xff;
    g_Buffers[0][3] = raw >> 8;
    if (g_Analog) {
        std::memcpy(g_Buffers[0] + 4, g_Sticks, kStickBytes);
    }
    g_Buffers[1][0] = kNoController;
}

}  // extern "C"
