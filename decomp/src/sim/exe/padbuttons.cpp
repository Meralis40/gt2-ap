// Resident code 800834fc-80083ae0 (padbuttons). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void PadButtons_ClearEvents(PadButtonState *state);
int Sys_CriticalSection(int leave);
extern unsigned char D_800A6F28;

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", FUN_800834fc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", FUN_800835dc);

void FUN_80083708(int param_1, unsigned char param_2)
{
    (&D_800A6F28)[param_1] = param_2;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", Pad_UpdateVibration);

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", Pad_UpdateDigital);

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", PadButtons_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", PadButtons_Update);

void PadButtons_ClearEvents(PadButtonState *state)
{
    int leave;

    leave = Sys_CriticalSection(0);
    state->heldAccum = 0;
    state->pressed = 0;
    state->released = 0;
    state->repeat = 0;
    Sys_CriticalSection(leave);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", PadButtons_Read);

void PadDevice_FlushInput(PadDevice *dev)
{
    int leave;

    leave = Sys_CriticalSection(0);
    (dev->buttons).releaseMask = 0;
    (dev->buttons).held = 0xffffffff;
    PadButtons_ClearEvents(&dev->buttons);
    Sys_CriticalSection(leave);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", Pad_RemapButtons);

INCLUDE_ASM("asm/sim/exe/nonmatchings/padbuttons", Pad_AnalogToButtons);
}
