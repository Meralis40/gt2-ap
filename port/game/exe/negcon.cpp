// Resident code 800831bc-800833a0 (negcon): the neGcon's handler: raw axes scaled by the
// calibration in g_NeGconCalib.
#include "common.h"

extern "C" {

extern NeGconCalib (&g_NeGconCalib)[2];
extern PadAnalogMap (&g_NeGconButtonMap)[];
extern unsigned char (&g_PadButtonMap)[16][2];

// The twist: 0 to 0x80 below the centre, 0x80 in the centre's dead zone, 0x80 to 0xff above it.
// Registers: the game keeps base in $t0 and the upper end in $t1; here they swap.
int NeGcon_ScaleTwist(int port, int axis, unsigned int value)
{
    NeGconCalib *calib = &g_NeGconCalib[port];
    int v = value & 0xff;
    int base;
    int min = calib->twist[0];
    int low = calib->twist[1];
    int high = calib->twist[2];
    int max = calib->twist[3];
    int d;
    int range;

    if (v < min) {
        return 0;
    }
    base = 0;
    if (v < low) {
        d = v - min;
        range = low - min;
    } else {
        if (v < high) {
            return 0x80;
        }
        base = 0x80;
        if (v >= max) {
            return 0xff;
        }
        d = v - high;
        range = max - high;
    }
    return (d << 7) / range + base;
}

// Button I, II or L (axis 1 to 3): 0 to 0xff between the calibrated ends.
int NeGcon_ScaleAxis(int port, int axis, unsigned int value)
{
    NeGconCalib *calib = &g_NeGconCalib[port];
    int i = axis - 1;
    int v = value & 0xff;
    unsigned char min = calib->axes[i].min;
    unsigned char max = calib->axes[i].max;

    if (v < min) {
        return 0;
    }
    if (v < max) {
        return ((v - min) << 8) / (max - min);
    }
    return 0xff;
}

// PadHandler type 2. Buttons from g_PadButtonMap plus I, II and L over their thresholds;
// analog = the twist and I, II and L scaled by the calibration.
void Pad_UpdateNeGcon(PadDevice *dev, int raw, unsigned short *analog, int sameType)
{
    int port = dev->port;
    PadButtonState *state = &dev->buttons;
    PadButton buttons;

    buttons = Pad_RemapButtons(raw, g_PadButtonMap[0], 16);
    PadButtons_Update(state, buttons | Pad_AnalogToButtons(analog, g_NeGconButtonMap, 3));
    dev->analogMask |= 0xf;
    dev->analog[0] = NeGcon_ScaleTwist(port, 0, analog[0]);
    dev->analog[1] = NeGcon_ScaleAxis(port, 1, analog[1]);
    dev->analog[2] = NeGcon_ScaleAxis(port, 2, analog[2]);
    dev->analog[3] = NeGcon_ScaleAxis(port, 3, analog[3]);
}

}
