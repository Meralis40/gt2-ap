// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace gt2ap::discio {

// GT2.OVL: a table of (offset, compressed size) little-endian u32 pairs, then
// one gzip stream per overlay. The table length is implied by the first
// offset. Every overlay is inflated to the same address by the resident loader.
inline constexpr std::uint32_t kOverlayLoadAddress = 0x80010000;
inline constexpr const char* kOverlayFile = "/GT2.OVL";

struct OverlayEntry {
    std::uint32_t offset = 0;
    std::uint32_t compressedSize = 0;
};

// Throws DiscError if the table is malformed or points outside the file.
std::vector<OverlayEntry> parseOverlayTable(std::span<const std::uint8_t> ovlFile);

std::vector<std::uint8_t> extractOverlay(std::span<const std::uint8_t> ovlFile, const OverlayEntry& entry);

} // namespace gt2ap::discio
