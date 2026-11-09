"""Event logic.

An event is in logic when its license and league requirements are met, and one of its allowed
cars is in reach: its manufacturer is unlocked, it costs no more than the price cap of the
licenses held, and with the unlocked tuning parts it reaches the event's power target.

The price cap keeps the logic from expecting a lot of grinding at each license level. The power
target is a share of the event's power limit, or a flat value per license for events without a
limit. Power is estimated as the stock power plus the gain of each part on its own (from
dump_dealers.py), scaled down a little since the gains don't add up exactly.
"""

import dataclasses
from dataclasses import dataclass
from functools import cache
from typing import TYPE_CHECKING, Optional

from typing_extensions import override

from BaseClasses import CollectionState
from NetUtils import JSONMessagePart
from rule_builder.rules import And, CanReachLocation, Has, Rule, True_

from .data import CARS, EVENTS, EVENTS_BY_CODE, Car, Event
from .items import (DIRT_TIRES, PROGRESSIVE_LICENSE, TROPHY, WEIGHT_REDUCTION,
                    manufacturer_item)

if TYPE_CHECKING:
    from . import GT2World

# Highest car or part price the logic expects, by Progressive License count. None: no cap.
PRICE_CAPS: list[Optional[int]] = [10_000, 25_000, 50_000, 100_000, 250_000, None, None]
START_BUDGET = PRICE_CAPS[0]  # the game starts with 10,000 Cr
# Power target, as a share of the event's power limit
POWER_RATIO = 0.9
# Power target of the events without a limit, by license tier
TIER_TARGETS = [140, 200, 250, 330, 420, 520, 520]
# Share of the summed part gains the logic counts on
GAIN_FACTOR = 0.9
# Best stock power under START_BUDGET that makes a manufacturer a normal (not hard) start
NORMAL_START_POWER = 140

# Power parts: item name -> PartId of each copy, in order
POWER_ITEMS: dict[str, tuple[int, ...]] = {
    "Progressive Muffler": (26, 27, 28),
    "Progressive Intercooler": (16, 17),
    "Performance Chip": (6,),
    "Displacement Increase": (7,),
    "Engine Balancing": (9,),
    "Port & Polish": (32,),
}
TURBO_ITEM, TURBO_KITS = "Progressive Turbo", (46, 47, 48, 49)
NA_TUNE_ITEM, NA_TUNES = "Progressive NA Tune", (29, 30, 31)
LOGIC_ITEMS = list(POWER_ITEMS) + [TURBO_ITEM, NA_TUNE_ITEM, WEIGHT_REDUCTION, DIRT_TIRES]
RACING_MOD_STAGE = 4  # the racing modification is the 4th Progressive Weight Reduction


def affordable(price: Optional[int], cap: Optional[int]) -> bool:
    return price is not None and (cap is None or price <= cap)


def best_stage(car: Car, stages: tuple[int, ...], count: int, cap: Optional[int]) -> int:
    """Best gain of the first count stages of a progressive part."""
    return max((car.parts[pid][1] for pid in stages[:count]
                if pid in car.parts and affordable(car.parts[pid][0], cap)), default=0)


def car_power(car: Car, counts: dict[str, int], cap: Optional[int], *, turbo_kits: bool = True,
              racing_mod: bool = True) -> float:
    gain = sum(best_stage(car, stages, counts[name], cap) for name, stages in POWER_ITEMS.items())
    # A turbo kit resets the NA tune-up and the reverse, so only one of them counts
    kit = best_stage(car, NA_TUNES, counts[NA_TUNE_ITEM], cap)
    if turbo_kits:
        kit = max(kit, best_stage(car, TURBO_KITS, counts[TURBO_ITEM], cap))
    if (racing_mod and car.rm and counts[WEIGHT_REDUCTION] >= RACING_MOD_STAGE
            and affordable(car.rm_price, cap)):
        gain += car.rm_gain
    return car.power + GAIN_FACTOR * (gain + kit)


ALL_PARTS = {name: 6 for name in LOGIC_ITEMS}


@dataclass(frozen=True)
class EventLogic:
    cars: tuple[Car, ...]
    target: float

    def car_ok(self, event: Event, car: Car, counts: dict[str, int], cap: Optional[int],
               power: bool = True) -> bool:
        if not affordable(car.price, cap):
            return False
        if event.category == "rally" and not (counts[DIRT_TIRES] and affordable(car.dirt_price, cap)):
            return False
        if event.racing_mod == "racing_mod_required" and not (
                counts[WEIGHT_REDUCTION] >= RACING_MOD_STAGE and affordable(car.rm_price, cap)):
            return False
        if event.turbo == "turbo_required" and not car.turbo and not any(
                pid in car.parts and affordable(car.parts[pid][0], cap)
                for pid in TURBO_KITS[:counts[TURBO_ITEM]]):
            return False
        if not power or self.target <= 0:
            return True
        return car_power(car, counts, cap, turbo_kits=event.turbo != "no_turbo",
                         racing_mod=event.racing_mod != "no_racing_mod") >= self.target


def eligible(event: Event, car: Car) -> bool:
    """The entry rules of Race_CheckEntry that don't depend on items."""
    if event.allowed_cars and car.code not in event.allowed_cars:
        return False
    if event.drive is not None and car.drive != event.drive:
        return False
    if event.max_power is not None and car.power > event.max_power:
        return False
    if event.category == "rally" and not car.dirt:
        return False
    if event.racing_mod == "racing_mod_required" and not car.rm:
        return False
    if event.turbo == "no_turbo" and car.turbo:
        return False
    if event.turbo == "turbo_required" and not car.turbo and not any(p in car.parts for p in TURBO_KITS):
        return False
    return True


def dominates(a: Car, b: Car) -> bool:
    """a is at least as good as b in every situation: same manufacturer, cheaper, at least as
    powerful, and every part b has is as cheap and as strong on a."""
    if a.maker != b.maker or a.price > b.price or a.power < b.power or a.turbo < b.turbo:
        return False
    for pid, (price, gain) in b.parts.items():
        if pid not in a.parts or a.parts[pid][0] > price or a.parts[pid][1] < gain:
            return False
    if b.rm and not (a.rm and a.rm_price <= b.rm_price and a.rm_gain >= b.rm_gain):
        return False
    if b.dirt and not (a.dirt and a.dirt_price <= b.dirt_price):
        return False
    return True


@cache
def event_logic(event: Event) -> EventLogic:
    """Cars worth checking for an event (dominated ones are dropped) and its power target."""
    cars = sorted((c for c in CARS.values() if eligible(event, c)), key=lambda c: (c.price, c.code))
    kept: list[Car] = []
    for car in cars:
        if not any(dominates(k, car) for k in kept):
            kept = [k for k in kept if not dominates(car, k)] + [car]
    if event.category in ("one_make", "synthesizer"):
        # One-make opponents drive the same models; synthesizer opponents follow the player's power
        target = 0.0
    else:
        target = POWER_RATIO * event.max_power if event.max_power else TIER_TARGETS[event.tier]
        # Never above what the best allowed car reaches with every part
        best = max((car_power(c, ALL_PARTS, None, turbo_kits=event.turbo != "no_turbo",
                              racing_mod=event.racing_mod != "no_racing_mod") for c in kept),
                   default=0.0)
        target = min(target, best)
    return EventLogic(tuple(sorted(kept, key=lambda c: c.price)), target)


def start_manufacturers(hard_start: bool) -> list[str]:
    """Manufacturers with a car under the starting budget; with a strong one unless hard_start."""
    best: dict[str, int] = {}
    for car in CARS.values():
        if car.price <= START_BUDGET:
            best[car.maker] = max(best.get(car.maker, 0), car.power)
    return sorted(m for m, power in best.items() if hard_start or power >= NORMAL_START_POWER)


def price_text(cap: Optional[int]) -> str:
    return "any price" if cap is None else f"{cap:,} Cr or less"


@dataclasses.dataclass()
class CarForEvent(Rule["GT2World"], game="Gran Turismo 2"):
    """A car allowed in the event, of an unlocked manufacturer, under the price cap of the licenses
    held, and with the unlocked parts reaching the event's power target (unless power is False:
    only finishing the race, not winning it)."""

    event_code: str
    power: bool = True

    @override
    def _instantiate(self, world: "GT2World") -> Rule.Resolved:
        if not event_logic(EVENTS_BY_CODE[self.event_code]).cars:
            raise ValueError(f"no car can enter {EVENTS_BY_CODE[self.event_code].name}")
        return self.Resolved(self.event_code, self.power, player=world.player,
                             caching_enabled=getattr(world, "rule_caching_enabled", False))

    class Resolved(Rule.Resolved):
        event_code: str
        power: bool

        def find_car(self, state: CollectionState) -> Optional[Car]:
            event = EVENTS_BY_CODE[self.event_code]
            logic = event_logic(event)
            items = state.prog_items[self.player]
            cap = PRICE_CAPS[min(items[PROGRESSIVE_LICENSE], len(PRICE_CAPS) - 1)]
            counts = {name: items[name] for name in LOGIC_ITEMS}
            return next((car for car in logic.cars if items[manufacturer_item(car.maker)]
                         and logic.car_ok(event, car, counts, cap, self.power)), None)

        @override
        def _evaluate(self, state: CollectionState) -> bool:
            return self.find_car(state) is not None

        @override
        def item_dependencies(self) -> dict[str, set[int]]:
            makers = {manufacturer_item(car.maker)
                      for car in event_logic(EVENTS_BY_CODE[self.event_code]).cars}
            return {name: {id(self)} for name in [PROGRESSIVE_LICENSE, *LOGIC_ITEMS, *makers]}

        @override
        def explain_json(self, state: CollectionState | None = None) -> list[JSONMessagePart]:
            target = event_logic(EVENTS_BY_CODE[self.event_code]).target if self.power else 0.0
            power = f" reaching {target:.0f} PS" if target > 0 else ""
            if state is None:
                return [{"type": "text", "text": f"An allowed car{power}"}]
            car = self.find_car(state)
            if car is not None:
                return [{"type": "text", "text": "Car: "},
                        {"type": "color", "color": "green", "text": car.name}]
            cap = PRICE_CAPS[min(state.prog_items[self.player][PROGRESSIVE_LICENSE],
                                 len(PRICE_CAPS) - 1)]
            return [{"type": "color", "color": "salmon",
                     "text": f"No allowed car at {price_text(cap)}{power}"}]

        @override
        def explain_str(self, state: CollectionState | None = None) -> str:
            return "".join(part["text"] for part in self.explain_json(state))

        @override
        def __str__(self) -> str:
            return self.explain_str()


def league_rule(world: "GT2World", event: Event) -> Rule["GT2World"]:
    if event.league not in ("pro", "world"):
        return True_()
    if world.options.league_trophies:
        needed = (world.options.pro_league_trophies if event.league == "pro"
                  else world.options.world_league_trophies).value
        return Has(TROPHY, count=needed)
    # Every race of the previous league won (Events_AllRegionalLeaguesWon, Events_AllProLeaguesWon)
    previous = "regional" if event.league == "pro" else "pro"
    return And(*(CanReachLocation(e.name) for e in EVENTS if e.league == previous))


def event_rule(world: "GT2World", event: Event, power: bool = True) -> Rule["GT2World"]:
    return (Has(PROGRESSIVE_LICENSE, count=event.tier) & league_rule(world, event)
            & CarForEvent(event.code, power))


def set_all_rules(world: "GT2World") -> None:
    for loc in world.get_locations():
        data = world.location_data[loc.name]
        if data.event is not None:
            world.set_rule(loc, event_rule(world, data.event, power=not data.finish))
        else:
            world.set_rule(loc, Has(PROGRESSIVE_LICENSE, count=data.tier))
    world.set_completion_rule(And(*(CanReachLocation(e.name) for e in EVENTS if e.league == "world")))
