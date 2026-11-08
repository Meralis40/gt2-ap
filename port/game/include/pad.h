// The controller driver's state (g_PadSystem).
#ifndef PAD_H
#define PAD_H

#include "gptr.h"

#include "resident.h"

// Raw buffer of a controller port (PsyQ PadInitDirect, 34 bytes).
struct PadRawBuffer {
    unsigned char status;       // 0 when the read succeeded
    unsigned char typeLen;
    unsigned char buttons[2];   // active low
    unsigned char analog[4];
    unsigned char extra[26];
};

// The actuator state of a port (g_PadSystem.ports).
struct PadPort {
    unsigned char align;    // the actuators still need PadSetActAlign
    unsigned char mode;     // 1: Pad_UpdateVibration sends 0x40 and one level
    unsigned char act[6];   // the buffer given to PadSetAct
};

struct PadSystem {
    VSyncCallback vsync;
    game_pointer<struct PadDevice> devices[2];
    PadPort ports[2];
    volatile PadRawBuffer raw[2];   // written by the pad interrupt
};

extern "C" {
extern PadSystem (&g_PadSystem);
}

// exe/padbuttons.cpp

// An analog axis mapped to a button (Pad_AnalogToButtons).
struct PadAnalogMap {
    unsigned short threshold; // the button is down from this value up
    unsigned char axis;
    unsigned char button;     // its bit
};

// The repeat timing of PadButtonState (g_PadRepeatStart and g_PadRepeatEnd).
struct PadRepeat {
    signed char start;
    signed char end;
};

#endif
