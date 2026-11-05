// Resident code 8005d6e0-8005d718 (main): the game's entry point.
#include "common.h"

extern "C" {

// Boot, then the main menu's overlay.
int main()
{
    Boot_Init();
    Overlay_Dispatch();
    Overlay_Start(1);
    return 0;
}

}
