// SPDX-License-Identifier: GPL-2.0-or-later
// Guest memory: the window at rt::base (gptr.h) and what is mapped in it.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gptr.h"  // rt::guest, rt::to_guest

namespace rt {

inline constexpr std::uint32_t kGuestBase = 0x80000000;
inline constexpr std::uint32_t kOverlayBase = 0x80010000;
inline constexpr std::size_t kGuestRam = 2u << 20;
inline constexpr std::size_t kGuestReserve = 16u << 20;  // room for added data, in reach of 24-bit links
inline constexpr std::uint32_t kScratchBase = 0x1f800000;
inline constexpr std::size_t kScratchSize = 0x1000;
// x86-64: the game runs on a stack inside the window, so the addresses of its locals are
// guest addresses too. The top 4 MB of the reserve, which the game never reaches.
inline constexpr std::uint32_t kGameStackBase = 0x80c00000;
inline constexpr std::size_t kGameStackSize = 4u << 20;
// Data the port adds to the game's (rt::guestAlloc), past RAM's end.
inline constexpr std::uint32_t kPortDataBase = 0x80200000;
inline constexpr std::size_t kPortDataSize = 4u << 20;

void mapGuestMemory();

// Runs fn on the game's stack: the host's on i686, kGameStackBase's on x86-64.
void runOnGameStack(void (*fn)());

// Reports a fault with the faulting and accessed addresses. A missed code pointer shows
// as a fault at a guest address; a missed hardware register as an access at 0x1f801xxx.
void installFaultHandler();

}  // namespace rt
