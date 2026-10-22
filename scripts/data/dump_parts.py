# SPDX-License-Identifier: GPL-2.0-or-later
# Dumps every tuning part (PartId) with its name, category and stage to JSON and Markdown.
#
# Input is a GT2.VOL extraction made with gt2-discinfo, for example:
#   gt2-discinfo vol extract <disc.cue> <dir> /carparam
# then:
#   python scripts/data/dump_parts.py <dir> "docs/game ids/parts.json" "docs/game ids/parts.md"
#
# Sources (Simulation disc, USA):
#   carparam/usa_unistrdb.dat.gz     part names (strings 0-49, indexed by PartId)
#   carparam/usa_gtmode_data.dat.gz  per-car part tables, to count the cars offering each part

import gzip
import json
import struct
import sys
from pathlib import Path

from dump_events import decode_car, load_unistrdb

# gtmode_data entry sizes, from 80092414
ENTRY_SIZES = [0xc, 0x10, 0x18, 0x14, 0xc, 0x1c, 0x4c, 0xc, 0xc, 0xc, 0xc, 0xc, 0x14, 0x10, 0xc,
               0x10, 0xc, 0x24, 0x4c, 0xc, 0xc, 0x20, 0x10, 0xc, 0x4, 0x40, 0x8, 0x10, 0x10, 0x8,
               0x48]
CAR_TABLE = 30

# Category: (game-data table, stage byte offset, Part_GetSlot slot, first PartId, PartId symbols).
# The stage of a part is PartId - first PartId + 1 (Part_Lookup, 80076570); stage 0 entries are
# the stock parts. Part_GetSlot (OVL4 8001706c) gives the g_CarWork_PartSlots index; tyres have
# no slot there.
CATEGORIES = [
    ("asc", 0x1b, 8, 0x14, ["PART_ASC"]),
    ("brakes", 0, 8, 1, ["PART_SPORTS_BRAKES"]),
    ("brake_balance", 1, 8, 2, ["PART_BRAKE_BALANCE"]),
    ("clutch", 0xf, 8, 7, ["PART_CLUTCH_SINGLE", "PART_CLUTCH_TWIN", "PART_CLUTCH_TRIPLE"]),
    ("computer", 0xa, 8, 10, ["PART_PERF_CHIP"]),
    ("displacement", 9, 8, 0xe, ["PART_DISPLACEMENT", "PART_DISPLACEMENT_2"]),
    ("engine_balancing", 8, 8, 0xc, ["PART_ENGINE_BALANCING"]),
    ("flywheel", 0xe, 8, 8, ["PART_FLYWHEEL_SPORTS", "PART_FLYWHEEL_SEMI", "PART_FLYWHEEL_RACING"]),
    ("transmission", 0x11, 8, 6, ["PART_TRANS_SPORTS", "PART_TRANS_SEMI", "PART_TRANS_FULL"]),
    ("intercooler", 0x13, 8, 0x10, ["PART_INTERCOOLER_SPORTS", "PART_INTERCOOLER_RACING"]),
    ("weight_reduction", 4, 0xb, 0x12, ["PART_WEIGHT_RED_1", "PART_WEIGHT_RED_2", "PART_WEIGHT_RED_3"]),
    ("lsd", 0x15, 8, 0x16, ["PART_LSD_1WAY", "PART_LSD_2WAY", "PART_LSD_15WAY", "PART_LSD_FULL",
                            "PART_YAW_CONTROL"]),
    ("muffler", 0x14, 8, 0x11, ["PART_MUFFLER_SPORTS", "PART_MUFFLER_SEMI", "PART_MUFFLER_RACING"]),
    ("na_tune", 0xb, 8, 0xd, ["PART_NA_TUNE_1", "PART_NA_TUNE_2", "PART_NA_TUNE_3"]),
    ("port_polish", 7, 8, 0xb, ["PART_PORT_POLISH"]),
    ("driveshaft", 0x10, 8, 9, ["PART_CARBON_DRIVESHAFT"]),
    ("racing_mod", 5, 0xe, 0x13, ["PART_RACING_MOD"]),
    ("suspension", 0x12, 8, 0, ["PART_SUSP_SPORTS", "PART_SUSP_SEMI", "PART_SUSP_FULL"]),
    ("tcs", 0x1c, 8, 0x15, ["PART_TCS"]),
    ("tires", 0x16, 8, None, ["PART_TIRES_SPORTS", "PART_TIRES_HARD", "PART_TIRES_MEDIUM",
                              "PART_TIRES_SOFT", "PART_TIRES_SUPER_SOFT", "PART_TIRES_SIMULATION",
                              "PART_TIRES_DIRT"]),
    ("turbo", 0xc, 8, 0xf, ["PART_TURBO_1", "PART_TURBO_2", "PART_TURBO_3", "PART_TURBO_4"]),
]


def find_part(entries, stage_offset, stage, car):
    """GameData_FindPart: price of the car's entry with that stage, None if not offered."""
    for e in entries.get(car, ()):
        if e[stage_offset] == stage and e[4] != 0:
            return struct.unpack_from("<i", e, 4)[0]
    return None


def main():
    vol, json_out, md_out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    data = gzip.decompress((vol / "carparam/usa_gtmode_data.dat.gz").read_bytes())
    names = load_unistrdb(gzip.decompress((vol / "carparam/usa_unistrdb.dat.gz").read_bytes()))
    # GTDT header like gtmode_race, but with no string table
    assert data[:4] == b"GTDT" and struct.unpack_from("<H", data, 6)[0] // 2 == len(ENTRY_SIZES)
    tables = []
    for i, sz in enumerate(ENTRY_SIZES):
        off, size = struct.unpack_from("<II", data, 8 + 8 * i)
        tables.append([data[off + k * sz:off + (k + 1) * sz] for k in range(size // sz)])
    cars = [struct.unpack_from("<I", e)[0] for e in tables[CAR_TABLE]]

    parts = []
    for category, table, stage_offset, slot, symbols in CATEGORIES:
        by_car = {}
        for e in tables[table]:
            by_car.setdefault(struct.unpack_from("<I", e)[0], []).append(e)
        # Generic "00000" entry, used when the car has none (only ASC and TCS have one)
        generic_code = next((c for c in by_car if decode_car(c) == "00000"), None)
        for k, symbol in enumerate(symbols):
            part_id = len(parts)
            stage = k + 1
            prices = []
            for car in cars:
                price = find_part(by_car, stage_offset, stage, car)
                if price is None and generic_code is not None:
                    price = find_part(by_car, stage_offset, stage, generic_code)
                if price is not None:
                    prices.append(price)
            parts.append({
                "id": part_id,
                "symbol": symbol,
                "name": names[part_id] if names[part_id] != "-" else None,
                "category": category,
                "stage": stage,
                "stages": len(symbols),
                "slot": slot,
                "carCount": len(prices),
                "minPrice": min(prices) if prices else None,
                "maxPrice": max(prices) if prices else None,
            })
    assert len(parts) == 50

    json_out.write_text(json.dumps({"parts": parts, "carTotal": len(cars)}, indent=1) + "\n",
                        encoding="utf-8")

    def opt(v):
        return "" if v is None else str(v)

    lines = [
        "# Tuning parts",
        "",
        "Generated by `scripts/data/dump_parts.py` from the Simulation disc (USA). "
        "`parts.json` holds the same data.",
        "",
        "- **ID** is the `PartId` enum: the bit index in `Car.partsOwned` (`Car_HasPart`, "
        "`PartsOwned_Set`) and the index of the part name in `unistrdb`.",
        "- **Stage** is the part's level in its category, starting at 1 "
        "(`Part_Lookup`, `80076570`). Stage 0 is the stock part, which is not a `PartId`.",
        "- **Slot** is the `g_CarWork_PartSlots` index (`Part_GetSlot`, OVL4 `8001706c`). "
        "Tyres have no slot there.",
        "- **Cars** is how many of the %d cars in the car list offer the part, and the prices "
        "are the lowest and highest over those cars." % len(cars),
        "- ID 8 (`PART_DISPLACEMENT_2`) has no name and no car offers it.",
        "",
        "| ID | Name | Category | Stage | Slot | Cars | Price |",
        "|---|---|---|---|---|---|---|",
    ]
    for p in parts:
        price = ""
        if p["minPrice"] is not None:
            price = ("%d" % p["minPrice"] if p["minPrice"] == p["maxPrice"]
                     else "%d–%d" % (p["minPrice"], p["maxPrice"]))
        lines.append("| %d | %s | %s | %d/%d | %s | %d | %s |" % (
            p["id"], p["name"] or "–", p["category"], p["stage"], p["stages"], opt(p["slot"]),
            p["carCount"], price))
    md_out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("parts:", len(parts), "cars:", len(cars))


if __name__ == "__main__":
    main()
