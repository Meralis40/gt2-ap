// Resident code 8007c36c-8007d024 (cdaudio). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CdAudio_ExecScript(CdAudio *cd, unsigned int *script);
int Sys_CriticalSection(int leave);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", Cd_BcdMsfToSector);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", Cd_LbaToPos);

unsigned int CdAudio_DefaultSectorFn(void)
{
    return 0;
}

void CdAudio_DefaultFn(void)
{
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_Init);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_RunScript);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_RunScript_8007C4F8);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_WaitIdle);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", Music_Stop);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", FUN_8007c5a8);

void CdAudio_ClearPending(void *cd)
{
    int leave;

    leave = Sys_CriticalSection(0);
    *(unsigned int *)((int)cd + 0x68) = 0;
    *(unsigned int *)((int)cd + 0x4c) = 0;
    Sys_CriticalSection(leave);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_QueueScript);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_ApplyVolume);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_SetMode);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_ExecScript);

void CdAudio_SetScript(CdAudio *cd, unsigned int *script)
{
    cd->script = script;
    if (cd->state != 3) {
        CdAudio_ExecScript(cd, script);
    }
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_RunQueued);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_Reset);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_OnSync);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_OnReady);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_VSync);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdGetSector_Thunk);

INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_SetupRead);
}
