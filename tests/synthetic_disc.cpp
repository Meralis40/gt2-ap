// SPDX-License-Identifier: GPL-2.0-or-later
#include "synthetic_disc.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace synthetic {

namespace {

constexpr std::size_t kSector = 2048;
constexpr std::uint32_t kRootLba = 18;
constexpr std::uint32_t kDataDirLba = 19;
constexpr std::uint32_t kCnfLba = 20;
constexpr std::uint32_t kBigLba = 21;

void put16Both(std::uint8_t* p, std::uint16_t v)
{
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8);
    p[2] = static_cast<std::uint8_t>(v >> 8);
    p[3] = static_cast<std::uint8_t>(v);
}

void put32Both(std::uint8_t* p, std::uint32_t v)
{
    for (int i = 0; i < 4; ++i) {
        p[i] = static_cast<std::uint8_t>(v >> (8 * i));
        p[7 - i] = static_cast<std::uint8_t>(v >> (8 * i));
    }
}

// Appends a directory record with an XA system use field; returns its length.
std::size_t putRecord(std::uint8_t* out, std::uint32_t lba, std::uint32_t size, bool dir,
                      const std::string& name, std::uint16_t xaAttr)
{
    const std::size_t nameLen = name.size();
    const std::size_t pad = nameLen % 2 == 0 ? 1 : 0;
    const std::size_t len = 33 + nameLen + pad + 14;
    std::memset(out, 0, len);
    out[0] = static_cast<std::uint8_t>(len);
    put32Both(out + 2, lba);
    put32Both(out + 10, size);
    out[25] = dir ? 0x02 : 0x00;
    put16Both(out + 28, 1);
    out[32] = static_cast<std::uint8_t>(nameLen);
    std::memcpy(out + 33, name.data(), nameLen);
    std::uint8_t* xa = out + 33 + nameLen + pad;
    xa[4] = static_cast<std::uint8_t>(xaAttr >> 8);
    xa[5] = static_cast<std::uint8_t>(xaAttr);
    xa[6] = 'X';
    xa[7] = 'A';
    return len;
}

std::uint8_t toBcd(std::uint32_t v)
{
    return static_cast<std::uint8_t>(((v / 10) << 4) | (v % 10));
}

} // namespace

std::vector<std::uint8_t> buildIso()
{
    std::vector<std::uint8_t> img(Layout::kSectors * kSector, 0);
    auto sector = [&](std::uint32_t lba) { return img.data() + lba * kSector; };

    // Primary volume descriptor.
    std::uint8_t* pvd = sector(16);
    pvd[0] = 1;
    std::memcpy(pvd + 1, "CD001", 5);
    pvd[6] = 1;
    std::memset(pvd + 40, ' ', 32);
    std::memcpy(pvd + 40, "GT2AP_TEST", 10);
    put32Both(pvd + 80, Layout::kSectors);
    put16Both(pvd + 128, 2048);
    putRecord(pvd + 156, kRootLba, kSector, true, std::string(1, '\0'), 0x8D55);
    pvd[156] = 34; // the root record in the PVD is fixed at 34 bytes
    std::memset(pvd + 156 + 34, 0, 14);

    // Volume descriptor set terminator.
    std::uint8_t* term = sector(17);
    term[0] = 255;
    std::memcpy(term + 1, "CD001", 5);
    term[6] = 1;

    // Root directory.
    std::uint8_t* p = sector(kRootLba);
    p += putRecord(p, kRootLba, kSector, true, std::string(1, '\0'), 0x8D55);
    p += putRecord(p, kRootLba, kSector, true, std::string(1, '\1'), 0x8D55);
    p += putRecord(p, kDataDirLba, kSector, true, "DATA", 0x8D55);
    putRecord(p, kCnfLba, static_cast<std::uint32_t>(std::strlen(kSystemCnf)), false, "SYSTEM.CNF;1", 0x0D55);

    // /DATA.
    p = sector(kDataDirLba);
    p += putRecord(p, kDataDirLba, kSector, true, std::string(1, '\0'), 0x8D55);
    p += putRecord(p, kRootLba, kSector, true, std::string(1, '\1'), 0x8D55);
    p += putRecord(p, kBigLba, Layout::kBigSize, false, "BIG.BIN;1", 0x0D55);
    putRecord(p, Layout::kMovieLba, Layout::kMovieSectors * kSector, false, "MOVIE.STR;1", 0x3D55);

    std::memcpy(sector(kCnfLba), kSystemCnf, std::strlen(kSystemCnf));
    for (std::uint32_t i = 0; i < Layout::kBigSize; ++i)
        sector(kBigLba)[i] = static_cast<std::uint8_t>(i % 251);
    for (std::uint32_t s = 0; s < Layout::kMovieSectors; ++s)
        std::memset(sector(Layout::kMovieLba + s), 0xA0 + static_cast<int>(s), kSector);
    return img;
}

std::vector<std::uint8_t> buildRawMode2(std::uint32_t firstLba)
{
    const std::vector<std::uint8_t> iso = buildIso();
    std::vector<std::uint8_t> raw(Layout::kSectors * 2352, 0);
    for (std::uint32_t s = 0; s < Layout::kSectors; ++s) {
        std::uint8_t* out = raw.data() + s * 2352;
        out[0] = 0x00;
        std::memset(out + 1, 0xFF, 10);
        out[11] = 0x00;
        const std::uint32_t abs = firstLba + s + 150;
        out[12] = toBcd(abs / (75 * 60));
        out[13] = toBcd((abs / 75) % 60);
        out[14] = toBcd(abs % 75);
        out[15] = 2;
        const bool form2 = s >= Layout::kMovieLba && s < Layout::kMovieLba + Layout::kMovieSectors;
        const std::uint8_t submode = form2 ? 0x64 : 0x08; // form2|realtime|audio vs data
        for (int i = 0; i < 2; ++i) {
            out[16 + i * 4 + 0] = static_cast<std::uint8_t>(form2 ? 1 : 0); // file
            out[16 + i * 4 + 1] = 0;             // channel
            out[16 + i * 4 + 2] = submode;
            out[16 + i * 4 + 3] = 0;
        }
        std::memcpy(out + 24, iso.data() + s * kSector, kSector);
    }
    return raw;
}

void writeFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& data)
{
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out)
        throw std::runtime_error("cannot write " + path.string());
}

void writeText(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream out(path, std::ios::binary);
    out << text;
}

TempDir::TempDir()
{
    static std::atomic<unsigned> counter{0};
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    m_path = std::filesystem::temp_directory_path() /
             ("gt2ap-test-" + std::to_string(stamp) + "-" + std::to_string(counter++));
    std::filesystem::create_directories(m_path);
}

TempDir::~TempDir()
{
    std::error_code ec;
    std::filesystem::remove_all(m_path, ec);
}

} // namespace synthetic
