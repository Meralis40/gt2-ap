// SPDX-License-Identifier: GPL-2.0-or-later
#include "gt2ap/discio/vol.hpp"

#include "gt2ap/discio/error.hpp"

#include <algorithm>
#include <cstring>
#include <set>
#include <string_view>

namespace gt2ap::discio {

namespace {

constexpr std::size_t kTableStart = 0x10;
constexpr std::size_t kRecordSize = 0x20;
constexpr std::size_t kNameOffset = 7;
constexpr std::size_t kNameSize = kRecordSize - kNameOffset;
constexpr std::uint8_t kFlagDirectory = 0x01;
constexpr std::uint8_t kFlagLast = 0x80;
constexpr std::uint32_t kSectorMask = ~std::uint32_t{0x7ff};
constexpr int kMaxDepth = 32;

std::uint32_t le32(std::span<const std::uint8_t> d, std::size_t pos)
{
    return std::uint32_t{d[pos]} | (std::uint32_t{d[pos + 1]} << 8) | (std::uint32_t{d[pos + 2]} << 16) |
           (std::uint32_t{d[pos + 3]} << 24);
}

std::uint16_t le16(std::span<const std::uint8_t> d, std::size_t pos)
{
    return static_cast<std::uint16_t>(d[pos] | (d[pos + 1] << 8));
}

struct Record {
    std::uint32_t mtime;
    std::uint16_t index;
    std::uint8_t flags;
    std::string name;
};

class DirectoryWalker {
public:
    DirectoryWalker(std::span<const std::uint8_t> dir, const std::vector<std::uint32_t>& table)
        : m_dir(dir), m_table(table), m_count(dir.size() / kRecordSize)
    {
    }

    void walk(std::size_t first, const std::string& path, int depth)
    {
        if (depth > kMaxDepth)
            throw DiscError("GT2.VOL: directory tree too deep at '" + path + "'");
        if (!m_visited.insert(first).second)
            throw DiscError("GT2.VOL: directory '" + path + "' visited twice");

        for (std::size_t i = first;; ++i) {
            if (i >= m_count)
                throw DiscError("GT2.VOL: unterminated directory '" + path + "'");
            const Record r = record(i);
            const bool isDir = (r.flags & kFlagDirectory) != 0;
            if (!(isDir && r.name == "..")) {
                checkName(r.name, path);
                const std::string child = path + r.name;
                if (isDir) {
                    walk(r.index, child + "/", depth + 1);
                } else {
                    if (r.index + std::size_t{1} >= m_table.size())
                        throw DiscError("GT2.VOL: '" + child + "' has no file table entry");
                    m_files.push_back({child, r.index, volExtent(m_table, r.index), r.mtime});
                }
            }
            if (r.flags & kFlagLast)
                break;
        }
    }

    std::vector<VolFileEntry> take() { return std::move(m_files); }

private:
    Record record(std::size_t i) const
    {
        const std::size_t pos = i * kRecordSize;
        const auto* raw = reinterpret_cast<const char*>(m_dir.data() + pos + kNameOffset);
        return {le32(m_dir, pos), le16(m_dir, pos + 4), m_dir[pos + 6],
                std::string(raw, strnlen(raw, kNameSize))};
    }

    // Names end up in host paths on extraction: refuse anything that could escape.
    static void checkName(const std::string& name, const std::string& dir)
    {
        bool ok = !name.empty() && name != "." && name != "..";
        for (const char c : name)
            ok = ok && static_cast<unsigned char>(c) >= 0x20 && c != '/' && c != '\\' && c != ':';
        if (!ok)
            throw DiscError("GT2.VOL: invalid file name in directory '" + dir + "'");
    }

    std::span<const std::uint8_t> m_dir;
    const std::vector<std::uint32_t>& m_table;
    std::size_t m_count;
    std::set<std::size_t> m_visited;
    std::vector<VolFileEntry> m_files;
};

} // namespace

std::vector<std::uint32_t> parseVolTable(std::span<const std::uint8_t> header, std::uint64_t volSize)
{
    if (header.size() < kTableStart + 8 || std::memcmp(header.data(), "GTFS", 4) != 0)
        throw DiscError("GT2.VOL: missing GTFS header");
    const std::size_t end = std::min(header.size(), kVolHeaderSize) & ~std::size_t{3};

    std::vector<std::uint32_t> table;
    for (std::size_t pos = kTableStart; pos < end; pos += 4) {
        const std::uint32_t entry = le32(header, pos);
        if (entry == 0 && !table.empty())
            return table;
        if (!table.empty() && (entry & kSectorMask) < table.back())
            throw DiscError("GT2.VOL: file table entry " + std::to_string(table.size()) + " goes backwards");
        if ((entry & kSectorMask) > volSize)
            throw DiscError("GT2.VOL: file table entry " + std::to_string(table.size()) + " past the end");
        table.push_back(entry);
    }
    throw DiscError("GT2.VOL: unterminated file table");
}

VolExtent volExtent(const std::vector<std::uint32_t>& table, std::uint32_t index)
{
    if (std::size_t{index} + 1 >= table.size())
        throw DiscError("GT2.VOL: no file " + std::to_string(index));
    // parseVolTable guarantees next & ~0x7ff >= this entry.
    return {table[index] & kSectorMask, (table[index + 1] & kSectorMask) - table[index]};
}

std::vector<VolFileEntry> parseVolDirectory(std::span<const std::uint8_t> directory,
                                            const std::vector<std::uint32_t>& table)
{
    DirectoryWalker walker(directory, table);
    walker.walk(0, "/", 0);
    return walker.take();
}

} // namespace gt2ap::discio
