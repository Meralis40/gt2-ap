# SPDX-License-Identifier: GPL-2.0-or-later
# Dumps every car with its manufacturer, drivetrain, engine type, stock power and maximum power
# (every tuning part installed) to JSON and Markdown.
#
# Power is not stored in the game data: the game computes it from the engine torque curve and the
# installed parts. Rather than reimplementing that, this script runs the game's own code in a
# MIPS emulator (needs `pip install unicorn`). Inputs, all from the Simulation disc (USA):
#   gt2-discinfo extract <disc.cue> /SCUS_944.88 <dir>/SCUS_944.88
#   gt2-discinfo overlays <disc.cue> <dir>                      (only ovl4.bin is used)
#   gt2-discinfo vol extract <disc.cue> <dir> /carparam
# then:
#   python scripts/data/dump_cars.py <dir> "docs/game ids/cars.json" "docs/game ids/cars.md"

import gzip
import json
import struct
import sys
from pathlib import Path

from unicorn import UC_ARCH_MIPS, UC_HOOK_CODE, UC_MODE_LITTLE_ENDIAN, UC_MODE_MIPS32, Uc
from unicorn.mips_const import (UC_MIPS_REG_A0, UC_MIPS_REG_A1, UC_MIPS_REG_A2, UC_MIPS_REG_A3,
                                UC_MIPS_REG_PC, UC_MIPS_REG_RA, UC_MIPS_REG_SP, UC_MIPS_REG_T1,
                                UC_MIPS_REG_V0)

from dump_events import MANUFACTURERS, decode_car, load_unistrdb
from dump_parts import CATEGORIES, ENTRY_SIZES

# Game functions (main executable, and OVL4 for the tuning shop ones)
GAMEDATA_RELOCATE = 0x80076ae8      # (data, entry sizes at 80092414)
PART_LOOKUP = 0x80076570            # Part_Lookup(partId, carCode, outPrice)
CAR_GET_DEFAULT_SETUP = 0x80076954  # Car_GetDefaultSetup(carCode, out)
CARWORK_SET_PART_SLOT = 0x8005eac0  # CarWork_SetPartSlot(work, slot, stage)
CARWORK_CLEAR = 0x80015404          # OVL4, memset(work, -1, 0x17e8)
CARWORK_LOAD_CAR = 0x80015428       # OVL4, per-slot part lists for a car
CARWORK_LOAD_SETUP = 0x80016c5c     # OVL4, copies a CarSetup into the work area
PART_GET_SLOT = 0x8001706c          # OVL4, Part_GetSlot(partId, out slot/stage)
CAR_STORE_SETUP = 0x80016f10        # OVL4, Car_StoreSetup(car, work): computes Car stats
G_CARWORK = 0x800b4490              # OVL4 bss

RETURN = 0x80001000
DATA = 0x80300000
SCRATCH = 0x80600000
CAR_TABLE, ENGINE_TABLE = 30, 6

# Car.stat94 bits 13-15 (Race_CheckEntry)
DRIVETRAINS = ["FR", "FF", "AWD", "MR", "RR"]
TURBO_KITS = [46, 47, 48, 49]
NA_TUNES = [29, 30, 31]
TIRES = range(39, 46)
RACING_MOD = 34
PART_SYMBOLS = {pid: symbol for pid, symbol in
                enumerate(s for _c, _t, _o, _s, symbols in CATEGORIES for s in symbols)}


class Game:
    """The game's code and GT mode data in a MIPS emulator. KSEG0 maps to physical memory."""

    def __init__(self, exe, ovl4, gtmode_data):
        mu = self.mu = Uc(UC_ARCH_MIPS, UC_MODE_MIPS32 + UC_MODE_LITTLE_ENDIAN)
        mu.mem_map(0, 0x800000)
        mu.mem_map(0x1f800000, 0x1000)  # scratchpad
        assert exe[:8] == b"PS-X EXE"
        self.write(struct.unpack_from("<I", exe, 0x18)[0], exe[0x800:])
        self.write(0x80010000, ovl4)
        self.write(DATA, gtmode_data)
        mu.hook_add(UC_HOOK_CODE, self._bios, begin=0xa0, end=0xa0)
        self.call(GAMEDATA_RELOCATE, DATA, 0x80092414)
        self.write32(0x8009286c, DATA)  # g_GtModeData
        self.write32(0x8009287c, DATA)  # g_GameDataBase
        self.write32(0x80092878, 2)     # GT mode data loaded
        self.write32(0x801c94e4, struct.unpack_from("<H", gtmode_data, 4)[0])
        out = SCRATCH
        self.slots = {}
        for pid in PART_SYMBOLS:
            if pid not in TIRES:
                self.call(PART_GET_SLOT, pid, out)
                self.slots[pid] = struct.unpack("<HH", self.read(out, 4))

    def _bios(self, mu, _addr, _size, _user):
        """BIOS A0 table, only the calls made on this code path."""
        fn = mu.reg_read(UC_MIPS_REG_T1)
        a0, a1, a2 = (mu.reg_read(r) for r in (UC_MIPS_REG_A0, UC_MIPS_REG_A1, UC_MIPS_REG_A2))
        if fn == 0x27:  # bcopy(src, dst, n)
            self.write(a1, self.read(a0, a2))
        else:
            raise RuntimeError("unhandled BIOS call A0:%02x" % fn)
        mu.reg_write(UC_MIPS_REG_PC, mu.reg_read(UC_MIPS_REG_RA))

    def write(self, addr, data):
        self.mu.mem_write(addr & 0x1fffffff, bytes(data))

    def read(self, addr, size):
        return bytes(self.mu.mem_read(addr & 0x1fffffff, size))

    def write32(self, addr, value):
        self.write(addr, struct.pack("<I", value))

    def call(self, fn, *args):
        assert len(args) <= 4
        for reg, value in zip((UC_MIPS_REG_A0, UC_MIPS_REG_A1, UC_MIPS_REG_A2, UC_MIPS_REG_A3), args):
            self.mu.reg_write(reg, value & 0xffffffff)
        self.mu.reg_write(UC_MIPS_REG_SP, 0x801ff000)
        self.mu.reg_write(UC_MIPS_REG_RA, RETURN)
        self.mu.emu_start(fn, RETURN, count=10_000_000)
        return self.mu.reg_read(UC_MIPS_REG_V0)

    def has_part(self, car, pid):
        return self.call(PART_LOOKUP, pid, car, 0) != 0xffffffff

    def stats(self, car, parts):
        """Installs parts in order like Tune_InstallPart, then returns (power, drivetrain)."""
        setup, record = SCRATCH, SCRATCH + 0x1000
        self.write(setup, bytes(0x84))
        self.write(record, bytes(0xa4))
        self.call(CAR_GET_DEFAULT_SETUP, car, setup)
        self.call(CARWORK_CLEAR, G_CARWORK)
        self.call(CARWORK_LOAD_CAR, G_CARWORK, car)
        self.call(CARWORK_LOAD_SETUP, G_CARWORK, setup)
        for pid in parts:
            self.call(CARWORK_SET_PART_SLOT, G_CARWORK, *self.slots[pid])
        self.call(CAR_STORE_SETUP, record, G_CARWORK)
        stat94, _stat96, flags98 = struct.unpack("<HHH", self.read(record + 0x94, 6))
        return flags98 & 0x3fff, DRIVETRAINS[stat94 >> 13]


def main():
    vol, json_out, md_out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    data = gzip.decompress((vol / "carparam/usa_gtmode_data.dat.gz").read_bytes())
    names = load_unistrdb(gzip.decompress((vol / "carparam/usa_unistrdb.dat.gz").read_bytes()))
    game = Game((vol / "SCUS_944.88").read_bytes(), (vol / "ovl4.bin").read_bytes(), data)

    tables = []
    for i, sz in enumerate(ENTRY_SIZES):
        off, size = struct.unpack_from("<II", data, 8 + 8 * i)
        tables.append([data[off + k * sz:off + (k + 1) * sz] for k in range(size // sz)])
    engines = {struct.unpack_from("<I", e)[0]: e for e in tables[ENGINE_TABLE]}

    # One part per category, highest stage first; engine kits are handled separately
    categories = [symbols for _c, _t, _o, _s, symbols in CATEGORIES]
    kits = set(TURBO_KITS + NA_TUNES + [RACING_MOD]) | set(TIRES)

    out = []
    for entry in tables[CAR_TABLE]:
        code, = struct.unpack_from("<I", entry)
        n1, n2 = struct.unpack_from("<HH", entry, 0x3c)
        # Engine table: strings for layout (I4, V6, Rotary...), valvetrain or rotor count, and
        # aspiration (NA, TURBO, MA = supercharger)
        engine = [names[struct.unpack_from("<H", engines[code], o)[0]].strip() for o in (4, 6, 8)]

        available = {pid for pid in PART_SYMBOLS if pid not in TIRES and game.has_part(code, pid)}
        base = []
        for symbols in categories:
            ids = [pid for pid in PART_SYMBOLS if PART_SYMBOLS[pid] in symbols
                   and pid in available and pid not in kits]
            if ids:
                base.append(max(ids))
        # Racing modification needs weight reduction 3 (Tune_CanBuyPart), which base installs
        if RACING_MOD in available:
            base.append(RACING_MOD)

        stock_power, drivetrain = game.stats(code, [])
        # A turbo kit resets the NA tune-up and the reverse (CarWork_SetPartSlot), so try each
        # kit on its own, installed first, then every other part at its highest stage
        best_power, best_kit = game.stats(code, base)[0], None
        for kit in NA_TUNES + TURBO_KITS:
            if kit in available:
                power = game.stats(code, [kit] + base)[0]
                if power > best_power:
                    best_power, best_kit = power, kit
        out.append({
            "code": decode_car(code),
            "carId": code,
            "manufacturer": MANUFACTURERS.get(entry[0x3a], "#%d" % entry[0x3a]),
            "manufacturerId": entry[0x3a],
            "name": (names[n1] + " " + names[n2]).strip(),
            "drivetrain": drivetrain,
            "engine": " ".join(s for s in engine if s not in ("", "-")),
            "stockPower": stock_power,
            "maxPower": best_power,
            "maxPowerKit": PART_SYMBOLS[best_kit] if best_kit is not None else None,
        })

    json_out.write_text(json.dumps({"cars": out}, indent=1, ensure_ascii=False) + "\n",
                        encoding="utf-8")
    lines = [
        "# Cars",
        "",
        "Generated by `scripts/data/dump_cars.py` from the Simulation disc (USA). "
        "`cars.json` holds the same data.",
        "",
        "- **Code** is the 5-character car code (`CarCode_Encode`); the list is the game-data car "
        "table (table 30), which also holds race-only and opponent cars.",
        "- **Drivetrain** comes from `Car.stat94` bits 13-15, as computed when a car is bought.",
        "- **Engine** is the three engine strings of the engine table (table 6): layout, "
        "valvetrain, and aspiration (NA, TURBO, MA = supercharger). Blank (`-`) strings are "
        "left out. These are the game's data, so some are wrong (the Vectra GSi 2.5 V6 is listed "
        "as I4).",
        "- Manufacturer `#35` is never shown as a dealership; its cars are Vauxhall models (see "
        "`manufacturers.md`).",
        "- **Power** is the value in `Car.flags98` bits 0-13, the one `Race_CheckEntry` compares "
        "with an event's power limit. The game computes it from the torque curve and the "
        "installed parts; the script runs that code (`Car_StoreSetup`) in an emulator.",
        "- **Max power** has every available part at its highest stage, racing modification "
        "included, plus the best NA tune-up or turbo kit (**Kit**). A turbo kit resets the NA "
        "tune-up and the reverse, so only one of them counts.",
        "",
        "| Code | Manufacturer | Name | Drivetrain | Engine | Power | Max power | Kit |",
        "|---|---|---|---|---|---|---|---|",
    ]
    for c in out:
        kit = c["maxPowerKit"]
        lines.append("| %s | %s | %s | %s | %s | %d | %d | %s |" % (
            c["code"], c["manufacturer"], c["name"], c["drivetrain"], c["engine"],
            c["stockPower"], c["maxPower"], kit.removeprefix("PART_") if kit else ""))
    md_out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("cars:", len(out))


if __name__ == "__main__":
    main()
