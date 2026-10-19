// SPDX-License-Identifier: GPL-2.0-or-later
// Builds small ISO9660/CD-XA disc images for tests (no game data needed).
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace synthetic {

// Layout of the synthetic filesystem (LBAs relative to the data track):
//   /SYSTEM.CNF          "BOOT = cdrom:\SCUS_944.55;1"
//   /DATA/BIG.BIN        3000 bytes, byte i = i % 251
//   /DATA/MOVIE.STR      2 sectors, Mode 2 Form 2 (interleaved)
struct Layout {
    static constexpr std::uint32_t kSectors = 26;
    static constexpr std::uint32_t kBigSize = 3000;
    static constexpr std::uint32_t kMovieLba = 23;
    static constexpr std::uint32_t kMovieSectors = 2;
};

inline const char* kSystemCnf = "BOOT = cdrom:\\SCUS_944.55;1\r\nTCB = 4\r\n";

// 2048-byte user data sectors.
std::vector<std::uint8_t> buildIso();
// The same filesystem as raw MODE2/2352 sectors (Form 1, except the movie: Form 2).
std::vector<std::uint8_t> buildRawMode2(std::uint32_t firstLba = 0);

void writeFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& data);
void writeText(const std::filesystem::path& path, const std::string& text);

// A fresh, empty directory under the system temp dir, removed on destruction.
class TempDir {
public:
    TempDir();
    ~TempDir();
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

} // namespace synthetic
