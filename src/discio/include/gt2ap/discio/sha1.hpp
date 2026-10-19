// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>

namespace gt2ap::discio {

// SHA-1, used to match dumps against redump.org hashes (not for security).
class Sha1 {
public:
    using Digest = std::array<std::uint8_t, 20>;

    Sha1();
    void update(std::span<const std::uint8_t> data);
    Digest finish();

    static Digest of(std::span<const std::uint8_t> data);
    static Digest ofFile(const std::filesystem::path& path);
    static std::string toHex(const Digest& digest);

private:
    void processBlock(const std::uint8_t* block);

    std::array<std::uint32_t, 5> m_state;
    std::array<std::uint8_t, 64> m_buffer{};
    std::size_t m_bufferLength = 0;
    std::uint64_t m_totalLength = 0;
};

} // namespace gt2ap::discio
