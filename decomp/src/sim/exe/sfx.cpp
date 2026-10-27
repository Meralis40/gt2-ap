// Resident code 80078370-80079140 (sfx). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void SfxVoice_KeyOff(unsigned char *handle);
void SfxVoice_Release(unsigned char *handle);
void Sfx_StopAll(...);
void *SoundBank_GetSample(void *bank, int index);
void SoundSample_Play(short *sample, int pan, int volume, int voice);
int SpuAlloc_SetPtr(int spuAddr);
extern unsigned int g_SoundDriverReady;
extern unsigned int g_SpuAllocPtr;
extern void *g_SpuBankPoolPtr;
extern int g_SpuBankPoolSize;

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", Spu_WaitVoicesSilent);

void Sfx_StopAllIfReady(void)
{
    if (g_SoundDriverReady != 0) {
        Sfx_StopAll();
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", Sfx_StopAll);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", SoundSample_Play);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", SfxLoop_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", SfxLoop_Update);

void SfxLoop_Stop(SfxLoop *loop)
{
    SfxVoice_KeyOff(&loop->voice);
    SfxVoice_Release(&loop->voice);
}

void SpuBank_Init(unsigned int *param_1, unsigned int param_2, unsigned int param_3)
{
    unsigned int uVar1;

    uVar1 = g_SpuAllocPtr;
    *param_1 = param_2;
    param_1[1] = param_3;
    param_1[2] = uVar1;
}

void SpuAlloc_Restore(void *mark)
{
    SpuAlloc_SetPtr(*(int *)((int)mark + 8));
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", SpuBank_Load);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", SpuBank_LoadFromPool);

void *SoundBank_GetSample(void *bank, int index)
{
    return (void *)(*(int *)(*(int *)bank + 0x1c) + index * 0x14);
}

void SoundBank_Play(void *bank, int index, int volume, int pitch, int voice)
{
    short *sample;

    sample = SoundBank_GetSample(bank, index);
    SoundSample_Play(sample, volume, pitch, voice);
}

int SpuAlloc_SetPtr(int spuAddr)
{
    int iVar1;

    iVar1 = g_SpuAllocPtr;
    g_SpuAllocPtr = spuAddr;
    return iVar1;
}

void SpuBankPool_Set(void *data, int size)
{
    g_SpuBankPoolPtr = data;
    g_SpuBankPoolSize = size;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", Inst_Relocate);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", EngineSound_CalcPitch);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", EngineSound_CalcVolume);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", EngineSound_Update);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", EngineSound_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", EngineSound_Stop);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", EngineSound_SetPriority);

INCLUDE_ASM("asm/sim/exe/nonmatchings/sfx", Engn_Relocate);
}
