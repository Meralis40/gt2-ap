// Resident code 80079140-8007ab14 (seq). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
unsigned char FUN_8007a7cc(...);
void SeqPlayer_ApplyVolume(SeqPlayer *p);
void SeqPlayer_Link(SeqPlayer *p);
void SeqPlayer_Unlink(SeqPlayer *p);
void SfxVoices_Update(...);
extern SeqChannel g_SeqChannels[25];
extern unsigned short g_SeqMasterVolume;
extern unsigned short g_SeqTickRate;
extern unsigned int g_SoundDriverReady;

void Sound_Tick(void)
{
    if (g_SoundDriverReady != 0) {
        SfxVoices_Update();
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoices_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqChannel_NoteOn);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqPlayer_RunEvents);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Seq_Tick);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqChannel_UpdateAll);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Sound_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Sound_SetTickRate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Seq_Init);

void Seq_SetTickRate(unsigned short hz)
{
    g_SeqTickRate = hz;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Spu_SetReverbType);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Spu_EnableReverb);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Spu_SetReverbDepth);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_SetReverb);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Spu_PanVolume);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqFile_Relocate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Seq_SetMasterVolume);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqPlayer_Unlink);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqPlayer_Setup);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqPlayer_Link);

void SeqPlayer_ApplyVolume(SeqPlayer *p)
{
    int iVar1;
    unsigned short *puVar2;
    int iVar3;

    iVar3 = 0;
    puVar2 = &p->tracks[0].effVol;
    iVar1 = (int)((unsigned int)g_SeqMasterVolume * (unsigned int)p->baseVol) >> 0xe;
    p->vol = (unsigned short)iVar1;
    do {
        iVar3 = iVar3 + 1;
        *puVar2 = (unsigned short)(iVar1 * (unsigned int)puVar2[-1] >> 0xe);
        puVar2 = puVar2 + 0x14;
    } while (iVar3 < 0x10);
}

void SeqPlayer_Start(SeqPlayer *p)
{
    SeqPlayer_Unlink(p);
    SeqPlayer_ApplyVolume(p);
    SeqPlayer_Link(p);
}

void SeqPlayer_Stop(SeqPlayer *p)
{
    SeqChannel *pSVar1;
    int iVar2;

    SeqPlayer_Unlink(p);
    iVar2 = 0;
    pSVar1 = g_SeqChannels;
    do {
        if (pSVar1->player == p) {
            pSVar1->player = 0;
        }
        iVar2 = iVar2 + 1;
        pSVar1 = pSVar1 + 1;
    } while (iVar2 < 0x19);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Spu_NoteToPitch);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_Start);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_SetPriority);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_SetPitch);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_SetVolume);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_KeyOff);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_Release);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", FUN_8007a7cc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqFade_Step);

void FUN_8007a8cc(int param_1, unsigned short param_2, unsigned int param_3)
{
    *(unsigned short *)(param_1 + 0x1c) = 0;
    FUN_8007a7cc(param_1 + 0x1c, *(unsigned short *)(param_1 + 4), param_2, param_3);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", FUN_8007a900);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_Alloc);

INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_Setup);
}
