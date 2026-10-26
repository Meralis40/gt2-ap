from dataclasses import dataclass
from typing import TYPE_CHECKING

from BaseClasses import Item, ItemClassification

from .data import MANUFACTURERS

if TYPE_CHECKING:
    from . import GT2World

PROGRESSIVE_LICENSE = "Progressive License"
TROPHY = "Trophy"


def manufacturer_item(maker: str) -> str:
    return f"Manufacturer: {maker}"


@dataclass(frozen=True)
class PartItem:
    name: str
    stages: tuple[tuple[int, ...], ...]  # the PartIds each copy unlocks, in order
    logic: bool  # used by the logic (power, racing modification, dirt tyres)


# Tuning parts. The stages follow the PartId order (docs/game ids/parts.md). The racing
# modification is the 4th stage of weight reduction, since the game requires weight reduction 3
# before it (Tune_CanBuyPart).
PART_ITEMS: list[PartItem] = [
    PartItem("Progressive Turbo", ((46,), (47,), (48,), (49,)), True),
    PartItem("Progressive NA Tune", ((29,), (30,), (31,)), True),
    PartItem("Progressive Muffler", ((26,), (27,), (28,)), True),
    PartItem("Progressive Intercooler", ((16,), (17,)), True),
    PartItem("Progressive Weight Reduction", ((18,), (19,), (20,), (34,)), True),
    PartItem("Performance Chip", ((6,),), True),
    PartItem("Displacement Increase", ((7,),), True),
    PartItem("Engine Balancing", ((9,),), True),
    PartItem("Port & Polish", ((32,),), True),
    PartItem("Dirt Tires", ((45,),), True),
    PartItem("Progressive Clutch", ((3,), (4,), (5,)), False),
    PartItem("Progressive Flywheel", ((10,), (11,), (12,)), False),
    PartItem("Progressive Transmission", ((13,), (14,), (15,)), False),
    PartItem("Progressive Suspension", ((35,), (36,), (37,)), False),
    PartItem("Progressive Racing Tires", ((39,), (40,), (41,), (42,), (43,)), False),
    PartItem("Simulation Tires", ((44,),), False),
    PartItem("Limited-Slip Differential", ((21, 22, 23, 24, 25),), False),
    PartItem("Sports Brakes", ((1,),), False),
    PartItem("Brake Balance Controller", ((2,),), False),
    PartItem("Carbon Driveshaft", ((33,),), False),
    PartItem("Active Stability Controller", ((0,),), False),
    PartItem("TCS Controller", ((38,),), False),
]
PART_ITEMS_BY_NAME = {p.name: p for p in PART_ITEMS}
DIRT_TIRES = "Dirt Tires"
WEIGHT_REDUCTION = "Progressive Weight Reduction"

# Credits, one amount per license tier (0 = no license, 6 = S): the filler is spread over the
# tiers like the locations are, so it mostly helps at the level it is found.
CREDIT_AMOUNTS = [3000, 5000, 10000, 20000, 35000, 50000, 75000]


def credit_item(amount: int) -> str:
    return f"{amount:,} Cr"


ITEM_NAME_TO_ID: dict[str, int] = {PROGRESSIVE_LICENSE: 1, TROPHY: 2}
ITEM_NAME_TO_ID.update({manufacturer_item(m): 100 + mid for m, mid in MANUFACTURERS.items()})
ITEM_NAME_TO_ID.update({p.name: 200 + i for i, p in enumerate(PART_ITEMS)})
ITEM_NAME_TO_ID.update({credit_item(a): 300 + i for i, a in enumerate(CREDIT_AMOUNTS)})

ITEM_GROUPS: dict[str, set[str]] = {
    "Manufacturers": {manufacturer_item(m) for m in MANUFACTURERS},
    "Tuning Parts": {p.name for p in PART_ITEMS},
    "Credits": {credit_item(a) for a in CREDIT_AMOUNTS},
}


class GT2Item(Item):
    game = "Gran Turismo 2"


def classification(world: "GT2World", name: str) -> ItemClassification:
    if name in (PROGRESSIVE_LICENSE, TROPHY) or name in ITEM_GROUPS["Manufacturers"]:
        return ItemClassification.progression
    part = PART_ITEMS_BY_NAME.get(name)
    if part is not None:
        if name == DIRT_TIRES and not world.options.rally_events:
            return ItemClassification.useful
        return ItemClassification.progression if part.logic else ItemClassification.useful
    return ItemClassification.filler


def create_item(world: "GT2World", name: str) -> GT2Item:
    return GT2Item(name, classification(world, name), ITEM_NAME_TO_ID[name], world.player)


def progression_item_names(world: "GT2World") -> list[str]:
    """Every non-filler item of the pool, the starting manufacturer and trophies excluded."""
    names = [PROGRESSIVE_LICENSE] * 6
    names += [manufacturer_item(m) for m in MANUFACTURERS if m != world.start_manufacturer]
    for part in PART_ITEMS:
        names += [part.name] * len(part.stages)
    return names


def create_all_items(world: "GT2World") -> None:
    pool = [create_item(world, name) for name in progression_item_names(world)]
    if world.options.league_trophies:
        pool += [create_item(world, TROPHY) for _ in range(world.trophy_count)]

    # Fill the rest with credits, spread over the tiers like the locations
    free = len(world.multiworld.get_unfilled_locations(world.player)) - len(pool)
    tiers = world.location_tiers()
    total = sum(tiers)
    counts = [free * n // total for n in tiers]
    for tier in world.random.choices(range(len(tiers)), weights=tiers, k=free - sum(counts)):
        counts[tier] += 1
    for tier, count in enumerate(counts):
        pool += [create_item(world, credit_item(CREDIT_AMOUNTS[tier])) for _ in range(count)]
    world.multiworld.itempool += pool


def random_filler(world: "GT2World") -> str:
    tiers = world.location_tiers()
    return credit_item(CREDIT_AMOUNTS[world.random.choices(range(len(tiers)), weights=tiers)[0]])
