// Puts the garage cars chosen in the arcade selection into the race setup
// (docs/arcade.md): both players' cars for a two-player race (raceType 0),
// the player's car for a time trial (6) or a single race (4).
#include "common.h"
#include "gamestate.h"

// The offset is added to the base as one value, as the game never folds the
// garages offset (0x3c74) into g_GameState's address.
inline Garage *GarageAt(int n)
{
    char *base = (char *)&g_GameState;
    int offset = 0x3c74 + n * (int)sizeof(Garage);
    return (Garage *)(base + offset);
}

inline void SetupPlayerCar(RaceSetup *setup, ArcadeRaceSel *sel, int i, int garage)
{
    Garage *g = GarageAt(garage);
    int none = -1;
    int slot = sel->slots[i];
    ArcadeSel_LoadGarageCar(garage, slot, sel->dirt);
    Car *car = &g->cars[slot];
    RaceSetup_SetCarFromGarage(setup, i, i, i + 3, sel->unkB6[i], car, none, none);
}

extern "C" void ArcadeSel_SetupRaceCars(ArcadeRaceSel *sel)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *raceSetup = (RaceSetup *)(base + offset);
    RaceSetup *setup = raceSetup;  // the game keeps both: race 4 uses raceSetup
    int i;
    int garage;

    switch (sel->raceType) {
    case 0:
        for (i = 0; i < 2; i++) {
            garage = sel->garages[i];
            if (garage >= 0) {
                SetupPlayerCar(setup, sel, i, garage);
            }
        }
        break;
    case 6:
        garage = sel->garage;
        if (garage >= 0) {
            Garage *g = GarageAt(garage);
            int slot = sel->slot;
            ArcadeSel_LoadGarageCar(garage, slot, sel->dirt);
            RaceSetup_SetCarFromGarage(setup, 0, 0, 3, sel->unk18, &g->cars[slot], garage, sel->slot);
        }
        break;
    case 4:
        garage = sel->garage;
        if (garage >= 0) {
            Garage *g = GarageAt(garage);
            ArcadeSel_LoadGarageCar(garage, sel->slot, sel->dirt);
            Car *car = &g->cars[sel->slot];
            RaceSetup_SetCarFromGarage(raceSetup, 0, -1, 3, sel->unk18, car, garage, sel->slot);
        }
        break;
    }
}
