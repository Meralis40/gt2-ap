// SPDX-License-Identifier: GPL-2.0-or-later
// Stands in for the PlayStation's interrupts. The game runs on one thread: wherever it
// waits, it calls Rt_Pump, which delivers what's pending as a callback, as an interrupt
// would have, and gives the host its turn.
#include "ap.h"
#include "ap_host.h"
#include "host.h"
#include "hw.h"
#include "game/include/hw.h"
#include "rt.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {

struct ScriptedInput {
    unsigned int vblank;
    unsigned short buttons;
};

using Clock = std::chrono::steady_clock;

constexpr auto kVblank = std::chrono::nanoseconds(16683350);  // NTSC, 59.94 Hz
constexpr int kDmaChannels = 7;
constexpr unsigned int kHblanksPerVblank = 263;
constexpr unsigned int kClocksPerVblank = 33868800 / 8 / 60;  // timer 2, at the system clock / 8
// A vblank falls on every so many pumps, never on the host's clock, which would make each run
// differ. The clock only paces them. This is how many CD sectors a vblank can load.
constexpr unsigned int kPumpsPerVblank = 32;

void (*g_VSyncCallback)(void);
void (*g_DmaCallbacks[kDmaChannels])(void);
bool g_DmaDone[kDmaChannels];
int g_InCallback;
Clock::time_point g_NextVblank = Clock::now();
unsigned int g_Pumps;
unsigned int g_Vblanks;
unsigned int g_Field;

// Options: a run that stops by itself and writes frames, without waiting for real time.
unsigned int g_MaxVblanks;
unsigned int g_DumpEvery;
std::filesystem::path g_DumpDir;
std::vector<ScriptedInput> g_Script;

std::uint16_t &timer(int index)
{
    return *reinterpret_cast<std::uint16_t *>(g_HwRegs + 0x100 + index * 0x10);
}

void dumpFrame()
{
    static std::vector<unsigned int> pixels(640 * 480);
    int width, height;

    Hw_GpuFrame(pixels.data(), &width, &height);
    auto path = g_DumpDir / ("frame" + std::to_string(g_Vblanks) + ".ppm");
    FILE *out = std::fopen(path.string().c_str(), "wb");
    if (out == nullptr) {
        Rt_Fatal("cannot write %s", path.string().c_str());
    }
    std::fprintf(out, "P6\n%d %d\n255\n", width, height);
    for (int i = 0; i < width * height; i++) {
        std::fwrite(&pixels[i], 3, 1, out);
    }
    std::fclose(out);
}

void vblank()
{
    g_Vblanks++;
    g_Field ^= 1;
    Ap_Poll();
    timer(1) = static_cast<std::uint16_t>(timer(1) + kHblanksPerVblank);
    timer(2) = static_cast<std::uint16_t>(timer(2) + kClocksPerVblank);
    if (g_MaxVblanks == 0) {
        Host_Frame();
    }
    for (const ScriptedInput &input : g_Script) {
        if (input.vblank == g_Vblanks) {
            Hw_PadSetButtons(input.buttons);
        }
    }
    Rt_RecordVblank();
    Hw_CdVblank();
    Hw_SpuVblank();
    Hw_PadVblank();
    if (g_VSyncCallback != nullptr) {
        g_InCallback++;
        g_VSyncCallback();
        g_InCallback--;
    }
    if (g_DumpEvery != 0 && g_Vblanks % g_DumpEvery == 0) {
        dumpFrame();
    }
    if (g_MaxVblanks != 0 && g_Vblanks >= g_MaxVblanks) {
        Rt_Log("stopping after %u vblanks", g_Vblanks);
        std::exit(0);
    }
}

void waitForNextVblank()
{
    Clock::time_point now = Clock::now();

    while (now < g_NextVblank) {
        std::this_thread::yield();
        now = Clock::now();
    }
    g_NextVblank += kVblank;
    if (now > g_NextVblank + 10 * kVblank) {
        g_NextVblank = now + kVblank;
    }
}

#if defined(_WIN32)
// A wait with no Rt_Pump in it hangs the game silently: this reports where.
volatile LONG g_PumpCount;

void watchdog(HANDLE game)
{
    LONG seen = -1;

    for (;;) {
        Sleep(2000);
        if (g_PumpCount != seen) {
            seen = g_PumpCount;
            continue;
        }
        SuspendThread(game);
        CONTEXT context{};
        context.ContextFlags = CONTEXT_CONTROL;
        GetThreadContext(game, &context);
        std::fprintf(stderr, "stuck: no pump for 2 s, at %08lx\n", context.Eip);
        auto frame = static_cast<std::uintptr_t>(context.Ebp);
        for (int i = 0; i < 16 && frame > 0x10000; i++) {
            auto *words = reinterpret_cast<std::uintptr_t *>(frame);
            std::fprintf(stderr, "  %08lx\n", static_cast<unsigned long>(words[1]));
            if (words[0] <= frame) {
                break;
            }
            frame = words[0];
        }
        std::fflush(stderr);
        TerminateProcess(GetCurrentProcess(), 5);
    }
}
#endif

}  // namespace

void Rt_StartWatchdog()
{
#if defined(_WIN32)
    HANDLE game;

    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &game, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);
    std::thread(watchdog, game).detach();
#endif
}

void Rt_StillAlive()
{
#if defined(_WIN32)
    g_PumpCount = g_PumpCount + 1;
#endif
}

void Rt_AddInput(unsigned int vblank, unsigned short buttons)
{
    g_Script.push_back({vblank, buttons});
}

void Rt_SetRunLimits(unsigned int maxVblanks, unsigned int dumpEvery, const char *dumpDir)
{
    g_MaxVblanks = maxVblanks;
    g_DumpEvery = dumpEvery;
    g_DumpDir = dumpDir;
}

extern "C" {

unsigned char g_HwRegs[0x1000];

void *VSyncCallback_Psyq(void (*func)(void))
{
    void (*previous)(void) = g_VSyncCallback;

    g_VSyncCallback = func;
    return reinterpret_cast<void *>(previous);
}

void *DMACallback(int channel, void (*func)(void))
{
    void (*previous)(void) = g_DmaCallbacks[channel];

    g_DmaCallbacks[channel] = func;
    return reinterpret_cast<void *>(previous);
}

int CheckCallback(void)
{
    return g_InCallback != 0;
}

void Rt_DmaDone(int channel)
{
    g_DmaDone[channel] = true;
}

unsigned int Hw_GpuStat(void)
{
    return 0x1c000000u | g_Field << 31;
}

void Rt_Pump(void)
{
#if defined(_WIN32)
    g_PumpCount = g_PumpCount + 1;
#endif
    if (g_InCallback != 0) {
        return;
    }
    g_InCallback++;
    for (int channel = 0; channel < kDmaChannels; channel++) {
        if (g_DmaDone[channel]) {
            g_DmaDone[channel] = false;
            if (g_DmaCallbacks[channel] != nullptr) {
                g_DmaCallbacks[channel]();
            }
        }
    }
    Hw_CdPump();
    Hw_CardPump();
    g_InCallback--;

    if (++g_Pumps < kPumpsPerVblank) {
        return;
    }
    g_Pumps = 0;
    if (g_MaxVblanks == 0) {
        waitForNextVblank();
    }
    vblank();
}

}  // extern "C"
