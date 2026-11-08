// ScreenViewLoop, the base of the resident screen loops (psxViewLoop, the boot
// logo, OVL5's psxMovieLoop). Its vtable and type name (.rodata), __tf function
// and typeinfo (.bss) are emitted here.
#include "common.h"

ScreenViewLoop::ScreenViewLoop()
{
    vptr.address = 0x800908ec;  // ScreenViewLoop's vtable
}

ScreenViewLoop::~ScreenViewLoop()
{
}

int ScreenViewLoop::Execute()
{
    v_Run();
    return 0;
}

void ScreenViewLoop::Run()
{
    v_Enter();
    while (v_Step()) {
        v_Flip();
    }
    v_Leave();
}

void ScreenViewLoop::Enter()
{
}

int ScreenViewLoop::Step()
{
    return 1;
}

void ScreenViewLoop::Flip()
{
}

void ScreenViewLoop::Leave()
{
}
