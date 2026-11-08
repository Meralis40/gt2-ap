// OVL2 800257e8-8002589c (arcadebgm): Arcade menu music.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "copies.h"
#include "common.h"

extern "C" {
extern MenuBgmBank (&g_ArcadeBgmBank);
extern unsigned char (&g_ArcadeBgmWork);
extern unsigned int (&g_SpuAllocPtr);

// Loads sound bank 0xeb (work 0x4800 bytes) and VOLID_SOUND_ARCADE_SEQ into
// g_ArcadeBgmBank, then restores the SPU allocation pointer.
void ArcadeBgm_Load(void)
{
    int spuAddr;

    spuAddr = g_SpuAllocPtr;
    MenuBgmBank_Init(&g_ArcadeBgmBank, &g_ArcadeBgmWork, 0x4800);
    MenuBgmBank_Load(&g_ArcadeBgmBank, 0xeb);
    MenuBgmBank_LoadSeq(&g_ArcadeBgmBank, VOLID_SOUND_ARCADE_SEQ);
    SpuAlloc_SetPtr(spuAddr);
}

// Plays sequence 0 of g_ArcadeBgmBank.
void ArcadeBgm_Play(void)
{
    MenuBgmBank_Play(&g_ArcadeBgmBank, 0);
}

// Stops g_ArcadeBgmBank.
void ArcadeBgm_Stop(void)
{
    MenuBgmBank_Stop(&g_ArcadeBgmBank);
}
}
