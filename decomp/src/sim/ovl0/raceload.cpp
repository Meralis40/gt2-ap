// OVL0 80018a68-80018d1c (raceload): Loading screen VSync callback.
// The functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"
#include "gte.h"

// The loading screen's vblank callback and its frame counter (g_RaceLoadFrame).
struct RaceLoadVSync {
    VSyncCallback cb;
    short frame;
};

extern "C" {
extern RaceLoadVSync g_RaceLoadVSync;
extern unsigned char g_Race_IsDemo;

// g_RaceSetup, from g_GameState and an offset local (the offset is too big for an addiu).
static inline RaceSetup *RaceSetup_Get()
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// Called at the start of RaceViewLoop::Enter: installs g_RaceLoadVSync (RaceLoad_VSyncDraw,
// frame 0) and sends the draw area (0,0)-(320,240), offset 0 and draw mode straight to GP0.
#ifdef NON_MATCHING
// The game loads &g_RaceLoadVSync into $a0 before the prologue and copies it to $s0 in
// the call's delay slot; GCC loads it into $s0 directly ("the address in $a0 first").
void RaceLoad_InstallVSync(void)
{
    unsigned int cmds[4];

    VSyncCallback_Install(&g_RaceLoadVSync.cb);
    g_RaceLoadVSync.cb.func = RaceLoad_VSyncDraw;
    g_RaceLoadVSync.frame = 0;
    cmds[0] = 0xe3000000;
    cmds[1] = 0xe403c140;
    cmds[2] = 0xe5000000;
    cmds[3] = 0xe1000020;
    Gpu_SendDirect(cmds, 4);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceload", RaceLoad_InstallVSync);
#endif

// Called at the end of RaceViewLoop::Enter: removes g_RaceLoadVSync.
#ifdef NON_MATCHING
// The game loads &g_RaceLoadVSync into $a0 before the prologue (and leaves the call's
// delay slot empty); GCC loads it after.
void RaceLoad_RemoveVSync(void)
{
    VSyncCallback_Uninstall(&g_RaceLoadVSync.cb);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceload", RaceLoad_RemoveVSync);
#endif

// VSync handler while a race loads (no image transfer running, not a demo, game 0, race
// type 4): reveals 6 bands of 320x22 at y = 61 + 24n, one every 40 frames from frame 80.
// Each band starts with menu sound 1, is copied from VRAM y = 301 + 24n for 16 frames and
// covered by a fading semi-transparent quad for 14 frames.
#ifdef NON_MATCHING
// The game loads &g_RaceLoadVSync into $a0 before the prologue and keeps it in $a0's
// stack slot (all saved registers are taken); GCC builds the address at each use.
void RaceLoad_VSyncDraw(void)
{
    RaceLoadVSync *self = &g_RaceLoadVSync;
    RaceSetup *setup;
    unsigned char *transfer;
    int busy;
    unsigned int cmds[9];
    short *h = (short *)cmds;
    int band;
    int bottom, srcY, top, start;

    busy = 0;
    setup = RaceSetup_Get();
    transfer = &g_GfxIrq.imageTransferBusy;
    if (transfer[0] != 0 || transfer[1] != 0) {
        busy = 1;
    }
    if (busy || g_Race_IsDemo != 0) {
        return;
    }
    if (setup->game != 0) {
        return;
    }
    if (setup->raceType != 4) {
        return;
    }
    bottom = 0x53;
    srcY = 0x12d;
    top = 0x3d;
    start = 0x50;
    self->frame++;
    for (band = 0; band < 6; band++) {
        unsigned int t = self->frame - start;
        int count = 0;

        if (t == 0) {
            Sfx_PlayMenu(1);
        }
        if (t < 16) {
            // GP0 0x80: copy the band from VRAM y = srcY.
            count = 4;
            cmds[0] = 0x80000000;
            h[2] = h[4] = 0;
            h[5] = top;
            h[3] = srcY;
            h[6] = 320;
            h[7] = 22;
        }
        if (t - 1 < 14) {
            // GP0 0x2a: a semi-transparent quad fading out over it.
            unsigned int *q = &cmds[count];
            short *qh = (short *)q;
            int fade = 16 - t;

            count += 5;
            q[0] = (fade << 3) | (fade << 11) | (fade << 19) | 0x2a000000;
            qh[2] = qh[6] = 0;
            qh[4] = qh[8] = 320;
            qh[3] = qh[5] = top;
            qh[7] = qh[9] = bottom;
        }
        if (count > 0) {
            Gpu_SendDirect(cmds, count);
        }
        bottom += 24;
        srcY += 24;
        top += 24;
        start += 40;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceload", RaceLoad_VSyncDraw);
#endif

// Gte_LoadScaled(scr, 0, 0, 0) with a zero translation (the game writes $zero straight
// to the GTE's translation registers).
void Gte_LoadRotationOnly(ViewScratch *scr)
{
    register int zero asm("$0");

    Gte_LoadScaled(scr, 0, 0, 0);
    gte_SetCtrlReg(GTE_TRX, zero);
    gte_SetCtrlReg(GTE_TRY, zero);
    gte_SetCtrlReg(GTE_TRZ, zero);
}
}
