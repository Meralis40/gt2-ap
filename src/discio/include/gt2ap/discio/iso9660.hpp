// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "gt2ap/discio/disc_image.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gt2ap::discio {

// CD-XA attribute bits from a directory record's system use area.
namespace xa {
inline constexpr std::uint16_t kMode2Form1 = 0x0800;
inline constexpr std::uint16_t kMode2Form2 = 0x1000;
inline constexpr std::uint16_t kInterleaved = 0x2000;
inline constexpr std::uint16_t kCdda = 0x4000;
inline constexpr std::uint16_t kDirectory = 0x8000;
} // namespace xa

struct FileEntry {
    std::string name;      // without the ";1" version suffix
    std::string path;      // absolute, '/'-separated, e.g. "/DATA/GT2.VOL"
    std::uint32_t lba = 0; // absolute LBA of the extent
    std::uint32_t size = 0;
    bool isDirectory = false;
    std::uint16_t xaAttributes = 0; // 0 if the record has no XA data

    // Form 2 or interleaved files (XA audio, STR video) must be read as stored sectors.
    bool needsRawRead() const { return (xaAttributes & (xa::kMode2Form2 | xa::kInterleaved)) != 0; }
    std::uint32_t sectorCount() const { return (size + kUserDataSize - 1) / kUserDataSize; }
};

// Read-only ISO9660 (with CD-XA extensions) filesystem on the first data track.
// Keeps a pointer to the DiscImage, which must outlive it and not be moved.
class Iso9660 {
public:
    explicit Iso9660(const DiscImage& disc);

    const std::string& volumeId() const { return m_volumeId; }
    const FileEntry& root() const { return m_root; }

    // Case-insensitive lookup; accepts '/' or '\' separators and an optional ";1".
    std::optional<FileEntry> find(std::string_view path) const;
    std::vector<FileEntry> list(const FileEntry& directory) const;
    // Visits every entry below `directory`, depth first.
    void walk(const FileEntry& directory, const std::function<void(const FileEntry&)>& visit) const;

    // File contents as user data (Form 1).
    std::vector<std::uint8_t> readFile(const FileEntry& file) const;
    // File contents as stored sectors (2336/2352 bytes each), for Form 2 / interleaved files.
    std::vector<std::uint8_t> readFileStored(const FileEntry& file) const;

private:
    void walkImpl(const FileEntry& directory, const std::function<void(const FileEntry&)>& visit,
                  int depth) const;

    const DiscImage* m_disc;
    std::uint32_t m_baseLba;
    std::string m_volumeId;
    FileEntry m_root;
};

} // namespace gt2ap::discio
