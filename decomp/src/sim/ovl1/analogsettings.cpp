// OVL1 8001aad4-8001d4b0 (analogsettings): Analog controller settings and calibration.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern GameState g_GameState;
extern char *g_AnalogSettings_Prompt;

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Options_KanjiReset);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Options_KanjiCache);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Options_KanjiDraw);

// Japanese only: caches the current prompt.
void AnalogSettings_CachePrompt(void)
{
    if (g_GameState.language == '\0') {
        Options_KanjiReset();
        Options_KanjiCache(g_AnalogSettings_Prompt);
    }
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", SteerCalib_Clamp);

void SteerCalib_SetMargin(SteerCalib *c, int v)
{
    c->margin = v;
    SteerCalib_Clamp(c);
}

void SteerCalib_SetMax(SteerCalib *c, int v)
{
    c->max = v;
    SteerCalib_Clamp(c);
}

void SteerCalib_SetCenter(SteerCalib *c, int v)
{
    c->center = v;
    SteerCalib_Clamp(c);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", SteerCalib_SetLimits);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_DrawSteerNA);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_DrawSteerValues);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_DrawSteerGauge);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", PedalCalib_Clamp);

void PedalCalib_SetMargin(PedalCalib *c, int v)
{
    c->margin = v;
    PedalCalib_Clamp(c);
}

void PedalCalib_SetMax(PedalCalib *c, int v)
{
    c->max = v;
    PedalCalib_Clamp(c);
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_DrawPedalNA);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_DrawPedalValues);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_DrawPedalGauge);

// NeGcon map of the steering slot.
unsigned char Analog_GetSteerMap(int port)
{
    return g_GameState.ports[port].buttonMaps[2][0];
}

// NeGcon map of the accelerator slot.
unsigned char Analog_GetAccelMap(int port)
{
    return g_GameState.ports[port].buttonMaps[2][2];
}

// NeGcon map of the brake slot.
unsigned char Analog_GetBrakeMap(int port)
{
    return g_GameState.ports[port].buttonMaps[2][3];
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Analog_GetSteerAxis);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Analog_GetAccelAxis);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Analog_GetBrakeAxis);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Analog_IsSteerAnalog);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Analog_IsAccelAnalog);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", Analog_IsBrakeAnalog);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", SteerCalib_Load);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", PedalCalib_Load);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", SteerCalib_Save);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", PedalCalib_Save);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_BuildSteps);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_Init);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_Reset);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_HideIcons);

// mode = 3 (inactive).
void AnalogSettings_Close(AnalogSettings *s)
{
    s->mode = 3;
}

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_Open);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_Update);

INCLUDE_ASM("asm/sim/ovl1/nonmatchings/analogsettings", AnalogSettings_Draw);
}
