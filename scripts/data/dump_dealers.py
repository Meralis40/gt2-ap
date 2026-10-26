# SPDX-License-Identifier: GPL-2.0-or-later
# Dumps every car sold in GT mode (new car dealerships and the used car lots) with its prices,
# and the power each tuning part adds on its own, to JSON and Markdown. The AP world logic uses it.
#
# Inputs, all from the Simulation disc (USA):
#   gt2-discinfo extract <disc.cue> /SCUS_944.88 <dir>/SCUS_944.88
#   gt2-discinfo overlays <disc.cue> <dir>                      (only ovl4.bin is used)
#   gt2-discinfo vol extract <disc.cue> <dir> /carparam
#   gt2-discinfo vol extract <disc.cue> <dir> /gtmenu/usa
#   gt2-discinfo vol extract <disc.cue> <dir> /.usedcar_usa
# then (needs `pip install unicorn`):
#   python scripts/data/dump_dealers.py <dir> "docs/game ids/dealers.json" "docs/game ids/dealers.md"
#
# Sources:
#   gtmenu/usa/gtmenudat.dat         dealer pages: kind 6 elements are the car tiles, car code at
#                                    +0x10 (SimMap_Update -> Txn_SetBuyNewCar); the page footer
#                                    byte 0 is the dealership's manufacturer
#   carparam/usa_gtmode_data.dat.gz  new price at +0x44 of the car list entry (Car_GetNewPrice)
#   .usedcar_usa                     60 used car periods of 10 days (UsedCar_LoadPeriod)

import gzip
import json
import struct
import sys
from pathlib import Path

from dump_cars import CAR_TABLE, NA_TUNES, PART_LOOKUP, RACING_MOD, SCRATCH, TURBO_KITS, Game
from dump_events import MANUFACTURERS, decode_car, load_unistrdb, menu_pages
from dump_parts import ENTRY_SIZES

# Parts that change the power Race_CheckEntry compares with the event limit. Weight reduction,
# chassis parts and tyres don't.
POWER_PARTS = [6, 7, 9, 16, 17, 26, 27, 28, 32, 34] + NA_TUNES + TURBO_KITS
DIRT_TIRES = 45
WEIGHT_REDUCTION_3 = 20
DEALER_TILE = 6
USED_PERIODS = 60


def dealer_cars(vol):
    """{car code: sorted dealership manufacturer IDs} from the dealer car tiles."""
    found = {}
    for _index, page in menu_pages(vol):
        elements, p = [], 8
        for _ in range(struct.unpack_from("<I", page, 4)[0]):
            a, b = struct.unpack_from("<HH", page, p)
            p += 4 + a * 12
            elements += [page[p + i * 0x4c:p + (i + 1) * 0x4c] for i in range(b)]
            p += b * 0x4c
        n, = struct.unpack_from("<I", page, p)
        elements += [page[p + 4 + i * 0x4c:p + 4 + (i + 1) * 0x4c] for i in range(n)]
        maker = page[p + 4 + n * 0x4c]
        for e in elements:
            flags, = struct.unpack_from("<I", e, 8)
            if flags & 0xffff == DEALER_TILE and flags >> 24 & 1:
                found.setdefault(struct.unpack_from("<I", e, 0x10)[0], set()).add(maker)
    return {code: sorted(makers) for code, makers in found.items()}


def used_cars(vol):
    """{car code: [(period, price)]} from every period of the used car file."""
    data = gzip.decompress((vol / ".usedcar_usa").read_bytes())
    assert data[:4] == b"UCAR"
    offsets = struct.unpack_from("<%dI" % (USED_PERIODS + 1), data, 8)
    found = {}
    for period in range(USED_PERIODS):
        block = data[offsets[period]:offsets[period + 1]]
        for maker in range(39):
            # Slot offsets count from the start of the period
            offset, count = struct.unpack_from("<HH", block, maker * 4)
            for i in range(count):
                code, price = struct.unpack_from("<II", block, offset + i * 8)
                found.setdefault(code, []).append((period, price & 0xffffff))
    return found


def part_price(game, code, pid):
    """Part_Lookup: the part's price for this car, None if the car doesn't offer it."""
    out = SCRATCH + 0x3000
    game.write32(out, 0)
    if game.call(PART_LOOKUP, pid, code, out) == 0xffffffff:
        return None
    return struct.unpack("<I", game.read(out, 4))[0]


def main():
    vol, json_out, md_out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    data = gzip.decompress((vol / "carparam/usa_gtmode_data.dat.gz").read_bytes())
    names = load_unistrdb(gzip.decompress((vol / "carparam/usa_unistrdb.dat.gz").read_bytes()))
    game = Game((vol / "SCUS_944.88").read_bytes(), (vol / "ovl4.bin").read_bytes(), data)
    off, size = struct.unpack_from("<II", data, 8 + 8 * CAR_TABLE)
    sz = ENTRY_SIZES[CAR_TABLE]
    entries = {struct.unpack_from("<I", data, off + k * sz)[0]: data[off + k * sz:off + (k + 1) * sz]
               for k in range(size // sz)}

    new, used = dealer_cars(vol), used_cars(vol)
    out = []
    for code in sorted(set(new) | set(used), key=decode_car):
        entry = entries.get(code)
        if entry is None:
            # nep2n: a dealer tile whose car isn't in the car list, so it can't be bought
            print("skipping %s: not in the car list" % decode_car(code))
            continue
        n1, n2 = struct.unpack_from("<HH", entry, 0x3c)
        stock, drivetrain = game.stats(code, [])
        parts, gains = {}, {}
        for pid in POWER_PARTS + [DIRT_TIRES]:
            price = part_price(game, code, pid)
            if price is None:
                continue
            parts[pid] = price
            if pid in POWER_PARTS:
                # Racing modification needs weight reduction 3 (Tune_CanBuyPart)
                extra = [WEIGHT_REDUCTION_3] if pid == RACING_MOD else []
                gains[pid] = game.stats(code, extra + [pid])[0] - stock
        periods = used.get(code, [])
        out.append({
            "code": decode_car(code),
            "carId": code,
            "manufacturer": MANUFACTURERS.get(entry[0x3a], "#%d" % entry[0x3a]),
            "manufacturerId": entry[0x3a],
            "name": (names[n1] + " " + names[n2]).strip(),
            "dealers": new.get(code, []),
            "newPrice": struct.unpack_from("<I", entry, 0x44)[0] if code in new else None,
            "usedPeriods": sorted({p for p, _ in periods}),
            "usedMinPrice": min((price for _, price in periods), default=None),
            "drivetrain": drivetrain,
            "stockPower": stock,
            "partPrices": {str(pid): price for pid, price in parts.items()},
            "powerGains": {str(pid): gain for pid, gain in gains.items()},
        })

    json_out.write_text(json.dumps({"cars": out}, indent=1, ensure_ascii=False) + "\n",
                        encoding="utf-8")
    lines = [
        "# Dealerships and used cars",
        "",
        "Generated by `scripts/data/dump_dealers.py` from the Simulation disc (USA). "
        "`dealers.json` holds the same data, with the price and power gain of each power part.",
        "",
        "- **Dealers** are the manufacturer IDs of the dealerships whose pages have a tile for the "
        "car (kind 6 elements, see `SimMap_Update`). **New** is its price there "
        "(`Car_GetNewPrice`, car list `+0x44`).",
        "- **Used** is the lowest price over the 60 periods of `.usedcar_usa` and **Periods** how "
        "many periods list the car. The lot changes every 10 days (`UsedCar_LoadPeriod`). Only "
        "the Japanese manufacturers have a used car lot.",
        "- **Power** is the stock power (see `cars.md`). In the JSON, `powerGains` is the power "
        "each part adds when it is the only part installed (the racing modification with weight "
        "reduction 3, which it needs). Gains don't add up exactly.",
        "",
        "| Code | Manufacturer | Name | Dealers | New | Used | Periods | Drive | Power |",
        "|---|---|---|---|---|---|---|---|---|",
    ]
    for c in out:
        lines.append("| %s | %s | %s | %s | %s | %s | %s | %s | %d |" % (
            c["code"], c["manufacturer"], c["name"], ", ".join(map(str, c["dealers"])),
            c["newPrice"] or "", c["usedMinPrice"] or "", len(c["usedPeriods"]) or "",
            c["drivetrain"], c["stockPower"]))
    md_out.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
