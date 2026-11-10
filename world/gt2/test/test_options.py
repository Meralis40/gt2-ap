from Options import OptionError

from ..data import CARS
from ..locations import LOCATION_GROUPS
from ..options import StartingManufacturer
from ..rules import START_BUDGET, start_manufacturers
from . import GT2TestBase


class TestDefault(GT2TestBase):
    def test_start_manufacturer(self) -> None:
        maker = self.world.start_manufacturer
        self.assertIn(maker, ("Honda", "Mazda", "Mitsubishi", "Nissan", "Toyota"))
        self.assertTrue(any(c.maker == maker and c.price <= START_BUDGET for c in CARS.values()))
        self.assertEqual(self.count(f"Manufacturer: {maker}"), 1)

    def test_optional_events_left_out(self) -> None:
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        for group in ("Rally Events", "Endurance Events", "One-Make Events", "Event Synthesizer",
                      "Finish Checks"):
            self.assertFalse(names & LOCATION_GROUPS[group], group)

    def test_trophies(self) -> None:
        trophies = [i for i in self.multiworld.itempool if i.name == "Trophy"]
        self.assertEqual(len(trophies), 22)


class TestAllEvents(GT2TestBase):
    options = {"rally_events": True, "endurance_events": True, "event_synthesizer": True,
               "one_make_events": True, "starting_manufacturer": "all"}

    def test_events_present(self) -> None:
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        for group in ("Rally Events", "Endurance Events", "One-Make Events", "Event Synthesizer"):
            self.assertTrue(LOCATION_GROUPS[group] <= names, group)


class TestStartingManufacturer(GT2TestBase):
    auto_construct = False

    def check(self, choice: str, expected: set[str]) -> None:
        self.options = {"starting_manufacturer": choice}
        self.world_setup()
        self.assertIn(self.world.start_manufacturer, expected, choice)
        self.assertEqual(set(start_manufacturers(choice)), expected, choice)

    def test_groups(self) -> None:
        hard = {"Daihatsu", "FIAT", "Subaru", "Suzuki"}
        standard = {"Honda", "Mazda", "Mitsubishi", "Nissan", "Toyota"}
        self.check("hard", hard)
        self.check("standard", standard)
        self.check("all", hard | standard)

    def test_by_name(self) -> None:
        for maker in ("FIAT", "Toyota"):
            self.check(maker.lower(), {maker})
        self.assertEqual(StartingManufacturer.get_option_name(StartingManufacturer.option_fiat),
                         "FIAT")


class TestFinishChecks(GT2TestBase):
    options = {"finish_checks": True, "one_make_events": True}

    def test_one_finish_per_event(self) -> None:
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        finished = names & LOCATION_GROUPS["Finish Checks"]
        self.assertTrue(finished)
        self.assertEqual({n.removesuffix(" Finished") for n in finished},
                         names & (LOCATION_GROUPS["Races"] | LOCATION_GROUPS["One-Make Events"]))


class TestExtraTrophies(GT2TestBase):
    options = {"extra_trophies": 8}

    def test_pool(self) -> None:
        self.assertEqual(len(self.get_items_by_name("Trophy")), 30)

    def test_leagues_open_early(self) -> None:
        self.collect_all_but(["Trophy"])
        self.collect(self.get_items_by_name("Trophy")[:22])
        self.assertTrue(self.can_reach_location("Gran Turismo World League Race 1"))


class TestNoTrophies(GT2TestBase):
    options = {"league_trophies": False}

    def test_no_trophy(self) -> None:
        self.assertFalse([i for i in self.multiworld.itempool if i.name == "Trophy"])

    def test_pro_league_needs_regional_leagues(self) -> None:
        self.collect_all_but(["Progressive License"])
        self.collect(self.get_items_by_name("Progressive License")[:4])
        self.assertTrue(self.can_reach_location("Japan Nationals Race 3"))
        self.assertTrue(self.can_reach_location("Pacific League Race 1"))


class TestTrophyErrors(GT2TestBase):
    auto_construct = False

    def test_too_many_trophies(self) -> None:
        self.options = {"world_league_trophies": 200}
        with self.assertRaises(OptionError):
            self.world_setup()

    def test_too_many_extra_trophies(self) -> None:
        self.options = {"world_league_trophies": 100, "extra_trophies": 100}
        with self.assertRaises(OptionError):
            self.world_setup()

    def test_pro_above_world(self) -> None:
        self.options = {"pro_league_trophies": 30, "world_league_trophies": 20}
        with self.assertRaises(OptionError):
            self.world_setup()

    def test_all_events_room(self) -> None:
        self.options = {"world_league_trophies": 200, "rally_events": True, "endurance_events": True,
                        "event_synthesizer": True, "one_make_events": True}
        self.world_setup()
        self.assertEqual(len(self.get_items_by_name("Trophy")), 200)
