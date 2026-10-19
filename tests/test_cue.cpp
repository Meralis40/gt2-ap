// SPDX-License-Identifier: GPL-2.0-or-later
#include "test.hpp"

#include "gt2ap/discio/cue.hpp"

using namespace gt2ap::discio;

TEST_CASE("cue: msf parsing")
{
    CHECK_EQ(parseMsf("00:00:00"), 0u);
    CHECK_EQ(parseMsf("00:02:00"), 150u);
    CHECK_EQ(parseMsf("01:00:74"), 4574u);
    CHECK_THROWS(parseMsf("00:60:00"));
    CHECK_THROWS(parseMsf("00:00:75"));
    CHECK_THROWS(parseMsf("0000"));
}

TEST_CASE("cue: single file, data and audio tracks")
{
    const CueSheet sheet = parseCue("\xEF\xBB\xBF" "FILE \"Game (Track 1).bin\" BINARY\r\n"
                                    "  TRACK 01 MODE2/2352\r\n"
                                    "    INDEX 01 00:00:00\r\n"
                                    "  TRACK 02 AUDIO\r\n"
                                    "    INDEX 00 10:00:00\r\n"
                                    "    INDEX 01 10:02:00\r\n");
    REQUIRE(sheet.files.size() == 1);
    CHECK_EQ(sheet.files[0], std::string("Game (Track 1).bin"));
    REQUIRE(sheet.tracks.size() == 2);
    CHECK(sheet.tracks[0].mode == TrackMode::Mode2_2352);
    CHECK(sheet.tracks[1].mode == TrackMode::Audio);
    CHECK_EQ(sheet.tracks[1].indexes.at(0), 45000u);
    CHECK_EQ(sheet.tracks[1].indexes.at(1), 45150u);
}

TEST_CASE("cue: rejects malformed sheets")
{
    CHECK_THROWS(parseCue(""));
    CHECK_THROWS(parseCue("TRACK 01 MODE2/2352\nINDEX 01 00:00:00\n"));       // no FILE
    CHECK_THROWS(parseCue("FILE \"a.bin\" BINARY\nTRACK 01 MODE2/2352\n"));   // no INDEX 01
    CHECK_THROWS(parseCue("FILE \"a.bin\" BINARY\nTRACK 01 CDG\nINDEX 01 00:00:00\n"));
    CHECK_THROWS(parseCue("FILE \"a.bin BINARY\n"));
}

TEST_CASE("cue: sector layout helpers")
{
    CHECK_EQ(sectorSize(TrackMode::Mode2_2352), 2352u);
    CHECK_EQ(sectorSize(TrackMode::Mode1_2048), 2048u);
    CHECK_EQ(userDataOffset(TrackMode::Mode2_2352), 24u);
    CHECK_EQ(userDataOffset(TrackMode::Mode1_2352), 16u);
    CHECK_EQ(userDataOffset(TrackMode::Mode2_2336), 8u);
}
