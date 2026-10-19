// SPDX-License-Identifier: GPL-2.0-or-later
#include "gt2ap/discio/overlay.hpp"

#include "gt2ap/discio/error.hpp"
#include "gt2ap/discio/gzip.hpp"

#include <string>

namespace gt2ap::discio {

namespace {

std::uint32_t readLe32(std::span<const std::uint8_t> d, std::size_t pos)
{
    return static_cast<std::uint32_t>(d[pos]) | static_cast<std::uint32_t>(d[pos + 1]) << 8 |
           static_cast<std::uint32_t>(d[pos + 2]) << 16 | static_cast<std::uint32_t>(d[pos + 3]) << 24;
}

} // namespace

std::vector<OverlayEntry> parseOverlayTable(std::span<const std::uint8_t> ovlFile)
{
    if (ovlFile.size() < 8)
        throw DiscError("GT2.OVL: file too small");
    const std::uint32_t tableSize = readLe32(ovlFile, 0);
    if (tableSize == 0 || tableSize % 8 != 0 || tableSize > ovlFile.size())
        throw DiscError("GT2.OVL: bad table size " + std::to_string(tableSize));

    std::vector<OverlayEntry> entries;
    for (std::size_t pos = 0; pos < tableSize; pos += 8) {
        const OverlayEntry e{readLe32(ovlFile, pos), readLe32(ovlFile, pos + 4)};
        if (e.offset < tableSize || static_cast<std::uint64_t>(e.offset) + e.compressedSize > ovlFile.size())
            throw DiscError("GT2.OVL: entry " + std::to_string(entries.size()) + " outside file");
        entries.push_back(e);
    }
    return entries;
}

std::vector<std::uint8_t> extractOverlay(std::span<const std::uint8_t> ovlFile, const OverlayEntry& entry)
{
    return gunzip(ovlFile.subspan(entry.offset, entry.compressedSize));
}

} // namespace gt2ap::discio
