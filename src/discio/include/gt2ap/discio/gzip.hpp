// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace gt2ap::discio {

// Raw DEFLATE (RFC 1951) decoder. Throws DiscError on malformed input.
std::vector<std::uint8_t> inflate(std::span<const std::uint8_t> data);

// Single-member gzip (RFC 1952) decoder; verifies CRC-32 and length.
std::vector<std::uint8_t> gunzip(std::span<const std::uint8_t> data);

std::uint32_t crc32(std::span<const std::uint8_t> data);

} // namespace gt2ap::discio
