// Resident code 8005d6e0-8005d718 (main): the game's entry point.
#include "common.h"

extern "C" {

// Boot, then the main menu's overlay. The runtime's main calls this, and runs the entries
// Overlay_StartAt asks for.
void Game_Main(void)
{
    Boot_Init();
    Overlay_Start(1);
}

}
