// psxViewLoop, the screen loop of the game's screens: one frame per Step
// (BeginFrame, BeginDraw, Update, Draw, EndDraw), shown by Flip and swapped
// from the vblank callback. Its vtable and type name (.rodata), __tf function
// and typeinfo (.bss) are emitted here.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "viewloop.h"

// Raw buffer of a controller port (PsyQ PadInitDirect, 34 bytes).
struct PadRawBuffer {
    unsigned char status;       // 0 when the read succeeded
    unsigned char typeLen;
    unsigned char buttons[2];   // active low
    unsigned char analog[4];
    unsigned char extra[26];
};

struct PadSystem {
    VSyncCallback vsync;
    struct PadDevice *devices[2];
    unsigned char ports[2][8];
    volatile PadRawBuffer raw[2];   // written by the pad interrupt
};

// L2 + R2 + L1 + R1 + START + SELECT held on port 2: Step skips Update and
// returns 1.
#define PAD_RAW_RESET 0xef79

extern "C" {
extern PadSystem g_PadSystem;
extern unsigned char g_CurrentVideoMode;
extern unsigned short g_VideoModeWidths[5];

void VSyncList_Add(VSyncCallback **list, VSyncCallback *node);
void VSyncList_Remove(VSyncCallback **list, VSyncCallback *node);
int Irq_WaitVSyncs(int count);
int Sys_CriticalSection(int leave);
void Timer_StartFrame();
int Timer_SinceFrameStart();
void Gpu_DrawSync();
void Gpu_WaitField();
void Gpu_ResetCommandBuffer();
void Gpu_SetDisplayMode(unsigned int mode);
void Gpu_SetHRange(int x0, int x1);
void Gpu_SetVRange(int y0, int y1);
void EnableDisplay(unsigned int arg);
void Dma_WaitOtc();
void ViewDrawContext_Begin(ViewDrawContext *ctx, RECT *area);
void ViewDrawContext_End(ViewDrawContext *ctx);
void ViewDrawContext_Submit(ViewDrawContext *ctx);
void ViewDrawContext_SwapOT(ViewDrawContext *ctx);
void ViewDrawContext_ClearOT(ViewDrawContext *ctx);
void PacketBuffer_Swap(PacketBuffer *buf);
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

// Not matching: g_LastBufferIndex and the new index get a0 and a1 the other way
// round (register allocation).
#ifdef NON_MATCHING
void psxViewLoop::Enter()
{
    prevLoop = g_CurrentViewLoop;
    g_CurrentViewLoop = this;
    bufferIndex = g_LastBufferIndex < bufferCount ? g_LastBufferIndex : 0;
    videoModeDelay = 3;
    videoModePending = 0;
    swapPending = 0;
    VSyncList_Add(&g_GfxIrq.vsyncCallbacks, &vsync);
    draw.otHead = 0;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__Enter);
#endif

// Not matching: the first scheduling pass moves result = 0 after the volatile
// status load, into the load delay, where the original keeps it first and
// stalls.
#ifdef NON_MATCHING
int psxViewLoop::Step()
{
    int result = 0;

    if (g_PadSystem.raw[1].status == 0) {
        volatile PadRawBuffer *raw = &g_PadSystem.raw[1];

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
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__Step);
#endif

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
