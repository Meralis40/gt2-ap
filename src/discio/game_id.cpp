// SPDX-License-Identifier: GPL-2.0-or-later
#include "gt2ap/discio/game_id.hpp"

#include <algorithm>
#include <cctype>

namespace gt2ap::discio {

namespace {

// Track 01 SHA-1 of the NTSC-U 1.2 discs, checked against real discs.
constexpr KnownDump kKnownDumps[] = {
    {"SCUS-94488", "1.2", "c4813703d2ad440ed36670c1ac2c1a66c9dc673d"}, // Simulation Mode
    {"SCUS-94455", "1.2", "68f226780ef551b746e1a0964541570ec04c8060"}, // Arcade Mode
};

std::string upper(std::string_view s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

} // namespace

const char* toString(Gt2Disc disc)
{
    switch (disc) {
    case Gt2Disc::Simulation: return "Gran Turismo 2 - Simulation Mode";
    case Gt2Disc::Arcade: return "Gran Turismo 2 - Arcade Mode";
    case Gt2Disc::Unknown: return "unknown";
    }
    return "unknown";
}

std::optional<GameInfo> parseSystemCnf(std::string_view text)
{
    const std::string upperText = upper(text);
    std::size_t pos = upperText.find("BOOT");
    // Skip "BOOT2" (PS2) and anything that is not "BOOT <spaces> =".
    while (pos != std::string::npos) {
        std::size_t p = pos + 4;
        while (p < upperText.size() && (upperText[p] == ' ' || upperText[p] == '\t'))
            ++p;
        if (p < upperText.size() && upperText[p] == '=')
            break;
        pos = upperText.find("BOOT", pos + 4);
    }
    if (pos == std::string::npos)
        return std::nullopt;

    std::size_t valueStart = upperText.find('=', pos) + 1;
    std::size_t valueEnd = upperText.find_first_of("\r\n", valueStart);
    if (valueEnd == std::string::npos)
        valueEnd = upperText.size();
    std::string value = upperText.substr(valueStart, valueEnd - valueStart);

    // "cdrom:\SCUS_944.88;1" -> "SCUS_944.88"
    const std::size_t lastSep = value.find_last_of("\\/:");
    if (lastSep != std::string::npos)
        value = value.substr(lastSep + 1);
    if (const std::size_t semi = value.find(';'); semi != std::string::npos)
        value.resize(semi);
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); }),
                value.end());
    if (value.empty())
        return std::nullopt;

    GameInfo info;
    info.bootFile = value;
    // "SCUS_944.88" -> "SCUS-94488"
    std::string serial;
    for (const char c : value) {
        if (c == '_')
            serial.push_back('-');
        else if (c != '.')
            serial.push_back(c);
    }
    info.serial = serial;
    if (serial == "SCUS-94488")
        info.disc = Gt2Disc::Simulation;
    else if (serial == "SCUS-94455")
        info.disc = Gt2Disc::Arcade;
    return info;
}

std::optional<GameInfo> identify(const Iso9660& fs)
{
    const auto cnf = fs.find("SYSTEM.CNF");
    if (!cnf || cnf->isDirectory)
        return std::nullopt;
    const auto data = fs.readFile(*cnf);
    return parseSystemCnf(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
}

std::span<const KnownDump> knownDumps()
{
    return kKnownDumps;
}

const KnownDump* findKnownDump(std::string_view track1Sha1)
{
    const std::string wanted = upper(track1Sha1);
    for (const KnownDump& dump : knownDumps())
        if (upper(dump.track1Sha1) == wanted)
            return &dump;
    return nullptr;
}

} // namespace gt2ap::discio
