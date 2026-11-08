// SPDX-License-Identifier: GPL-2.0-or-later
// Starts the game: maps guest memory, loads the executable from the disc and runs
// its entry point, then each overlay entry it asks for.
//
// Usage: gt2ap [<disc .cue>] [--vblanks <n>] [--dump <every> <dir>] [--input <vblank> <buttons>]...
//              [--record <file>] [--replay <file>] [--connect <server> <slot> [<password>]]
// With no disc it takes the one saved in gt2ap.cfg, or asks for one and saves it.
// --vblanks stops after n vblanks, without waiting for real time; --dump writes a frame
// every <every> vblanks to <dir>; --input holds buttons from a vblank on; --record saves the
// memory card and every vblank's buttons to a file, and --replay plays one back; --connect
// joins an Archipelago room without the connection page. Test runs, recordings and replays
// don't show that page, since none of them can type on it.
#include "ap.h"
#include "ap_host.h"
#include "config.h"
#include "functions.h"
#include "memmap.h"
#include "rt.h"
#include "host.h"
#include "hw.h"

#include "gt2ap/discio/disc_image.hpp"
#include "gt2ap/discio/error.hpp"
#include "gt2ap/discio/iso9660.hpp"
#include "gt2ap/discio/overlay.hpp"
#include "gt2ap/discio/sha1.hpp"

#include <csetjmp>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>

extern "C" {
void Sys_Init(void);
void Game_Main(void);
}

namespace {

using Bytes = std::vector<std::uint8_t>;

constexpr std::uint32_t kBssStart = 0x801c93b0;
constexpr std::uint32_t kBssEnd = 0x801f0d60;
constexpr std::size_t kExeHeaderSize = 0x800;
constexpr int kOverlayCount = 6;
// The addresses in the port are this executable's, NTSC-U 1.2's.
constexpr const char *kExeSha1 = "3030aa271c0a4022fc69ce09d76a6bc75e69a32a";

Bytes g_Overlays[kOverlayCount];

// What Rt_RunEntry leaves for the loop in main.
std::jmp_buf g_EntryJump;
void *g_Entry;
int g_EntryArgs[4];

std::filesystem::path pathFromUtf8(const std::string &text)
{
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

std::string pathToUtf8(const std::filesystem::path &path)
{
    std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

// Reads the executable and the overlays. Returns what's wrong with the image, if anything.
std::string readDisc(const std::filesystem::path &cue, Bytes &executable)
{
    namespace d = gt2ap::discio;

    try {
        d::DiscImage disc = d::DiscImage::open(cue);
        d::Iso9660 iso(disc);
        auto exe = iso.find("/SCUS_944.88");
        auto ovl = iso.find(d::kOverlayFile);
        if (!exe || !ovl) {
            return "This isn't the Gran Turismo 2 Simulation disc.";
        }
        executable = iso.readFile(*exe);
        if (d::Sha1::toHex(d::Sha1::of(executable)) != kExeSha1) {
            return "This Gran Turismo 2 Simulation disc isn't the NTSC-U 1.2 release.";
        }
        Bytes pack = iso.readFile(*ovl);
        auto entries = d::parseOverlayTable(pack);
        for (int i = 0; i < kOverlayCount; i++) {
            g_Overlays[i] = d::extractOverlay(pack, entries.at(i));
        }
    } catch (const std::exception &e) {
        return e.what();
    }
    return {};
}

// No disc on the command line: last run's, or the player's choice, which is then saved.
// Without a window there's nobody to ask, and it returns false.
bool findDisc(bool window, std::filesystem::path &disc, Bytes &exe)
{
    std::string problem;
    std::string saved = Cfg_Get("disc");

    if (!saved.empty()) {
        disc = pathFromUtf8(saved);
        problem = readDisc(disc, exe);
        if (problem.empty()) {
            return true;
        }
        problem = saved + "\n\n" + problem;
    }
    if (!window) {
        return false;
    }
    for (;;) {
        if (!problem.empty()) {
            Host_Alert(problem);
        }
        std::string picked = Host_PickDisc();
        if (picked.empty()) {
            std::exit(0);
        }
        disc = std::filesystem::absolute(pathFromUtf8(picked));
        problem = readDisc(disc, exe);
        if (problem.empty()) {
            Cfg_Set("disc", pathToUtf8(disc));
            return true;
        }
    }
}

// A PS-X EXE: the text's address and size are at +0x18 and +0x1c of the header.
void loadExecutable(const Bytes &exe)
{
    if (exe.size() < kExeHeaderSize || std::memcmp(exe.data(), "PS-X EXE", 8) != 0) {
        Rt_Fatal("the executable has no PS-X EXE header");
    }
    std::uint32_t address;
    std::uint32_t size;
    std::memcpy(&address, exe.data() + 0x18, 4);
    std::memcpy(&size, exe.data() + 0x1c, 4);
    if (size > exe.size() - kExeHeaderSize) {
        Rt_Fatal("the executable is truncated");
    }
    std::memcpy(rt::guest(address), exe.data() + kExeHeaderSize, size);
    std::memset(rt::guest(kBssStart), 0, kBssEnd - kBssStart);
    Rt_Log("loaded %u bytes at %08x", size, address);
}

}  // namespace

extern "C" {

void Rt_Log(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
}

void Rt_Fatal(const char *format, ...)
{
    va_list args;

    std::fputs("fatal: ", stderr);
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
    std::exit(1);
}

void Rt_Unimplemented(const char *what)
{
    Rt_Fatal("not implemented: %s", what);
}

void Rt_LoadOverlay(int index)
{
    const Bytes &image = g_Overlays[index];

    std::memcpy(rt::guest(rt::kOverlayBase), image.data(), image.size());
    rt::setOverlayFunctions(index);
    Rt_Log("overlay %d loaded", index);
}

void Rt_RunEntry(void *entry, int a, int b, int c, int d)
{
    g_Entry = entry;
    g_EntryArgs[0] = a;
    g_EntryArgs[1] = b;
    g_EntryArgs[2] = c;
    g_EntryArgs[3] = d;
    std::longjmp(g_EntryJump, 1);
}

}  // extern "C"

// The game longjmps from deep in one overlay to start another; this frame stays alive.
static void runGame()
{
    if (setjmp(g_EntryJump) == 0) {
        Sys_Init();
        Rt_Log("Sys_Init done");
        Game_Main();
    } else {
        auto entry = reinterpret_cast<void (*)(int, int, int, int)>(g_Entry);
        entry(g_EntryArgs[0], g_EntryArgs[1], g_EntryArgs[2], g_EntryArgs[3]);
    }
}

int main(int argc, char **argv)
{
    try {
        rt::mapGuestMemory();
        rt::installFaultHandler();
        unsigned int maxVblanks = 0;
        unsigned int dumpEvery = 0;
        const char *dumpDir = "";
        const char *cue = nullptr;
        const char *recordPath = nullptr;
        const char *replayPath = nullptr;
        bool connecting = false;
        for (int i = 1; i < argc; i++) {
            if (std::strcmp(argv[i], "--vblanks") == 0 && i + 1 < argc) {
                maxVblanks = std::strtoul(argv[++i], nullptr, 0);
            } else if (std::strcmp(argv[i], "--input") == 0 && i + 2 < argc) {
                unsigned int vblank = std::strtoul(argv[++i], nullptr, 0);
                Rt_AddInput(vblank, static_cast<unsigned short>(std::strtoul(argv[++i], nullptr, 0)));
            } else if (std::strcmp(argv[i], "--dump") == 0 && i + 2 < argc) {
                dumpEvery = std::strtoul(argv[++i], nullptr, 0);
                dumpDir = argv[++i];
            } else if (std::strcmp(argv[i], "--audio") == 0 && i + 1 < argc) {
                Hw_SpuDumpTo(argv[++i]);
            } else if (std::strcmp(argv[i], "--record") == 0 && i + 1 < argc) {
                recordPath = argv[++i];
            } else if (std::strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
                replayPath = argv[++i];
            } else if (std::strcmp(argv[i], "--connect") == 0 && i + 2 < argc) {
                const char *server = argv[++i];
                const char *slot = argv[++i];
                const char *password = i + 1 < argc && argv[i + 1][0] != '-' ? argv[++i] : "";
                Ap_SetLogin(server, slot, password);
                connecting = true;
            } else {
                cue = argv[i];
            }
        }
        Rt_SetRunLimits(maxVblanks, dumpEvery, dumpDir);
        if (maxVblanks != 0 || replayPath != nullptr || recordPath != nullptr) {
            Ap_DisablePage();
        }
        if (replayPath != nullptr) {
            Rt_ReplayOpen(replayPath, maxVblanks != 0);
        }
        if (recordPath != nullptr) {
            Rt_RecordOpen(recordPath);
        }
        if (replayPath != nullptr && connecting) {
            Ap_ConnectNow();
        }
        if (maxVblanks == 0) {
            Host_Open();
        }
        std::filesystem::path disc;
        Bytes exe;
        if (cue != nullptr) {
            disc = cue;
            std::string problem = readDisc(disc, exe);
            if (!problem.empty()) {
                Rt_Fatal("%s: %s", cue, problem.c_str());
            }
        } else if (!findDisc(maxVblanks == 0, disc, exe)) {
            std::fputs("usage: gt2ap [<disc.cue>] [--vblanks <n>] [--dump <every> <dir>] [--record <file>] [--replay <file>]\n", stderr);
            return 2;
        }
        Rt_StartWatchdog();
        loadExecutable(exe);
        Hw_CdOpen(disc);
    } catch (const std::exception &e) {
        Rt_Fatal("%s", e.what());
    }

    rt::runOnGameStack(runGame);
    return 0;
}
