// OVL0 80029d58-8002bcf0 (racepause): Pause menu, countdown and result banners.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern char g_PauseMenu_Cursor;
extern char g_PauseMenu_Blink;

// Cursor 0, blink 0.
void PauseMenu_Reset(void *view)
{
    g_PauseMenu_Cursor = '\0';
    g_PauseMenu_Blink = '\0';
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", PauseMenu_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", PauseMenu_Draw);

// cnt = intro + 240.
void Countdown_Init(short *cnt, short introFrames)
{
    *cnt = introFrames + 0xf0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", Countdown_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", Countdown_Draw);

// anim = -1.
void ResultBanner_Hide(short *anim)
{
    *anim = -1;
}

// anim = 0.
void ResultBanner_Start(short *anim)
{
    *anim = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", ResultBanner_Step);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", Hud_DrawZoomSprite);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", ResultBanner_Draw);

// Timeline and banner off.
void Result_Init(short *res)
{
    *res = -1;
    ResultBanner_Hide(res + 1);
}

// Starts the result timeline.
void Result_Start(short *res)
{
    *res = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", Result_Update);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", FUN_OVL0__8002aaf0);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", Result_DrawTitle);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", Result_DrawTimeRow);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", Result_DrawPointsRow);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/racepause", Result_Draw);
}
