// SPDX-License-Identifier: GPL-2.0-or-later
#include "gt2ap/discio/disc_image.hpp"

#include "gt2ap/discio/error.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

namespace gt2ap::discio {

namespace fs = std::filesystem;

namespace {

constexpr std::array<std::uint8_t, 12> kSync = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

std::string lowerExtension(const fs::path& p)
{
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::uint64_t fileSize(const fs::path& p)
{
    std::error_code ec;
    const auto size = fs::file_size(p, ec);
    if (ec)
        throw DiscError("cannot access '" + p.string() + "': " + ec.message());
    return size;
}

} // namespace

DiscImage::~DiscImage() = default;

DiscImage DiscImage::open(const fs::path& path)
{
    DiscImage image = lowerExtension(path) == ".cue" ? fromCue(path) : fromSingleFile(path);
    image.openStreams();
    return image;
}

DiscImage DiscImage::fromCue(const fs::path& cuePath)
{
    std::ifstream in(cuePath, std::ios::binary);
    if (!in)
        throw DiscError("cannot open '" + cuePath.string() + "'");
    std::stringstream buffer;
    buffer << in.rdbuf();
    const CueSheet sheet = parseCue(buffer.str());

    DiscImage image;
    const fs::path baseDir = cuePath.parent_path();
    for (const std::string& name : sheet.files)
        image.m_files.push_back(baseDir / fs::path(std::u8string_view(
                                              reinterpret_cast<const char8_t*>(name.data()), name.size())));

    // Lay the files out one after another on the disc. PREGAP/POSTGAP sectors are
    // not stored in the files but still shift the absolute LBAs that follow.
    std::uint32_t discCursor = 0;
    for (std::size_t f = 0; f < image.m_files.size(); ++f) {
        std::vector<const CueTrack*> fileTracks;
        for (const CueTrack& t : sheet.tracks)
            if (t.fileIndex == f)
                fileTracks.push_back(&t);
        if (fileTracks.empty())
            throw DiscError("cue: FILE '" + sheet.files[f] + "' has no tracks");

        const std::uint32_t size = sectorSize(fileTracks.front()->mode);
        for (const CueTrack* t : fileTracks)
            if (sectorSize(t->mode) != size)
                throw DiscError("cue: mixed sector sizes within one FILE are not supported");

        const std::uint64_t bytes = fileSize(image.m_files[f]);
        if (bytes % size != 0)
            throw DiscError("'" + image.m_files[f].string() + "' is not a whole number of " +
                            std::to_string(size) + "-byte sectors");
        const auto fileSectors = static_cast<std::uint32_t>(bytes / size);

        std::uint32_t gapShift = 0; // PREGAP/POSTGAP sectors inserted so far in this file
        for (std::size_t i = 0; i < fileTracks.size(); ++i) {
            const CueTrack& ct = *fileTracks[i];
            const std::uint32_t index1 = ct.indexes.at(1);
            gapShift += ct.pregapFrames;

            // The track's stored data ends where the next track in the same file begins
            // (its INDEX 00 if present, i.e. the next track's pregap stored in the file).
            std::uint32_t end = fileSectors;
            if (i + 1 < fileTracks.size())
                end = fileTracks[i + 1]->indexes.begin()->second;
            if (index1 > end || index1 > fileSectors)
                throw DiscError("cue: track " + std::to_string(ct.number) + " starts past its data");

            Track track;
            track.number = ct.number;
            track.mode = ct.mode;
            track.fileIndex = f;
            track.fileOffset = std::uint64_t{index1} * size;
            track.startLba = discCursor + gapShift + index1;
            track.sectorCount = end - index1;
            image.m_tracks.push_back(track);

            gapShift += ct.postgapFrames;
        }
        discCursor += fileSectors + gapShift;
    }
    return image;
}

DiscImage DiscImage::fromSingleFile(const fs::path& path)
{
    const std::uint64_t bytes = fileSize(path);
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw DiscError("cannot open '" + path.string() + "'");
    std::array<std::uint8_t, 16> head{};
    in.read(reinterpret_cast<char*>(head.data()), static_cast<std::streamsize>(head.size()));

    Track track;
    track.number = 1;
    if (bytes % kRawSectorSize == 0 && in.gcount() == 16 && std::equal(kSync.begin(), kSync.end(), head.begin())) {
        // Raw sectors: the header's mode byte tells Mode 1 from Mode 2.
        track.mode = head[15] == 1 ? TrackMode::Mode1_2352 : TrackMode::Mode2_2352;
        track.sectorCount = static_cast<std::uint32_t>(bytes / kRawSectorSize);
    } else if (bytes % kUserDataSize == 0) {
        track.mode = TrackMode::Mode1_2048;
        track.sectorCount = static_cast<std::uint32_t>(bytes / kUserDataSize);
    } else {
        throw DiscError("'" + path.string() + "' is not a recognised disc image (use the .cue)");
    }

    DiscImage image;
    image.m_files.push_back(path);
    image.m_tracks.push_back(track);
    return image;
}

void DiscImage::openStreams()
{
    for (const fs::path& p : m_files) {
        auto stream = std::make_unique<std::ifstream>(p, std::ios::binary);
        if (!*stream)
            throw DiscError("cannot open '" + p.string() + "'");
        m_streams.push_back(std::move(stream));
    }
}

const Track& DiscImage::firstDataTrack() const
{
    for (const Track& t : m_tracks)
        if (t.isData())
            return t;
    throw DiscError("disc has no data track");
}

const Track* DiscImage::trackAt(std::uint32_t lba) const
{
    for (const Track& t : m_tracks)
        if (t.contains(lba))
            return &t;
    return nullptr;
}

const Track& DiscImage::requireTrack(std::uint32_t lba) const
{
    const Track* track = trackAt(lba);
    if (!track)
        throw DiscError("LBA " + std::to_string(lba) + " is outside the image");
    return *track;
}

std::uint32_t DiscImage::readStoredSector(std::uint32_t lba, std::span<std::uint8_t> out) const
{
    const Track& track = requireTrack(lba);
    const std::uint32_t size = sectorSize(track.mode);
    if (out.size() < size)
        throw DiscError("readStoredSector: buffer too small");

    std::ifstream& stream = *m_streams[track.fileIndex];
    const std::uint64_t offset = track.fileOffset + std::uint64_t{lba - track.startLba} * size;
    stream.clear();
    stream.seekg(static_cast<std::streamoff>(offset));
    stream.read(reinterpret_cast<char*>(out.data()), size);
    if (stream.gcount() != static_cast<std::streamsize>(size))
        throw DiscError("short read at LBA " + std::to_string(lba));
    return size;
}

void DiscImage::readSector(std::uint32_t lba, std::span<std::uint8_t, kUserDataSize> out) const
{
    const Track& track = requireTrack(lba);
    if (!track.isData())
        throw DiscError("LBA " + std::to_string(lba) + " is in an audio track");
    std::array<std::uint8_t, kRawSectorSize> raw{};
    readStoredSector(lba, raw);
    std::memcpy(out.data(), raw.data() + userDataOffset(track.mode), kUserDataSize);
}

bool DiscImage::isForm2(std::uint32_t lba) const
{
    const Track& track = requireTrack(lba);
    if (track.mode != TrackMode::Mode2_2352 && track.mode != TrackMode::Mode2_2336)
        return false;
    std::array<std::uint8_t, kRawSectorSize> raw{};
    readStoredSector(lba, raw);
    // Subheader: file, channel, submode, coding (repeated). Submode bit 5 = Form 2.
    const std::size_t subheader = track.mode == TrackMode::Mode2_2352 ? 16 : 0;
    return (raw[subheader + 2] & 0x20) != 0;
}

} // namespace gt2ap::discio
