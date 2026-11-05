// SPDX-License-Identifier: GPL-2.0-or-later
// Guest memory, at its real addresses.
#pragma once

#include <cstddef>
#include <cstdint>

namespace rt {

inline constexpr std::uint32_t kGuestBase = 0x80000000;
inline constexpr std::uint32_t kOverlayBase = 0x80010000;
inline constexpr std::size_t kGuestRam = 2u << 20;
inline constexpr std::size_t kGuestReserve = 16u << 20;  // room for added data, in reach of 24-bit links
inline constexpr std::uint32_t kScratchBase = 0x1f800000;
inline constexpr std::size_t kScratchSize = 0x1000;

inline void *guest(std::uint32_t address)
{
    return reinterpret_cast<void *>(static_cast<std::uintptr_t>(address));
}

void mapGuestMemory();

// Reports a fault with the faulting and accessed addresses. A missed code pointer shows
// as a fault at a guest address; a missed hardware register as an access at 0x1f801xxx.
void installFaultHandler();

}  // namespace rt
