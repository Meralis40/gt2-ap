// SPDX-License-Identifier: GPL-2.0-or-later
// Guest data slots that hold a function address: see port/tools/gen_fixups.py.
#pragma once

#include <cstdint>

namespace rt {

struct Fixup {
    std::uint32_t slot;  // guest address of the word
    void (*function)();  // null ends the table
};

void applyFixups(const Fixup *table);

}  // namespace rt
