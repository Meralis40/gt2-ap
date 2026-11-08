// Screen loops of the resident code (docs/decomp.md), and the display state
// they share with the rest of the game.
#ifndef VIEWLOOP_H
#define VIEWLOOP_H

#include "gptr.h"
#include "vtable.h"

// PsyQ libgpu rectangle.
struct RECT {
    short x, y, w, h;
};

// Node of a priority-sorted list of vblank callbacks (VSyncList_Add).
struct VSyncCallback {
    game_pointer<VSyncCallback> prev;
    game_pointer<VSyncCallback> next;
    game_function<void()> func;
    unsigned int priority;

    VSyncCallback(void (*f)(), unsigned int prio) : func(f), priority(prio) {}
};

// Ordering table and draw layers of a view (ViewDrawContext_*, 0x20 bytes).
struct ViewDrawContext {
    game_pointer<struct DrawLayer> layers;
    game_pointer<void> otBase;
    game_pointer<void> otHead;               // frame to send at the next Submit, 0 for none
    game_pointer<void> otStart;
    game_pointer<void> otEnd;
    unsigned short otSize;
    unsigned short otHalf;
    unsigned int drawMode;      // texture page bits of the E1 draw-mode packet ViewDrawContext_End appends
    game_pointer<void> tailPackets;          // terminator packets EndDraw appends (draw mode, cache flush)
};

// Double-buffered primitive pool (PacketBuffer_Swap); g_PacketPtr allocates
// from the current half.
struct PacketBuffer {
    game_pointer<void> base;
    unsigned int size;          // words, both halves
    unsigned int half;          // 0 or size/2
};

// Game-side interrupt and display state, cleared by Irq_Install.
struct GfxIrqState {
    unsigned int vblankTotal;   // every vblank; Irq_WaitVSyncs returns it
    int vblanksSinceFlip;       // reset by Irq_WaitVSyncs and Gpu_WaitField
    game_pointer<VSyncCallback> vsyncCallbacks;  // priority-sorted list run by Irq_VSyncHandler
    game_function<void()> gpuDmaHook;       // called after each GPU DMA completion; overlays chain their own
    unsigned int displayMode;   // last GP1(08) mode written by Gpu_SetDisplayMode
    int gpuDmaCount;
    int gpuDmaDoneTime;         // Timer_SinceFrameStart at the last GPU DMA completion
    int gpuDmaDoneHblanks;
    unsigned int gpustatAtWait; // GPUSTAT when Irq_WaitVSyncs started; Gpu_WaitField waits for its bit 31 to flip
    unsigned char imageTransferBusy;    // 1 during Gpu_TransferImage
    unsigned char imageDmaPending;      // 1 from Gpu_TransferImage's DMA start until Irq_GpuDmaHandler
    unsigned char pad26[2];
};

// The resident screen loops. ScreenViewLoop (screenviewloop.cpp) holds only the
// vtable pointer: Execute calls Run, which runs Enter, then Flip after each
// Step until Step returns 0, then Leave. psxViewLoop (viewloop.cpp, 0x6c
// bytes) draws and shows a frame per Step, double or triple buffered, and
// swaps the display from the vblank callback. The virtual functions are
// declared in their vtable order.
class ScreenViewLoop {
public:
    game_vptr vptr;  // the console's vtable pointer: its vtable's address
    ScreenViewLoop();
    ~ScreenViewLoop();
    void Run();
    void Enter();
    int Step();
    void Flip();
    void Leave();
    int Execute();
    // Calls through the vtable word (rt/vtable.h).
    void v_Run() { return vptr.slot<void(ScreenViewLoop *)>(0)(this); }
    void v_Enter() { return vptr.slot<void(ScreenViewLoop *)>(1)(this); }
    int v_Step() { return vptr.slot<int(ScreenViewLoop *)>(2)(this); }
    void v_Flip() { return vptr.slot<void(ScreenViewLoop *)>(3)(this); }
    void v_Leave() { return vptr.slot<void(ScreenViewLoop *)>(4)(this); }
};

class psxViewLoop : public ScreenViewLoop {
public:
    game_pointer<psxViewLoop> prevLoop;      // loop below this one (g_CurrentViewLoop)
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
    ~psxViewLoop();
    void Enter();
    int Step();
    void Flip();
    void Leave();
    void BeginFrame();
    int Update();
    void BeginDraw();
    void Draw();
    void EndDraw();
    void DrawFrame();
    void SwapDisplay();
    void ApplyVideoMode();
    void UpdateDisplay();
    void SetVideoMode(unsigned int mode);
    static void VSyncHandler();
    // Calls through the vtable word (rt/vtable.h).
    void v_BeginFrame() { return vptr.slot<void(ScreenViewLoop *)>(5)(this); }
    int v_Update() { return vptr.slot<int(ScreenViewLoop *)>(6)(this); }
    void v_BeginDraw() { return vptr.slot<void(ScreenViewLoop *)>(7)(this); }
    void v_Draw() { return vptr.slot<void(ScreenViewLoop *)>(8)(this); }
    void v_EndDraw() { return vptr.slot<void(ScreenViewLoop *)>(9)(this); }
    void v_DrawFrame() { return vptr.slot<void(ScreenViewLoop *)>(10)(this); }
    void v_SwapDisplay() { return vptr.slot<void(ScreenViewLoop *)>(11)(this); }
    void v_ApplyVideoMode() { return vptr.slot<void(ScreenViewLoop *)>(12)(this); }
    void v_UpdateDisplay() { return vptr.slot<void(ScreenViewLoop *)>(13)(this); }
};

// A psxViewLoop drawing into two buffers one above the other in VRAM
// (viewloopdb.cpp, 0x70 bytes): BeginDraw offsets the draw area by the
// current buffer's y, SwapDisplay shows it.
class psxViewLoopDoubleBuffer : public psxViewLoop {
public:
    short bufferY[2];           // VRAM y of each buffer: 0 and 240, or 0 and 0 in 480-line modes

    psxViewLoopDoubleBuffer();
    ~psxViewLoopDoubleBuffer();
    void Leave();
    void BeginDraw();
    void SwapDisplay();
    void SetVideoMode(unsigned int mode);
};

extern "C" {
extern GfxIrqState (&g_GfxIrq);
extern unsigned char (&g_LastBufferIndex);
extern game_pointer<psxViewLoop> (&g_CurrentViewLoop);
}

#endif
