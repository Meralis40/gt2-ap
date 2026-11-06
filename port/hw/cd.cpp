// SPDX-License-Identifier: GPL-2.0-or-later
// The drive, at the level of libcd's CdControlF: commands complete on the next pump and
// reads deliver their sectors from the disc image. Data reads run as fast as they're
// pumped, but XA streams run at drive speed: the game follows them with CdlGetlocL, so its
// timing depends on it, and the sound chip plays their audio at that rate. CD audio isn't played.
#include "hw.h"
#include "rt.h"

#include "gt2ap/discio/disc_image.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace {

namespace d = gt2ap::discio;

using Callback = void (*)(unsigned char status, unsigned char *result);

enum Command {
    kSetloc = 2, kPlay = 3, kReadN = 6, kStandby = 7, kStop = 8, kPause = 9, kSetfilter = 13, kSetmode = 14,
    kMute = 11, kDemute = 12, kGetlocL = 16, kGetlocP = 17, kGetTN = 19, kGetTD = 20, kReadS = 27,
};
enum Status { kDataReady = 1, kComplete = 2 };
enum StatusBits { kStatSpinning = 0x02, kStatReading = 0x20 };
enum ModeBits { kModeXaFilter = 0x08, kModeWholeSector = 0x20, kModeXa = 0x40, kModeDoubleSpeed = 0x80 };
enum SubmodeBits { kSubmodeAudio = 0x04, kSubmodeRealTime = 0x40 };

constexpr std::size_t kRawSectorSize = 2352;
constexpr std::size_t kHeaderOffset = 12;
constexpr std::size_t kFileOffset = 16;
constexpr std::size_t kChannelOffset = 17;
constexpr std::size_t kSubmodeOffset = 18;
constexpr std::size_t kDataOffset = 24;
constexpr int kFirstSectorOffset = 150;  // 2 seconds of pregap
// Drive speed, in units such that 75 sectors a second at 59.94 vblanks a second is exact: XA
// plays at this rate, and must neither run out nor pile up.
constexpr int kUnitsPerVblank = 1001;
constexpr int kUnitsPerSector = 800;
constexpr int kMaxUnits = 3 * kUnitsPerSector;

std::optional<d::DiscImage> g_Disc;
Callback g_OnSync, g_OnReady;
std::uint8_t g_Mode;
std::uint32_t g_Location;
bool g_Reading;
std::array<std::uint8_t, kRawSectorSize> g_Sector;
std::size_t g_SectorPos;
std::uint32_t g_ResultWords[2];
bool g_SyncPending;
int g_StreamUnits;
std::uint8_t g_FilterFile, g_FilterChannel;
bool g_Muted;

std::uint8_t toBcd(int value)
{
    return static_cast<std::uint8_t>((value / 10) << 4 | (value % 10));
}

int fromBcd(std::uint8_t value)
{
    return (value >> 4) * 10 + (value & 0xf);
}

std::uint8_t stat()
{
    return kStatSpinning | (g_Reading ? kStatReading : 0);
}

void setResult(std::uint8_t a = 0, std::uint8_t b = 0, std::uint8_t c = 0, std::uint8_t d = 0, std::uint8_t e = 0,
               std::uint8_t f = 0, std::uint8_t g = 0, std::uint8_t h = 0)
{
    const std::uint8_t bytes[8] = {a, b, c, d, e, f, g, h};

    std::memcpy(g_ResultWords, bytes, sizeof(bytes));
}

bool streamingXa()
{
    return (g_Mode & kModeXa) != 0;
}

bool atEndOfDisc()
{
    const d::Track &last = g_Disc->tracks().back();

    return g_Location >= last.startLba + last.sectorCount;
}

void readSector()
{
    if (!g_Disc) {
        Rt_Fatal("reading the CD needs a disc image");
    }
    try {
        if (g_Disc->readStoredSector(g_Location, g_Sector) != kRawSectorSize) {
            Rt_Fatal("the disc image needs raw 2352-byte sectors");
        }
    } catch (const std::exception &e) {
        Rt_Fatal("reading sector %u: %s", g_Location, e.what());
    }
    g_Location++;
    g_SectorPos = g_Mode & kModeWholeSector ? kHeaderOffset : kDataOffset;
}

}  // namespace

void Hw_CdOpen(const std::filesystem::path &cue)
{
    g_Disc = d::DiscImage::open(cue);
}

extern "C" {

void *CdSyncCallback(Callback callback)
{
    Callback previous = g_OnSync;

    g_OnSync = callback;
    return reinterpret_cast<void *>(previous);
}

void *CdReadyCallback(Callback callback)
{
    Callback previous = g_OnReady;

    g_OnReady = callback;
    return reinterpret_cast<void *>(previous);
}

int CdControlF(unsigned char command, unsigned char *param)
{
    setResult(stat());
    switch (command) {
    case kSetloc:
        g_Location = (fromBcd(param[0]) * 60 + fromBcd(param[1])) * 75 + fromBcd(param[2]) - kFirstSectorOffset;
        break;
    case kReadN:
    case kReadS:
        if (!g_Reading) {
            Hw_SpuXaClear();
        }
        g_Reading = true;
        setResult(stat());
        break;
    case kPlay:
        Rt_Log("CD audio isn't played yet");
        break;
    case kStandby:
    case kStop:
    case kPause:
        g_Reading = false;
        setResult(stat());
        break;
    case kSetfilter:
        g_FilterFile = param[0];
        g_FilterChannel = param[1];
        break;
    case kMute:
    case kDemute:
        g_Muted = command == kMute;
        break;
    case kSetmode:
        g_Mode = param[0];
        break;
    case kGetlocL:
        std::memcpy(g_ResultWords, g_Sector.data() + kHeaderOffset, sizeof(g_ResultWords));
        break;
    case kGetlocP:
        setResult(1, 1, 0, 0, 0, g_Sector[kHeaderOffset], g_Sector[kHeaderOffset + 1], g_Sector[kHeaderOffset + 2]);
        break;
    case kGetTN:
        setResult(stat(), 1, 1);
        break;
    case kGetTD: {
        const d::Track &last = g_Disc->tracks().back();
        std::uint64_t sectors = param[0] != 0 ? 0 : last.startLba + last.sectorCount;
        int seconds = static_cast<int>((sectors + kFirstSectorOffset) / 75);
        setResult(stat(), toBcd(seconds / 60), toBcd(seconds % 60));
        break;
    }
    default:
        break;
    }
    g_SyncPending = true;
    return 1;
}

int CdGetSector(void *dst, int words)
{
    std::size_t size = static_cast<std::size_t>(words) * 4;

    if (g_SectorPos + size > g_Sector.size()) {
        Rt_Fatal("CdGetSector past the end of the sector");
    }
    std::memcpy(dst, g_Sector.data() + g_SectorPos, size);
    g_SectorPos += size;
    return 1;
}

void Hw_CdVblank(void)
{
    g_StreamUnits = std::min(g_StreamUnits + kUnitsPerVblank, kMaxUnits);
}

int Hw_CdPump(void)
{
    if (g_SyncPending) {
        g_SyncPending = false;
        if (g_OnSync != nullptr) {
            std::uint32_t result[2] = {g_ResultWords[0], g_ResultWords[1]};
            g_OnSync(kComplete, reinterpret_cast<unsigned char *>(result));
        }
        return 1;
    }
    if (g_Reading) {
        if (streamingXa()) {
            int cost = g_Mode & kModeDoubleSpeed ? kUnitsPerSector / 2 : kUnitsPerSector;

            if (g_StreamUnits < cost) {
                return 0;
            }
            g_StreamUnits -= cost;
            if (g_Disc && atEndOfDisc()) {
                return 0;  // the stream runs out here: CdlGetlocL stops moving, and the game resets
            }
        }
        readSector();
        setResult(stat());
        // XA audio goes to the decoder: the CPU only sees the position, with CdlGetlocL.
        if (streamingXa() && (g_Sector[kSubmodeOffset] & (kSubmodeAudio | kSubmodeRealTime)) == (kSubmodeAudio | kSubmodeRealTime)) {
            bool wanted = !(g_Mode & kModeXaFilter) || (g_Sector[kFileOffset] == g_FilterFile &&
                                                        g_Sector[kChannelOffset] == g_FilterChannel);
            if (wanted && !g_Muted) {
                Hw_SpuXaSector(g_Sector.data());
            }
            return 1;
        }
        if (g_OnReady != nullptr) {
            std::uint32_t result[2] = {g_ResultWords[0], g_ResultWords[1]};
            g_OnReady(kDataReady, reinterpret_cast<unsigned char *>(result));
        }
        return 1;
    }
    return 0;
}

}  // extern "C"
