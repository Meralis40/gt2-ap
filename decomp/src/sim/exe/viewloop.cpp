// Resident code 80080820-80080eec (viewloop). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Dma_WaitOtc(...);
void EnableDisplay(unsigned int param_1);
void Gpu_DrawSync(...);
void Gpu_SetDisplayMode(unsigned int mode);
void Gpu_SetHRange(int x0, int x1);
void Gpu_SetVRange(int y0, int y1);
void PacketBuffer_Swap(PacketBuffer *buf);
void VSyncList_Remove(VSyncCallback **list, VSyncCallback *node);
void ViewDrawContext_Begin(ViewDrawContext *ctx, Rect16 *area);
void ViewDrawContext_ClearOT(ViewDrawContext *ctx);
void ViewDrawContext_End(ViewDrawContext *ctx);
void ViewDrawContext_Submit(ViewDrawContext *ctx);
void ViewDrawContext_SwapOT(ViewDrawContext *ctx);
extern psxViewLoop *g_CurrentViewLoop;
extern GfxIrqState g_GfxIrq;
extern unsigned char g_LastBufferIndex;

void psxViewLoop__VSyncHandler(void)
{
    (*g_CurrentViewLoop->vptr->UpdateDisplay)(g_CurrentViewLoop);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__psxViewLoop);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__psxViewLoop_dtor);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__SetVideoMode);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__Enter);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__Step);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__Flip);

void psxViewLoop__Leave(psxViewLoop *this_)
{
    VSyncList_Remove(&g_GfxIrq.vsyncCallbacks, &this_->vsync);
    Gpu_DrawSync();
    g_LastBufferIndex = this_->bufferIndex;
    g_CurrentViewLoop = this_->prevLoop;
}

void psxViewLoop__BeginFrame(psxViewLoop *this_)
{
}

int psxViewLoop__Update(psxViewLoop *this_)
{
    return 1;
}

void psxViewLoop__BeginDraw(psxViewLoop *this_)
{
    ViewDrawContext *ctx;

    ctx = &this_->draw;
    ViewDrawContext_Submit(ctx);
    ViewDrawContext_SwapOT(ctx);
    ViewDrawContext_ClearOT(ctx);
    PacketBuffer_Swap(&this_->packets);
    ViewDrawContext_Begin(ctx, (Rect16 *)&this_->drawX);
    Dma_WaitOtc();
}

void psxViewLoop__Draw(psxViewLoop *this_)
{
}

void psxViewLoop__EndDraw(psxViewLoop *this_)
{
    ViewDrawContext_End(&this_->draw);
}

void psxViewLoop__DrawFrame(psxViewLoop *this_)
{
    (*this_->vptr->BeginDraw)(this_);
    (*this_->vptr->Draw)(this_);
    (*this_->vptr->EndDraw)(this_);
    (*this_->vptr->Flip)(this_);
}

void psxViewLoop__SwapDisplay(psxViewLoop *this_)
{
    this_->swapPending = '\0';
}

void psxViewLoop__ApplyVideoMode(psxViewLoop *this_)
{
    this_->interlaced = this_->videoMode >> 2 & 1;
    Gpu_SetDisplayMode((unsigned int)this_->videoMode);
    Gpu_SetHRange((int)this_->dispX, (int)this_->dispW);
    Gpu_SetVRange((int)this_->dispY, (int)this_->dispH);
    EnableDisplay((unsigned int)this_->videoArg);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop__UpdateDisplay);

INCLUDE_ASM("asm/sim/exe/nonmatchings/viewloop", psxViewLoop____tf);
}
