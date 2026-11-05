// psxViewLoop, the screen loop of the game's screens: one frame per Step
// (BeginFrame, BeginDraw, Update, Draw, EndDraw), shown by Flip and swapped
// from the vblank callback. Its vtable and type name (.rodata), __tf function
// and typeinfo (.bss) are emitted here.
#include "common.h"

// L2 + R2 + L1 + R1 + START + SELECT held on port 2: Step skips Update and
// returns 1.
#define PAD_RAW_RESET 0xef79

extern "C" {
extern unsigned char g_CurrentVideoMode;
extern unsigned short g_VideoModeWidths[5];

}

void psxViewLoop::VSyncHandler()
{
    g_CurrentViewLoop->UpdateDisplay();
}

psxViewLoop::psxViewLoop() : vsync(VSyncHandler, 0)
{
    vsyncInterval = 1;
    videoModeDelay = 3;
    bufferIndex = 0;
    bufferCount = 1;
    noDrawSync = 0;
    interlaced = 0;
    videoArg = 0;
}

psxViewLoop::~psxViewLoop()
{
}

// mode: GP1(08) bits; the draw and display areas become the full screen.
void psxViewLoop::SetVideoMode(unsigned int mode)
{
    int width = 4;

    if ((mode & 0x40) == 0) {
        width = mode & 3;
    }
    videoMode = mode;
    drawArea.x = 0;
    drawArea.y = 0;
    drawArea.w = g_VideoModeWidths[width];
    short h;

    if ((mode & 4) == 0) {
        h = 240;
    } else {
        h = 480;
    }
    drawArea.h = h;
    dispArea = drawArea;
    if (g_CurrentVideoMode != mode) {
        g_CurrentVideoMode = mode;
        interlaced = mode >> 2 & 1;
        Irq_WaitVSyncs(-1);
        EnableDisplay(1);
        Gpu_SetDisplayMode(mode);
    }
}

// The index goes through a byte local stored after videoModeDelay: that gives
// g_LastBufferIndex a0 and the index a1, as in the original.
void psxViewLoop::Enter()
{
    unsigned char index;

    prevLoop = g_CurrentViewLoop;
    g_CurrentViewLoop = this;
    index = g_LastBufferIndex < bufferCount ? g_LastBufferIndex : 0;
    videoModeDelay = 3;
    bufferIndex = index;
    videoModePending = 0;
    swapPending = 0;
    VSyncList_Add(&g_GfxIrq.vsyncCallbacks, &vsync);
    draw.otHead = 0;
}

// The status byte is read before the pointer to the buffers is taken: that
// keeps result = 0 first and &g_PadSystem in a register, as in the original.
int psxViewLoop::Step()
{
    int result = 0;
    int status = g_PadSystem.raw[1].status;
    volatile PadRawBuffer *raw = &g_PadSystem.raw[1];

    if (status == 0) {
        if ((raw->buttons[1] << 8 | raw->buttons[0]) == PAD_RAW_RESET) {
            result = 1;
        }
    }
    Timer_StartFrame();
    BeginFrame();
    BeginDraw();
    if (result == 0) {
        result = Update();
    }
    Draw();
    EndDraw();
    return result;
}

void psxViewLoop::Flip()
{
    int index;
    int leave;

    frameStartTime = Timer_SinceFrameStart();
    if (noDrawSync == 0) {
        Gpu_DrawSync();
    }
    index = bufferIndex + 1;
    if (index >= bufferCount) {
        index = 0;
    }
    bufferIndex = index;
    swapPending = vsyncInterval;
    if (videoModeDelay != 0 && --videoModeDelay == 0) {
        videoModePending = 1;
    }
    Irq_WaitVSyncs(vsyncInterval);
    if (noDrawSync != 0) {
        Gpu_ResetCommandBuffer();
    }
    leave = Sys_CriticalSection(0);
    if (swapPending != 0) {
        swapPending = 0;
        SwapDisplay();
        if (videoModePending != 0) {
            videoModePending = 0;
            ApplyVideoMode();
        }
    }
    Sys_CriticalSection(leave);
    if (interlaced != 0) {
        Gpu_WaitField();
    }
    frameEndTime = Timer_SinceFrameStart();
}

void psxViewLoop::Leave()
{
    VSyncList_Remove(&g_GfxIrq.vsyncCallbacks, &vsync);
    Gpu_DrawSync();
    g_LastBufferIndex = bufferIndex;
    g_CurrentViewLoop = prevLoop;
}

void psxViewLoop::BeginFrame()
{
}

int psxViewLoop::Update()
{
    return 1;
}

void psxViewLoop::BeginDraw()
{
    ViewDrawContext *ctx = &draw;

    ViewDrawContext_Submit(ctx);
    ViewDrawContext_SwapOT(ctx);
    ViewDrawContext_ClearOT(ctx);
    PacketBuffer_Swap(&packets);
    ViewDrawContext_Begin(ctx, &drawArea);
    Dma_WaitOtc();
}

void psxViewLoop::Draw()
{
}

void psxViewLoop::EndDraw()
{
    ViewDrawContext_End(&draw);
}

void psxViewLoop::DrawFrame()
{
    BeginDraw();
    Draw();
    EndDraw();
    Flip();
}

void psxViewLoop::SwapDisplay()
{
    swapPending = 0;
}

void psxViewLoop::ApplyVideoMode()
{
    interlaced = videoMode >> 2 & 1;
    Gpu_SetDisplayMode(videoMode);
    Gpu_SetHRange(dispArea.x, dispArea.w);
    Gpu_SetVRange(dispArea.y, dispArea.h);
    EnableDisplay(videoArg);
}

// vblank callback: swaps the display once swapPending vblanks have passed
// since the last flip.
void psxViewLoop::UpdateDisplay()
{
    int vblanks = g_GfxIrq.vblanksSinceFlip;
    int swap = swapPending;
    int modePending = videoModePending;

    if (swap != 0 && vblanks >= swap) {
        swapPending = 0;
        SwapDisplay();
        if (modePending != 0) {
            videoModePending = 0;
            ApplyVideoMode();
        }
    }
}
