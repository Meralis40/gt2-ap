// OVL0 80014b54-80014e28 (racepad): Race controller input.
// The functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

// PadDevice from analogMask to unk58 (0x48-0x59), copied as one block to RacePad.analogCopy.
struct PadAnalogState {
    unsigned short mask;
    unsigned short axes[4];
    unsigned char unk0a[6];
    short unk10;
};

extern "C" {
// Opens the pad device and links its saved key config (g_GameState.ports[port]).
void RacePad_Open(RacePad *pad, int port)
{
    GameState *gs;

    PadDevice_Open(&pad->dev, port, g_PadHandlers);
    gs = &g_GameState;
    pad->config = &gs->ports[port];
}

// Reads the pad and maps it to race actions with the key config of its controller kind
// (0 digital, 1 analog, 2 NeGcon, 3 type 0xe): action bits, and for each action mapped
// to an analog axis (map byte >= 0x80) its value, pedal axes (>= 0xa0) folded to 0..0xff.
#ifdef NON_MATCHING
// Register allocation: the game keeps 1 and 0x7f in $t0 and $t5 across the loop (GCC
// doesn't hoist the 0x7f), which gives the pointers and counters other registers.
void RacePad_Update(RacePad *pad)
{
    int kind;
    int type;
    unsigned char *map;
    PortConfig *config;
    unsigned int held, pressed, released, repeat;
    unsigned int heldActs, pressedActs, releasedActs, repeatActs;
    int i, off;
    PadAnalogState *analog;
    unsigned short *values;

    PadButtons_Read(&pad->dev.buttons, &pad->raw);
    kind = 0;
    *(PadAnalogState *)pad->analogCopy = *(PadAnalogState *)&pad->dev.analogMask;
    type = pad->dev.type;
    config = pad->config;
    switch (type) {
    case 5:
    case 7:
        kind = 1;
        break;
    case 2:
        kind = 2;
        break;
    case 14:
        kind = 3;
        break;
    }
    map = config->buttonMaps[kind];
    heldActs = 0;
    pressedActs = 0;
    releasedActs = 0;
    repeatActs = 0;
    analog = (PadAnalogState *)pad->analogCopy;
    values = &pad->analogActs; // values[1 + i] is analogValue[i]
    held = pad->raw.held;
    pressed = pad->raw.pressed;
    released = pad->raw.released;
    repeat = pad->raw.repeat;
    pad->analogActs = 0;
    for (i = 0, off = 2; i < 11; off += 2, i++) {
        int b = map[i];

        if (b < 0x80) {
            unsigned int bit = 1 << b;
            unsigned int act = 1 << i;

            if (held & bit) {
                heldActs |= act;
            }
            if (pressed & bit) {
                pressedActs |= act;
            }
            if (released & bit) {
                releasedActs |= act;
            }
            if (repeat & bit) {
                repeatActs |= act;
            }
        } else {
            int axis = b & 0x1f;
            int full = 0x7f;
            int v = analog->axes[axis];
            int bit = 1 << axis;

            if (((PadAnalogState *)pad->analogCopy)->mask & bit) {
                pad->analogActs |= 1 << i;
                if (b >= 0xa0) {
                    if (b < 0xc0 || (b >= 0xe0 && v < 0x80)) {
                        v = full - v;
                    } else {
                        v = v - 0x80;
                    }
                    if (v < 0) {
                        v = 0;
                    }
                    if (v >= 0x80) {
                        v = 0x7f;
                    }
                    v = (v << 1) | (v >> 7);
                }
                *(unsigned short *)((char *)values + off) = v;
            }
        }
    }
    pad->actHeld = heldActs;
    pad->actPressed = pressedActs;
    pad->actReleased = releasedActs;
    pad->actRepeat = repeatActs;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepad", RacePad_Update);
#endif
}
