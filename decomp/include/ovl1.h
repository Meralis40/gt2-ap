// Types of the OVL1 code (the main menu), exported from the Ghidra project
// (DataTypeWriter) and adapted for cc1plus as resident.h is. Only the types that
// the decompiled OVL1 files use. See docs/decomp.md.
#ifndef OVL1_H
#define OVL1_H

#include "resident.h"
#include "viewloop.h"

// A page of the main menu. Unlike OVL2's MenuPage, its callbacks only take the
// back flag and the layer; the pages find the menu in g_CurrentMenu. Ghidra's
// MenuPage; a RootMenuPage adds a delay and the back flag after it.
typedef void MenuPageEnterFn(int back);
typedef int MenuPageUpdateFn(int arg);
typedef void MenuPageDrawFn(DrawLayer *layer);

struct BaseMenuPage {
    MenuPageEnterFn *enter; // 1 when returning from a child page
    MenuPageUpdateFn *update; // returns 1 push, 2 back, 3/4 leave the menu
    MenuPageDrawFn *draw;
    unsigned int titleColor; // RGB of the title bar
    char *title; // NULL = no title bar
};

// Main menu loop (0x220 bytes), derived from the resident psxViewLoopDoubleBuffer
// (viewloop.h), like OVL2's ArcadeMenu. Only its fields so far.
class BaseMenu : public psxViewLoopDoubleBuffer {
public:
    unsigned char hookActive; // hook runs each frame while it returns non-zero
    unsigned char pad71[3];
    unsigned char (*hook)(void *, int, int);
    void *hookArg;
    DrawLayer layers[6]; // priorities 0,5,1,2,3,4
    PadDevice pads[2]; // tentative: ports 0 and 1
    PadState padState[2]; // copied from pads each Update
    BaseMenuPage *activePage;
    BaseMenuPage *leavingPage; // drawn fading out during a back transition
    BaseMenuPage *pageStack[16];
    signed char pageDepth;
    unsigned char transitionBack;
    short transitionTimer; // 16 down to 0
    unsigned int drawStamp; // copy of DAT_801f0698
    int frameCount;
    unsigned char pad218[4];
    unsigned char result; // non-zero after the arcade menu: start a race
    unsigned char pad21d[3];
};

struct KeyConfig {
    int padType;
    unsigned char analogMap[4];
    unsigned char buttons[11];
    unsigned char pad13[5];
};

struct KeyConfigPage {
    short row[2];
    short blink;
    short delay;
};

struct SteerCalib {
    int center;
    int max;
    int margin;
    int upper;
    int lower;
};

struct PedalCalib {
    int max;
    int margin;
};

struct AnalogSettings {
    short step;
    short port;
    short mode;
    short pad06;
    SteerCalib steer;
    PedalCalib accel;
    PedalCalib brake;
    unsigned char icons[3][28];
};

struct TabSelector {
    short x;
    short y;
    short width;
    short unk06;
    char count;
    char selected;
    short anim;
    short slide;
    char previous;
    unsigned char hadInput;
    unsigned char active;
    unsigned char skipInput;
    unsigned char pad12[2];
    void *callback;
    void *userData;
};

#endif
