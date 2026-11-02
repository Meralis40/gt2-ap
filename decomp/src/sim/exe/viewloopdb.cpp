// psxViewLoopDoubleBuffer, a psxViewLoop with two display buffers in VRAM. Its
// vtable and type name (.rodata), __tf function and typeinfo (.bss) are emitted
// here.
#include "resident.h"

extern "C" {
extern RECT g_DrawArea;

void Gpu_DrawSync();
void Gpu_SetDisplayStart(unsigned int x, int y);
void ViewDrawContext_Submit(ViewDrawContext *ctx);
}

psxViewLoopDoubleBuffer::psxViewLoopDoubleBuffer()
{
    bufferCount = 2;
}

psxViewLoopDoubleBuffer::~psxViewLoopDoubleBuffer()
{
}

// Shows the last frame, draws two more so that both buffers hold it, then
// leaves.
void psxViewLoopDoubleBuffer::Leave()
{
    Flip();
    DrawFrame();
    DrawFrame();
    ViewDrawContext_Submit(&draw);
    Gpu_DrawSync();
    psxViewLoop::Leave();
}

void psxViewLoopDoubleBuffer::SetVideoMode(unsigned int mode)
{
    bufferY[0] = 0;
    if ((mode & 4) == 0) {
        bufferY[1] = 240;
    } else {
        bufferY[1] = 0;
    }
    psxViewLoop::SetVideoMode(mode);
}

void psxViewLoopDoubleBuffer::BeginDraw()
{
    int off = bufferY[bufferIndex];
    int x = drawArea.x;
    int y = drawArea.y;
    int w = drawArea.w;
    int h = drawArea.h;

    g_DrawArea.x = x;
    g_DrawArea.y = y + off;
    g_DrawArea.w = w;
    g_DrawArea.h = h;
    psxViewLoop::BeginDraw();
}

void psxViewLoopDoubleBuffer::SwapDisplay()
{
    Gpu_SetDisplayStart(0, bufferY[bufferIndex]);
}
