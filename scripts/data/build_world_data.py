# SPDX-License-Identifier: GPL-2.0-or-later
# Builds the data file of the AP world (world/gt2/data/gt2.json) from the dumps in docs/game ids:
# events.json, dealers.json and parts.json. Only what the world logic needs is kept.
#
#   python scripts/data/build_world_data.py "docs/game ids" world/gt2/data/gt2.json

import json
import sys
from pathlib import Path

# Logic tiers: the number of Progressive Licenses that earns each license (License_IsEarned hook)
LICENSE_TIERS = {None: 0, "B": 1, "A": 2, "IC": 3, "IB": 4, "IA": 5, "S": 6}
REGIONAL_LEAGUES = ("GJL", "GBL", "GFL", "GGL", "GIL", "GUL")
PRO_LEAGUES = ("PFL", "EPL")
WORLD_LEAGUE = ("GTW",)
# Races of 30 laps or more: the 300 km, 200 miles, 2 hours, all-night, 100 miles and 30 laps
ENDURANCE_LAPS = 30
TURBO_KITS = {46, 47, 48, 49}
RACING_MOD = 34
DIRT_TIRES = 45


def category(event):
    if event["kind"] == "event_synthesizer":
        return "synthesizer"
    if event["category"] == "One-Make":
        return "one_make"
    if event["dirtTires"]:
        return "rally"
    if event["laps"] >= ENDURANCE_LAPS:
        return "endurance"
    return "race"


def league(code):
    for name, prefixes in (("regional", REGIONAL_LEAGUES), ("pro", PRO_LEAGUES),
                           ("world", WORLD_LEAGUE)):
        if code.startswith(prefixes):
            return name
    return None


def main():
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    events = json.loads((src / "events.json").read_text(encoding="utf-8"))["events"]
    dealers = json.loads((src / "dealers.json").read_text(encoding="utf-8"))["cars"]
    cars_info = {c["code"]: c for c in
                 json.loads((src / "cars.json").read_text(encoding="utf-8"))["cars"]}

    cars = {}
    for c in dealers:
        if c["manufacturer"].startswith("#"):
            continue  # Vauxhall: the dealership is never shown
        prices = [p for p in (c["newPrice"], c["usedMinPrice"]) if p]
        parts = {int(pid): [price, c["powerGains"].get(pid, 0)]
                 for pid, price in c["partPrices"].items()}
        cars[c["code"]] = {
            "name": c["name"],
            "maker": c["manufacturer"],
            "price": min(prices),
            "used": c["newPrice"] is None,
            "drive": c["drivetrain"],
            "power": c["stockPower"],
            "turbo": "TURBO" in cars_info[c["code"]]["engine"].split(),
            "parts": {str(pid): v for pid, v in sorted(parts.items())
                      if pid not in (RACING_MOD, DIRT_TIRES)},
            "rm": RACING_MOD in parts,
            "rmPrice": parts.get(RACING_MOD, [None])[0],
            "rmGain": parts.get(RACING_MOD, [0, 0])[1],
            "dirt": DIRT_TIRES in parts,
            "dirtPrice": parts.get(DIRT_TIRES, [None])[0],
        }

    out_events = []
    for e in events:
        if e["kind"] not in ("race", "event_synthesizer") or e["unreachable"]:
            continue
        out_events.append({
            "index": e["index"],
            "code": e["code"],
            "name": e["name"],
            "category": category(e),
            "league": league(e["code"]),
            "tier": LICENSE_TIERS[e["license"]],
            "drive": e["drivetrain"],
            "turbo": e["turbo"],
            "racingMod": e["racingMod"],
            "maxPower": e["maxPower"],
            "allowedCars": e["allowedCars"],
            "prize": e["prizes"][0],
        })

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps({"events": out_events, "cars": cars}, indent=0,
                              ensure_ascii=False) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
