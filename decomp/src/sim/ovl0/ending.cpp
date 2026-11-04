// OVL0 8005a474-8005a77c (ending): Ending.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern QuickMenu *g_QuickMenu;
extern short g_Ending_Fade;
extern QuickMenuPage g_Page_Ending;
extern QuickMenuPage g_Page_Quit;
extern Font g_OVL0_FontC;
extern Font g_OVL0_FontD;
extern unsigned char D_8005D5A4[];
extern unsigned char D_8005D5A8[];
extern unsigned char D_8005D5AC[];
extern signed char sR_CONGRATULATIONS[];

// Root page after winning the GT World League: 16-frame delay, music 0.
void EndingRoot_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x10;
    QuickMenu_PlayMusic(0);
}

// Pushes the ending page after the delay.
int EndingRoot_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int result = 0;

    if (page->timer > 0 && --page->timer == 0) {
        QuickMenu_PushPage(menu, &g_Page_Ending);
        result = 1;
    }
    return result;
}

// Empty.
void EndingRoot_Draw(DrawLayer *layer)
{
}

// Resets the fade counter.
void Ending_Enter(int back)
{
    g_Ending_Fade = 0;
}

// Fades in; a confirm button then starts the fade out and pushes the quit page.
int Ending_Update(int active)
{
    QuickMenu &menu = *g_QuickMenu;
    int result = 0;
    PadState *pad = &menu.padState[0];

    if (g_Ending_Fade < 0) {
        if (g_Ending_Fade < -1) {
            g_Ending_Fade++;
        }
        return 0;
    }
    if (++g_Ending_Fade > 0x24) {
        g_Ending_Fade = 0x24;
    }
    if (g_Ending_Fade >= 0x20 && (active & 1) && (pad->pressed & 0xa00)) {
        Sfx_PlayMenu(3);
        g_Ending_Fade = -0xc;
        QuickMenu_PushPage(&menu, &g_Page_Quit);
        result = 1;
    }
    return result;
}

// The congratulations, fading from black.
void Ending_Draw(DrawLayer *layer)
{
    unsigned int black = 0x2000000;
    TextCtx text;
    uint *ot = (uint *)layer->ot;
    int fade;

    if (g_Ending_Fade != -1) {
        TextCtx *ctx;
        unsigned char *from;
        signed char *s;
        int spacing;

        fade = g_Ending_Fade;
        if (fade < 0) {
            fade = -fade;
        } else {
            fade -= 0x18;
        }
        Text_Init(&text, 6);
        ctx = &text;
        ctx->ot = ot;
        Text_SetFont(ctx, &g_OVL0_FontC);
        *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
        from = (unsigned char *)&black;
        ctx->rgb = Rgb_Lerp(from, D_8005D5A4, fade, 0xc);
        s = sR_CONGRATULATIONS;
        spacing = 1;
        Text_DrawCentered(ctx, s, 0xb0, 0xb4, spacing);
        Text_SetFont(ctx, &g_OVL0_FontD);
        ctx->rgb = Rgb_Lerp(from, D_8005D5A8, fade, 0xc);
        Text_DrawCentered(ctx, s + 0x11, 0xb0, 0x104, spacing);
        ctx->rgb = Rgb_Lerp(from, D_8005D5AC, fade, 0xc);
        Text_DrawCentered(ctx, s + 0x2d, 0x98, 0x140, spacing);
        Text_DrawCentered(ctx, s + 0x4e, 0xc8, 0x15e, spacing);
    }
}
}
