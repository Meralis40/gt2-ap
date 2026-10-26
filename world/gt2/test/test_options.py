from Options import OptionError

from ..data import CARS
from ..locations import LOCATION_GROUPS
from ..rules import START_BUDGET
from . import GT2TestBase


class TestDefault(GT2TestBase):
    def test_start_manufacturer(self) -> None:
        maker = self.world.start_manufacturer
        self.assertIn(maker, ("Honda", "Mazda", "Mitsubishi", "Nissan", "Toyota"))
        self.assertTrue(any(c.maker == maker and c.price <= START_BUDGET for c in CARS.values()))
        self.assertEqual(self.count(f"Manufacturer: {maker}"), 1)

    def test_optional_events_left_out(self) -> None:
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        for group in ("Rally Events", "Endurance Events", "One-Make Events", "Event Synthesizer"):
            self.assertFalse(names & LOCATION_GROUPS[group], group)

    def test_trophies(self) -> None:
        trophies = [i for i in self.multiworld.itempool if i.name == "Trophy"]
        self.assertEqual(len(trophies), 22)


class TestAllEvents(GT2TestBase):
    options = {"rally_events": True, "endurance_events": True, "event_synthesizer": True,
               "one_make_events": True, "hard_start": True}

    def test_events_present(self) -> None:
        names = {loc.name for loc in self.multiworld.get_locations(self.player)}
        for group in ("Rally Events", "Endurance Events", "One-Make Events", "Event Synthesizer"):
            self.assertTrue(LOCATION_GROUPS[group] <= names, group)


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

    def test_pro_above_world(self) -> None:
        self.options = {"pro_league_trophies": 30, "world_league_trophies": 20}
        with self.assertRaises(OptionError):
            self.world_setup()

    def test_all_events_room(self) -> None:
        self.options = {"world_league_trophies": 200, "rally_events": True, "endurance_events": True,
                        "event_synthesizer": True, "one_make_events": True}
        self.world_setup()
        self.assertEqual(len(self.get_items_by_name("Trophy")), 200)
