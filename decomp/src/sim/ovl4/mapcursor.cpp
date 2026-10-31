// OVL4 8001e22c-8001ec0c (mapcursor): Map cursor.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {


// Centres the cursor at (0x100, 0xfc) with no hotspot.
void MapCursor_Init(MapCursor *cursor)
{
    cursor->x = 0x100;
    cursor->prevX = 0x100;
    cursor->drawX = 0x100;
    cursor->y = 0xfc;
    cursor->prevY = 0xfc;
    cursor->drawY = 0xfc;
    cursor->moveFrames = '\0';
    cursor->posX = 0;
    cursor->posY = 0;
    cursor->velX = 0;
    cursor->velY = 0;
    cursor->blink = '\0';
    cursor->hotspot = (MenuElement *)0x0;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/mapcursor", MapCursor_Bump);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/mapcursor", MapCursor_Update);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/mapcursor", MapCursor_ResetToDefault);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/mapcursor", MapCursor_MoveTo);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/mapcursor", MapCursor_Draw);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/mapcursor", FUN_OVL4__8001eab8);
}
