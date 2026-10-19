// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "gt2ap/discio/cue.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <vector>

namespace gt2ap::discio {

inline constexpr std::uint32_t kUserDataSize = 2048;
inline constexpr std::uint32_t kRawSectorSize = 2352;

// A track resolved against its image file: absolute disc position and file location.
struct Track {
    int number = 0;
    TrackMode mode = TrackMode::Audio;
    std::size_t fileIndex = 0;     // index into DiscImage::files()
    std::uint64_t fileOffset = 0;  // byte offset of the track's INDEX 01 in its file
    std::uint32_t startLba = 0;    // absolute LBA of INDEX 01 (LBA 0 = MSF 00:02:00)
    std::uint32_t sectorCount = 0; // sectors stored in the file from INDEX 01 on

    bool isData() const { return mode != TrackMode::Audio; }
    bool contains(std::uint32_t lba) const { return lba >= startLba && lba - startLba < sectorCount; }
};

// Read-only access to a CD image: a .cue sheet with its .bin files, or a single
// .bin/.iso file. Not thread-safe (file streams are shared).
class DiscImage {
public:
    // Opens a .cue (any number of FILE/TRACK entries) or a single raw .bin/.iso.
    static DiscImage open(const std::filesystem::path& path);

    const std::vector<Track>& tracks() const { return m_tracks; }
    const std::vector<std::filesystem::path>& files() const { return m_files; }
    // First data track (where the ISO9660 filesystem lives). Throws if none.
    const Track& firstDataTrack() const;
    // Track containing an absolute LBA, or nullptr.
    const Track* trackAt(std::uint32_t lba) const;

    // Reads the 2048 bytes of user data of a data sector (Mode 1, or Mode 2 Form 1 layout).
    void readSector(std::uint32_t lba, std::span<std::uint8_t, kUserDataSize> out) const;
    // Reads a sector as stored in the image (2048, 2336 or 2352 bytes, see sectorSize()).
    // Returns the number of bytes written; `out` must hold at least that many.
    std::uint32_t readStoredSector(std::uint32_t lba, std::span<std::uint8_t> out) const;
    // True if the sector is Mode 2 Form 2 (XA audio, video). False when it cannot be told.
    bool isForm2(std::uint32_t lba) const;

    DiscImage(DiscImage&&) noexcept = default;
    DiscImage& operator=(DiscImage&&) noexcept = default;
    ~DiscImage();

private:
    DiscImage() = default;
    static DiscImage fromCue(const std::filesystem::path& cuePath);
    static DiscImage fromSingleFile(const std::filesystem::path& path);
    void openStreams();
    const Track& requireTrack(std::uint32_t lba) const;

    std::vector<std::filesystem::path> m_files;
    std::vector<Track> m_tracks;
    std::vector<std::unique_ptr<std::ifstream>> m_streams;
};

} // namespace gt2ap::discio
