from ..data import EVENTS
from ..rules import event_logic
from . import GT2TestBase


class TestLogic(GT2TestBase):
    options = {"one_make_events": True, "rally_events": True}

    def test_every_event_has_a_car(self) -> None:
        for event in EVENTS:
            self.assertTrue(event_logic(event).cars, event.name)

    def test_start_events(self) -> None:
        # Only the starting manufacturer: the B tests and the no-license races
        self.assertTrue(self.can_reach_location("B License Test 1"))
        self.assertTrue(self.can_reach_location("Sunday Cup Race 1"))
        self.assertFalse(self.can_reach_location("A License Test 1"))
        self.assertFalse(self.can_reach_location("UK Nationals Race 1"))

    def test_racing_modification(self) -> None:
        self.collect_all_but(["Progressive Weight Reduction"])
        self.assertTrue(self.can_reach_location("Civic Race (Normal)"))
        self.collect(self.get_items_by_name("Progressive Weight Reduction")[:3])
        self.assertFalse(self.can_reach_location("Civic Race (Racing)"))
        self.collect(self.get_items_by_name("Progressive Weight Reduction")[3:])
        self.assertTrue(self.can_reach_location("Civic Race (Racing)"))

    def test_manufacturer(self) -> None:
        self.collect_all_but(["Manufacturer: Honda"])
        if self.world.start_manufacturer != "Honda":
            self.assertFalse(self.can_reach_location("Civic Race (Normal)"))

    def test_dirt_tires(self) -> None:
        self.collect_all_but(["Dirt Tires"])
        self.assertFalse(self.can_reach_location("Tahiti Maze Rally Race 1"))
        self.collect_by_name(["Dirt Tires"])
        self.assertTrue(self.can_reach_location("Tahiti Maze Rally Race 1"))

    def test_trophies(self) -> None:
        self.collect_all_but(["Trophy"])
        self.assertFalse(self.can_reach_location("Pacific League Race 1"))
        trophies = self.get_items_by_name("Trophy")
        self.collect(trophies[:11])
        self.assertTrue(self.can_reach_location("Pacific League Race 1"))
        self.assertFalse(self.can_reach_location("Gran Turismo World League Race 1"))
        self.collect(trophies[11:])
        self.assertTrue(self.can_reach_location("Gran Turismo World League Race 1"))
