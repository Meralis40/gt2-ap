// SPDX-License-Identifier: GPL-2.0-or-later
// The game's data names. Game code declares each as a reference, extern T (&name), and
// gen/guest_data.cpp defines the slot behind it; bindGuestData points every slot at
// base + its PlayStation address once the window is mapped (docs/port.md).
#pragma once

#include <cstdint>

namespace rt {

struct GuestData {
    std::uintptr_t *slot;  // null ends the table
    std::uint32_t address;
};

extern const GuestData kGuestData[];

void bindGuestData();

}  // namespace rt
