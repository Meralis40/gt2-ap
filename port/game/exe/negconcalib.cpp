// Resident code 8006a348-8006a3fc (negconcalib): Applies the saved neGcon calibration.
// OVL1 builds it too.
#include "common.h"

extern "C" {
extern NeGconCalib (&g_NeGconCalib)[2];

// Copies both ports' neGcon calibration from the options to g_NeGconCalib.
void Options_ApplyNeGconCalib(GameState *state)
{
    g_NeGconCalib[0] = state->ports[0].negconCalib;
    g_NeGconCalib[1] = state->ports[1].negconCalib;
}
}
