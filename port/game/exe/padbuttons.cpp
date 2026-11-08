// Resident code 800834fc-80083ae0 (padbuttons). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "common.h"

extern "C" {
extern unsigned char (&D_800A6F28);
extern int (&D_800A6F20)[2];
extern short (&g_InitRange)[2][2];
extern unsigned char (&D_800A6F2C)[];
extern unsigned char (&D_800A6F34)[];
extern unsigned char (&g_PadButtonMap)[16][2];
extern PadRepeat (&g_PadRepeatStart); // with g_PadRepeatEnd

// Sends force as one actuator byte: its level (0-15, from 256ths) with 0x10 or 0x20
// for its sign, 0 when it is 0.
void FUN_800834fc(PadDevice *dev)
{
    int port = dev->port;
    int p = port << 4;

    if (PadGetState(p) == 6) {
        int v = dev->force;
        PadSystem *sys = &g_PadSystem; // the address isn't folded into %lo(g_PadSystem + 0x18)
        PadPort *pp = &sys->ports[port];
        int dir = 0x10;

        if (v < 0) {
            dir = 0x20;
            v = -v;
        }
        v = (v + 0xff) >> 8;
        v -= 1; // in one expression, the test becomes one of the shift
        if (v == -1) {
            v = dir = 0;
        }
        if (v >= 0x10) {
            v = 0xf;
        }
        pp->act[0] = v | dir;
        PadSetAct(p, pp->act, 1);
        if (pp->align != 0) {
            if (PadSetActAlign(p, D_800A6F2C) != 0) {
                pp->align = 0;
            }
        }
    }
}

// The update of a one-axis analog pad: the buttons, then the axis scaled to 0-255
// over the port's calibrated range.
void FUN_800835dc(PadDevice *dev, int buttons, unsigned short *analog, int a3)
{
    PadButtonState *state = &dev->buttons;
    int port;
    int lo;
    int hi;
    int v;
    short *r;

    PadButtons_Update(state, Pad_RemapButtons(buttons, g_PadButtonMap[0], 16));
    port = dev->port;
    if (a3 == 0 || buttons != 0) {
        D_800A6F20[port] = 0;
    }
    D_800A6F20[port]++;
    dev->analogMask |= 1;
    r = g_InitRange[port];
    v = analog[0];
    lo = r[0];
    hi = r[1];
    if (v > 0x7fff) {
        v -= 0x10000;
    }
    if (v < lo) {
        v = lo;
    }
    if (v > hi) {
        v = hi;
    }
    dev->analog[0] = (v - lo) * 255 / (hi - lo);
}

void FUN_80083708(int param_1, unsigned char param_2)
{
    (&D_800A6F28)[param_1] = param_2;
}

// Sends the two motor levels (vibration0 and vibration1 in whole units, up to 255).
void Pad_UpdateVibration(PadDevice *dev)
{
    int a = dev->vibration0 >> 4;
    int port = dev->port;
    int p = port << 4;
    PadSystem *sys = &g_PadSystem;
    PadPort *pp = &sys->ports[port];
    int mode = pp->mode;
    int b;

    port = dev->vibration1; // port reused: loaded early, shifted after a's clamps
    if (a < 0) {
        a = -a;
    }
    if (a >= 0x100) {
        a = 0xff;
    }
    b = port >> 4;
    if (b < 0) {
        b = -b;
    }
    if (b >= 0x100) {
        b = 0xff;
    }
    if (mode == 1) {
        if (a == 0) {
            a = b >= 0x61;
        }
        b = a;
        a = 0x40;
    }
    pp->act[0] = a;
    pp->act[1] = b;
    PadSetAct(p, pp->act, 2);
    if (PadGetState(p) == 6) {
        if (pp->align != 0) {
            if (PadSetActAlign(p, D_800A6F34) != 0) {
                pp->align = 0;
            }
        }
    }
}

void Pad_UpdateDigital(PadDevice *dev, int buttons)
{
    PadButtonState *state = &dev->buttons;

    PadButtons_Update(state, Pad_RemapButtons(buttons, g_PadButtonMap[0], 16));
    dev->analogMask = 0;
}

void PadButtons_Init(PadButtonState *state)
{
    memset(state, 0, sizeof(PadButtonState));
    *(PadRepeat *)&state->repeatStart = g_PadRepeatStart; // one copy, as two lb
    state->held = 0xffffffff;
}

// Adds a frame's buttons: the events, and each button's hold count for the repeat.
void PadButtons_Update(PadButtonState *state, PadButton buttons)
{
    PadButton bit = 1;
    unsigned char *h = state->holdFrames;
    PadButton repeat = state->repeat;
    int start = state->repeatStart;
    int end = state->repeatEnd;
    PadButton prev = state->held;
    PadButton accum = state->heldAccum;
    PadButton mask = state->releaseMask;
    PadButton changed;

    state->releaseMask = 0xffffffff;
    state->held = buttons;
    changed = prev ^ buttons;
    state->heldAccum = accum | buttons;
    state->pressed |= changed & buttons;
    state->released = (state->released | (changed & prev)) & mask;
    do {
        int n = *h;

        if (buttons & bit) {
            n++;
        } else {
            n = 0;
        }
        h++; // before the store, which is h[-1]
        if (n == end) {
            n = start;
        }
        h[-1] = n;
        if (n == start) {
            repeat |= bit;
        }
        bit <<= 1;
    } while (h != state->holdFrames + 32);
    state->repeat = repeat;
}

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

// Copies the events since the last read into out and clears them.
PadState *PadButtons_Read(PadButtonState *state, PadState *out)
{
    int leave = Sys_CriticalSection(0);

    *out = *(PadState *)&state->heldAccum;
    PadButtons_ClearEvents(state);
    Sys_CriticalSection(leave);
    return out;
}

void PadDevice_FlushInput(PadDevice *dev)
{
    int leave;

    leave = Sys_CriticalSection(0);
    (dev->buttons).releaseMask = 0;
    (dev->buttons).held = 0xffffffff;
    PadButtons_ClearEvents(&dev->buttons);
    Sys_CriticalSection(leave);
}

// Maps buttons through count (from bit, to bit) pairs.
PadButton Pad_RemapButtons(PadButton buttons, unsigned char *map, int count)
{
    PadButton out = 0;

    if (count != 0) {
        do {
            int from = map[0];
            int to = map[1];

            if (buttons & (1 << from)) {
                out |= 1 << to;
            }
            count--;
            map += 2;
        } while (count != 0);
    }
    return out;
}

PadButton Pad_AnalogToButtons(unsigned short *analog, PadAnalogMap *map, int count)
{
    PadButton out = 0;

    if (count != 0) {
        do {
            int t = map->threshold;
            int v = analog[map->axis];
            int b = map->button;

            if (v >= t) { // signed: the values are read into ints first
                out |= 1 << b;
            }
            map++;
            count--;
        } while (count != 0);
    }
    return out;
}
}
