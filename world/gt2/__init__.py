from typing import Any, ClassVar

from BaseClasses import Region, Tutorial
from Options import OptionError
from rule_builder.cached_world import CachedRuleBuilderWorld
from worlds.AutoWorld import WebWorld

from . import items, locations, rules
from .data import MANUFACTURERS
from .options import OPTION_GROUPS, SLOT_DATA_OPTIONS, GT2Options


class GT2WebWorld(WebWorld):
    theme = "partyTime"
    option_groups = OPTION_GROUPS
    tutorials = [Tutorial(
        "Multiworld Setup Guide",
        "A guide to setting up Gran Turismo 2 for Archipelago.",
        "English",
        "setup_en.md",
        "setup/en",
        ["Meralis40"],
    )]


class GT2World(CachedRuleBuilderWorld):
    """
    Gran Turismo 2 is a racing game for the PlayStation. In the Simulation Mode, earn licenses,
    buy and tune cars and win races around the world, up to the GT World League. The licenses,
    the manufacturers and the tuning parts are items.
    """
    game = "Gran Turismo 2"
    web = GT2WebWorld()
    options_dataclass = GT2Options
    options: GT2Options
    origin_region_name = "GT Mode"

    item_name_to_id = items.ITEM_NAME_TO_ID
    location_name_to_id = locations.LOCATION_NAME_TO_ID
    item_name_groups = items.ITEM_GROUPS
    location_name_groups = locations.LOCATION_GROUPS

    # Universal Tracker: every option that matters is in the slot data
    ut_can_gen_without_yaml: ClassVar[bool] = True

    start_manufacturer: str
    trophy_count: int
    location_data: dict[str, locations.LocationData]

    def generate_early(self) -> None:
        passthrough = getattr(self.multiworld, "re_gen_passthrough", {}).get(self.game)
        if passthrough is not None:
            for key, value in passthrough["options"].items():
                option = getattr(self.options, key, None)
                if option is not None:
                    setattr(self.options, key, option.from_any(value))
            self.start_manufacturer = passthrough["start_manufacturer"]
        else:
            self.start_manufacturer = self.random.choice(
                rules.start_manufacturers(bool(self.options.hard_start)))
        self.multiworld.push_precollected(self.create_item(items.manufacturer_item(self.start_manufacturer)))

        self.location_data = {loc.name: loc for loc in locations.enabled_locations(self)}
        self.trophy_count = 0
        if self.options.league_trophies:
            pro, world = self.options.pro_league_trophies.value, self.options.world_league_trophies.value
            if pro > world:
                raise OptionError(f"{self.player_name}: Pro League Trophies ({pro}) can't be more "
                                  f"than World League Trophies ({world})")
            room = len(self.location_data) - len(items.progression_item_names(self))
            if world > room:
                raise OptionError(f"{self.player_name}: World League Trophies is {world}, but only "
                                  f"{room} locations are left for Trophies with these options")
            self.trophy_count = world

    def create_regions(self) -> None:
        region = Region(self.origin_region_name, self.player, self.multiworld)
        region.locations = [locations.GT2Location(self.player, loc.name, loc.id, region)
                            for loc in self.location_data.values()]
        self.multiworld.regions.append(region)

    def create_items(self) -> None:
        items.create_all_items(self)

    def create_item(self, name: str) -> items.GT2Item:
        return items.create_item(self, name)

    def get_filler_item_name(self) -> str:
        return items.random_filler(self)

    def set_rules(self) -> None:
        rules.set_all_rules(self)

    def location_tiers(self) -> list[int]:
        """Number of locations per license tier (0 to 6)."""
        tiers = [0] * 7
        # Item link groups don't run generate_early: use every location
        data = getattr(self, "location_data", None)
        for loc in data.values() if data is not None else locations.ALL_LOCATIONS:
            tiers[loc.tier] += 1
        return tiers

    def fill_slot_data(self) -> dict[str, Any]:
        return {
            "options": self.options.as_dict(*SLOT_DATA_OPTIONS),
            "start_manufacturer": self.start_manufacturer,
            "start_manufacturer_id": MANUFACTURERS[self.start_manufacturer],
        }

    @staticmethod
    def interpret_slot_data(slot_data: dict[str, Any]) -> dict[str, Any]:
        # Universal Tracker: regenerate from the slot data
        return slot_data
