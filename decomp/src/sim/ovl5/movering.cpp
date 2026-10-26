// Frame ring and display helpers of the movie screen.
#include "common.h"
#include "movie.h"

extern "C" {
void Movie_InitRing()
{
    int i;

    for (i = 31; i >= 0; i--) {
        g_MovieRing[i] = g_MovieFrames[i];
    }
}

void Movie_DisplayOn()
{
    Irq_WaitVSyncs(-1);
    EnableDisplay(1);
    Irq_WaitVSyncs(3);
}

// Clears VRAM (960 x 512 halfwords) to black, 8 lines at a time.
void Movie_ClearVram()
{
    unsigned int black[0xf00];
    RECT rect;
    int y;

    Movie_DisplayOn();
    memset(black, 0, sizeof(black));
    y = 0;
    while (1) {
        if (y >= 0x200) {
            break;
        }
        rect.x = 0;
        rect.y = y;
        rect.w = 0x3c0;
        rect.h = 8;
        Gpu_TransferImage(&rect, black, 0);
        Gpu_DrawSync();
        y += 8;
    }
}
}
