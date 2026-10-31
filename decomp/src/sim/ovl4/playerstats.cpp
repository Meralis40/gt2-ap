// OVL4 80019c60-80019f5c (playerstats): Player statistics and garage totals.
#include "ovl4.h"

extern "C" {
extern GameState g_GameState;
extern int g_EventCount;

int PlayerStats_GetEventResult(PlayerStats *stats, int event);

// g_GameState.stats.racesEntered.
int PlayerStats_GetRacesEntered(void)
{
    return g_GameState.stats.racesEntered;
}

// g_GameState.stats.racesWon.
int PlayerStats_GetRacesWon(void)
{
    return g_GameState.stats.racesWon;
}

// Wins per race entered, in hundredths of a percent. Wins are capped so the
// product fits.
int PlayerStats_GetWinRate(void)
{
    unsigned int won = PlayerStats_GetRacesWon();
    unsigned int entered = PlayerStats_GetRacesEntered();

    if (entered == 0)
        return 0;
    if (won > 429496)
        won = 429496;
    return won * 10000 / entered;
}

// The average finish place times 100.
int PlayerStats_GetRatio50(void)
{
    GameState *gs = &g_GameState;
    PlayerStats *stats = &gs->stats;
    unsigned int count = stats->finishCount;
    unsigned int sum = stats->finishPlaceSum;

    if (count == 0)
        return 0;
    return sum * 100 / count;
}

// Events won, in hundredths of a percent of the events (less 29).
int Events_GetWinPercent(void)
{
    GameState *gs = &g_GameState;
    int i;
    int won = 0;
    PlayerStats *stats = &gs->stats;

    for (i = 0; i < g_EventCount; i++) {
        if (PlayerStats_GetEventResult(stats, i) == 1)
            won++;
    }
    return won * 10000 / (g_EventCount - 29);
}

// The total prize money, as a high and low part.
void PlayerStats_GetPrizeMoney(int *hi, unsigned int *lo)
{
    GameState *gs = &g_GameState;
    PlayerStats *stats = &gs->stats;

    *hi = stats->prizeMoneyHi;
    *lo = stats->prizeMoneyLo;
}

// g_GameState.garages[0].carCount.
int Garage_GetCarCount(void)
{
    return (int)g_GameState.garages[0].carCount;
}

// The value of every car in the garage, in units of 100,000,000 and the
// rest.
void Garage_GetTotalValue(int *hi, unsigned int *lo)
{
    GameState *gs = &g_GameState;
    Garage *garage = &gs->garages[0];
    unsigned int sum = 0;
    int high = 0;
    int i;

    for (i = 0; i < garage->carCount; i++) {
        Car *car = &garage->cars[i];
        sum += car->value;
        if (sum > 100000000) {
            high++;
            sum -= 100000000;
        }
    }
    *hi = high;
    *lo = sum;
}

// The garage slot of the car with the most power, or -1 if none.
int Garage_FindMostPowerfulCar(void)
{
    GameState *gs = &g_GameState;
    Garage *garage = &gs->garages[0];
    int best = -1;
    int maxPower = 0;
    int i;

    for (i = 0; i < garage->carCount; i++) {
        Car *car = &garage->cars[i];
        int power = car->flags98 & 0x3fff;
        if (maxPower < power) {
            maxPower = power;
            best = i;
        }
    }
    return best;
}

// The number of license tests holding the given medal.
int License_CountMedals(int medal)
{
    int count = 0;
    int i, j;

    for (i = 0; i < 6; i++) {
        GameState *gs = &g_GameState;
        LicenseTest *tests = gs->licenses[i];
        for (j = 0; j < 10; j++) {
            signed char m = tests[j].medal;
            if (m == medal)
                count++;
        }
    }
    return count;
}
}
