// OVL2 800136f0-80013844 (carlogoarc): Car logo archive.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern unsigned char g_OVL2_FileBuf[];
extern unsigned int g_VolLba;


// state = 0.
void CarLogoArc_Init(short *state)
{
    *state = 0;
}

// Reads the car logo archive (volume file 12) into g_OVL2_FileBuf, when the menu's CD
// read is free. 0: waiting for it; 1 otherwise.
int CarLogoArc_Update(short *state, ArcadeMenu *menu)
{
    AsyncRead *read = &menu->cdRead;
    unsigned char *buf = g_OVL2_FileBuf;
    int sector = g_VolLba + VolId_GetSector((VolBootId)0xc);
    int size = VolId_GetSize((VolBootId)0xc);
    int result = 1;
    int status;

    switch (*state) {
    case 0:
        if (menu->cdBusy != 0) {
            result = 0;
            break;
        }
        menu->cdBusy = 1;
        (*state)++;
    case 1:
        if (AsyncRead_Start_OVL2(read, buf, sector, size) != 0) {
            (*state)++;
        }
        break;
    case 2:
        switch (AsyncRead_Poll_OVL2(read, &status)) {
        case 0:
            *state = 1;
            break;
        case 2:
            menu->cdBusy = 0;
            (*state)++;
            break;
        }
        break;
    }
    return result;
}
}
