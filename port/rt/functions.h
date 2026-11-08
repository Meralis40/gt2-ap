// SPDX-License-Identifier: GPL-2.0-or-later
// The PlayStation addresses of the port's functions, per unit: see port/tools/gen_functions.py.
#pragma once

#include <cstdint>

namespace rt {

struct FunctionEntry {
    std::uint32_t address;  // ascending; 0 ends the table
    void (*function)();
};

// The overlay whose functions answer for 0x80010000 onwards; -1 for none.
void setOverlayFunctions(int index);

}  // namespace rt
