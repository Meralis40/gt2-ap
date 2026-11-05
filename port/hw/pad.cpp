// SPDX-License-Identifier: GPL-2.0-or-later
// libpad: a digital controller in port 1, port 2 empty. The game reads the buffers it gave
// PadInitDirect each vblank.
#include "hw.h"

#include <cstdint>

namespace {

constexpr int kStateStable = 6;
constexpr std::uint8_t kDigitalPad = 0x41;
constexpr std::uint8_t kNoController = 0xff;

std::uint8_t *g_Buffers[2];
std::uint16_t g_Held;

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

void Hw_PadVblank(void)
{
    if (g_Buffers[0] == nullptr) {
        return;
    }
    std::uint16_t raw = static_cast<std::uint16_t>(~g_Held);

    g_Buffers[0][0] = 0;
    g_Buffers[0][1] = kDigitalPad;
    g_Buffers[0][2] = raw & 0xff;
    g_Buffers[0][3] = raw >> 8;
    g_Buffers[1][0] = kNoController;
}

}  // extern "C"
