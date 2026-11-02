// Resident code 80060410-80060924 (garage2): the rest of garage.cpp, a file of its own
// for its .rodata, CarWork_ResetSetting's jump table, which comes after the tables of
// garage.cpp's CarWork_ApplySetting and CarWork_GetSettingRange. Ghidra's
// decompilation; the functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern SpuBank g_CommonSoundBank;
extern unsigned char D_8009160C[][2]; // the two samples of each menu sound
extern char D_80091620[];

// Sets setting back to the fitted parts' defaults.
void CarWork_ResetSetting(CarWork *work, int setting)
{
    int i, j;
    int none;
    short ratios[8];
    short minRatios[8];
    short maxRatios[8];

    switch (setting) {
    case 0: {
        unsigned char *e = work->susp[work->partSlots[0]].bytes;
        work->setup.t12_60[0] = e[0x1f];
        work->setup.t12_60[1] = e[0x22];
        break;
    }
    case 1: {
        unsigned char *e = work->susp[work->partSlots[0]].bytes;
        work->setup.t12_5a[2] = e[0x15];
        work->setup.t12_5a[3] = e[0x18];
        break;
    }
    case 2: {
        unsigned char *e = work->susp[work->partSlots[0]].bytes;
        work->setup.t12_60[4] = e[0x2a];
        work->setup.t12_60[5] = e[0x2d];
        work->setup.t12_60[8] = e[0x38];
        work->setup.t12_60[9] = e[0x3b];
        // ^ 1 into a local gives the game's xori.
        none = CarWork_HasFullSuspension(work) ^ 1;
        if (none) {
            work->setup.t12_60[6] = e[0x31];
            work->setup.t12_60[7] = e[0x34];
            work->setup.t12_60[10] = e[0x3f];
            work->setup.t12_60[11] = e[0x42];
        }
        break;
    }
    case 3: {
        unsigned char *e = work->susp[work->partSlots[0]].bytes;
        work->setup.t12_60[6] = e[0x31];
        work->setup.t12_60[7] = e[0x34];
        work->setup.t12_60[10] = e[0x3f];
        work->setup.t12_60[11] = e[0x42];
        break;
    }
    case 4: {
        unsigned char *e = work->susp[work->partSlots[0]].bytes;
        work->setup.t12_5a[0] = e[0xb];
        work->setup.t12_5a[1] = e[0xe];
        break;
    }
    case 5:
        work->setup.setting_5e[0] = 0x80;
        work->setup.setting_5e[1] = 0x80;
        break;
    case 6: {
        unsigned char *e = work->susp[work->partSlots[0]].bytes;
        work->setup.t12_60[12] = e[0x46];
        work->setup.t12_60[13] = e[0x4a];
        break;
    }
    case 7: {
        unsigned char *e = work->brakeBal[work->partSlots[2]].bytes;
        work->setup.t01_50[0] = e[0xc];
        work->setup.t01_50[1] = e[0xc];
        break;
    }
    case 8: {
        unsigned char *e = work->trans[work->partSlots[6]].bytes;
        work->setup.t11_4c = *(unsigned short *)&e[0x1a];
        i = 0;
    loop:
        if (i < CarWork_GetGearCount(work)) {
            int n = i + 1;
            work->setup.t11_values[n] = ((short *)&e[0xc])[i];
            i = n;
            goto loop;
        }
        if (CarWork_TransIsCustom(work, work->partSlots[6])) {
            CarWork_GetGearRange(work, ratios, minRatios, maxRatios);
            for (j = 0; j < 8; j++) {
                work->setup.t11_values[j] = ratios[j];
            }
        }
        break;
    }
    case 9: {
        work->setup.t11_4e = (work->trans + work->partSlots[6])[0].bytes[0x21];
        break;
    }
    case 0x10: {
        unsigned char *e = (*(work->racingMod + work->partSlots[0x13])).bytes;
        work->setup.t05_52 = e[0x12];
        work->setup.t05_53 = e[0x15];
        break;
    }
    case 0xe:
        work->setup.t1b_74 = (work->asc + work->partSlots[0x14])[0].bytes[0xb];
        break;
    case 0xf:
        work->setup.t1c_75 = (work->tcs + work->partSlots[0x15])[0].bytes[0xc];
        break;
    case 10:
    case 13: {
        unsigned char *e = work->lsd[work->partSlots[0x16]].bytes;
        work->setup.t15_6e[0] = e[0xd];
        work->setup.t15_6e[1] = e[0x17];
        break;
    }
    case 11: {
        unsigned char *e = work->lsd[work->partSlots[0x16]].bytes;
        work->setup.t15_6e[2] = e[0x10];
        work->setup.t15_6e[3] = e[0x1a];
        break;
    }
    case 12: {
        unsigned char *e = work->lsd[work->partSlots[0x16]].bytes;
        work->setup.t15_6e[4] = e[0x13];
        work->setup.t15_6e[5] = e[0x1d];
        break;
    }
    }
}

// Plays menu sound sfx: one sample in the centre, or two panned left and right.
void Sfx_PlayMenu(int sfx)
{
    unsigned char (*t)[2] = D_8009160C;
    unsigned char *e = t[sfx];
    int left = e[0];
    int right = e[1];

    if (left == right) {
        SoundBank_Play(&g_CommonSoundBank, right, 0x40, 0x4000, -1);
    } else {
        SoundBank_Play(&g_CommonSoundBank, left, 0, 0x4000, -1);
        SoundBank_Play(&g_CommonSoundBank, right, 0x7f, 0x4000, -1);
    }
}

int FUN_800608e8(unsigned int param_1, int param_2)
{
    unsigned int uVar1;
    char *pcVar2;
    int iVar3;

    iVar3 = 4;
    do {
        uVar1 = param_1 & 0x3f;
        param_1 = param_1 >> 6;
        pcVar2 = (char *)(param_2 + iVar3);
        iVar3 = iVar3 + -1;
        *pcVar2 = D_80091620[uVar1];
    } while (iVar3 != -1);
    *(unsigned char *)(param_2 + 5) = 0;
    return param_2;
}
}
