// SPDX-License-Identifier: GPL-2.0-or-later
// Small table-free DEFLATE decoder in the style of zlib's puff.c: canonical
// Huffman codes decoded one bit at a time. Speed is irrelevant for the few
// hundred KB of game data we decompress.
#include "gt2ap/discio/gzip.hpp"

#include "gt2ap/discio/error.hpp"

#include <array>

namespace gt2ap::discio {

namespace {

constexpr int kMaxBits = 15;
constexpr int kMaxLitLen = 288;
constexpr int kMaxDist = 30;

class BitReader {
public:
    explicit BitReader(std::span<const std::uint8_t> data) : m_data(data) {}

    std::uint32_t bits(int count)
    {
        while (m_bitCount < count) {
            if (m_pos >= m_data.size())
                throw DiscError("deflate: unexpected end of data");
            m_bitBuffer |= static_cast<std::uint32_t>(m_data[m_pos++]) << m_bitCount;
            m_bitCount += 8;
        }
        const std::uint32_t value = m_bitBuffer & ((1u << count) - 1u);
        m_bitBuffer >>= count;
        m_bitCount -= count;
        return value;
    }

    // Drops the remaining bits of the current byte (stored blocks).
    void alignToByte()
    {
        m_bitBuffer = 0;
        m_bitCount = 0;
    }

    std::uint16_t le16()
    {
        const std::uint8_t lo = byte();
        return static_cast<std::uint16_t>(lo | byte() << 8);
    }

    std::uint8_t byte()
    {
        if (m_pos >= m_data.size())
            throw DiscError("deflate: unexpected end of data");
        return m_data[m_pos++];
    }

    std::size_t position() const { return m_pos; }

private:
    std::span<const std::uint8_t> m_data;
    std::size_t m_pos = 0;
    std::uint32_t m_bitBuffer = 0;
    int m_bitCount = 0;
};

struct Huffman {
    std::array<std::uint16_t, kMaxBits + 1> count{};
    std::array<std::uint16_t, kMaxLitLen> symbol{};
};

// Builds a canonical code from per-symbol bit lengths. Incomplete codes are
// accepted (a single distance code is legal); over-subscribed ones are not.
Huffman buildHuffman(std::span<const std::uint8_t> lengths)
{
    Huffman h;
    for (const std::uint8_t len : lengths)
        ++h.count[len];
    if (h.count[0] == lengths.size())
        return h;

    int left = 1;
    for (int len = 1; len <= kMaxBits; ++len) {
        left <<= 1;
        left -= h.count[static_cast<std::size_t>(len)];
        if (left < 0)
            throw DiscError("deflate: over-subscribed Huffman code");
    }

    std::array<std::uint16_t, kMaxBits + 1> offsets{};
    for (std::size_t len = 1; len < kMaxBits; ++len)
        offsets[len + 1] = static_cast<std::uint16_t>(offsets[len] + h.count[len]);
    for (std::size_t sym = 0; sym < lengths.size(); ++sym)
        if (lengths[sym] != 0)
            h.symbol[offsets[lengths[sym]]++] = static_cast<std::uint16_t>(sym);
    return h;
}

int decodeSymbol(BitReader& in, const Huffman& h)
{
    int code = 0;  // bits read so far, MSB first
    int first = 0; // first code of the current length
    int index = 0; // index of that code in h.symbol
    for (std::size_t len = 1; len <= kMaxBits; ++len) {
        code |= static_cast<int>(in.bits(1));
        const int count = h.count[len];
        if (code - count < first)
            return h.symbol[static_cast<std::size_t>(index + (code - first))];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    throw DiscError("deflate: invalid Huffman code");
}

constexpr std::array<std::uint16_t, 29> kLengthBase = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,
                                                       15, 17, 19, 23, 27, 31, 35, 43, 51,  59,
                                                       67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr std::array<std::uint8_t, 29> kLengthExtra = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
                                                       2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr std::array<std::uint16_t, 30> kDistBase = {
    1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
constexpr std::array<std::uint8_t, 30> kDistExtra = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                                                     6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

void inflateCodes(BitReader& in, std::vector<std::uint8_t>& out, const Huffman& lit, const Huffman& dist)
{
    for (;;) {
        const int sym = decodeSymbol(in, lit);
        if (sym < 256) {
            out.push_back(static_cast<std::uint8_t>(sym));
            continue;
        }
        if (sym == 256)
            return;

        const auto li = static_cast<std::size_t>(sym - 257);
        if (li >= kLengthBase.size())
            throw DiscError("deflate: invalid length symbol");
        const std::size_t length = kLengthBase[li] + in.bits(kLengthExtra[li]);

        const auto di = static_cast<std::size_t>(decodeSymbol(in, dist));
        if (di >= kDistBase.size())
            throw DiscError("deflate: invalid distance symbol");
        const std::size_t distance = kDistBase[di] + in.bits(kDistExtra[di]);
        if (distance > out.size())
            throw DiscError("deflate: distance too far back");

        // Byte by byte: source and destination may overlap.
        const std::size_t from = out.size() - distance;
        for (std::size_t i = 0; i < length; ++i)
            out.push_back(out[from + i]);
    }
}

void inflateStored(BitReader& in, std::vector<std::uint8_t>& out)
{
    in.alignToByte();
    const std::uint16_t len = in.le16();
    const std::uint16_t nlen = in.le16();
    if (len != static_cast<std::uint16_t>(~nlen))
        throw DiscError("deflate: stored block length mismatch");
    for (std::uint16_t i = 0; i < len; ++i)
        out.push_back(in.byte());
}

void inflateFixed(BitReader& in, std::vector<std::uint8_t>& out)
{
    static const auto codes = [] {
        std::array<std::uint8_t, kMaxLitLen> litLengths{};
        for (std::size_t i = 0; i < kMaxLitLen; ++i)
            litLengths[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
        std::array<std::uint8_t, kMaxDist> distLengths{};
        distLengths.fill(5);
        return std::pair{buildHuffman(litLengths), buildHuffman(distLengths)};
    }();
    inflateCodes(in, out, codes.first, codes.second);
}

void inflateDynamic(BitReader& in, std::vector<std::uint8_t>& out)
{
    const std::size_t nlen = in.bits(5) + 257;
    const std::size_t ndist = in.bits(5) + 1;
    const std::size_t ncode = in.bits(4) + 4;
    if (nlen > 286 || ndist > kMaxDist)
        throw DiscError("deflate: bad code counts");

    static constexpr std::array<std::uint8_t, 19> kOrder = {16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                                            11, 4,  12, 3, 13, 2, 14, 1, 15};
    std::array<std::uint8_t, 19> codeLengths{};
    for (std::size_t i = 0; i < ncode; ++i)
        codeLengths[kOrder[i]] = static_cast<std::uint8_t>(in.bits(3));
    const Huffman lencode = buildHuffman(codeLengths);

    std::array<std::uint8_t, kMaxLitLen + kMaxDist> lengths{};
    std::size_t index = 0;
    while (index < nlen + ndist) {
        const int sym = decodeSymbol(in, lencode);
        if (sym < 16) {
            lengths[index++] = static_cast<std::uint8_t>(sym);
            continue;
        }
        std::uint8_t value = 0;
        std::size_t repeat = 0;
        if (sym == 16) {
            if (index == 0)
                throw DiscError("deflate: repeat with no previous length");
            value = lengths[index - 1];
            repeat = 3 + in.bits(2);
        } else if (sym == 17) {
            repeat = 3 + in.bits(3);
        } else {
            repeat = 11 + in.bits(7);
        }
        if (index + repeat > nlen + ndist)
            throw DiscError("deflate: too many code lengths");
        while (repeat-- > 0)
            lengths[index++] = value;
    }
    if (lengths[256] == 0)
        throw DiscError("deflate: missing end-of-block code");

    const std::span<const std::uint8_t> all(lengths);
    inflateCodes(in, out, buildHuffman(all.first(nlen)), buildHuffman(all.subspan(nlen, ndist)));
}

// Returns the decoded data; `consumed` receives the number of input bytes used.
std::vector<std::uint8_t> inflateImpl(std::span<const std::uint8_t> data, std::size_t& consumed)
{
    BitReader in(data);
    std::vector<std::uint8_t> out;
    bool last = false;
    while (!last) {
        last = in.bits(1) != 0;
        switch (in.bits(2)) {
        case 0: inflateStored(in, out); break;
        case 1: inflateFixed(in, out); break;
        case 2: inflateDynamic(in, out); break;
        default: throw DiscError("deflate: invalid block type");
        }
    }
    consumed = in.position();
    return out;
}

std::uint32_t readLe32(std::span<const std::uint8_t> d, std::size_t pos)
{
    return static_cast<std::uint32_t>(d[pos]) | static_cast<std::uint32_t>(d[pos + 1]) << 8 |
           static_cast<std::uint32_t>(d[pos + 2]) << 16 | static_cast<std::uint32_t>(d[pos + 3]) << 24;
}

} // namespace

std::uint32_t crc32(std::span<const std::uint8_t> data)
{
    static const auto table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t n = 0; n < 256; ++n) {
            std::uint32_t c = n;
            for (int k = 0; k < 8; ++k)
                c = (c & 1u) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            t[n] = c;
        }
        return t;
    }();
    std::uint32_t crc = 0xFFFFFFFFu;
    for (const std::uint8_t b : data)
        crc = table[(crc ^ b) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

std::vector<std::uint8_t> inflate(std::span<const std::uint8_t> data)
{
    std::size_t consumed = 0;
    return inflateImpl(data, consumed);
}

std::vector<std::uint8_t> gunzip(std::span<const std::uint8_t> data)
{
    constexpr std::uint8_t kFhcrc = 0x02, kFextra = 0x04, kFname = 0x08, kFcomment = 0x10;
    if (data.size() < 18 || data[0] != 0x1F || data[1] != 0x8B)
        throw DiscError("gzip: bad magic");
    if (data[2] != 8)
        throw DiscError("gzip: unsupported compression method");
    const std::uint8_t flags = data[3];

    std::size_t pos = 10;
    auto need = [&](std::size_t n) {
        if (pos + n > data.size())
            throw DiscError("gzip: truncated header");
    };
    if (flags & kFextra) {
        need(2);
        const std::size_t xlen = static_cast<std::size_t>(data[pos]) | static_cast<std::size_t>(data[pos + 1]) << 8;
        pos += 2;
        need(xlen);
        pos += xlen;
    }
    for (const std::uint8_t flag : {kFname, kFcomment}) {
        if (!(flags & flag))
            continue;
        do
            need(1);
        while (data[pos++] != 0);
    }
    if (flags & kFhcrc) {
        need(2);
        pos += 2;
    }

    std::size_t consumed = 0;
    std::vector<std::uint8_t> out = inflateImpl(data.subspan(pos), consumed);
    pos += consumed;
    if (pos + 8 > data.size())
        throw DiscError("gzip: truncated trailer");
    if (readLe32(data, pos) != crc32(out))
        throw DiscError("gzip: CRC-32 mismatch");
    if (readLe32(data, pos + 4) != static_cast<std::uint32_t>(out.size()))
        throw DiscError("gzip: length mismatch");
    return out;
}

} // namespace gt2ap::discio
