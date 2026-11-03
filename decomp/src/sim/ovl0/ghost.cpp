// OVL0 8003ef18-8003fe8c (ghost): Ghost recording and playback.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", Ghost_SumHeader);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", GhostCar_Init);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", Angle_Lerp);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", GhostFrame_Capture);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", GhostFrame_Apply);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", TrackDist_Lerp);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", GhostCar_Interpolate);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", GhostCar_ApplyFrame);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", GhostRec_StartLap);

// Same as GhostRec_StartLap with mode 2 and no lap time.
void GhostRec_StartFree(CarPhys *car)
{
    unsigned char *puVar1;
    unsigned short *puVar2;

    puVar1 = TimeTrial_GetLapLog();
    puVar2 = TimeTrial_GetLapSlot();
    CarPhys_SaveState((int *)(puVar2 + 4), car);
    puVar1[1] = 2;
    *puVar1 = 1;
    *(unsigned short *)(puVar1 + 2) = 0;
    *puVar2 = 0;
    *(unsigned char *)(puVar2 + 1) = 0;
    *(unsigned char *)((int)puVar2 + 3) = 0;
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", GhostPlay_RestartLap);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", GhostPlay_Start);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", Ghost_ShouldHold);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ghost", Ghost_CommitLap);

// Clears a 0x342-byte ghost recording state.
void GhostRecState_Clear(void *state)
{
    memset(state, 0, 0x342);
    *(unsigned char *)state = 0;
    *(unsigned char *)((int)state + 1) = 0;
}

// Clears a ghost record header (0xe0 bytes) and sets its time to 359999999.
void GhostRecord_Clear(void *record)
{
    memset(record, 0, 0xe0);
    *(unsigned int *)((int)record + 4) = 359999999;
}

// Clears the ghost player (two 0x5c-byte frames, frame index, pending restart, weight
// 0x800, clock phase).
void GhostPlayer_Clear(void *player)
{
    memset(player, 0, 0xb8);
    *(unsigned short *)((int)player + 0xb8) = 0;
    *(unsigned short *)((int)player + 0xbe) = 0x800;
    *(unsigned short *)((int)player + 0xba) = 0;
    *(unsigned int *)((int)player + 0xc0) = 0;
}
}
