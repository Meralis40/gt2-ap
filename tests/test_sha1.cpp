// SPDX-License-Identifier: GPL-2.0-or-later
#include "test.hpp"

#include "gt2ap/discio/sha1.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

using gt2ap::discio::Sha1;

namespace {
std::string hexOf(std::string_view s)
{
    return Sha1::toHex(Sha1::of({reinterpret_cast<const std::uint8_t*>(s.data()), s.size()}));
}
} // namespace

TEST_CASE("sha1: FIPS 180 test vectors")
{
    CHECK_EQ(hexOf(""), std::string("da39a3ee5e6b4b0d3255bfef95601890afd80709"));
    CHECK_EQ(hexOf("abc"), std::string("a9993e364706816aba3e25717850c26c9cd0d89d"));
    CHECK_EQ(hexOf("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
             std::string("84983e441c3bd26ebaae4aa1f95129e5e54670f1"));
}

TEST_CASE("sha1: one million 'a' fed in uneven chunks")
{
    const std::vector<std::uint8_t> data(1'000'000, 'a');
    Sha1 sha;
    std::size_t pos = 0, chunk = 1;
    while (pos < data.size()) {
        const std::size_t n = std::min(chunk, data.size() - pos);
        sha.update({data.data() + pos, n});
        pos += n;
        chunk = chunk * 3 + 1;
    }
    CHECK_EQ(Sha1::toHex(sha.finish()), std::string("34aa973cd4c4daa4f61eeb2bdbad27316534016f"));
}
