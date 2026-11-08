// OVL4 800226a0-80022874 (menubgmbank): Menu music bank. OVL2 builds it too.
#include "common.h"

extern "C" {
extern GameState (&g_GameState);

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
    SeqFile_Relocate((SeqFile *)bank->seqData);
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
}
