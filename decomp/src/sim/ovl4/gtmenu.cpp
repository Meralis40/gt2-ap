// OVL4 80013bd4-80014380 (gtmenu): GTMenu, the GT mode screen loop.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(psxViewLoopDoubleBuffer *this_, int in_chrg);
extern psxViewLoop_vtbl GTMenu__vtable;
void PadDevice_Close(PadDevice *dev);
void psxViewLoopDoubleBuffer__Leave(psxViewLoopDoubleBuffer *this_);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmenu", GTMenu__GTMenu);

// Destructor (GCC 2): resets the vptr; deletes this_ if (in_chrg & 1).
void GTMenu__GTMenu_dtor(GTMenu *this_, int in_chrg)
{
    (this_->base).base.vptr = &GTMenu__vtable;
    psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor(&this_->base, in_chrg);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmenu", GTMenu__Enter);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmenu", GTMenu__Update);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmenu", GTMenu__Draw);

// GTMenu__Leave: closes both pads.
void GTMenu__Leave(GTMenu *this_)
{
    psxViewLoopDoubleBuffer__Leave(&this_->base);
    PadDevice_Close(this_->pads);
    PadDevice_Close(this_->pads + 1);
}

void FUN_OVL4__800142c4(void)
{
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtmenu", GTMenu_GoBack);

// Queues a MenuView_ShowCar call, run by GTMenu__BeginFrame after delay frames (or
// right after a pending page change). garageSlot -1 shows the car with its default
// setup.
void GTMenu_QueueCarPreview(GTMenu *menu, int delay, int baseCarId, int carId, int colorIdx, int garageSlot, int hasWheel, int wheel, int dirt)
{
    menu->pendingCarDelay = (char)delay;
    menu->pendingCarBaseId = baseCarId;
    menu->pendingCarId = carId;
    menu->pendingCarColor = colorIdx;
    menu->pendingCarSlot = garageSlot;
    menu->pendingCarHasWheel = (unsigned char)hasWheel;
    menu->pendingCarWheel = wheel;
    menu->pendingCarDirt = dirt;
}
}
