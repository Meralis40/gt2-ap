// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace gt2ap::discio {

enum class TrackMode {
    Audio,      // AUDIO, 2352 bytes per sector
    Mode1_2048, // MODE1/2048, user data only
    Mode1_2352, // MODE1/2352, raw sector
    Mode2_2336, // MODE2/2336, subheader + data + EDC, no sync/header
    Mode2_2352, // MODE2/2352, raw sector (what PS1 dumps use)
};

// Bytes per sector as stored in the image file.
std::uint32_t sectorSize(TrackMode mode);
// Offset of the 2048-byte user data inside a stored sector (Mode 2: Form 1 layout).
std::uint32_t userDataOffset(TrackMode mode);
const char* toString(TrackMode mode);

// One TRACK entry of a cue sheet, as written in the file. Frame values are
// relative to the start of the track's FILE (75 frames per second).
struct CueTrack {
    int number = 0;
    TrackMode mode = TrackMode::Audio;
    std::size_t fileIndex = 0;              // index into CueSheet::files
    std::map<int, std::uint32_t> indexes;   // INDEX number -> frame offset in file
    std::uint32_t pregapFrames = 0;         // PREGAP (not stored in the file)
    std::uint32_t postgapFrames = 0;        // POSTGAP (not stored in the file)
};

struct CueSheet {
    std::vector<std::string> files; // FILE names, as written (relative to the cue)
    std::vector<CueTrack> tracks;
};

// Parses cue sheet text. Throws DiscError on syntax errors or unsupported content.
CueSheet parseCue(std::string_view text);

// Parses "mm:ss:ff" into a frame count. Throws DiscError on malformed input.
std::uint32_t parseMsf(std::string_view msf);

} // namespace gt2ap::discio
