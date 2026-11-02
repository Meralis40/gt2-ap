// The GT mode screen loops of OVL4: GTMenu, the menu itself, and GTLoadingMenu,
// the dark screen shown after it (gtmenu.cpp), and GTBattleMenu, which picks the
// battle opponents (gtbattlemenu.cpp). They derive from the resident
// psxViewLoopDoubleBuffer (viewloop.h); the virtual functions are declared in
// their vtable order.
#ifndef GTMENU_H
#define GTMENU_H

#include "ovl4.h"

// GT mode main menu loop (0x864 bytes). Constructed on GTMode_Run's stack.
class GTMenu : public psxViewLoopDoubleBuffer {
public:
    signed char frameParity;    // toggles every Update; passed to the page draw
    signed char quit;           // Update returns 0 (leave the loop) once non-zero
    short blankFrames;          // draw.otHead is cleared (the next BeginDraw sends nothing) while > 0; 2 on Enter, 4 after a page change
    signed char hookActive;     // hook runs in BeginFrame and Update while it returns non-zero
    unsigned char pad75[3];
    unsigned char (*hook)(void *, int, int);
    void *hookArg;
    DrawLayer layers[3];        // priorities 1, 4, 3
    PadDevice pads[2];          // ports 0 and 1
    PadState padState[2];
    unsigned char pad198[4];
    short inputDelay;           // 8 on Enter; pads are ignored while > 0
    unsigned char pad19e[2];
    int pendingCarBaseId;       // pending MenuView_ShowCar call, queued by GTMenu_QueueCarPreview
    int pendingCarId;
    int pendingCarColor;        // colour index in the car colour list
    int pendingCarSlot;         // garage slot, -1 = not owned (dealer default setup)
    signed char pendingCarDelay;    // frames until the call; 0 = none
    unsigned char pendingCarHasWheel;
    unsigned char pad1b2[2];
    int pendingCarWheel;
    int pendingCarDirt;
    int pendingPage;            // MenuPage_Enter arguments
    int pendingPageArg;
    signed char pendingPageDelay;   // frames until MenuPage_Enter; 0 = none
    unsigned char pad1c5[3];
    int previewWheel;           // wheel shop: wheel code to show on the preview car (CarPreview_SetWheel)
    signed char previewDelay;   // frames until CarPreview_SetWheel
    signed char previewWheelColor;  // index into DAT_80091a70 (Wheel_GetVariant)
    short listMode;             // 0 map cursor, 1 garage list (g_GarageList), 2 used car list (g_UsedCarList), 3 car wash: waits for view.carWash to end
    MapCursor cursor;
    int canGoBack;              // square/triangle go to view.backPage only when bit 0 is set; cleared on shop confirmation pages
    unsigned char pad1fc[0x100];
    GTMenuView view;
    int beginFrameTime;         // Timer_SinceFrameStart timestamps, for profiling
    int updateStartTime;
    int drawStartTime;
    int drawEndTime;

    GTMenu();
    virtual ~GTMenu();
    virtual void Enter();
    virtual void Leave();
    virtual void BeginFrame();  // menudraw (still assembly)
    virtual int Update();
    virtual void Draw();
};

// A dark grey screen for 12 frames (0x84 bytes).
class GTLoadingMenu : public psxViewLoopDoubleBuffer {
public:
    DrawLayer layer;
    short frames;               // Update leaves once it goes below 0

    GTLoadingMenu();
    virtual ~GTLoadingMenu();
    virtual void Enter();
    virtual int Update();
    virtual void Draw();
};

// Picks the battle opponents (0x84 bytes): not interactive, a progress bar while
// every GT race data car is evaluated (one per frame), then the pool is picked.
class GTBattleMenu : public psxViewLoopDoubleBuffer {
public:
    short state;                // 0 evaluating the cars, 1 sorting, 2 picking the pool
    unsigned char pad72[2];
    DrawLayer layer;

    GTBattleMenu();
    virtual ~GTBattleMenu();
    virtual void Enter();
    virtual int Update();
    virtual void Draw();
};

extern "C" {
extern GTMenu *g_GTMenu;
void GTMenu_GoBack(GTMenu *menu);
void GTMenu_QueueCarPreview(GTMenu *menu, int delay, int baseCarId, int carId, int colorIdx, int garageSlot,
                            int hasWheel, int wheel, int dirt);
}

#endif
