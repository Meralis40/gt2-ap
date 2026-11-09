from ..data import EVENTS, EVENTS_BY_CODE
from ..rules import event_logic
from . import GT2TestBase


class TestLogic(GT2TestBase):
    options = {"one_make_events": True, "rally_events": True}

    def test_every_event_has_a_car(self) -> None:
        for event in EVENTS:
            self.assertTrue(event_logic(event).cars, event.name)

    def test_opponent_targets(self) -> None:
        # Events without a power limit follow their opponents: 98 PS, then 198 and 248 PS
        targets = [event_logic(EVENTS_BY_CODE[code]).target for code in ("SND0001", "SND0002", "SND0003")]
        self.assertEqual(targets, [98, 198, 248])
        self.assertEqual(event_logic(EVENTS_BY_CODE["FREERACE01"]).target, 0)
        # Under a power limit too: 4WD Challenge Race 1 (350 PS limit) has opponents of 248 PS
        self.assertEqual(event_logic(EVENTS_BY_CODE["4WD0001"]).target, 248)

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


class TestFinishChecks(GT2TestBase):
    options = {"finish_checks": True, "hard_start": True}

    def test_finish_needs_no_power(self) -> None:
        # Any start enters the Sunday Cup with a stock car under 10,000 Cr
        self.assertTrue(self.can_reach_location("Sunday Cup Race 1 Finished"))
        self.assertFalse(self.can_reach_location("UK Nationals Race 1 Finished"))

    def test_finish_follows_entry(self) -> None:
        self.collect_all_but(["Progressive License"])
        self.assertFalse(self.can_reach_location("Japan Nationals Race 1 Finished"))
        self.collect(self.get_items_by_name("Progressive License")[:4])
        self.assertTrue(self.can_reach_location("Japan Nationals Race 1 Finished"))
