# Gran Turismo 2

## What does randomization do to this game?

Only the Simulation Mode is randomized (the Arcade Mode will come in a later version). The
licenses, the manufacturers and the tuning parts become items:

- **Progressive License** (6): each one earns the next license, in the game's order: B, A,
  International C, International B, International A, S. License tests still need the previous
  license, so each Progressive License also opens the next license's tests.
- **Manufacturer: \<name\>** (35): you can only buy and race cars of the manufacturers you have.
  This also gates their one-make events. You start with one manufacturer that sells a car under
  10,000 Cr, new or used.
- **Tuning parts**: a part can only be bought once its item is found. Multi-stage parts are
  progressive: each copy unlocks the next stage (Progressive Turbo has 4 copies for the 4 turbo
  kits, and so on).
- **Trophy** (with League Trophies on): the Pacific League and Euro League need a number of
  Trophies (11 by default) instead of every regional league race won, and the GT World League
  needs more (22 by default) instead of every Pacific and Euro League race won. Extra Trophies
  adds more Trophies to the pool than the World League needs, so the leagues open before every
  Trophy is found.
- **Credits** ("5,000 Cr" and so on) fill the remaining locations. Their amounts follow the license
  levels of the locations, to cut down the grinding.

### Racing modification

The **racing modification is the 4th Progressive Weight Reduction**. The game only sells it to
cars with weight reduction stage 3, so it comes after the 3 weight reduction stages. Racing
versions of one-make events (Civic Race (Racing), and so on) need all 4.

## When do items take effect?

The client shows the manufacturers you can use when you first enter the GT mode menus, and says why
when it refuses a car or a part.

A race's check is sent as soon as you cross the line in 1st place, and the item it holds arrives
right away, so you may see it during the finish. Licenses, manufacturers, parts and trophies count
from then on, but you can only use them back in the GT mode menus. Credits are only added there:
if the save you reload is older, the credits received since are added again, so none are lost.

Checks the server already has, from another game on your slot or `/send_location`, show in your
save once you're in the GT mode menus: a race won as 1st place, a race finished as 6th, and a
license test as a Kid's prize. A championship shows as won once all its races are. You don't get
their prize money or cars.

## What is the goal?

Win every race of the Gran Turismo World League.

## What are the locations?

- Winning (1st place) each race of the GT Mode events.
- Passing each license test (any medal), and earning each license (all of its 10 tests).

Some event categories are off by default and can be turned on in the options: rally events,
endurance events (30 laps or more), the Event Synthesizer and the one-make events. When a
category is off, its races are not locations.

## How does the logic decide what is reachable?

A race is in logic when its license and league requirements are met, and one of the cars allowed
in it is within reach:

- its manufacturer is unlocked;
- its price is under a cap that depends on the licenses held, so the logic doesn't expect a lot of
  grinding: 10,000 Cr without a license, then 25,000 Cr, 50,000 Cr, 100,000 Cr and 250,000 Cr,
  and no cap from the International A License on;
- with the tuning parts unlocked (and priced under the same cap) it gets close to the race's
  power limit (90% of it). Races without a power limit need a power that depends on their license.

Prize cars are not counted by the logic.

## Can I choose the starting manufacturer?

The Starting Manufacturer option picks the manufacturer unlocked from the start, among those with
a car under 10,000 Cr:

- Standard (the default): Honda, Mazda, Mitsubishi, Nissan or Toyota, which all have a car of
  195 PS or more under 10,000 Cr;
- Hard: Daihatsu, FIAT, Subaru or Suzuki. Their cars under 10,000 Cr have 64 PS at most, so the
  start needs more tuning;
- All: any of the nine;
- or one of the nine by name.
