// SPDX-License-Identifier: GPL-2.0-or-later
// Calls through game_function words: from a function's PlayStation address to the host
// function, and back (rt/gptr.h). Functions the console didn't have, the port's own,
// get an address from kAddedBase on when first stored, which no guest code can reach.
#include "functions.h"
#include "gptr.h"
#include "rt.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

extern const rt::FunctionEntry g_Functions_exe[];
extern const rt::FunctionEntry g_Functions_ovl0[];
extern const rt::FunctionEntry g_Functions_ovl1[];
extern const rt::FunctionEntry g_Functions_ovl2[];
extern const rt::FunctionEntry g_Functions_ovl3[];
extern const rt::FunctionEntry g_Functions_ovl4[];
extern const rt::FunctionEntry g_Functions_ovl5[];

namespace rt {

namespace {

constexpr std::uint32_t kAddedBase = 0xc0000000;  // kseg2: nothing of the game lives there

struct Table {
    const FunctionEntry *begin;
    const FunctionEntry *end;
    std::unordered_map<void *, std::uint32_t> addresses;
};

Table makeTable(const FunctionEntry *entries)
{
    Table t{entries, entries, {}};
    while (t.end->function) {
        t.addresses.emplace(reinterpret_cast<void *>(t.end->function), t.end->address);
        t.end++;
    }
    return t;
}

struct Tables {
    Table exe = makeTable(g_Functions_exe);
    Table overlays[6] = {makeTable(g_Functions_ovl0), makeTable(g_Functions_ovl1), makeTable(g_Functions_ovl2),
                         makeTable(g_Functions_ovl3), makeTable(g_Functions_ovl4), makeTable(g_Functions_ovl5)};
    std::vector<void *> added;
    std::unordered_map<void *, std::uint32_t> addedAddresses;
};

Tables &tables()
{
    static Tables t;
    return t;
}

int g_Overlay = -1;

void *find(const Table &t, std::uint32_t address)
{
    const FunctionEntry *e = std::lower_bound(
        t.begin, t.end, address, [](const FunctionEntry &e, std::uint32_t a) { return e.address < a; });
    return e != t.end && e->address == address ? reinterpret_cast<void *>(e->function) : nullptr;
}

}  // namespace

void setOverlayFunctions(int index)
{
    g_Overlay = index;
}

void *callTarget(std::uint32_t address)
{
    Tables &t = tables();
    void *f = nullptr;

    if (address >= kAddedBase) {
        std::uint32_t i = (address - kAddedBase) / 4;
        if (i < t.added.size()) {
            return t.added[i];
        }
    } else {
        if (g_Overlay >= 0) {
            f = find(t.overlays[g_Overlay], address);
        }
        if (!f) {
            f = find(t.exe, address);
        }
    }
    if (!f) {
        Rt_Fatal("no function at guest address %08x (overlay %d)", address, g_Overlay);
    }
    return f;
}

std::uint32_t functionAddress(void *function)
{
    Tables &t = tables();
    const Table *search[] = {g_Overlay >= 0 ? &t.overlays[g_Overlay] : nullptr, &t.exe};

    for (const Table *table : search) {
        if (table) {
            auto it = table->addresses.find(function);
            if (it != table->addresses.end()) {
                return it->second;
            }
        }
    }
    // An overlay's function stored while another is loaded: only for later comparison.
    for (const Table &table : t.overlays) {
        auto it = table.addresses.find(function);
        if (it != table.addresses.end()) {
            return it->second;
        }
    }
    auto it = t.addedAddresses.find(function);
    if (it != t.addedAddresses.end()) {
        return it->second;
    }
    std::uint32_t address = kAddedBase + static_cast<std::uint32_t>(t.added.size()) * 4;
    t.added.push_back(function);
    t.addedAddresses.emplace(function, address);
    return address;
}

}  // namespace rt
