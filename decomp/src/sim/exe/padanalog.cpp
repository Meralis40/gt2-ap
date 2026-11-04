// Resident code 80085858-80085974 (padanalog): an ASCII-to-Shift-JIS helper and the
// handler of the analog pads.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

extern unsigned char g_PadButtonMap[16][2];

// Shift-JIS code of a JIS X 0208 code (row << 8 | cell).
int Ascii_ToSjis(int c)
{
    int cell = c & 0xff;
    int code = ((c & 0xff00) + 0x2100) >> 1 ^ 0xa000;

    if (!(c & 0x100)) {
        code -= 2;
    } else {
        code = code + 0x20 - (cell < 0x60);
    }
    return code + cell;
}

// 0..0x5a -> 0..0x7f, 0x5b..0xa4 -> 0x80 (dead zone), 0xa5..0xff -> 0x80..0xff.
int Pad_ScaleStick(int value)
{
    int range = 0x5b;
    int base = 0;

    if (value >= range) {
        if (value < 0xa5) {
            return 0x80;
        }
        base = 0x80;
        value -= 0xa5;
    }
    return (value << 7) / range + base;
}

// PadHandler types 5 and 7. Buttons from g_PadButtonMap; analog = the sticks through
// Pad_ScaleStick.
void Pad_UpdateAnalog(PadDevice *dev, int raw, unsigned short *analog, int sameType)
{
    PadButtonState *state = &dev->buttons;

    PadButtons_Update(state, Pad_RemapButtons(raw, g_PadButtonMap[0], 16));
    dev->analogMask |= 0xf;
    dev->analog[0] = Pad_ScaleStick(analog[0]);
    dev->analog[1] = Pad_ScaleStick(analog[1]);
    dev->analog[2] = Pad_ScaleStick(analog[2]);
    dev->analog[3] = Pad_ScaleStick(analog[3]);
}

}
