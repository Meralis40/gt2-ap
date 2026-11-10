from dataclasses import dataclass

from Options import Choice, DefaultOnToggle,OptionGroup, PerGameCommonOptions, Range, Toggle


class LeagueTrophies(DefaultOnToggle):
    """Replace the league requirements with Trophy items.

    Normally the Pacific and Euro Leagues need every regional league race won, and the GT World
    League needs every Pacific and Euro League race won. With this on, they need a number of
    Trophies instead (see Pro League Trophies and World League Trophies)."""
    display_name = "League Trophies"


class ProLeagueTrophies(Range):
    """Trophies needed to enter the Pacific League and the Euro League."""
    display_name = "Pro League Trophies"
    range_start = 1
    range_end = 250
    default = 11


class WorldLeagueTrophies(Range):
    """Trophies needed to enter the GT World League. This is also the number of Trophies in the
    item pool, with Extra Trophies added. It can't be lower than Pro League Trophies, and the
    Trophies must fit in the locations left after the other progression items, or generation
    fails."""
    display_name = "World League Trophies"
    range_start = 1
    range_end = 250
    default = 22


class ExtraTrophies(Range):
    """Trophies added to the item pool beyond the World League Trophies, so the leagues open
    before every Trophy is found."""
    display_name = "Extra Trophies"
    range_start = 0
    range_end = 250
    default = 0


class StartingManufacturer(Choice):
    """The manufacturer unlocked from the start, among those with a car under 10,000 Cr.

    Standard: one of Honda, Mazda, Mitsubishi, Nissan and Toyota, which all have a car of 195 PS
    or more under 10,000 Cr.
    Hard: one of Daihatsu, FIAT, Subaru and Suzuki, whose cars under 10,000 Cr have 64 PS at most.
    All: any of the nine.
    Or one of them by name."""
    display_name = "Starting Manufacturer"
    option_standard = 0
    option_hard = 1
    option_all = 2
    option_daihatsu = 3
    option_fiat = 4
    option_honda = 5
    option_mazda = 6
    option_mitsubishi = 7
    option_nissan = 8
    option_subaru = 9
    option_suzuki = 10
    option_toyota = 11
    default = option_standard

    @classmethod
    def get_option_name(cls, value: int) -> str:
        return "FIAT" if value == cls.option_fiat else super().get_option_name(value)


class RallyEvents(Toggle):
    """Add the rally events (dirt tyres required) as locations."""
    display_name = "Rally Events"


class EnduranceEvents(Toggle):
    """Add the endurance events (30 laps or more) as locations."""
    display_name = "Endurance Events"


class EventSynthesizer(Toggle):
    """Add the Event Synthesizer races (S License) as locations."""
    display_name = "Event Synthesizer"


class OneMakeEvents(Toggle):
    """Add the one-make events of every manufacturer as locations. Their entry needs the
    manufacturer's item, and the racing ones the racing modification (the 4th Progressive Weight
    Reduction)."""
    display_name = "One-Make Events"


class FinishChecks(Toggle):
    """Add a second location for every event location, checked by finishing the race at any
    position. Its logic only needs to enter the race: the license, the league and an allowed car
    under the price cap, without the power target of a win."""
    display_name = "Finish Checks"


@dataclass
class GT2Options(PerGameCommonOptions):
    league_trophies: LeagueTrophies
    pro_league_trophies: ProLeagueTrophies
    world_league_trophies: WorldLeagueTrophies
    extra_trophies: ExtraTrophies
    starting_manufacturer: StartingManufacturer
    rally_events: RallyEvents
    endurance_events: EnduranceEvents
    event_synthesizer: EventSynthesizer
    one_make_events: OneMakeEvents
    finish_checks: FinishChecks


OPTION_GROUPS = [
    OptionGroup("Leagues", [LeagueTrophies, ProLeagueTrophies, WorldLeagueTrophies,
                                   ExtraTrophies]),
    OptionGroup("Events", [RallyEvents, EnduranceEvents, EventSynthesizer, OneMakeEvents,
                                  FinishChecks]),
]

# Options sent to the client and used by the tracker to regenerate the world
SLOT_DATA_OPTIONS = ["league_trophies", "pro_league_trophies", "world_league_trophies",
                     "extra_trophies", "starting_manufacturer","rally_events", "endurance_events",
                     "event_synthesizer", "one_make_events", "finish_checks"]
