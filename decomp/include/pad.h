// The controller driver's state (g_PadSystem). Include resident.h or viewloop.h
// first, for VSyncCallback.
#ifndef PAD_H
#define PAD_H

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
    struct PadDevice *devices[2];
    PadPort ports[2];
    volatile PadRawBuffer raw[2];   // written by the pad interrupt
};

extern "C" {
extern PadSystem g_PadSystem;
}

#endif
