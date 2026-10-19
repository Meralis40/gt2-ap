// SPDX-License-Identifier: GPL-2.0-or-later
// gt2-discinfo: inspect, verify and extract the user's GT2 disc images.

#include "gt2ap/discio/disc_image.hpp"
#include "gt2ap/discio/error.hpp"
#include "gt2ap/discio/game_id.hpp"
#include "gt2ap/discio/iso9660.hpp"
#include "gt2ap/discio/sha1.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace gt2ap::discio;
namespace fs = std::filesystem;

namespace {

void usage()
{
    std::fputs("usage:\n"
               "  gt2-discinfo info <image.cue|bin|iso>\n"
               "  gt2-discinfo ls <image> [dir]\n"
               "  gt2-discinfo extract <image> <path-on-disc> <output-file>\n"
               "  gt2-discinfo extract-all <image> <output-dir>\n"
               "\n"
               "Form 2 / interleaved files (XA audio, STR video) are written as stored sectors.\n",
               stderr);
}

void writeFile(const fs::path& path, const std::vector<std::uint8_t>& data)
{
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out)
        throw DiscError("cannot write '" + path.string() + "'");
}

std::string attributes(const FileEntry& e)
{
    if (e.isDirectory)
        return "dir ";
    if (e.xaAttributes & xa::kInterleaved)
        return "intl";
    if (e.xaAttributes & xa::kMode2Form2)
        return "frm2";
    return "    ";
}

int cmdInfo(const fs::path& imagePath)
{
    const DiscImage disc = DiscImage::open(imagePath);
    std::printf("Tracks:\n");
    for (const Track& t : disc.tracks())
        std::printf("  %02d %-10s LBA %7u  %7u sectors  %s\n", t.number, toString(t.mode), t.startLba,
                    t.sectorCount, disc.files()[t.fileIndex].filename().string().c_str());

    const Iso9660 iso(disc);
    std::printf("Volume ID: %s\n", iso.volumeId().c_str());
    if (const auto game = identify(iso)) {
        std::printf("Serial:    %s (boot %s)\n", game->serial.c_str(), game->bootFile.c_str());
        std::printf("Game:      %s\n", toString(game->disc));
    } else {
        std::printf("Serial:    none (no SYSTEM.CNF)\n");
    }

    const fs::path& track1File = disc.files()[disc.firstDataTrack().fileIndex];
    const std::string sha1 = Sha1::toHex(Sha1::ofFile(track1File));
    std::printf("Track 01 SHA-1: %s\n", sha1.c_str());
    if (const KnownDump* dump = findKnownDump(sha1))
        std::printf("Verified:  %s revision %.*s\n", std::string(dump->serial).c_str(),
                    static_cast<int>(dump->revision.size()), dump->revision.data());
    else
        std::printf("Verified:  no (not a known NTSC-U 1.2 dump)\n");
    return 0;
}

int cmdLs(const fs::path& imagePath, const std::string& dir)
{
    const DiscImage disc = DiscImage::open(imagePath);
    const Iso9660 iso(disc);
    const auto start = iso.find(dir);
    if (!start || !start->isDirectory) {
        std::fprintf(stderr, "not a directory: %s\n", dir.c_str());
        return 1;
    }
    iso.walk(*start, [](const FileEntry& e) {
        std::printf("%s %10u  LBA %7u  %s\n", attributes(e).c_str(), e.size, e.lba, e.path.c_str());
    });
    return 0;
}

int cmdExtract(const fs::path& imagePath, const std::string& pathOnDisc, const fs::path& output)
{
    const DiscImage disc = DiscImage::open(imagePath);
    const Iso9660 iso(disc);
    const auto entry = iso.find(pathOnDisc);
    if (!entry || entry->isDirectory) {
        std::fprintf(stderr, "no such file: %s\n", pathOnDisc.c_str());
        return 1;
    }
    writeFile(output, entry->needsRawRead() ? iso.readFileStored(*entry) : iso.readFile(*entry));
    return 0;
}

int cmdExtractAll(const fs::path& imagePath, const fs::path& outDir)
{
    const DiscImage disc = DiscImage::open(imagePath);
    const Iso9660 iso(disc);
    std::size_t count = 0;
    iso.walk(iso.root(), [&](const FileEntry& e) {
        const fs::path target = outDir / fs::path(e.path.substr(1));
        if (e.isDirectory) {
            fs::create_directories(target);
            return;
        }
        fs::create_directories(target.parent_path());
        writeFile(target, e.needsRawRead() ? iso.readFileStored(e) : iso.readFile(e));
        ++count;
    });
    std::printf("Extracted %zu files to %s\n", count, outDir.string().c_str());
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    const std::vector<std::string> args(argv + 1, argv + argc);
    try {
        if (args.size() == 2 && args[0] == "info")
            return cmdInfo(args[1]);
        if ((args.size() == 2 || args.size() == 3) && args[0] == "ls")
            return cmdLs(args[1], args.size() == 3 ? args[2] : "/");
        if (args.size() == 4 && args[0] == "extract")
            return cmdExtract(args[1], args[2], args[3]);
        if (args.size() == 3 && args[0] == "extract-all")
            return cmdExtractAll(args[1], args[2]);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    usage();
    return 2;
}
