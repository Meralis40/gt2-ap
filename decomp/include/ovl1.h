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

// A root or list page: BaseMenuPage, then its delay and back flag.
struct RootMenuPage {
    BaseMenuPage page;
    int delay;
    int back;
};

// Main menu loop (0x220 bytes), derived from the resident psxViewLoopDoubleBuffer
// (viewloop.h), like OVL2's ArcadeMenu. Only its fields so far.
class BaseMenu : public psxViewLoopDoubleBuffer {
public:
    signed char hookActive; // hook runs each frame while it returns non-zero
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
    signed char transitionBack;
    short transitionTimer; // 16 down to 0
    unsigned int drawStamp; // copy of DAT_801f0698
    int frameCount;
    unsigned char pad218[4];
    signed char result; // non-zero when a replay was chosen: start it
    unsigned char pad21d[3];

    BaseMenu();
    virtual ~BaseMenu();
    virtual void Enter();
    virtual void Leave();
    virtual void BeginFrame();
    virtual int Update();
    virtual void Draw();
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
    signed char count;
    signed char selected;
    short anim;
    short slide;
    signed char previous;
    signed char hadInput;
    signed char active;
    signed char skipInput;
    unsigned char pad12[2];
    void *callback;
    void *userData;
};

// What TabSelector_Update and TabSelector_Draw pass to the callback.
struct TabEvent {
    TabSelector *tabs;
    PadState *pad;
    uint *ot;
    short x;
    short y;
    short brightness;
    short pad12[3];
};

// ovl1/dataxfer.cpp

// What DataXfer_Setup takes.
struct DataXferConfig {
    unsigned int heap; // the work area (g_OVL1_EndFontInfo)
    KanjiFont *font;
    short *cardSlots;
    Font *fontSmall;
    Font *fontMedium;
    Font *fontLarge;
    short textPri;
};

// ovl1/menusprite.cpp

// A menu row image: a sprite that opens with a fade and closes as two bands.
struct MenuSprite {
    unsigned char flags; // 1 glow when selected, 2 semi-transparent, 4 zoom trail, 0x18 blend mode, 0x60 anchor
    unsigned char unk01;
    short zoomWidth; // distance of the trail copies at the start of the open animation
    short x;
    short y;
    unsigned char maxBrightness;
    unsigned char pad09[3];
    struct SpriteDesc *sprite; // ovl4.h
    short anim; // 0 to 12 opening, -1 hidden, below -1 closing
    short brightness;
};

// MenuSprite_Init's style.
struct MenuSpriteStyle {
    unsigned char flags;
    unsigned char brightness;
    short zoomWidth;
};

#endif
