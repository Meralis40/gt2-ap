// SPDX-License-Identifier: GPL-2.0-or-later
// The vtable word of the game's classes (the screen loops, menus and race loops).
//
// A C++ vtable pointer is 8 bytes on x86-64 and moves the fields after it, which decompiled
// code reaches by the console's offsets. So these classes have no C++ virtual functions:
// they keep a 4-byte word where the console has its vtable pointer, holding the address of
// the class's vtable on the console, and v_F() calls F through the table the port builds
// for that address (gen/vtables.cpp, port/tools/devirtualize.py). A constructor sets the
// word after its base's did, as the console's do.
#pragma once

#include "gptr.h"  // rt::u32; game code has no C++ library headers

namespace rt {
// The port's table for the vtable at address; each slot calls the class's function.
void *const *vtable(u32 address);
}  // namespace rt

struct game_vptr {
    rt::u32 address;

    template <class F>
    F *slot(int index) const
    {
        return reinterpret_cast<F *>(rt::vtable(address)[index]);
    }
};

static_assert(sizeof(game_vptr) == 4);
