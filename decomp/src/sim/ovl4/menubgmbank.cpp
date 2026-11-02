// OVL4 800226a0-80022964 (menubgmbank): Menu music bank.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern unsigned int g_SpuAllocPtr;
extern MenuBgmBank g_MenuBgmBank;
extern unsigned char g_MenuBgm_SpuWork;
extern unsigned char DAT_OVL4__80050000;
extern int g_MenuBgm_Loaded;
extern VolBootId g_MenuBgm_Files;
extern GameState g_GameState;

// Clears the bank and sets up its sample bank in work (size bytes).
void MenuBgmBank_Init(MenuBgmBank *bank, void *work, int size)
{
    memset(bank, 0, sizeof(MenuBgmBank));
    SpuBank_Init((SpuBank *)bank, (SeqBank *)work, size);
}

void FUN_OVL4__800226f4(MenuBgmBank *param_1)
{
    MenuBgmBank_Stop(param_1);
}

// Loads the sample bank volId; the sequence goes after it in the buffer.
void MenuBgmBank_Load(MenuBgmBank *bank, int volId)
{
    int size = SpuBank_Load((SpuBank *)bank, volId, 0, 1);

    bank->seqData = (char *)bank->buffer + ((size + 3) & ~3);
}

// Reads a sequence file into seqData and prepares it.
void MenuBgmBank_LoadSeq(MenuBgmBank *bank, VolBootId id)
{
    VolId_ReadFile(id, bank->seqData);
    SeqFile_Relocate(bank->seqData);
}

// Starts song seq of the loaded sequence, at the music volume option.
void MenuBgmBank_Play(MenuBgmBank *bank, int seq)
{
    MenuBgmBank_Stop(bank);
    Seq_SetMasterVolume((unsigned short)(g_GameState.options2[5] * 0x4000 / 0xff));
    SeqPlayer_Setup(&bank->seq, (SeqSong *)((char *)bank->seqData + (seq * 0x48 + 0xc)), (SeqBank *)bank->buffer);
    SeqPlayer_Start(&bank->seq);
    bank->playing = 1;
}

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
