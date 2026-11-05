// OVL4 80022874-80022964 (gtbgm): The GT mode menu music, in g_MenuBgmBank.
#include "common.h"

extern "C" {
extern unsigned int g_SpuAllocPtr;
extern MenuBgmBank g_MenuBgmBank;
extern unsigned char g_MenuBgm_SpuWork;
extern int g_MenuBgm_Loaded;
extern VolBootId g_MenuBgm_Files;

// Loads sound bank volume file 0xec into SPU RAM for g_MenuBgmBank (work buffer
// 0x7800 bytes), then restores the SPU allocation pointer.
void MenuBgm_LoadBank(void)
{
    int spuAddr;

    spuAddr = g_SpuAllocPtr;
    Spu_WaitVoicesSilent();
    MenuBgmBank_Init(&g_MenuBgmBank, &g_MenuBgm_SpuWork, 0x7800);
    MenuBgmBank_Load(&g_MenuBgmBank, 0xec);
    SpuAlloc_SetPtr(spuAddr);
}

// Loads sequence g_MenuBgm_Files[track] (unless already loaded) and starts it.
void MenuBgm_Play(int track)
{
    if (g_MenuBgm_Loaded != track) {
        g_MenuBgm_Loaded = track;
        MenuBgmBank_LoadSeq(&g_MenuBgmBank, (&g_MenuBgm_Files)[track]);
        MenuBgmBank_Play(&g_MenuBgmBank, 0);
    }
}

// Forgets the loaded sequence and stops the player.
void MenuBgm_Stop(void)
{
    g_MenuBgm_Loaded = -1;
    MenuBgmBank_Stop(&g_MenuBgmBank);
}
}
