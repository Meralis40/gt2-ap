// SPDX-License-Identifier: GPL-2.0-or-later
#include "test.hpp"

#include "gt2ap/discio/vol.hpp"

#include <cstring>
#include <string>
#include <vector>

using namespace gt2ap::discio;

namespace {

void putLe32(std::vector<std::uint8_t>& v, std::size_t pos, std::uint32_t x)
{
    for (int i = 0; i < 4; ++i)
        v[pos + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(x >> (8 * i));
}

struct Rec {
    std::uint16_t index;
    std::uint8_t flags;
    const char* name;
};

std::vector<std::uint8_t> directory(const std::vector<Rec>& recs)
{
    std::vector<std::uint8_t> dir(recs.size() * 0x20);
    for (std::size_t i = 0; i < recs.size(); ++i) {
        const std::size_t pos = i * 0x20;
        putLe32(dir, pos, 0x3868f5c3);
        dir[pos + 4] = static_cast<std::uint8_t>(recs[i].index);
        dir[pos + 5] = static_cast<std::uint8_t>(recs[i].index >> 8);
        dir[pos + 6] = recs[i].flags;
        std::memcpy(dir.data() + pos + 7, recs[i].name, std::strlen(recs[i].name));
    }
    return dir;
}

// Layout of the real GT2.VOL in miniature:
//   /.carinfo        file 2
//   /sound/          records 3..5
//   /sound/sys.ins   file 3
//   /arc.dat         file 4
std::vector<std::uint8_t> sampleDirectory()
{
    return directory({
        {2, 0x00, ".carinfo"},
        {3, 0x01, "sound"},
        {4, 0x80, "arc.dat"},
        {0, 0x01, ".."},
        {3, 0x80, "sys.ins"},
    });
}

// File table: 0 = header, 1 = directory, 2..4 = files; sizes 0x1234, 0x800, 0x10.
std::vector<std::uint32_t> sampleTableEntries()
{
    return {0x0000 | 0x700, 0x0800 | 0x760, 0x1000 | 0x5cc, 0x2800 | 0x000, 0x3000 | 0x7f0, 0x3800, 0};
}

std::vector<std::uint8_t> header(const std::vector<std::uint32_t>& entries)
{
    std::vector<std::uint8_t> h(0x100);
    std::memcpy(h.data(), "GTFS", 4);
    for (std::size_t i = 0; i < entries.size(); ++i)
        putLe32(h, 0x10 + 4 * i, entries[i]);
    return h;
}

} // namespace

TEST_CASE("vol: file table parsing and extents")
{
    const auto table = parseVolTable(header(sampleTableEntries()), 0x3800);
    REQUIRE(table.size() == 6);
    CHECK_EQ(volExtent(table, 0).size, 0x100u);
    CHECK_EQ(volExtent(table, 1).offset, 0x800u);
    CHECK_EQ(volExtent(table, 2).offset, 0x1000u);
    CHECK_EQ(volExtent(table, 2).size, 0x1234u);
    CHECK_EQ(volExtent(table, 3).size, 0x800u);
    CHECK_EQ(volExtent(table, 4).size, 0x10u);
    CHECK_THROWS(volExtent(table, 5));
}

TEST_CASE("vol: malformed file tables are rejected")
{
    auto badMagic = header(sampleTableEntries());
    badMagic[0] = 'X';
    CHECK_THROWS(parseVolTable(badMagic, 0x3800));
    CHECK_THROWS(parseVolTable(header(sampleTableEntries()), 0x3000)); // past the end of the VOL
    auto backwards = sampleTableEntries();
    backwards[3] = 0x0800;
    CHECK_THROWS(parseVolTable(header(backwards), 0x3800));
    std::vector<std::uint32_t> unterminated(0x3c, 0x800);
    CHECK_THROWS(parseVolTable(header(unterminated), 0x3800));
}

TEST_CASE("vol: directory walk")
{
    const auto table = parseVolTable(header(sampleTableEntries()), 0x3800);
    const auto files = parseVolDirectory(sampleDirectory(), table);
    REQUIRE(files.size() == 3);
    CHECK_EQ(files[0].path, std::string("/.carinfo"));
    CHECK_EQ(files[0].extent.size, 0x1234u);
    CHECK_EQ(files[1].path, std::string("/sound/sys.ins"));
    CHECK_EQ(files[1].index, 3u);
    CHECK_EQ(files[2].path, std::string("/arc.dat"));
    CHECK_EQ(files[2].extent.offset, 0x3000u);
    CHECK_EQ(files[2].mtime, 0x3868f5c3u);
}

TEST_CASE("vol: unsafe or broken directories are rejected")
{
    const auto table = parseVolTable(header(sampleTableEntries()), 0x3800);
    CHECK_THROWS(parseVolDirectory(directory({{2, 0x80, "../evil"}}), table));
    CHECK_THROWS(parseVolDirectory(directory({{2, 0x80, "a\\b"}}), table));
    CHECK_THROWS(parseVolDirectory(directory({{2, 0x80, ".."}}), table));       // ".." as a file
    CHECK_THROWS(parseVolDirectory(directory({{9, 0x80, "far"}}), table));      // no table entry
    CHECK_THROWS(parseVolDirectory(directory({{2, 0x00, "open"}}), table));     // no last-record flag
    CHECK_THROWS(parseVolDirectory(directory({{0, 0x81, "loop"}}), table));     // directory points at root
    CHECK_THROWS(parseVolDirectory(directory({{7, 0x81, "nowhere"}}), table));  // child past the end
}
