// OVL4 800226a0-80022964 (menubgmbank): Menu music bank.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
int *SeqFile_Relocate(int *file);
void VolId_ReadFile(VolBootId id, void *dst);
void SeqPlayer_Stop(SeqPlayer *p);
int SpuAlloc_SetPtr(int spuAddr);
void Spu_WaitVoicesSilent(void);
void MenuBgmBank_Init(MenuBgmBank *bank, void *work, int size);
void MenuBgmBank_Load(MenuBgmBank *bank, int volId);
extern unsigned int g_SpuAllocPtr;
extern MenuBgmBank g_MenuBgmBank;
extern unsigned char g_MenuBgm_SpuWork;
void MenuBgmBank_Play(MenuBgmBank *bank, int seq);
extern unsigned char DAT_OVL4__80050000;
extern int g_MenuBgm_Loaded;
extern VolBootId g_MenuBgm_Files;

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menubgmbank", MenuBgmBank_Init);

void FUN_OVL4__800226f4(MenuBgmBank *param_1)
{
    MenuBgmBank_Stop(param_1);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menubgmbank", MenuBgmBank_Load);

// Reads a sequence file into seqData and prepares it.
void MenuBgmBank_LoadSeq(MenuBgmBank *bank, VolBootId id)
{
    VolId_ReadFile(id, bank->seqData);
    SeqFile_Relocate(bank->seqData);
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/menubgmbank", MenuBgmBank_Play);

// Stops the sequence if playing.
void MenuBgmBank_Stop(MenuBgmBank *bank)
{
    if (bank->playing != '\0') {
        SeqPlayer_Stop(&bank->seq);
    }
    bank->playing = '\0';
}

/* Loads sound bank volume file 0xec into SPU RAM for g_MenuBgmBank (work buffer 0x7800 bytes), then
      restores the SPU allocation pointer. */

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

/* Forgets the loaded sequence and stops the player. */

void MenuBgm_Stop(void)
{
    g_MenuBgm_Loaded = -1;
    MenuBgmBank_Stop(&g_MenuBgmBank);
}
}
