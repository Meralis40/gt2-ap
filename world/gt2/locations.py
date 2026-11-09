from dataclasses import dataclass
from typing import TYPE_CHECKING, Optional

from BaseClasses import Location

from .data import EVENTS, LICENSE_TESTS, LICENSES, Event

if TYPE_CHECKING:
    from . import GT2World

# Location IDs: events won 1000 + event index (the PlayerStats.eventResults slot), events finished
# 3000 + event index, license tests 2000 + tier * 10 + test, licenses 2100 + tier. Tier 1 is B,
# 6 is S.
EVENT_BASE, TEST_BASE, LICENSE_BASE, FINISH_BASE = 1000, 2000, 2100, 3000

CATEGORY_GROUPS = {
    "race": "Races",
    "rally": "Rally Events",
    "endurance": "Endurance Events",
    "one_make": "One-Make Events",
    "synthesizer": "Event Synthesizer",
}


@dataclass(frozen=True)
class LocationData:
    name: str
    id: int
    tier: int  # Progressive Licenses needed to enter
    event: Optional[Event] = None
    finish: bool = False  # finished at any position, not won


def finish_name(event: Event) -> str:
    return f"{event.name} Finished"


def license_name(tier: int) -> str:
    return f"{LICENSES[tier - 1]} License"


ALL_LOCATIONS: list[LocationData] = [LocationData(e.name, EVENT_BASE + e.index, e.tier, e)
                                     for e in EVENTS]
ALL_LOCATIONS += [LocationData(finish_name(e), FINISH_BASE + e.index, e.tier, e, finish=True)
                  for e in EVENTS]
for _tier in range(1, len(LICENSES) + 1):
    # A license's tests need the previous license (License_CheckEntry)
    ALL_LOCATIONS += [LocationData(f"{license_name(_tier)} Test {n + 1}",
                                   TEST_BASE + _tier * 10 + n, _tier - 1)
                      for n in range(LICENSE_TESTS)]
    ALL_LOCATIONS.append(LocationData(license_name(_tier), LICENSE_BASE + _tier, _tier - 1))

LOCATION_NAME_TO_ID = {loc.name: loc.id for loc in ALL_LOCATIONS}

LOCATION_GROUPS: dict[str, set[str]] = {group: set() for group in CATEGORY_GROUPS.values()}
LOCATION_GROUPS["License Tests"] = set()
LOCATION_GROUPS["Finish Checks"] = set()
for _loc in ALL_LOCATIONS:
    if _loc.finish:
        LOCATION_GROUPS["Finish Checks"].add(_loc.name)
    elif _loc.event is not None:
        LOCATION_GROUPS[CATEGORY_GROUPS[_loc.event.category]].add(_loc.name)
    else:
        LOCATION_GROUPS["License Tests"].add(_loc.name)


class GT2Location(Location):
    game = "Gran Turismo 2"


def enabled_locations(world: "GT2World") -> list[LocationData]:
    options = world.options
    enabled = {
        "race": True,
        "rally": bool(options.rally_events),
        "endurance": bool(options.endurance_events),
        "one_make": bool(options.one_make_events),
        "synthesizer": bool(options.event_synthesizer),
    }
    return [loc for loc in ALL_LOCATIONS if loc.event is None
            or (enabled[loc.event.category] and (not loc.finish or options.finish_checks))]
