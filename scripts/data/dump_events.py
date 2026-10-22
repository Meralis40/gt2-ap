# SPDX-License-Identifier: GPL-2.0-or-later
# Dumps every GT mode event and its entry restrictions to JSON and Markdown.
#
# Input is a GT2.VOL extraction made with gt2-discinfo, for example:
#   gt2-discinfo vol extract <disc.cue> <dir> /carparam
#   gt2-discinfo vol extract <disc.cue> <dir> /gtmenu/usa
# then:
#   python scripts/data/dump_events.py <dir> "docs/game ids/events.json" "docs/game ids/events.md"
#
# Sources (Simulation disc, USA):
#   carparam/usa_gtmode_race.dat.gz  events (table 0, 0x9c bytes each), allowed-car lists (table 2)
#   carparam/usa_gtmode_data.dat.gz  car list (table 30) for car names and manufacturers
#   carparam/usa_unistrdb.dat.gz     car name strings
#   gtmenu/usa/gtmenudat.dat         menu pages, to find which events have a menu entry

import gzip
import json
import struct
import sys
import zlib
from pathlib import Path

CODE_ALPHABET = "-0123456789abcdefghijklmnopqrstuvwxyz"

# From docs/game ids/manufacturers.md
MANUFACTURERS = {
    0: "Acura", 1: "Alfa Romeo", 2: "Aston Martin", 3: "Audi", 4: "BMW", 5: "Chevrolet",
    6: "Citroen", 7: "Daihatsu", 8: "Dodge", 9: "FIAT", 10: "Ford", 11: "Honda", 13: "Jaguar",
    14: "Lancia", 16: "Lister", 17: "Lotus", 18: "Mazda", 19: "Mercedes-Benz", 21: "Mini & MG",
    22: "Mitsubishi", 23: "Nissan", 24: "Opel", 25: "Peugeot", 26: "Plymouth", 27: "Renault",
    28: "RUF", 29: "Shelby", 30: "Subaru", 31: "Suzuki", 32: "Tommy kaira", 33: "Toyota",
    34: "TVR", 36: "Vector", 37: "Venturi", 38: "Volkswagen",
}

# EventRules.flags fields, see Race_CheckEntry (OVL4 8001973c)
LICENSES = [None, "B", "A", "IC", "IB", "IA", "S"]
DRIVETRAINS = [None, "FF", "FR", "MR", "RR", "AWD"]
TURBO_RULES = [None, "no_turbo", "turbo_required", None]
RACING_MOD_RULES = [None, "no_racing_mod", "racing_mod_required", "stock_only"]

# Hard-coded lists in OVL4 (80050bb4, 80050bf8)
REGIONAL_LEAGUES = ["GJL0001", "GJL0002", "GJL0003", "GBL0001", "GBL0002", "GBL0003",
                    "GFL0001", "GFL0002", "GGL0001", "GGL0002", "GGL0003", "GIL0001",
                    "GIL0002", "GUL0001", "GUL0002", "GUL0003"]
PRO_LEAGUES = ["PFL0001", "PFL0002", "PFL0003", "EPL0001", "EPL0002", "EPL0003"]
# Run through FUN_OVL4__80012c6c with no event index, so no result is stored
SPECIAL_EVENTS = {"G400", "G1000", "GMAX"}


def decode_car(code):
    return "".join(CODE_ALPHABET[(code >> s) & 63] for s in (24, 18, 12, 6, 0))


def gtdt_tables(data, entry_sizes):
    assert data[:4] == b"GTDT"
    count = struct.unpack_from("<H", data, 6)[0] // 2
    tables = []
    for i in range(count):
        off, size = struct.unpack_from("<II", data, 8 + 8 * i)
        sz = entry_sizes[i]
        tables.append([data[off + k * sz:off + (k + 1) * sz] for k in range(size // sz)])
    # The string table follows the last data table
    off, size = struct.unpack_from("<II", data, 8 + 8 * count)
    strings, p = [], off + 2
    for _ in range(struct.unpack_from("<H", data, off)[0]):
        n = data[p]
        strings.append(data[p + 1:p + 1 + n].decode("latin-1"))
        p += n + 2
    return tables, strings


def load_unistrdb(data):
    assert data[4:8] == b"WSDB"
    strings, p = [], 10
    for _ in range(struct.unpack_from("<H", data, 8)[0]):
        n = struct.unpack_from("<H", data, p)[0]
        strings.append(data[p + 2:p + 2 + 2 * n].decode("utf-16-le", "replace"))
        p += 2 + 2 * n + 2
    return strings


def load_cars(vol):
    data = gzip.decompress((vol / "carparam/usa_gtmode_data.dat.gz").read_bytes())
    names = load_unistrdb(gzip.decompress((vol / "carparam/usa_unistrdb.dat.gz").read_bytes()))
    count = struct.unpack_from("<H", data, 6)[0] // 2
    off, size = struct.unpack_from("<II", data, 8 + 8 * (count - 1))
    cars = {}
    for k in range(size // 0x48):
        e = data[off + k * 0x48:off + (k + 1) * 0x48]
        code, = struct.unpack_from("<I", e)
        n1, n2 = struct.unpack_from("<HH", e, 0x3c)
        cars[code] = {
            "code": decode_car(code),
            "manufacturer": MANUFACTURERS.get(e[0x3a], "#%d" % e[0x3a]),
            "name": (names[n1] + " " + names[n2]).strip(),
        }
    return cars


def menu_pages(vol):
    """Yields (page index, page data) for every gzip record of gtmenudat.dat."""
    data = (vol / "gtmenu/usa/gtmenudat.dat").read_bytes()
    pos, index = 0, 0
    while True:
        pos = data.find(b"\x1f\x8b\x08", pos)
        if pos < 0:
            return
        d = zlib.decompressobj(31)
        page = d.decompress(data[pos:])
        yield index, page
        index += 1
        pos = len(data) - len(d.unused_data)


def menu_event_codes(vol):
    """Event codes of selectable race elements (flag bit 27), see SimMap_Update."""
    found = {}
    for index, page in menu_pages(vol):
        elements, p = [], 8
        for _ in range(struct.unpack_from("<I", page, 4)[0]):
            a, b = struct.unpack_from("<HH", page, p)
            p += 4 + a * 12
            elements += [page[p + i * 0x4c:p + (i + 1) * 0x4c] for i in range(b)]
            p += b * 0x4c
        n, = struct.unpack_from("<I", page, p)
        elements += [page[p + 4 + i * 0x4c:p + 4 + (i + 1) * 0x4c] for i in range(n)]
        for e in elements:
            flags, = struct.unpack_from("<I", e, 8)
            if flags >> 27 & 1:
                code = e[0xc:0x1c].split(b"\0")[0].decode("latin-1")
                found.setdefault(code, []).append(index)
    return found


def main():
    vol, json_out, md_out = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
    race = gzip.decompress((vol / "carparam/usa_gtmode_race.dat.gz").read_bytes())
    # Entry sizes from 80092490
    (events, _opponents, car_lists), strings = gtdt_tables(race, [0x9c, 0x60, 0x80])
    cars = load_cars(vol)
    in_menu = menu_event_codes(vol)

    used = set()

    def car(code):
        used.add(code)
        return cars[code]["code"]

    out = []
    for index, e in enumerate(events):
        code = strings[struct.unpack_from("<H", e, 0)[0]]
        course = strings[struct.unpack_from("<H", e, 2)[0]]
        max_power, bonus = struct.unpack_from("<hH", e, 0x96)
        list_id = e[0x76]
        allowed = []
        if list_id:
            allowed = [car(c) for c in struct.unpack_from("<32I", car_lists[list_id - 1]) if c]
        prize_cars = [car(c) for c in struct.unpack_from("<4I", e, 0x84) if c]

        if code.startswith("L"):
            kind = "license"
        elif code in SPECIAL_EVENTS:
            kind = "special"
        else:
            kind = "race"
        ev = {
            "index": index,
            "code": code,
            "kind": kind,
            "course": None if course == "none" else course,
            "category": strings[struct.unpack_from("<H", e, 0x94)[0]],
            "laps": e[0x45],
            "inMenu": code in in_menu,
            "license": LICENSES[e[0x47]],
            "drivetrain": DRIVETRAINS[e[0x77]],
            "dirtTires": e[0x75] == 1,
            "turbo": TURBO_RULES[e[0x9a]],
            "racingMod": RACING_MOD_RULES[e[0x9b]],
            "maxPower": max_power if max_power > 0 else None,
            "allowedCars": allowed,
            "requiresWon": [],
            "prizes": [struct.unpack_from("<H", e, 0x78 + 2 * i)[0] * 100 for i in range(6)],
            "championshipPrize": bonus * 100,  # paid for winning the series, set on race 01 only
            "prizeCars": prize_cars,
        }
        if code[:3] in ("PFL", "EPL"):
            ev["requiresWon"] = REGIONAL_LEAGUES
        elif code[:3] == "GTW":
            ev["requiresWon"] = PRO_LEAGUES
        # Championships: the menu starts xxx0501, which runs xxx0501..xxx0505 in a row
        if len(code) >= 4 and code[-4:-2] == "05":
            ev["championship"] = {"first": code[:-2] + "01", "race": int(code[-2:]), "races": 5}
        out.append(ev)

    car_table = {cars[c]["code"]: {"manufacturer": cars[c]["manufacturer"], "name": cars[c]["name"]}
                 for c in sorted(used, key=lambda c: cars[c]["code"])}
    json_out.write_text(json.dumps({"events": out, "cars": car_table}, indent=1, ensure_ascii=False) + "\n",
                        encoding="utf-8", newline="\n")

    def opt(v):
        return "" if v in (None, False, []) else str(v)

    lines = [
        "# GT mode events",
        "",
        "Generated by `scripts/data/dump_events.py` from the Simulation disc (USA).",
        "Full data, including allowed and prize cars, is in [events.json](events.json).",
        "",
        "- `#` is the event index: the slot in `PlayerStats.eventResults` (0x801c99f8, one nibble",
        "  per event, 0 = never finished, 1-6 = best place).",
        "- Kind `license` rows are the per-license entries; their prize car is the license reward.",
        "  `special` events (G400, G1000, GMAX) run without an event index and store no result.",
        "- Course `random` means the game picks a course when the race starts.",
        "- Menu `no` means no menu element starts the event. Championship races 02-05 run after",
        "  01; FPM0001 and FPS0001 have no entry at all.",
        "- The restriction columns follow `Race_CheckEntry` (OVL4 8001973c), in its check order:",
        "  required leagues, dirt tyres, license, drivetrain, turbo, racing modification, max power",
        "  (PS), allowed cars. An empty cell means no restriction.",
        "- `championshipPrize` (JSON only) is paid for winning a championship; it is set on race 01",
        "  of each series and 0 everywhere else.",
        "- Requires: PFL/EPL need every regional league race (GJL, GBL, GFL, GGL, GIL, GUL) won;",
        "  GTW needs every PFL/EPL race won.",
        "",
        "| # | Code | Kind | Course | Laps | Menu | License | Drive | Dirt | Turbo | Racing mod | Max PS | Cars | Requires | 1st prize | Prize cars |",
        "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|",
    ]
    for ev in out:
        req = ""
        if ev["requiresWon"]:
            req = "regional leagues" if ev["requiresWon"] is REGIONAL_LEAGUES else "pro leagues"
        lines.append("| %d | %s | %s | %s | %d | %s | %s | %s | %s | %s | %s | %s | %s | %s | %d | %s |" % (
            ev["index"], ev["code"], ev["kind"], ev["course"] or "random", ev["laps"],
            "yes" if ev["inMenu"] else "no", opt(ev["license"]), opt(ev["drivetrain"]),
            "yes" if ev["dirtTires"] else "", opt(ev["turbo"]), opt(ev["racingMod"]),
            opt(ev["maxPower"]), len(ev["allowedCars"]) or "", req, ev["prizes"][0],
            ", ".join("%s %s" % (car_table[c]["manufacturer"], car_table[c]["name"])
                      for c in ev["prizeCars"])))
    md_out.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    print("events:", len(out), "in menu:", sum(e["inMenu"] for e in out))


if __name__ == "__main__":
    main()
