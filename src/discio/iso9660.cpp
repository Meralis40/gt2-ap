// SPDX-License-Identifier: GPL-2.0-or-later
#include "gt2ap/discio/iso9660.hpp"

#include "gt2ap/discio/error.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>

namespace gt2ap::discio {

namespace {

constexpr std::uint32_t kPvdSector = 16;
constexpr int kMaxDepth = 32;

std::uint32_t le32(const std::uint8_t* p)
{
    return std::uint32_t{p[0]} | (std::uint32_t{p[1]} << 8) | (std::uint32_t{p[2]} << 16) |
           (std::uint32_t{p[3]} << 24);
}

std::uint16_t be16(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

std::string trimRight(std::string s)
{
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0'))
        s.pop_back();
    return s;
}

std::string stripVersion(std::string_view name)
{
    const std::size_t semi = name.find(';');
    return std::string(name.substr(0, semi));
}

bool iequals(std::string_view a, std::string_view b)
{
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](unsigned char x, unsigned char y) {
               return std::toupper(x) == std::toupper(y);
           });
}

std::string joinPath(const std::string& dir, const std::string& name)
{
    return dir == "/" ? "/" + name : dir + "/" + name;
}

// Parses one directory record. `parentPath` builds FileEntry::path.
FileEntry parseRecord(const std::uint8_t* rec, std::uint32_t baseLba, const std::string& parentPath)
{
    const std::uint8_t length = rec[0];
    const std::uint8_t nameLength = rec[32];
    if (33u + nameLength > length)
        throw DiscError("iso9660: corrupt directory record");

    FileEntry entry;
    entry.lba = baseLba + le32(rec + 2);
    entry.size = le32(rec + 10);
    entry.isDirectory = (rec[25] & 0x02) != 0;

    const char* rawName = reinterpret_cast<const char*>(rec + 33);
    if (nameLength == 1 && (rawName[0] == '\0' || rawName[0] == '\1'))
        entry.name = rawName[0] == '\0' ? "." : "..";
    else
        entry.name = stripVersion(std::string_view(rawName, nameLength));
    // Names end up in host paths on extraction: refuse anything that could escape.
    if (entry.name.empty() || entry.name.find_first_of("/\\:") != std::string::npos ||
        (entry.name == ".." && !(nameLength == 1 && rawName[0] == '\1')))
        throw DiscError("iso9660: invalid file name in directory '" + parentPath + "'");
    entry.path = joinPath(parentPath, entry.name);

    // System use area (padded to an even offset) may hold the 14-byte CD-XA record.
    const std::size_t su = 33u + nameLength + ((nameLength % 2 == 0) ? 1u : 0u);
    if (su + 14 <= length && rec[su + 6] == 'X' && rec[su + 7] == 'A')
        entry.xaAttributes = be16(rec + su + 4);
    return entry;
}

} // namespace

Iso9660::Iso9660(const DiscImage& disc)
    : m_disc(&disc), m_baseLba(disc.firstDataTrack().startLba)
{
    std::array<std::uint8_t, kUserDataSize> pvd{};
    disc.readSector(m_baseLba + kPvdSector, pvd);
    if (pvd[0] != 1 || std::memcmp(pvd.data() + 1, "CD001", 5) != 0)
        throw DiscError("iso9660: no primary volume descriptor");
    const std::uint32_t blockSize = std::uint32_t{pvd[128]} | (std::uint32_t{pvd[129]} << 8);
    if (blockSize != kUserDataSize)
        throw DiscError("iso9660: unsupported logical block size " + std::to_string(blockSize));

    m_volumeId = trimRight(std::string(reinterpret_cast<const char*>(pvd.data() + 40), 32));
    m_root = parseRecord(pvd.data() + 156, m_baseLba, "/");
    m_root.name = "";
    m_root.path = "/";
}

std::vector<FileEntry> Iso9660::list(const FileEntry& directory) const
{
    if (!directory.isDirectory)
        throw DiscError("iso9660: '" + directory.path + "' is not a directory");

    std::vector<FileEntry> entries;
    std::array<std::uint8_t, kUserDataSize> sector{};
    for (std::uint32_t s = 0; s < directory.sectorCount(); ++s) {
        m_disc->readSector(directory.lba + s, sector);
        std::size_t offset = 0;
        // Records never cross sector boundaries; a zero length byte pads to the next sector.
        while (offset < kUserDataSize && sector[offset] != 0) {
            const std::uint8_t length = sector[offset];
            if (length < 34 || offset + length > kUserDataSize)
                throw DiscError("iso9660: corrupt directory '" + directory.path + "'");
            FileEntry entry = parseRecord(sector.data() + offset, m_baseLba, directory.path);
            if (entry.name != "." && entry.name != "..")
                entries.push_back(std::move(entry));
            offset += length;
        }
    }
    return entries;
}

std::optional<FileEntry> Iso9660::find(std::string_view path) const
{
    FileEntry current = m_root;
    std::size_t pos = 0;
    while (pos < path.size()) {
        const std::size_t sep = path.find_first_of("/\\", pos);
        const std::string_view part = path.substr(pos, sep == std::string_view::npos ? path.npos : sep - pos);
        pos = sep == std::string_view::npos ? path.size() : sep + 1;
        if (part.empty())
            continue;
        if (!current.isDirectory)
            return std::nullopt;

        const std::string wanted = stripVersion(part);
        bool found = false;
        for (FileEntry& entry : list(current)) {
            if (iequals(entry.name, wanted)) {
                current = std::move(entry);
                found = true;
                break;
            }
        }
        if (!found)
            return std::nullopt;
    }
    return current;
}

void Iso9660::walk(const FileEntry& directory, const std::function<void(const FileEntry&)>& visit) const
{
    walkImpl(directory, visit, 0);
}

void Iso9660::walkImpl(const FileEntry& directory, const std::function<void(const FileEntry&)>& visit,
                       int depth) const
{
    if (depth > kMaxDepth)
        throw DiscError("iso9660: directory tree too deep at '" + directory.path + "'");
    for (const FileEntry& entry : list(directory)) {
        visit(entry);
        if (entry.isDirectory)
            walkImpl(entry, visit, depth + 1);
    }
}

std::vector<std::uint8_t> Iso9660::readFile(const FileEntry& file) const
{
    std::vector<std::uint8_t> data(std::size_t{file.sectorCount()} * kUserDataSize);
    for (std::uint32_t s = 0; s < file.sectorCount(); ++s)
        m_disc->readSector(file.lba + s,
                           std::span<std::uint8_t, kUserDataSize>(data.data() + std::size_t{s} * kUserDataSize,
                                                                  kUserDataSize));
    data.resize(file.size);
    return data;
}

std::vector<std::uint8_t> Iso9660::readFileRange(const FileEntry& file, std::uint64_t offset,
                                                 std::size_t size) const
{
    if (offset > file.size || size > file.size - offset)
        throw DiscError("iso9660: read past the end of '" + file.path + "'");

    std::vector<std::uint8_t> data(size);
    std::array<std::uint8_t, kUserDataSize> sector{};
    std::size_t done = 0;
    while (done < size) {
        const std::uint64_t pos = offset + done;
        const auto lba = file.lba + static_cast<std::uint32_t>(pos / kUserDataSize);
        const auto skip = static_cast<std::size_t>(pos % kUserDataSize);
        const std::size_t n = std::min(size - done, std::size_t{kUserDataSize} - skip);
        if (n == kUserDataSize) {
            m_disc->readSector(lba, std::span<std::uint8_t, kUserDataSize>(data.data() + done, kUserDataSize));
        } else {
            m_disc->readSector(lba, sector);
            std::memcpy(data.data() + done, sector.data() + skip, n);
        }
        done += n;
    }
    return data;
}

std::vector<std::uint8_t> Iso9660::readFileStored(const FileEntry& file) const
{
    std::vector<std::uint8_t> data;
    std::array<std::uint8_t, kRawSectorSize> sector{};
    for (std::uint32_t s = 0; s < file.sectorCount(); ++s) {
        const std::uint32_t n = m_disc->readStoredSector(file.lba + s, sector);
        data.insert(data.end(), sector.begin(), sector.begin() + static_cast<std::ptrdiff_t>(n));
    }
    return data;
}

} // namespace gt2ap::discio
