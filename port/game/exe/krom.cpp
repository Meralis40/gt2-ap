// Resident code 800804f8-80080820 (krom): finds a JIS X 0208 character's glyph in the BIOS
// Kanji ROM, skipping the cells the ROM leaves out of each row.
#include "common.h"

extern "C" {
extern game_pointer<unsigned short> (&g_KanjiSpaceAddr);
extern short (&D_80095970)[];          // the glyph index of each row's first cell
extern signed char (&D_8009598C)[];    // row 8's first cells, -1 for none
extern signed char (&D_8009599C)[];    // row 10's first cells, -1 for none

// Caches the ROM address of the full-width space (SJIS 0x8140).
void Font_Init()
{
    unsigned int *addr = Krom2RawAdd(0xffff8140);

    g_KanjiSpaceAddr = (unsigned short *)addr;
}

// The ROM address of a JIS X 0208 character's 15 rows of pixels, the full-width space's
// when the ROM doesn't have it. Not PsyQ's Krom2RawAdd, which takes Shift-JIS.
// Needs its jump table in .rodata, and the game adjusts the index in delay slots and shares
// its tails (not tried).
unsigned short *KanjiFont_Krom2RawAdd(unsigned int code)
{
    int c = code - 0x2121;
    int cell = c & 0xff;
    int row = (c >> 8) & 0xff;
    int index = cell;
    int start = -0x376;
    unsigned short *rom = g_KanjiSpaceAddr;
    unsigned short *space = rom;

    if (cell >= 0x5e && c != 0xc5e) return space;
    if (row >= 0xf) {
        if (row >= 0x2e && (row != 0x2e || cell >= 0x33)) return space;
    } else {
        start = D_80095970[row];
        if (row >= 8) rom -= 0x1000;
        switch (row) {
        case 1:
            if (cell < 0xe) break;
            if (cell < 0x19) return space;
            index = cell - 0xb;
            if (cell < 0x21) break;
            if (cell < 0x29) return space;
            index = cell - 0x13;
            if (cell < 0x30) break;
            if (cell < 0x3b) return space;
            index = cell - 0x1e;
            if (cell < 0x4a) break;
            if (cell < 0x51) return space;
            index = cell - 0x25;
            if (cell < 0x59) break;
            if (cell < 0x5d) return space;
            index = cell - 0x29;
            break;
        case 2:
            if (cell < 0xf) return space;
            if (cell < 0x19) break;
            if (cell < 0x20) return space;
            index = cell - 7;
            if (cell < 0x3a) break;
            if (cell < 0x40) return space;
            index = cell - 0xd;
            if (cell >= 0x5a) return space;
            break;
        case 3:
            if (cell >= 0x53) return space;
            break;
        case 4:
            if (cell >= 0x56) return space;
            break;
        case 5:
            if (cell < 0x18) break;
            if (cell < 0x20) return space;
            index = cell - 8;
            if (cell >= 0x38) return space;
            break;
        case 6:
            if (cell < 0x21) break;
            if (cell < 0x30) return space;
            index = cell - 0xf;
            if (cell >= 0x51) return space;
            break;
        case 7:
            if (cell >= 0x20) return space;
            break;
        case 8:
            if (cell < 0x10) {
                index = D_8009598C[cell];
                if (index == -1) return space;
                break;
            }
            index = cell - 7;
            if (cell >= 0x1c) return space;
            break;
        case 9:
            if (cell <= 0) return space;
            if (cell < 0xe) break;
            if (cell < 0x11) return space;
            index = cell - 3;
            if (cell >= 0x1e) return space;
            break;
        case 10:
            if (cell < 0xe) {
                index = D_8009599C[cell];
                if (index == -1) return space;
                break;
            }
            index = cell - 5;
            if (cell < 0x10) break;
            if (cell < 0x20) return space;
            index = cell - 0x15;
            if (cell >= 0x30) return space;
            break;
        case 11:
            if (cell < 0x18) break;
            if (cell < 0x19) return space;
            index = cell - 1;
            if (cell >= 0x57) return space;
            break;
        case 12:
            if (cell < 0x1b) break;
            if (cell < 0x1c) return space;
            index = cell - 1;
            if (cell < 0x23) break;
            if (cell < 0x24) return space;
            index = cell - 2;
            if (cell < 0x3f) break;
            index = cell - 1;
            if (cell < 0x56) break;
            if (cell == 0x7f) return rom + 0xcf3;
            return space;
        case 13:
        case 14:
            return space;
        }
    }
    return rom + (row * 94 + index + start) * 15;
}

}
