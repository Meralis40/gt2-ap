// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace gt2ap::discio {

// GT2.VOL ("GTFS"): the game's asset archive (everything except music).
//
// Header (first 0xc000 bytes): "GTFS", three u32, then the file table at +0x10.
// Each table entry is a u32: entry & ~0x7ff is the file's byte offset in GT2.VOL,
// entry & 0x7ff the unused bytes at the end of its last sector, so
// size(i) = (table[i + 1] & ~0x7ff) - table[i]. The table ends with the VOL size,
// then 0. File 0 is the header itself, file 1 the directory.
//
// Directory: 0x20-byte records { u32 mtime, u16 index, u8 flags, char name[25] }.
// flags bit 0 = directory, bit 7 = last record of its directory. A file's index
// is its table index; a directory's index is the record number of its first
// child, which is always a ".." record. The root starts at record 0.
inline constexpr const char* kVolFile = "/GT2.VOL";
inline constexpr std::size_t kVolHeaderSize = 0xc000;
inline constexpr std::uint32_t kVolDirectoryIndex = 1;

struct VolExtent {
    std::uint32_t offset = 0; // byte offset in GT2.VOL
    std::uint32_t size = 0;
};

struct VolFileEntry {
    std::string path;        // absolute, e.g. "/carobj/hairr.cdo.gz"
    std::uint32_t index = 0; // file table index
    VolExtent extent;
    std::uint32_t mtime = 0; // Unix time
};

// Parses the file table from the start of GT2.VOL (at most kVolHeaderSize bytes).
// Throws DiscError on a bad magic, a missing terminator, or offsets that decrease
// or go past volSize.
std::vector<std::uint32_t> parseVolTable(std::span<const std::uint8_t> header, std::uint64_t volSize);

// Throws DiscError if index has no following entry.
VolExtent volExtent(const std::vector<std::uint32_t>& table, std::uint32_t index);

// Walks the directory (the contents of file kVolDirectoryIndex) and returns every
// file, depth first in directory order. Throws DiscError on unsafe names, indices
// out of range, unterminated or cyclic directories.
std::vector<VolFileEntry> parseVolDirectory(std::span<const std::uint8_t> directory,
                                            const std::vector<std::uint32_t>& table);

} // namespace gt2ap::discio
