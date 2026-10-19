// SPDX-License-Identifier: GPL-2.0-or-later
#include "gt2ap/discio/sha1.hpp"

#include "gt2ap/discio/error.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <vector>

namespace gt2ap::discio {

Sha1::Sha1() : m_state{0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u} {}

void Sha1::processBlock(const std::uint8_t* block)
{
    std::array<std::uint32_t, 80> w{};
    for (std::size_t i = 0; i < 16; ++i)
        w[i] = (std::uint32_t{block[i * 4]} << 24) | (std::uint32_t{block[i * 4 + 1]} << 16) |
               (std::uint32_t{block[i * 4 + 2]} << 8) | std::uint32_t{block[i * 4 + 3]};
    for (std::size_t i = 16; i < 80; ++i)
        w[i] = std::rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    std::uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], e = m_state[4];
    for (std::size_t i = 0; i < 80; ++i) {
        std::uint32_t f, k;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5A827999u;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }
        const std::uint32_t temp = std::rotl(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = std::rotl(b, 30);
        b = a;
        a = temp;
    }
    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;
    m_state[4] += e;
}

void Sha1::update(std::span<const std::uint8_t> data)
{
    m_totalLength += data.size();
    std::size_t i = 0;
    if (m_bufferLength > 0) {
        const std::size_t take = std::min(data.size(), m_buffer.size() - m_bufferLength);
        std::memcpy(m_buffer.data() + m_bufferLength, data.data(), take);
        m_bufferLength += take;
        i = take;
        if (m_bufferLength < m_buffer.size())
            return;
        processBlock(m_buffer.data());
        m_bufferLength = 0;
    }
    for (; i + 64 <= data.size(); i += 64)
        processBlock(data.data() + i);
    m_bufferLength = data.size() - i;
    std::memcpy(m_buffer.data(), data.data() + i, m_bufferLength);
}

Sha1::Digest Sha1::finish()
{
    const std::uint64_t bitLength = m_totalLength * 8;
    const std::uint8_t pad = 0x80;
    update({&pad, 1});
    const std::uint8_t zero = 0;
    while (m_bufferLength != 56)
        update({&zero, 1});
    std::array<std::uint8_t, 8> length{};
    for (std::size_t i = 0; i < 8; ++i)
        length[i] = static_cast<std::uint8_t>(bitLength >> (56 - 8 * i));
    update(length);

    Digest digest{};
    for (std::size_t i = 0; i < 5; ++i)
        for (std::size_t j = 0; j < 4; ++j)
            digest[i * 4 + j] = static_cast<std::uint8_t>(m_state[i] >> (24 - 8 * j));
    return digest;
}

Sha1::Digest Sha1::of(std::span<const std::uint8_t> data)
{
    Sha1 sha;
    sha.update(data);
    return sha.finish();
}

Sha1::Digest Sha1::ofFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw DiscError("cannot open '" + path.string() + "'");
    Sha1 sha;
    std::vector<std::uint8_t> buffer(1 << 20);
    while (in) {
        in.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        sha.update({buffer.data(), static_cast<std::size_t>(in.gcount())});
    }
    return sha.finish();
}

std::string Sha1::toHex(const Digest& digest)
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(40);
    for (const std::uint8_t byte : digest) {
        out.push_back(kHex[byte >> 4]);
        out.push_back(kHex[byte & 0x0F]);
    }
    return out;
}

} // namespace gt2ap::discio
