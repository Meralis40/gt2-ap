// Resident code 80078370-80079140 (sfx): sound effects on the SPU voices (one-shot
// samples, loops and the cars' engine sounds), and loading sound banks into SPU RAM.
#include "resident.h"

extern "C" {
#define SPU_VOICES ((SpuVoiceRegs *)0x1f801c00)

void SfxVoices_Update(void);
void SfxVoice_Start(signed char *handle, SfxParams *params);
void SfxVoice_SetPriority(signed char *handle, uchar priority);
void SfxVoice_SetPitch(signed char *handle, ushort pitch);
void SfxVoice_SetVolume(signed char *handle, short *vol);
void SfxVoice_KeyOff(signed char *handle);
void SfxVoice_Release(signed char *handle);
int Spu_NoteToPitch(int note, int base);
void Spu_PanVolume(short *out, int vol, int pan);
int SpuFile_LoadNegId(int id, SeqBank *ram, int spu, int isEngine);
int SpuFile_Load(int id, SeqBank *ram, int spu, int isEngine);
void Sfx_StopAll(void);
SeqRegion *SoundBank_GetSample(SpuBank *bank, int index);
void SoundSample_Play(SeqRegion *sample, int pan, int volume, int voice);
void SfxLoop_Stop(SfxLoop *loop);
void SpuBank_Init(SpuBank *bank, SeqBank *ram, int size);
int SpuAlloc_SetPtr(int spuAddr);
void EngineSound_CalcPitch(EngineSound *es, int i, int rpm, int smooth);
void EngineSound_CalcVolume(EngineSound *es, int i, int rpm, int smooth);
extern unsigned int g_SoundDriverReady;
extern unsigned int g_SpuAllocPtr;
extern void *g_SpuBankPoolPtr;
extern int g_SpuBankPoolSize;
extern SfxVoice g_SfxVoices[24];
extern uchar D_801C9994; // sound effect volume, 0-255
extern short g_SinTable[];

// Waits until the envelopes of all 24 SPU voices are at 0, or about 6.8 million
// tries.
void Spu_WaitVoicesSilent(void)
{
    int tries;
    int busy;
    int i;

    tries = 0;
    if (g_SoundDriverReady != 0) {
        busy = 1;
        while (busy != 0 && tries <= 0x675bff) {
            busy = 0;
            for (i = 0; i < 24; i++) {
                if (SPU_VOICES[i].envx != 0) {
                    busy = 1;
                }
            }
            tries++;
        }
    }
}

void Sfx_StopAllIfReady(void)
{
    if (g_SoundDriverReady != 0) {
        Sfx_StopAll();
    }
}

// Keys off all 24 voices and takes them from their owners, with the driver off
// meanwhile.
void Sfx_StopAll(void)
{
    int ready;
    int i;
    SfxVoice *v;

    ready = g_SoundDriverReady;
    g_SoundDriverReady = 0;
    for (i = 0, v = g_SfxVoices; i < 24; i++, v++) {
        v->dirty = 2;
        *(int *)&v->volL = 0; // both halves, as one word
        v->keyOff = 1;
        v->keyOn = 0;
        v->lock = 0;
        v->state = 0;
        if (v->owner != 0) {
            *v->owner = -1;
            v->owner = 0;
        }
    }
    SfxVoices_Update();
    g_SoundDriverReady = ready;
}

// Plays a bank's sample once on a free voice, at volume (0-255, times the sound
// effect volume) and pan (0x40 = centre). voice is the priority, or -1 for the
// sample's.
void SoundSample_Play(SeqRegion *sample, int pan, int volume, int voice)
{
    SfxParams params;

    if (volume != 0) {
        volume = D_801C9994 * volume / 255;
        volume = volume * sample->vol >> 14;
        if (volume >= 0x4000) {
            volume = 0x3fff;
        }
        Spu_PanVolume(params.vol, volume, pan);
        params.pitch = Spu_NoteToPitch(sample->low << 8, sample->root);
        params.duration = sample->duration;
        params.adsr = sample->adsr;
        params.addr = sample->addr;
        if (voice >= 0) {
            params.priority = voice;
        } else {
            params.priority = sample->priority;
        }
        params.flags = sample->flags;
        SfxVoice_Start(0, &params);
    }
}

void SfxLoop_Init(SfxLoop *loop, SeqRegion *sample)
{
    loop->sample = sample;
    loop->voice = -1;
}

// Plays the loop at the volume pair (0-255, times the sound effect volume) and
// pitch (0x1000 = the sample's): starts it if it has no voice, stops it at 0.
// priority is -1 to keep the sample's.
void SfxLoop_Update(SfxLoop *loop, int left, int right, int priority, int pitch)
{
    SfxParams params;
    int p;

    left = D_801C9994 * left / 255;
    right = D_801C9994 * right / 255;
    if ((left | right) == 0) {
        SfxLoop_Stop(loop);
        return;
    }
    params.vol[0] = left < 0x4000 ? left : 0x3fff;
    params.vol[1] = right < 0x4000 ? right : 0x3fff;
    p = (ushort)Spu_NoteToPitch(loop->sample->low << 8, loop->sample->root) * pitch >> 12;
    if (loop->voice >= 0) {
        if (priority >= 0) {
            SfxVoice_SetPriority(&loop->voice, priority);
        }
        SfxVoice_SetPitch(&loop->voice, p);
        SfxVoice_SetVolume(&loop->voice, params.vol);
        return;
    }
    params.pitch = p;
    params.duration = loop->sample->duration;
    params.adsr = loop->sample->adsr;
    params.addr = loop->sample->addr;
    params.priority = priority >= 0 ? priority : loop->sample->priority;
    params.flags = loop->sample->flags;
    SfxVoice_Start(&loop->voice, &params);
}

void SfxLoop_Stop(SfxLoop *loop)
{
    SfxVoice_KeyOff(&loop->voice);
    SfxVoice_Release(&loop->voice);
}

// Sets bank up to load into size bytes at ram, its samples at the next free SPU
// address.
void SpuBank_Init(SpuBank *bank, SeqBank *ram, int size)
{
    int spu;

    spu = g_SpuAllocPtr;
    bank->ram = ram;
    bank->size = size;
    bank->spu = spu;
}

// Frees the SPU memory from bank's samples on.
void SpuAlloc_Restore(SpuBank *bank)
{
    SpuAlloc_SetPtr(bank->spu);
}

// Loads boot file id (or -id) into the bank, its samples to the next free SPU
// address, which it moves past them. Returns the bytes read into RAM.
int SpuBank_Load(SpuBank *bank, int id, int isEngine, int negId)
{
    int (*load)(int, SeqBank *, int, int);
    int n;

    if (negId != 0) {
        load = SpuFile_LoadNegId;
    } else {
        load = SpuFile_Load;
    }
    n = load(id, bank->ram, bank->spu, isEngine);
    g_SpuAllocPtr += (bank->ram->spuSize + 0x1f) & ~0xf;
    return n;
}

// SpuBank_Load into the rest of the pool (SpuBankPool_Set), which it moves past
// the file.
void SpuBank_LoadFromPool(SpuBank *bank, int id, int isEngine, int negId)
{
    uchar *ptr;
    int size;
    int n;

    ptr = (uchar *)g_SpuBankPoolPtr;
    size = g_SpuBankPoolSize;
    SpuBank_Init(bank, (SeqBank *)ptr, size);
    n = SpuBank_Load(bank, id, isEngine, negId);
    g_SpuBankPoolPtr = ptr + n;
    g_SpuBankPoolSize = size - n;
}

SeqRegion *SoundBank_GetSample(SpuBank *bank, int index)
{
    return &bank->ram->regions[index];
}

// SoundSample_Play for sample index of the bank.
void SoundBank_Play(SpuBank *bank, int index, int pan, int volume, int voice)
{
    SeqRegion *sample;

    sample = SoundBank_GetSample(bank, index);
    SoundSample_Play(sample, pan, volume, voice);
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

// Relocates an instrument file loaded at bank, with its samples at SPU address spu
// (0 to leave them). Returns bank, or 0 if it isn't one.
SeqBank *Inst_Relocate(SeqBank *bank, int spu)
{
    unsigned int i;
    int delta;
    int old;

    if (bank->magic != 0x54534e49) {
        return 0;
    }
    i = 0;
    delta = (int)bank - bank->base;
    bank->base = (int)bank;
    *(int *)&bank->unk14 += delta;
    *(int *)&bank->regions += delta;
    *(int *)&bank->programs += delta;
    for (i = 0; i < bank->programCount; i++) {
        *(int *)&bank->programs[i] += delta;
    }
    if (spu != 0) {
        old = bank->spu;
        bank->spu = spu;
        spu -= old;
        for (i = 0; i < bank->regionCount; i++) {
            bank->regions[i].addr += (unsigned int)spu >> 3;
        }
    }
    return bank;
}

// Sets voice i's pitch for rpm in its range; smooth keeps it within half to twice
// the last one.
void EngineSound_CalcPitch(EngineSound *es, int i, int rpm, int smooth)
{
    EngnRange *r;
    unsigned int pitch;

    r = &es->ranges[es->voices[i].range];
    pitch = (unsigned int)(r->pitch * rpm) / r->rpm;
    pitch = pitch * es->pitchScale >> 12;
    if (pitch == 0) {
        pitch = 1;
    }
    if (smooth != 0) {
        if (es->voices[i].pitch * 2 < pitch) {
            pitch = es->voices[i].pitch * 2;
        } else if (pitch < es->voices[i].pitch >> 1) {
            pitch = es->voices[i].pitch >> 1;
        }
    }
    es->voices[i].pitch = pitch;
}

// Sets voice i's volume pair for rpm: full in its range, faded on a sine towards
// the other voice's range between the two; smooth moves it at most 0x1000.
void EngineSound_CalcVolume(EngineSound *es, int i, int rpm, int smooth)
{
    EngnRange *r;
    EngnRange *o;
    int other;
    int fade; // 0x4000 = full, then the volume
    int left;
    int right;
    int curL;
    int curR;

    r = &es->ranges[es->voices[i].range];
    fade = 0x4000;
    other = es->voices[(i + 1) & 1].range;
    if (other >= 0) {
        o = &es->ranges[other];
        if (rpm < r->lo) {
            fade = ((rpm - o->hi) << 14) / (r->lo - o->hi);
        } else if (r->hi < rpm) {
            fade = ((o->lo - rpm) << 14) / (o->lo - r->hi);
        }
    }
    fade = g_SinTable[((unsigned int)fade >> 4) & 0xfff] * 0x3fff >> 12;
    fade = (unsigned int)(fade * r->vol) >> 14;
    left = (unsigned int)(es->volL * fade) >> 14;
    right = (unsigned int)(es->volR * fade) >> 14;
    if (smooth != 0) {
        curL = es->voices[i].vol[0];
        curR = es->voices[i].vol[1];
        if (curL + 0x1000 < left) {
            left = curL + 0x1000;
        } else if (left < curL - 0x1000) {
            left = curL - 0x1000;
        }
        if (curR + 0x1000 < right) {
            right = curR + 0x1000;
        } else if (right < curR - 0x1000) {
            right = curR - 0x1000;
        }
    }
    es->voices[i].vol[0] = left;
    es->voices[i].vol[1] = right;
}

// Plays the engine at es->rpm: picks the range it is in (voice 0), or the two it is
// between, and starts, moves or stops the two voices to match.
void EngineSound_Update(EngineSound *es)
{
    int rpm;
    int i;
    EngnRange *r;
    EngnRange *prev;
    int t;
    SfxParams params;

    rpm = es->rpm;
    if (rpm < 200) {
        rpm = 200;
    }
    prev = 0;
    r = es->ranges;
    for (i = 0; i < es->count; i++, r++) {
        if (r->lo <= rpm && rpm <= r->hi) {
            es->voices[0].range = i;
            es->voices[1].range = -1;
            break;
        }
        if (prev != 0 && prev->hi < rpm && rpm < r->lo) {
            es->voices[0].range = i - 1;
            es->voices[1].range = i;
            break;
        }
        prev = r;
    }
    t = es->voices[0].range;
    if (t == es->voices[1].prevRange || es->voices[1].range == es->voices[0].prevRange) {
        es->voices[0].range = es->voices[1].range;
        es->voices[1].range = t;
    }
    for (i = 0; i < 2; i++) {
        if (es->voices[i].range == es->voices[i].prevRange) {
            if (es->voices[i].voice >= 0) {
                if (es->voices[i].range >= 0) {
                    EngineSound_CalcPitch(es, i, rpm, 1);
                    EngineSound_CalcVolume(es, i, rpm, 1);
                    SfxVoice_SetPitch(&es->voices[i].voice, es->voices[i].pitch);
                    SfxVoice_SetVolume(&es->voices[i].voice, es->voices[i].vol);
                }
                continue;
            }
        } else if (es->voices[i].voice >= 0) {
            SfxVoice_KeyOff(&es->voices[i].voice);
            SfxVoice_Release(&es->voices[i].voice);
        }
        if (es->voices[i].range >= 0) {
            EngineSound_CalcPitch(es, i, rpm, 0);
            EngineSound_CalcVolume(es, i, rpm, 0);
            es->voices[i].addr = es->ranges[es->voices[i].range].addr >> 3;
            params.volPair = es->voices[i].volPair;
            params.pitch = es->voices[i].pitch;
            params.addr = es->voices[i].addr;
            params.adsr = 0x3000f;
            params.priority = es->priority;
            params.duration = 0;
            params.flags = 1;
            SfxVoice_Start(&es->voices[i].voice, &params);
        }
        es->voices[i].prevRange = es->voices[i].range;
    }
}

// Sets es up to play set n of the ENGN file at priority, silent until its first
// update.
void EngineSound_Init(EngineSound *es, EngnFile *file, int n, int priority)
{
    int i;

    es->priority = priority;
    es->count = file->sets[n].count;
    es->ranges = file->sets[n].ranges;
    es->pitchScale = 0x1000;
    es->volL = 0x3fff;
    es->volR = 0x3fff;
    es->unk06 = 0x4000;
    for (i = 0; i < 2; i++) {
        es->voices[i].voice = -1;
        es->voices[i].range = -1;
        es->voices[i].prevRange = -1;
    }
}

void EngineSound_Stop(EngineSound *es)
{
    int i;

    for (i = 0; i < 2; i++) {
        if (es->voices[i].voice >= 0) {
            SfxVoice_KeyOff(&es->voices[i].voice);
            SfxVoice_Release(&es->voices[i].voice);
        }
    }
}

void EngineSound_SetPriority(EngineSound *es, uchar priority)
{
    int i;

    es->priority = priority;
    for (i = 0; i < 2; i++) {
        if (es->voices[i].voice >= 0) {
            SfxVoice_SetPriority(&es->voices[i].voice, priority);
        }
    }
}

// Relocates an engine sound file loaded at file, with its samples at SPU address
// spu (0 to leave them). Returns file, or 0 if it isn't one.
EngnFile *Engn_Relocate(EngnFile *file, int spu)
{
    int i;
    unsigned int j;
    int delta;
    int old;
    EngnRange *r;

    if (file->magic != 0x4e474e45) {
        return 0;
    }
    delta = (int)file - file->base;
    file->base = (int)file;
    *(int *)&file->unk14 += delta;
    for (i = 0; i < 2; i++) {
        if (file->sets[i].ranges != 0) {
            file->sets[i].ranges = (EngnRange *)((int)file->sets[i].ranges + delta);
        }
    }
    if (spu != 0) {
        old = file->spu;
        file->spu = spu;
        spu -= old;
        for (i = 0; i < 2; i++) {
            r = file->sets[i].ranges;
            for (j = 0; j < file->sets[i].count; j++) {
                r->addr += spu;
                r++;
            }
        }
    }
    return file;
}
}
