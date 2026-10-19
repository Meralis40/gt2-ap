// SPDX-License-Identifier: GPL-2.0-or-later
#include "gt2ap/discio/cue.hpp"

#include "gt2ap/discio/error.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>

namespace gt2ap::discio {

std::uint32_t sectorSize(TrackMode mode)
{
    switch (mode) {
    case TrackMode::Mode1_2048: return 2048;
    case TrackMode::Mode2_2336: return 2336;
    case TrackMode::Audio:
    case TrackMode::Mode1_2352:
    case TrackMode::Mode2_2352: return 2352;
    }
    return 2352;
}

std::uint32_t userDataOffset(TrackMode mode)
{
    switch (mode) {
    case TrackMode::Mode1_2048: return 0;
    case TrackMode::Mode1_2352: return 16; // sync (12) + header (4)
    case TrackMode::Mode2_2336: return 8;  // subheader (8)
    case TrackMode::Mode2_2352: return 24; // sync (12) + header (4) + subheader (8)
    case TrackMode::Audio: return 0;
    }
    return 0;
}

const char* toString(TrackMode mode)
{
    switch (mode) {
    case TrackMode::Audio: return "AUDIO";
    case TrackMode::Mode1_2048: return "MODE1/2048";
    case TrackMode::Mode1_2352: return "MODE1/2352";
    case TrackMode::Mode2_2336: return "MODE2/2336";
    case TrackMode::Mode2_2352: return "MODE2/2352";
    }
    return "?";
}

namespace {

std::string upper(std::string_view s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

// Splits a cue line into tokens, honouring double quotes.
std::vector<std::string> tokenize(std::string_view line)
{
    std::vector<std::string> tokens;
    std::size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        if (i >= line.size())
            break;
        if (line[i] == '"') {
            const std::size_t end = line.find('"', i + 1);
            if (end == std::string_view::npos)
                throw DiscError("cue: unterminated quote");
            tokens.emplace_back(line.substr(i + 1, end - i - 1));
            i = end + 1;
        } else {
            std::size_t end = i;
            while (end < line.size() && !std::isspace(static_cast<unsigned char>(line[end])))
                ++end;
            tokens.emplace_back(line.substr(i, end - i));
            i = end;
        }
    }
    return tokens;
}

int parseInt(std::string_view s, const char* what)
{
    int value = 0;
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || ptr != s.data() + s.size() || value < 0)
        throw DiscError(std::string("cue: invalid ") + what + " '" + std::string(s) + "'");
    return value;
}

TrackMode parseMode(std::string_view s)
{
    const std::string m = upper(s);
    if (m == "AUDIO") return TrackMode::Audio;
    if (m == "MODE1/2048") return TrackMode::Mode1_2048;
    if (m == "MODE1/2352") return TrackMode::Mode1_2352;
    if (m == "MODE2/2336") return TrackMode::Mode2_2336;
    if (m == "MODE2/2352") return TrackMode::Mode2_2352;
    throw DiscError("cue: unsupported track mode '" + std::string(s) + "'");
}

} // namespace

std::uint32_t parseMsf(std::string_view msf)
{
    const std::size_t c1 = msf.find(':');
    const std::size_t c2 = c1 == std::string_view::npos ? c1 : msf.find(':', c1 + 1);
    if (c1 == std::string_view::npos || c2 == std::string_view::npos)
        throw DiscError("cue: invalid time '" + std::string(msf) + "'");
    const int m = parseInt(msf.substr(0, c1), "minutes");
    const int s = parseInt(msf.substr(c1 + 1, c2 - c1 - 1), "seconds");
    const int f = parseInt(msf.substr(c2 + 1), "frames");
    if (s >= 60 || f >= 75)
        throw DiscError("cue: invalid time '" + std::string(msf) + "'");
    return static_cast<std::uint32_t>((m * 60 + s) * 75 + f);
}

CueSheet parseCue(std::string_view text)
{
    CueSheet sheet;
    std::size_t pos = 0;
    int lineNo = 0;
    while (pos <= text.size()) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string_view::npos)
            end = text.size();
        std::string_view line = text.substr(pos, end - pos);
        pos = end + 1;
        ++lineNo;
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);
        if (lineNo == 1 && line.size() >= 3 && line.substr(0, 3) == "\xEF\xBB\xBF")
            line.remove_prefix(3); // UTF-8 BOM

        const auto tokens = tokenize(line);
        if (tokens.empty())
            continue;
        const std::string keyword = upper(tokens[0]);
        auto need = [&](std::size_t n) {
            if (tokens.size() < n)
                throw DiscError("cue: line " + std::to_string(lineNo) + ": missing arguments for " + keyword);
        };

        if (keyword == "FILE") {
            need(2);
            sheet.files.push_back(tokens[1]);
        } else if (keyword == "TRACK") {
            need(3);
            if (sheet.files.empty())
                throw DiscError("cue: TRACK before FILE");
            CueTrack track;
            track.number = parseInt(tokens[1], "track number");
            track.mode = parseMode(tokens[2]);
            track.fileIndex = sheet.files.size() - 1;
            sheet.tracks.push_back(track);
        } else if (keyword == "INDEX") {
            need(3);
            if (sheet.tracks.empty())
                throw DiscError("cue: INDEX before TRACK");
            CueTrack& track = sheet.tracks.back();
            if (track.fileIndex != sheet.files.size() - 1)
                throw DiscError("cue: tracks spanning several FILEs are not supported");
            track.indexes[parseInt(tokens[1], "index number")] = parseMsf(tokens[2]);
        } else if (keyword == "PREGAP" || keyword == "POSTGAP") {
            need(2);
            if (sheet.tracks.empty())
                throw DiscError("cue: " + keyword + " before TRACK");
            (keyword == "PREGAP" ? sheet.tracks.back().pregapFrames : sheet.tracks.back().postgapFrames) =
                parseMsf(tokens[1]);
        }
        // REM, CATALOG, FLAGS, TITLE, PERFORMER, ISRC, ... carry nothing we need.
    }

    if (sheet.tracks.empty())
        throw DiscError("cue: no tracks");
    for (const CueTrack& t : sheet.tracks)
        if (!t.indexes.contains(1))
            throw DiscError("cue: track " + std::to_string(t.number) + " has no INDEX 01");
    return sheet;
}

} // namespace gt2ap::discio
