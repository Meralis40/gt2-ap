// Screen loops of the resident code (docs/decomp.md), and the display state
// they share with the rest of the game.
#ifndef VIEWLOOP_H
#define VIEWLOOP_H

// PsyQ libgpu rectangle.
struct RECT {
    short x, y, w, h;
};

// Node of a priority-sorted list of vblank callbacks (VSyncList_Add).
struct VSyncCallback {
    VSyncCallback *prev;
    VSyncCallback *next;
    void (*func)();
    unsigned int priority;

    VSyncCallback(void (*f)(), unsigned int prio) : func(f), priority(prio) {}
};

// Ordering table and draw layers of a view (ViewDrawContext_*, 0x20 bytes).
struct ViewDrawContext {
    struct DrawLayer *layers;
    void *otBase;
    void *otHead;               // frame to send at the next Submit, 0 for none
    void *otStart;
    void *otEnd;
    unsigned short otSize;
    unsigned short otHalf;
    int unk18;
    void *tailPackets;
};

// Double-buffered primitive pool (PacketBuffer_Swap).
struct PacketBuffer {
    void *base;
    unsigned int size;
    unsigned int half;
};

// Game-side interrupt and display state (Irq_Install).
struct GfxIrq {
    unsigned int vblankTotal;
    int vblanksSinceFlip;       // reset by Irq_WaitVSyncs and Gpu_WaitField
    VSyncCallback *vsyncCallbacks;
    void (*gpuDmaHook)();
};

// The resident screen loops. ScreenViewLoop (screenviewloop.cpp) holds only the
// vtable pointer: Execute calls Run, which runs Enter, then Flip after each
// Step until Step returns 0, then Leave. psxViewLoop (viewloop.cpp, 0x6c
// bytes) draws and shows a frame per Step, double or triple buffered, and
// swaps the display from the vblank callback. The virtual functions are
// declared in their vtable order.
class ScreenViewLoop {
public:
    ScreenViewLoop();
    virtual ~ScreenViewLoop();
    virtual void Run();
    virtual void Enter();
    virtual int Step();
    virtual void Flip();
    virtual void Leave();
    int Execute();
};

class psxViewLoop : public ScreenViewLoop {
public:
    psxViewLoop *prevLoop;      // loop below this one (g_CurrentViewLoop)
    VSyncCallback vsync;        // calls UpdateDisplay
    int vsyncInterval;          // vblanks per frame
    unsigned char videoModeDelay;   // frames until ApplyVideoMode
    unsigned char bufferIndex;
    unsigned char bufferCount;
    unsigned char videoModePending;
    unsigned char swapPending;  // vblanks until SwapDisplay, 0 for none
    unsigned char noDrawSync;
    unsigned char interlaced;   // 480 lines: Flip waits for the field
    unsigned char videoMode;    // GP1(08) display mode
    unsigned char videoArg;     // EnableDisplay argument
    RECT drawArea;
    RECT dispArea;
    ViewDrawContext draw;
    PacketBuffer packets;
    int frameStartTime;         // Timer_SinceFrameStart at Flip
    int frameEndTime;

    psxViewLoop();
    virtual ~psxViewLoop();
    virtual void Enter();
    virtual int Step();
    virtual void Flip();
    virtual void Leave();
    virtual void BeginFrame();
    virtual int Update();
    virtual void BeginDraw();
    virtual void Draw();
    virtual void EndDraw();
    virtual void DrawFrame();
    virtual void SwapDisplay();
    virtual void ApplyVideoMode();
    virtual void UpdateDisplay();
    void SetVideoMode(unsigned int mode);
    static void VSyncHandler();
};

// A psxViewLoop drawing into two buffers one above the other in VRAM
// (viewloopdb.cpp, 0x70 bytes): BeginDraw offsets the draw area by the
// current buffer's y, SwapDisplay shows it.
class psxViewLoopDoubleBuffer : public psxViewLoop {
public:
    short bufferY[2];           // VRAM y of each buffer: 0 and 240, or 0 and 0 in 480-line modes

    psxViewLoopDoubleBuffer();
    virtual ~psxViewLoopDoubleBuffer();
    virtual void Leave();
    virtual void BeginDraw();
    virtual void SwapDisplay();
    void SetVideoMode(unsigned int mode);
};

extern "C" {
extern GfxIrq g_GfxIrq;
extern unsigned char g_LastBufferIndex;
extern psxViewLoop *g_CurrentViewLoop;
}

#endif
