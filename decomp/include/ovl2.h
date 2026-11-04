// Types of the OVL2 code (the arcade menus), exported from the Ghidra project
// (DataTypeWriter) and adapted for cc1plus as resident.h is. Only the types that
// the decompiled OVL2 files use. See docs/decomp.md.
#ifndef OVL2_H
#define OVL2_H

#include "resident.h"
#include "movie.h"

class ArcadeMenu;

// A page of the arcade menu, its callbacks called with the menu.
struct MenuPage {
    void (*enter)(ArcadeMenu *menu, int back); // back: 1 when returning from a child page
    int (*update)(ArcadeMenu *menu, int active); // active: 0 for the leaving page; returns 1 push, 2 back, 3 to 6 leave the menu
    void (*draw)(ArcadeMenu *menu, DrawLayer *layer);
    unsigned int titleColor; // RGB of the title bar
    char *title; // NULL = no title bar
};

struct AsyncRead {
    signed char busy;
    signed char state;
};

struct ArcTextItem {
    unsigned char charsPerStep;
    unsigned char echoDiv;
    short revealSteps;
    short zoomWidth;
    short blinkSteps;
    short fadeSteps;
    char spacing;
    char height;
    unsigned short flags;
    unsigned char brightness;
    unsigned char pad0f;
    short x;
    short y;
    unsigned char rgbFrom[4];
    unsigned char rgbTo[4];
    Font *font;
    char *text;
    short anim;
    short animEnd;
    short length;
    short pad2a;
};

struct ArcDialog {
    short x;
    short y;
    ArcTextItem message;
    ArcTextItem option0;
    ArcTextItem option1;
    char choice;
    unsigned char pad89;
    unsigned char boxHeight;
    unsigned char boxWidth;
    short anim;
    short flash;
    unsigned short flags;
    short pad92;
    unsigned char pad94[4];
};

struct ArcIconItem {
    short pulseSteps;
    short revealSteps;
    unsigned char flags;
    unsigned char pad05;
    short zoomWidth;
    short x;
    short y;
    unsigned char brightness;
    unsigned char pulseBrightness;
    unsigned short scale;
    void *sprite;
    short anim;
    short pad16;
};

// A sprite of a texture page (Sprite_AddScaled).
struct ArcSprite {
    unsigned char u;
    unsigned char v;
    unsigned short clut;
    short w;
    short h;
    unsigned short tpage;
};

// A menu list row: a text item (kind bit 0 clear) or an icon item (set); kind < 0 is disabled.
struct ArcMenuItem {
    signed char kind;
    char pad01;
    short height; // 0 or less: the list's itemHeight
    void *item; // ArcTextItem or ArcIconItem
    void *tmpl; // its template (TextItem_Init_OVL2, IconItem_Init)
    void *data; // its text or sprite
    int value; // MenuList_Update_OVL2's result when chosen
};

// The OVL2 menu list (0x20 bytes), another version of the resident MenuList, with ArcMenuItem rows.
struct ArcMenuList {
    signed char count;
    unsigned char flags; // 4: Up and Down wrap
    signed char visibleRows;
    char pad03;
    short cursorWidth;
    short itemHeight;
    short itemGap;
    short pad0a;
    ArcMenuItem *items;
    short x;
    short y;
    signed char selected;
    signed char revealIndex;
    short scrollAnim; // -8 to 8, toward 0
    short blinkTimer; // 0 to 60
    short revealTimer; // shows the next item at 0, every 6 frames
    short state; // 0 to 45 while open; -1 closed, -0x41 up to -1 closing
    short pad1e;
};

struct ArcGridPicker;

// A row and an item of a grid picker.
struct ArcGridPos {
    short row;
    short item;
};

// What GridPicker_Draw passes to the item callback, for the current and the previous item.
struct ArcGridPickerDraw {
    ArcGridPicker *picker;
    void *ot;
    short x;
    short y;
    short brightness; // 0x80 shown
    ArcGridPos pos;
    short pad12;
    void *arg; // ArcGridPicker.drawArg
};

// What GridPicker_Init copies.
struct ArcGridPickerTmpl {
    short x;
    short y;
    short w;
    short h;
    void (*draw)(ArcGridPickerDraw *ctx, int item);
    void *drawArg;
};

// The car and course selection pages' picker (0x2c bytes): rows of items, Left/Right change the
// item, Up/Down the row, the item sliding in; the callback draws each item.
struct ArcGridPicker {
    short x;
    short y;
    short w;
    short h;
    ArcGridPos cur;
    short rowItems[2]; // items per row
    short rowCount;
    ArcGridPos prev; // the item sliding out; row -1: none
    short anim; // open 0 to 12 then 12 to 56 (arrows), close -17 up to -1
    short slide; // -12 to 12, toward 0
    short slideVertical; // 1: the slide is between rows
    signed char moving; // the row has more than one item (Left/Right shown)
    unsigned char pad1d[3];
    void (*draw)(ArcGridPickerDraw *ctx, int item);
    void *drawArg;
    signed char *enabled; // per item; NULL: all
};

struct ArcColorChips {
    short selected;
    short count;
    unsigned short colors[64];
    short anim;
};

struct ArcStatBar {
    short anim;
    short pad02;
    ArcTextItem label;
    char *text;
};

struct ArcSpecTable {
    ArcTextItem weightLabel;
    ArcTextItem weightValue;
    char weightText[64];
    ArcTextItem powerLabel;
    ArcTextItem powerValue;
    char powerText[128];
    ArcTextItem torqueLabel;
    ArcTextItem torqueValue;
    char torqueText[128];
    short labelWidth;
    short valueWidth;
    short anim;
    short pad24e;
};

// The power graph of the car spec panel (PowerGraphView_*_OVL2), 0x44 bytes.
struct ArcPowerGraph {
    short state[0x14];
    void *powerCurve;
    void *torqueCurve;
    unsigned char pad30[0x14];
};

// The arcade car spec panel (g_ArcCarSpecPanel): two slots that alternate, so that the old car's
// widgets close while the new one's open.
struct ArcCarSpecPanel {
    ArcTextItem name[2];
    ArcIconItem icon[2];
    ArcPowerGraph graph[2];
    unsigned char levels[2][4]; // the 3 meters' levels
    ArcStatBar bars[2][3];
    ArcSpecTable table[2];
    ArcColorChips chips[2];
    char nameText[2][0x100];
    unsigned char powerCurve[2][0xa0];
    unsigned char torqueCurve[2][0xa0];
    signed char shown[2];
    signed char slot; // the current one
};

struct ArcRecordPanel {
    signed char active;
    char pad01;
    short anim;
    void *data;
};

struct ArcCoursePreviewMovie {
    signed char state;
    signed char cdClaimed;
    signed char stopRequested;
    signed char started;
    void *buffer;
    short movie;
    short pad0a;
};

struct ArcCarRating {
    unsigned char unk00;
    unsigned char dirtTires;
    unsigned char pwrClass;
    unsigned char unk03;
    short power;
    short unk06;
    short weight;
};

// The car ratings of a garage, in the order of its list.
struct ArcGarageInfo {
    short count;
    short pad02;
    Garage *garage;
    ArcCarRating ratings[100];
};

// A course's or a car's stat byte.
struct ArcBonusFlags {
    unsigned char bit0 : 1;
    unsigned char bit1 : 1;
    unsigned char bit2 : 1;
};

// A course of the arcade course lists (0x20 bytes, ends with an unk04 of 0).
struct ArcCourse {
    char *name;
    int unk04;
    int pad08[2];
    unsigned int hash; // Str_Hash of name
    short license;
    short pad16;
    int stat; // byte of PlayerStats, -1: none
    signed char unlocked;
    unsigned char pad1d[3];
};

// An entry of the course map file's header: entry 0's nameOffset is the count, and the
// name of map i is at entry i + 1.
struct ArcCourseMapEntry {
    int nameOffset;
    int pad04;
    int size;
    int sector;
};

// The course map of the course selection page (0x224 bytes).
struct ArcCourseMap {
    short tpage;
    short pad02;
    ArcCourseMapEntry *info;
    void *buf;
    void *buf2;
    short state; // 1 wait for the CD, 2/3 read, 4 wait for the hook, 11 show
    short pad12;
    unsigned int hash; // the requested map
    signed char ready; // drawn
    signed char loaded;
    signed char hookActive;
    signed char hookPending;
    signed char decoded;
    signed char unk1d;
    signed char cdClaimed;
    signed char stopRequested;
    unsigned int hashes[129];
};

// A bonus course of the arcade menu (g_ArcBonusCourses, ends with a code of 0).
struct ArcBonusCourse {
    int code;
    int pad04[2];
    int stat; // byte of PlayerStats, bit 1 or 2 set when won; negative: none
};

// What GuestLoad_Setup uses.
struct ArcGuestLoadConfig {
    void *buffer;
    void *font;
    short *slots;
};

// Arcade 3D car preview: streams a car model, its textures and wheel from the CD, inflates them in a menu hook and draws them with a ground disc. Two of them (ArcadeMenu.carViews).
struct ArcCarView {
    unsigned short clutY; // CarModelInst.unk06
    unsigned short tpage; // CarModelInst.tpage
    unsigned short firstFile; // CarAssetEntry.firstFile (model file; +1 textures)
    unsigned char pad6[2];
    unsigned int carId;
    char vramSlot; // texture/CLUT slot (0x18 + view index)
    char colorIdx;
    unsigned char loaded; // model, textures and wheel ready: drawable
    unsigned char perfDone; // performance computed (CarParams_GetPerformance) for this load
    unsigned char loadState; // ArcCarView_Update state: 1 start, 2/3 model read, 4/5 texture read, 6/7 wheel read, 8 wait for decode
    unsigned char hasWheel; // car has a non-default wheel
    unsigned char pad12[2];
    unsigned int wheelFile; // Wheel_GetFileId
    unsigned char hookActive; // decode hook installed (ArcCarView_DecodeHook)
    unsigned char hookPending; // install the decode hook when the menu hook is free
    unsigned char decoded; // decode finished
    unsigned char decodeState; // ArcCarView_DecodeHook state 0-6
    unsigned char cdBusy; // owns the menu CD read
    unsigned char cancel; // stop the read on the next update
    unsigned char modelRead;
    unsigned char texRead;
    unsigned char wheelRead;
    signed char colorDirty; // re-apply the colour on the next BeginFrame
    unsigned char pad22[2];
    unsigned int modelBytes; // bytes read so far
    unsigned int modelSize;
    unsigned int texBytes;
    unsigned int texSize;
    unsigned int wheelBytes;
    unsigned int wheelSize;
    unsigned int decodePos; // output bytes inflated so far
    void *model; // inflated CarModelHeader
    void *tex; // inflated textures: colour count (u16), palette blocks of 0x240 from +0x20, image at +0x43a0
    void *modelGz; // compressed model read from the CD
    void *texGz; // compressed textures read from the CD
    void *wheelBuf; // wheel model / TIM
    GzipStream *gzip; // shared inflate state
    GroundDisc disc; // ground shadow disc
    CarSetup setup;
    CarParams params;
    EnginePerf perf; // maxPower, powerRpm, maxTorque, torqueRpm used by the spec panel
    int hasPerf; // perf computed from params (1) or copied from the garage Car (0)
    unsigned short wheelColor; // index into g_WheelOffsetTable; 2 by default
    unsigned char pad446[2];
};

// Car maker logo with two double-buffered VRAM slots (ArcadeMenu.logos, two of them).
struct ArcLogo {
    short vramPage; // texture page of the two logo slots (x (p&0xf)*64, y (p&0x10)*16); slot n at +n*0x80 lines
    unsigned char pad2[2];
    unsigned char *archive; // arc_carlogo archive in memory (offset table at +4)
    signed char cur; // slot used by the last request (alternates 0/1)
    unsigned char pad9;
    short w[2]; // logo width per slot (pixels)
    short h[2]; // logo height per slot
    unsigned char pad12[2];
    unsigned int carId[2]; // car whose logo is in each slot
    signed char ready[2]; // slot uploaded and drawable
    short archIdx[2]; // archive image index; -1: read from the CD (logoFile+1) into timBuf
    signed char pending[2]; // upload on the next BeginFrame
    unsigned char pad24;
    signed char loadState; // CD read state: 1 wait for the drive, 2 start, 3 poll
    signed char cdBusy; // owns the menu CD read
    signed char cancel; // stop the read on the next update
    void *timBuf; // buffer for a logo read from the CD
};

// The course preview movie player (0x5c bytes): OVL5's MoviePlayer, which OVL2's copy
// of the movie code plays (Movie_*_OVL2, src/sim/ovl2/copies.h), then the state of
// the course selection page's player (StrPlayer_Poll).
struct ArcStrPlayer : MoviePlayer {
    void *work;
    short movie; // index in the stream table
    unsigned char started;
    unsigned char frameReady;
    unsigned char decoding;
    unsigned char frameDone;
    unsigned char pad5a[2];
};

struct ResultCarView {
    int unk00;
    Camera cam;
    int pos[3];
    short rotX;
    short rotY;
    short rotZ;
    short pad_b2;
    int unk_b4;
    int unk_b8;
    int unk_bc;
    short viewport[4];
    short frustum[6];
    unsigned char drawDisc;
    unsigned char discSemi;
    unsigned char pad_d6[2];
    unsigned int discRgb;
};

// Arcade mode menu loop (0x4b8 bytes), derived from the resident psxViewLoopDoubleBuffer
// (viewloop.h), like OVL4's GTMenu; arcademenu.cpp emits its vtable. Constructed on
// ArcadeMode_Run's stack.
class ArcadeMenu : public psxViewLoopDoubleBuffer {
public:
    signed char hookActive; // hook runs each frame (BeginFrame step 0, Update step 1) while it returns non-zero
    unsigned char pad71[3];
    unsigned char (*hook)(void *, int, int);
    void *hookArg;
    DrawLayer layers[6]; // priorities 0,5,1,2,3,4
    PadDevice pads[2];
    PadState padState[2];
    TextCtx *text; // shared text context (0x800efd38)
    MenuPage *activePage; // page being updated or drawn
    MenuPage *leavingPage; // drawn fading out during a transition
    MenuPage *pageStack[16];
    signed char pageDepth;
    unsigned char pad211;
    short transitionTimer; // 16 down to 0
    signed char transitionBack;
    unsigned char pad215[3];
    unsigned int drawStamp; // g_GfxIrq.gpuDmaDoneTime
    int frameCount;
    int pageFrames; // reset by Enter, counted by Update
    AsyncRead cdRead; // menu CD read, shared by the car views, logos and logo archive
    signed char cdBusy; // a client owns cdRead
    unsigned char pad227;
    ArcCarView *carViews[2];
    unsigned char pad230[8];
    int logoArcReady; // CarLogoArc_Update result; forces the logo uploads
    ArcLogo logos[2];
    ArcCourseMap courseMap; // set up by CourseMap_Init (ArcadeMode_Run's frame gives its size)

    ArcadeMenu();
    virtual ~ArcadeMenu();
    virtual void Enter();
    virtual void Leave();
    virtual void BeginFrame();  // menupages.cpp
    virtual int Update();
    virtual void Draw();
};

// What ArcadeRace_Setup leaves for the loading screen (g_ArcadeLoadInfo, 0x154 bytes).
struct ArcadeLoadInfo {
    unsigned int seeds[2];      // per car view: bits 0-1 the camera preset, 0-11 the yaw, 4-12 the roll
    short unk08;                // picks the race type 6 title
    short raceType;             // 0, 4 (the car list) and 6 (a TIM) change the screen
    unsigned short carCount;
    char name[0x42];
    struct {
        unsigned int carId;
        unsigned short rgb;     // CarInfoA_GetColorRgb
        unsigned short pad;
    } cars[32];
    void *tim;                  // race type 6: uploaded to 0x1c0, 0x100
};

// The arcade race loading screen (0x1ac bytes), derived from the resident
// psxViewLoopDoubleBuffer (viewloop.h); arcaderacingmenu.cpp emits its vtable.
// Constructed on ArcadeMode_Run's stack, after ArcadeRace_Setup. Shows the one or
// two cars of ArcadeLoadInfo for 16 frames.
class ArcadeRacingMenu : public psxViewLoopDoubleBuffer {
public:
    unsigned char pad70[0xe8];
    DrawLayer layers[4];        // the two car views, the text and the background
    short frames;               // 16 down to 0; leaves below 0
    short textDelay;            // Update sets it to 3, Draw counts it down
    int presets[2];             // the car views' camera presets (ArcadeRacingMenu_SetCamera)
    short yaws[2];
    short rolls[2];

    ArcadeRacingMenu();
    virtual ~ArcadeRacingMenu();
    virtual void Enter();
    virtual void Leave();
    virtual int Update();
    virtual void Draw();
};


#endif
