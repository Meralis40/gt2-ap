// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "gt2ap/discio/iso9660.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace gt2ap::discio {

enum class Gt2Disc { Unknown, Simulation, Arcade };
const char* toString(Gt2Disc disc);

struct GameInfo {
    std::string serial;     // e.g. "SCUS-94488"
    std::string bootFile;   // executable path on the disc, e.g. "SCUS_944.88"
    Gt2Disc disc = Gt2Disc::Unknown;
};

// Reads SYSTEM.CNF and identifies the disc. Returns nullopt if there is no SYSTEM.CNF
// or no BOOT line (not a PS1 game disc).
std::optional<GameInfo> identify(const Iso9660& fs);

// Parses the BOOT line of a SYSTEM.CNF ("BOOT = cdrom:\SCUS_944.88;1").
std::optional<GameInfo> parseSystemCnf(std::string_view text);

// A dump verified against redump.org: SHA-1 of the data track file (Track 01).
struct KnownDump {
    std::string_view serial;
    std::string_view revision; // e.g. "1.2"
    std::string_view track1Sha1;
};

// Dumps we accept. Only NTSC-U 1.2 is targeted (see docs/PLAN.md).
std::span<const KnownDump> knownDumps();
const KnownDump* findKnownDump(std::string_view track1Sha1);

} // namespace gt2ap::discio
