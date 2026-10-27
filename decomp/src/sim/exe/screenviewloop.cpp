// ScreenViewLoop, the base of the resident screen loops (psxViewLoop, the boot
// logo, OVL5's psxMovieLoop). Its vtable and type name (.rodata), __tf function
// and typeinfo (.bss) are emitted here.
#include "viewloop.h"

ScreenViewLoop::ScreenViewLoop()
{
}

ScreenViewLoop::~ScreenViewLoop()
{
}

int ScreenViewLoop::Execute()
{
    Run();
    return 0;
}

void ScreenViewLoop::Run()
{
    Enter();
    while (Step()) {
        Flip();
    }
    Leave();
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
