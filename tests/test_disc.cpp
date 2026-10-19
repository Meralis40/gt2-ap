// SPDX-License-Identifier: GPL-2.0-or-later
#include "test.hpp"

#include "synthetic_disc.hpp"

#include "gt2ap/discio/disc_image.hpp"
#include "gt2ap/discio/game_id.hpp"
#include "gt2ap/discio/iso9660.hpp"

using namespace gt2ap::discio;
using synthetic::Layout;

namespace {

// Checks the synthetic filesystem through any image format.
void checkFilesystem(const DiscImage& disc)
{
    const Iso9660 iso(disc);
    CHECK_EQ(iso.volumeId(), std::string("GT2AP_TEST"));

    const auto game = identify(iso);
    REQUIRE(game.has_value());
    CHECK_EQ(game->serial, std::string("SCUS-94455"));
    CHECK_EQ(game->bootFile, std::string("SCUS_944.55"));
    CHECK(game->disc == Gt2Disc::Simulation);

    const auto big = iso.find("data\\big.bin;1");
    REQUIRE(big.has_value());
    CHECK_EQ(big->path, std::string("/DATA/BIG.BIN"));
    CHECK(!big->needsRawRead());
    const auto bytes = iso.readFile(*big);
    REQUIRE(bytes.size() == Layout::kBigSize);
    bool contentOk = true;
    for (std::size_t i = 0; i < bytes.size(); ++i)
        contentOk = contentOk && bytes[i] == static_cast<std::uint8_t>(i % 251);
    CHECK(contentOk);

    const auto movie = iso.find("/DATA/MOVIE.STR");
    REQUIRE(movie.has_value());
    CHECK(movie->needsRawRead());
    CHECK_EQ(movie->sectorCount(), Layout::kMovieSectors);

    CHECK(!iso.find("/DATA/NOPE.BIN").has_value());
    CHECK(!iso.find("/SYSTEM.CNF/X").has_value());

    std::size_t files = 0, dirs = 0;
    iso.walk(iso.root(), [&](const FileEntry& e) { (e.isDirectory ? dirs : files)++; });
    CHECK_EQ(files, std::size_t{3});
    CHECK_EQ(dirs, std::size_t{1});
}

} // namespace

TEST_CASE("disc: plain 2048-byte .iso")
{
    synthetic::TempDir dir;
    synthetic::writeFile(dir.path() / "test.iso", synthetic::buildIso());
    const DiscImage disc = DiscImage::open(dir.path() / "test.iso");
    REQUIRE(disc.tracks().size() == 1);
    CHECK(disc.tracks()[0].mode == TrackMode::Mode1_2048);
    checkFilesystem(disc);
}

TEST_CASE("disc: raw MODE2/2352 .bin without cue")
{
    synthetic::TempDir dir;
    synthetic::writeFile(dir.path() / "test.bin", synthetic::buildRawMode2());
    const DiscImage disc = DiscImage::open(dir.path() / "test.bin");
    REQUIRE(disc.tracks().size() == 1);
    CHECK(disc.tracks()[0].mode == TrackMode::Mode2_2352);
    checkFilesystem(disc);
}

TEST_CASE("disc: cue with data track and audio track in separate files")
{
    synthetic::TempDir dir;
    synthetic::writeFile(dir.path() / "Game (Track 1).bin", synthetic::buildRawMode2());
    // 2 s pregap stored in the audio file (INDEX 00), then 10 sectors of audio.
    synthetic::writeFile(dir.path() / "Game (Track 2).bin", std::vector<std::uint8_t>((150 + 10) * 2352, 0));
    synthetic::writeText(dir.path() / "Game.cue", "FILE \"Game (Track 1).bin\" BINARY\n"
                                                  "  TRACK 01 MODE2/2352\n"
                                                  "    INDEX 01 00:00:00\n"
                                                  "FILE \"Game (Track 2).bin\" BINARY\n"
                                                  "  TRACK 02 AUDIO\n"
                                                  "    INDEX 00 00:00:00\n"
                                                  "    INDEX 01 00:02:00\n");
    const DiscImage disc = DiscImage::open(dir.path() / "Game.cue");
    REQUIRE(disc.tracks().size() == 2);
    const Track& audio = disc.tracks()[1];
    CHECK_EQ(audio.startLba, Layout::kSectors + 150);
    CHECK_EQ(audio.sectorCount, 10u);
    CHECK_EQ(audio.fileOffset, std::uint64_t{150} * 2352);
    CHECK(disc.trackAt(Layout::kSectors + 100) == nullptr); // pregap is not addressable data

    std::array<std::uint8_t, kUserDataSize> buffer{};
    CHECK_THROWS(disc.readSector(audio.startLba, buffer));
    CHECK_THROWS(disc.readSector(audio.startLba + 10, buffer));

    CHECK(disc.isForm2(Layout::kMovieLba));
    CHECK(!disc.isForm2(16));
    checkFilesystem(disc);
}

TEST_CASE("disc: cue with PREGAP shifts later tracks")
{
    synthetic::TempDir dir;
    auto image = synthetic::buildRawMode2();
    image.resize(image.size() + 20 * 2352, 0); // 20 audio sectors after the data
    synthetic::writeFile(dir.path() / "game.bin", image);
    synthetic::writeText(dir.path() / "game.cue", "FILE \"game.bin\" BINARY\n"
                                                  "  TRACK 01 MODE2/2352\n"
                                                  "    INDEX 01 00:00:00\n"
                                                  "  TRACK 02 AUDIO\n"
                                                  "    PREGAP 00:02:00\n"
                                                  "    INDEX 01 00:00:26\n");
    const DiscImage disc = DiscImage::open(dir.path() / "game.cue");
    REQUIRE(disc.tracks().size() == 2);
    CHECK_EQ(disc.tracks()[0].sectorCount, Layout::kSectors);
    CHECK_EQ(disc.tracks()[1].startLba, Layout::kSectors + 150);
    CHECK_EQ(disc.tracks()[1].sectorCount, 20u);
    checkFilesystem(disc);
}

TEST_CASE("disc: rejects unusable images")
{
    synthetic::TempDir dir;
    synthetic::writeFile(dir.path() / "odd.bin", std::vector<std::uint8_t>(1000, 0));
    CHECK_THROWS(DiscImage::open(dir.path() / "odd.bin"));
    CHECK_THROWS(DiscImage::open(dir.path() / "missing.cue"));

    synthetic::writeText(dir.path() / "missing-bin.cue", "FILE \"nope.bin\" BINARY\nTRACK 01 MODE2/2352\nINDEX 01 00:00:00\n");
    CHECK_THROWS(DiscImage::open(dir.path() / "missing-bin.cue"));

    // Valid size but no ISO9660 filesystem.
    synthetic::writeFile(dir.path() / "blank.iso", std::vector<std::uint8_t>(32 * 2048, 0));
    const DiscImage blank = DiscImage::open(dir.path() / "blank.iso");
    CHECK_THROWS(Iso9660(blank));
}

TEST_CASE("game id: SYSTEM.CNF parsing")
{
    auto sim = parseSystemCnf("BOOT = cdrom:\\SCUS_944.55;1\r\nTCB = 4\r\n");
    REQUIRE(sim.has_value());
    CHECK(sim->disc == Gt2Disc::Simulation);

    auto arcade = parseSystemCnf("boot=cdrom:SCUS_944.88;1");
    REQUIRE(arcade.has_value());
    CHECK_EQ(arcade->serial, std::string("SCUS-94488"));
    CHECK(arcade->disc == Gt2Disc::Arcade);

    auto other = parseSystemCnf("BOOT = cdrom:\\SLUS_123.45;1");
    REQUIRE(other.has_value());
    CHECK(other->disc == Gt2Disc::Unknown);

    CHECK(!parseSystemCnf("BOOT2 = cdrom0:\\SLUS_200.00;1").has_value());
    CHECK(!parseSystemCnf("").has_value());
    CHECK(findKnownDump("0000") == nullptr);
}
