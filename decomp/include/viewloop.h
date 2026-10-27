// Screen loops of the resident code (docs/decomp.md).
#ifndef VIEWLOOP_H
#define VIEWLOOP_H

// The resident screen loops. ScreenViewLoop (screenviewloop.cpp) holds only the
// vtable pointer: Execute calls Run, which runs Enter, then Flip after each
// Step until Step returns 0, then Leave. psxViewLoop (0x6c bytes, still
// assembly) adds the frame, draw and display steps. The virtual functions are
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
    unsigned char pad04[0x14];
    int vsyncInterval;
    unsigned char videoModeDelay;
    unsigned char bufferIndex;
    unsigned char bufferCount;
    unsigned char videoModePending;
    unsigned char pad20[0x4c];

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
};

#endif
