// Resident code 80079140-8007ab14 (seq). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"
#include "common.h"

extern "C" {
struct SeqSong {
    ushort vol;
    ushort pad02;
    int tempo;
    uchar *tracks[16];
};

// A sequence file ("SEQG"): its songs, with the track pointers as offsets from base.
struct SeqFile {
    int magic;
    int base;
    unsigned int count;
    SeqSong songs[1];
};

void FUN_8007a7cc(SeqFade *f, int from, int to, int ms);
void SeqPlayer_ApplyVolume(SeqPlayer *p);
void SeqPlayer_Link(SeqPlayer *p);
void SeqPlayer_Unlink(SeqPlayer *p);
void SeqPlayer_RunEvents(SeqPlayer *p);
void SeqChannel_NoteOn(SeqPlayer *p, SeqTrack *t, int note, int velocity, int duration);
void SeqChannel_UpdateAll(void);
int SeqFade_Step(SeqFade *f);
void SfxVoices_Update(void);
int SfxVoice_Alloc(int priority);
void SfxVoice_Setup(int voice, SfxParams *params);
void SfxVoice_Start(signed char *handle, SfxParams *params);
void SfxVoice_SetPitch(signed char *handle, ushort pitch);
void SfxVoice_SetVolume(signed char *handle, short *vol);
void SfxVoice_KeyOff(signed char *handle);
void SfxVoice_Release(signed char *handle);
int Spu_NoteToPitch(int note, int base);
void Spu_PanVolume(short *out, int vol, int pan);
void Sound_SetTickRate(int hz);
void Seq_SetTickRate(unsigned short hz);
extern ushort D_8008FDB8[]; // pitch of each 1/8 semitone in an octave
extern ushort D_800901B8[128]; // pan volume curve
extern ushort D_800900B8[128]; // velocity volume curve
extern ushort g_ReverbPresets[10][32]; // the SPU reverb registers 0x1f801dc0-0x1f801dfe
extern ushort g_ReverbWorkAddr[10]; // work area start / 8
extern SeqState g_Seq;
extern SeqChannel g_SeqChannels[25];
extern unsigned short g_SeqMasterVolume;
extern unsigned short g_SeqTickRate;
extern int g_SeqReady;
extern SoundDriver g_SoundDriver;
extern int D_80092E80; // SfxVoices_Update count
extern unsigned int D_80092E84; // voices keyed on
extern unsigned int g_SoundDriverReady;

void Sound_Tick(void)
{
    if (g_SoundDriverReady != 0) {
        SfxVoices_Update();
    }
}

// The vsync part of the sound driver: keys the voices on and off, writes the
// registers the main code changed, and stops the voices whose envelope or time is up.
void SfxVoices_Update(void)
{
    SoundDriver *drv;
    ushort *voiceRegs;
    ushort *reg;
    SfxVoice *v;
    unsigned int off;
    unsigned int dirty;
    unsigned int on;
    unsigned int bit;
    unsigned int mask;
    int flags;
    int left;
    int right;
    int adsr1;
    int adsr2;
    int addr;
    int i;

    dirty = 0;
    off = 0;
    on = 0;
    drv = &g_SoundDriver;
    v = drv->voices;
    bit = 1;
    voiceRegs = (ushort *)0x1f801c00;
    D_80092E80++;
    for (i = 0; i < 24; i++, v++, bit <<= 1) {
        if (v->lock != 2) {
            if (v->playing != 0 && v->keyOn == 0) {
                if (voiceRegs[i * 8 + 6] == 0 || v->elapsed > v->duration) {
                    v->playing = 0;
                    v->keyOff = 1;
                    if (v->state == 1) {
                        if (v->owner != 0) {
                            *v->owner = -1;
                        }
                        v->owner = 0;
                        v->state = 0;
                    }
                }
            }
            if (v->keyOff != 0) {
                if (v->keyedOn != 0) {
                    off |= bit;
                    v->keyedOn = 0;
                }
                v->keyOff = 0;
            }
            if (v->dirty != 0) {
                dirty |= bit;
            }
            if (v->keyOn != 0) {
                if (v->keyedOn != 0) {
                    off |= bit;
                }
                on |= bit;
                v->keyedOn = 1;
                v->playing = 1;
                v->keyOn = 0;
            }
        }
        if (v->playing != 0) {
            v->age++;
            if (v->duration != 0) {
                v->elapsed += voiceRegs[i * 8 + 2];
            }
        }
    }
    if (off != 0) {
        ushort *spu = (ushort *)0x1f801c00;

        spu[0xc7] = off >> 16;
        spu[0xc6] = off;
        D_80092E84 &= ~off;
    }
    reg = (ushort *)0x1f801c00;
    if (dirty != 0) {
        v = drv->voices;
        do {
            if (dirty & 1) {
                flags = v->dirty;
                v->dirty = 0;
                if (flags & 1) {
                    reg[2] = v->pitch;
                }
                if (flags & 2) {
                    left = v->volL & 0x7fff;
                    right = v->volR & 0x7fff;
                    reg[0] = left;
                    reg[1] = right;
                }
                if (flags & 4) {
                    adsr1 = v->adsr1;
                    adsr2 = v->adsr2;
                    addr = v->addr;
                    reg[4] = adsr1;
                    reg[5] = adsr2;
                    reg[3] = addr;
                }
            }
            reg += 8;
            dirty >>= 1;
            v++;
        } while (dirty != 0);
    }
    if (on != 0) {
        ushort *spu = (ushort *)0x1f801c00;

        spu[0xc4] = on;
        spu[0xc5] = on >> 16;
        D_80092E84 |= on;
    }
    if (drv->reverbDirty != 0) {
        mask = 0;
        v = drv->voices;
        bit = 1;
        for (i = 0; i < 24; i++, v++, bit <<= 1) {
            if (v->reverb != 0) {
                mask |= bit;
            }
        }
        ushort *spu = (ushort *)0x1f801c00;

        spu[0xcc] = mask;
        spu[0xcd] = mask >> 16;
        drv->reverbDirty = 0;
    }
}

// Plays note (in semitones) with velocity on track t of p, on a free channel for each
// of the program's regions that covers it; duration is in ticks from this frame's.
#ifdef NON_MATCHING
// Registers: the game keeps the bank and the program's volume on the stack and the
// player in a saved register, and allocates the rest differently.
void SeqChannel_NoteOn(SeqPlayer *p, SeqTrack *t, int note, int velocity, int duration)
{
    SeqBank *bank;
    SeqProgram *prog;
    SeqRegion *r;
    SeqChannel *c;
    SfxParams params;
    int vol;
    int pan;
    int bend;
    int pitch;
    int rvol;
    int idx;
    int n;
    int i;

    bank = p->instBank;
    prog = bank->programs[t->program];
    vol = (t->effVol * prog->vol) >> 14;
    vol = (vol * D_800900B8[velocity]) >> 14;
    pan = t->pan + prog->pan;
    for (i = 0; i < prog->count; i++) {
        r = &bank->regions[prog->regions[i]];
        if (note < r->low || note > r->high) {
            continue;
        }
        idx = g_Seq.nextChannel;
        c = &g_SeqChannels[idx];
        for (n = 0; n < 25; n++) {
            idx++;
            c++;
            if (idx == 25) {
                idx = 0;
                c = g_SeqChannels;
            }
            if (c->spuVoice < 0) {
                break;
            }
        }
        if (n == 25) {
            continue;
        }
        rvol = (vol * r->vol) >> 14;
        pitch = note << 8;
        bend = t->pitchBend;
        g_Seq.nextChannel = idx;
        n = pan + r->pan - 0x80;
        if (n < 0) {
            n = 0;
        } else if (n >= 0x80) {
            n = 0x7f;
        }
        c->player = p;
        c->track = t;
        c->program = prog;
        c->region = r;
        c->lastBend = bend;
        c->note = note;
        c->velocity = velocity;
        c->releaseTime = duration + p->ticksThisFrame;
        c->lastVol = t->effVol;
        c->lastPan = t->pan;
        params.adsr = r->adsr;
        params.addr = r->addr;
        params.priority = r->priority;
        params.duration = r->duration;
        params.flags = r->flags;
        if (bend != 0) {
            if (bend > 0) {
                pitch += (bend * r->bendUp) >> 5;
            } else {
                pitch += (bend * r->bendDown) >> 5;
            }
        }
        params.pitch = Spu_NoteToPitch(pitch, r->root);
        Spu_PanVolume(params.vol, (rvol * rvol) >> 14, n);
        SfxVoice_Start(&c->spuVoice, &params);
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqChannel_NoteOn);
#endif

// Runs the events due on each of p's tracks this frame. An event byte with bit 7 set
// is a note (then the velocity and the duration); bit 6 a pitch bend (14 bits);
// bit 5 a loop start with a count; 0 the end of the track, 1 a loop start with the
// count in the next byte, 2 a loop end, 3 the program, 4 the volume, 5 the pan, 6 the
// tempo (3 bytes). Each event is followed by the delta to the next.
#ifdef NON_MATCHING
// Registers: the game keeps the delta bytes in other registers and steps the pointer
// first in their loops, and doesn't merge the two loop starts' common tail. Its
// switch's jump table would also need this file's .rodata subsegment (0x8008fd98).
void SeqPlayer_RunEvents(SeqPlayer *p)
{
    SeqTrack *t;
    uchar *ptr;
    uchar count;
    int ev;
    int vel;
    int len;
    int c;
    int d;
    int i;

    for (i = 0, t = p->tracks; i < 16; i++, t++) {
        ptr = t->ptr;
        if (ptr == 0) {
            continue;
        }
        t->delta -= p->ticksThisFrame;
        if (t->delta <= 0) {
            do {
                ev = *ptr++;
                if (ev & 0x80) {
                    vel = *ptr++;
                    ev &= 0x7f;
                    len = *ptr++;
                    if (len & 0x80) {
                        len &= 0x7f;
                        do {
                            c = *ptr++;
                            len <<= 7;
                            len += c & 0x7f;
                        } while (c & 0x80);
                    }
                    SeqChannel_NoteOn(p, t, ev, vel, len + p->ticksThisFrame + t->delta);
                } else if (ev & 0x40) {
                    t->pitchBend = (((ev & 0x3f) << 8) | *ptr++) - 0x2000;
                } else if (ev & 0x20) {
                    c = (ev & 0x1f) + 1;
                    d = t->loopDepth + 1;
                    t->loopCount[d] = c;
                    t->loopStart[d] = ptr;
                    t->loopDepth = d;
                } else {
                    switch (ev) {
                    case 0:
                        ptr = 0;
                        goto next;
                    case 1:
                        d = t->loopDepth + 1;
                        t->loopCount[d] = *ptr++;
                        t->loopStart[d] = ptr;
                        t->loopDepth = d;
                        break;
                    case 2:
                        count = t->loopCount[t->loopDepth];
                        if (count != 0) {
                            if (count != 0xff) {
                                t->loopCount[t->loopDepth] = count - 1;
                            }
                            ptr = t->loopStart[t->loopDepth];
                        } else {
                            t->loopDepth--;
                        }
                        break;
                    case 3:
                        t->program = *ptr++;
                        break;
                    case 4:
                        t->trackVol = D_800900B8[*ptr++];
                        t->effVol = (p->vol * t->trackVol) >> 14;
                        break;
                    case 5:
                        t->pan = *ptr++;
                        break;
                    case 6:
                        p->tempo = (ptr[0] << 16) | (ptr[1] << 8) | ptr[2];
                        ptr += 3;
                        break;
                    }
                }
                ev = *ptr++;
                if (ev & 0x80) {
                    ev &= 0x7f;
                    do {
                        c = *ptr++;
                        ev <<= 7;
                        ev += c & 0x7f;
                    } while (c & 0x80);
                }
                t->delta += ev;
            } while (t->delta <= 0);
        }
    next:
        t->ptr = ptr;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqPlayer_RunEvents);
#endif

// The vsync part of the sequencer: runs each player's events for the ticks due,
// steps the fades, then updates the channels.
void Seq_Tick(void)
{
    SeqPlayer *p;

    if (g_SoundDriverReady != 0 && g_SeqReady != 0) {
        for (p = g_Seq.players; p != 0; p = p->next) {
            p->ticksThisFrame = 0;
            while (p->tickAcc > 0) {
                p->ticksThisFrame++;
                p->tickAcc -= p->tempo;
            }
            p->tickAcc += p->tickPeriod;
            SeqPlayer_RunEvents(p);
            if (p->fade.count != 0) {
                p->baseVol += SeqFade_Step(&p->fade);
                SeqPlayer_ApplyVolume(p);
            }
        }
        if (g_Seq.masterFade.count != 0) {
            g_Seq.masterVol += SeqFade_Step(&g_Seq.masterFade);
            for (p = g_Seq.players; p != 0; p = p->next) {
                SeqPlayer_ApplyVolume(p);
            }
        }
        SeqChannel_UpdateAll();
    }
}

// Follows each sounding channel's track: stops it when its player has gone, keys it
// off when its time is up, and sets its pitch and volume when the track's bend,
// volume or pan changed.
#ifdef NON_MATCHING
// Registers: the game keeps the volume pair's stack address in a saved register
// and so has one more, which shifts the temporaries' registers.
void SeqChannel_UpdateAll(void)
{
    SeqChannel *c;
    SeqProgram *prog;
    SeqRegion *r;
    SeqTrack *t;
    short vol[2];
    int v;
    int pan;
    int i;

    c = g_SeqChannels;
    for (i = 0; i < 25; i++, c++) {
        if (c->spuVoice < 0) {
            continue;
        }
        if (c->player == 0) {
            SfxVoice_KeyOff(&c->spuVoice);
            SfxVoice_Release(&c->spuVoice);
            continue;
        }
        if (c->releaseTime > 0) {
            c->releaseTime -= c->player->ticksThisFrame;
            if (c->releaseTime <= 0) {
                SfxVoice_KeyOff(&c->spuVoice);
            }
        }
        if (c->track->pitchBend != c->lastBend) {
            r = c->region;
            if (c->track->pitchBend > 0) {
                v = r->bendUp;
            } else {
                v = r->bendDown;
            }
            SfxVoice_SetPitch(&c->spuVoice,
                              Spu_NoteToPitch((c->note << 8) + ((v * c->track->pitchBend) >> 5), r->root));
            c->lastBend = c->track->pitchBend;
        }
        t = c->track;
        if (t->effVol != c->lastVol || t->pan != c->lastPan) {
            prog = c->program;
            v = (t->effVol * prog->vol) >> 14;
            v = (v * D_800900B8[c->velocity]) >> 14;
            r = c->region;
            v = (v * r->vol) >> 14;
            pan = r->pan + t->pan + prog->pan - 0x80;
            if (pan < 0) {
                pan = 0;
            } else if (pan >= 0x80) {
                pan = 0x7f;
            }
            Spu_PanVolume(vol, (v * v) >> 14, pan);
            SfxVoice_SetVolume(&c->spuVoice, vol);
            c->lastVol = c->track->effVol;
            c->lastPan = c->track->pan;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqChannel_UpdateAll);
#endif

void Sound_Init(void)
{
    volatile short *spu;

    if (g_SoundDriverReady == 0) {
        spu = (volatile short *)0x1f801c00;
        spu[0xc0] = 0x3fff;
        spu[0xc1] = 0x3fff;
        memset(&g_SoundDriver, 0, sizeof(SoundDriver));
        g_SoundDriver.nextVoice = 23;
        Sound_SetTickRate(60);
        g_SoundDriverReady = 1;
    }
}

void Sound_SetTickRate(int hz)
{
    g_SoundDriver.tickRate = hz;
    g_SoundDriver.tickScale = (hz << 8) / 60;
}

void Seq_Init(void)
{
    SeqChannel *c;
    int i;

    if (g_SeqReady == 0) {
        memset(&g_Seq, 0, sizeof(SeqState));
        g_Seq.players = 0;
        g_Seq.masterVol = 0x3fff;
        g_Seq.nextChannel = 0;
        c = g_Seq.channels;
        for (i = 0; i < 25; i++) {
            c->spuVoice = -1;
            c++;
        }
        Seq_SetTickRate(60);
        g_SeqReady = 1;
    }
}

void Seq_SetTickRate(unsigned short hz)
{
    g_SeqTickRate = hz;
}

// PsyQ's stdarg.h: va_start takes the last parameter's address.
#define va_start(ap, last) (ap = (char *)(&(last) + 1))
#define va_arg(ap, type) (((type *)(ap += sizeof(type)))[-1])

// Sets up the SPU reverb as preset type (its registers and work area start), and returns
// the work area's size. Types 7 (echo) and 8 (delay) take a delay (0-127) after type, and
// echo a feedback after that. Reverb is off while the registers change.
#ifdef NON_MATCHING
// The game copies the registers through two pointers each for the preset and the SPU
// (one for the first word, one for the other three) and so has one register more;
// GCC uses one each, which changes every register after. Its four-word body, word
// loads and store order are kept.
int Spu_SetReverbType(int mode, ...)
{
    char *ap;
    volatile ushort *spu = (volatile ushort *)0x1f801c00;
    ushort *preset;
    unsigned int *src;
    volatile ushort *dst;
    int delay;
    int n;
    unsigned int w0;
    unsigned int w1;
    unsigned int w2;
    unsigned int w3;
    int type;

    va_start(ap, mode);
    type = mode & 0xff;
    if (type == 7 || type == 8) {
        delay = (unsigned int)(va_arg(ap, int) << 13) / 127;
        preset = g_ReverbPresets[type];
        preset[10] = delay - preset[0];
        delay >>= 1;
        preset[11] = delay - preset[1];
        preset[12] = preset[13] + delay;
        preset[16] = preset[17] + delay;
        preset[26] = preset[28] + delay;
        preset[27] = preset[29] + delay;
    }
    if (type == 7) {
        g_ReverbPresets[7][7] = (va_arg(ap, int) * 0x8100) / 127;
    }
    spu[0xc2] = 0;
    spu[0xc3] = 0;
    spu[0xd5] &= 0xff7f;
    spu[0xd1] = g_ReverbWorkAddr[type];
    src = (unsigned int *)g_ReverbPresets[type];
    dst = &spu[0xe0];
    n = 4;
    while (n-- != 0) {
        w0 = src[0];
        w1 = src[1];
        w2 = src[2];
        w3 = src[3];
        src += 4;
        dst[0] = w0;
        dst[2] = w1;
        dst[4] = w2;
        dst[6] = w3;
        dst[1] = w0 >> 16;
        dst[3] = w1 >> 16;
        dst[5] = w2 >> 16;
        dst[7] = w3 >> 16;
        dst += 8;
    }
    g_SoundDriver.reverbOn = 1;
    spu[0xd5] |= 0x80;
    return (0x10000 - g_ReverbWorkAddr[type]) << 3;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", Spu_SetReverbType);
#endif

void Spu_EnableReverb(int on)
{
    volatile ushort *spu = (volatile ushort *)0x1f801c00;

    spu[0xd5] = (spu[0xd5] & 0xff7f) | ((on & 1) << 7);
}

void Spu_SetReverbDepth(short left, short right)
{
    *(short *)0x1f801d84 = left;
    *(short *)0x1f801d86 = right;
}

// Turns reverb on or off for the voice *handle, if it still has it.
#ifdef NON_MATCHING
// Handwritten, like the other functions that take a voice handle: it keeps its
// temporaries in $t registers and loads constants with both addiu and ori.
void SfxVoice_SetReverb(signed char *handle, int on)
{
    SfxVoice *v;
    uchar lock;

    if (*handle < 0) {
        return;
    }
    v = &g_SoundDriver.voices[*handle];
    lock = v->lock;
    v->lock = 1;
    if (v->owner == handle) {
        v->reverb = on;
        g_SoundDriver.reverbDirty = 1;
    }
    v->lock = lock;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_SetReverb);
#endif

void Spu_PanVolume(short *out, int vol, int pan)
{
    int left;
    int right;

    left = vol * D_800901B8[127 - pan];
    right = vol * D_800901B8[pan];
    out[0] = left >> 14;
    out[1] = right >> 14;
}

// Makes file's track pointers point into it where it is now. Returns file, or 0 if it
// isn't a sequence file.
SeqFile *SeqFile_Relocate(SeqFile *file)
{
    unsigned int i;
    int j;
    int delta;

    if (file->magic != 0x47514553) {
        return 0;
    }
    delta = (int)file - file->base;
    file->base = (int)file;
    for (i = 0; i < file->count; i++) {
        for (j = 0; j < 16; j++) {
            if (file->songs[i].tracks[j] != 0) {
                // Added as an int: the pointer is read again for it.
                *(int *)&file->songs[i].tracks[j] += delta;
            }
        }
    }
    return file;
}

void Seq_SetMasterVolume(int vol)
{
    SeqPlayer *p;

    p = g_Seq.players;
    g_Seq.masterVol = vol;
    while (p != 0) {
        SeqPlayer_ApplyVolume(p);
        p = p->next;
    }
}

void SeqPlayer_Unlink(SeqPlayer *p)
{
    SeqPlayer *it;
    SeqPlayer *prev;

    it = g_Seq.players;
    prev = 0;
    while (it != 0) {
        if (it == p) {
            if (prev != 0) {
                prev->next = it->next;
            } else {
                g_Seq.players = it->next;
            }
            return;
        }
        prev = it;
        it = it->next;
    }
}

// Sets p up to play song with the instruments in bank, from the start, and unlinks it
// (SeqPlayer_Start links it).
#ifdef NON_MATCHING
// 1 word: the game stores the stepped track pointer after the first delta byte (in the
// branch's delay slot), GCC before it.
void SeqPlayer_Setup(SeqPlayer *p, SeqSong *song, SeqBank *bank)
{
    SeqTrack *t;
    int c;
    int i;

    SeqPlayer_Unlink(p);
    p->next = 0;
    p->baseVol = song->vol;
    p->vol = (g_Seq.masterVol * song->vol) >> 14;
    p->instBank = bank;
    p->tickAcc = p->tickPeriod = 480000000 / g_Seq.tickRate;
    p->tempo = song->tempo;
    for (i = 0; i < 16; i++) {
        t = &p->tracks[i];
        t->ptr = song->tracks[i];
        if ((t->delta = *t->ptr++) & 0x80) {
            t->delta &= 0x7f;
            do {
                t->delta = (t->delta << 7) + ((c = *t->ptr++) & 0x7f);
            } while (c & 0x80);
        }
        t->trackVol = 0x4000;
        t->effVol = p->vol;
        t->pitchBend = 0;
        t->pan = 0;
        t->program = 0;
        t->loopDepth = -1;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SeqPlayer_Setup);
#endif

void SeqPlayer_Link(SeqPlayer *p)
{
    p->next = g_Seq.players;
    g_Seq.players = p;
}

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

// The SPU pitch for note (in 1/256 semitones) on a sample whose root is base.
int Spu_NoteToPitch(int note, int base)
{
    int octave;
    ushort pitch;

    octave = -2;
    note -= base;
    if (note < 0) {
        do {
            note += 0xc00;
            octave--;
        } while (note < 0);
    } else {
        while (note >= 0xc00) {
            note -= 0xc00;
            octave++;
        }
    }
    pitch = D_8008FDB8[(unsigned int)note >> 3];
    if (octave <= 0) {
        return (ushort)(pitch >> -octave);
    }
    return (ushort)(pitch << octave);
}

void SfxVoice_Start(signed char *handle, SfxParams *params)
{
    int voice;

    voice = SfxVoice_Alloc(params->priority);
    if (handle != 0) {
        *handle = voice;
    }
    if (voice >= 0) {
        g_SoundDriver.voices[voice].state = 1;
        g_SoundDriver.voices[voice].owner = handle;
        SfxVoice_Setup(voice, params);
    }
}

// Sets the priority of the voice *handle, if it still has it.
#ifdef NON_MATCHING
// Handwritten, like the other functions that take a voice handle: it keeps its
// temporaries in $t registers and loads constants with both addiu and ori.
void SfxVoice_SetPriority(signed char *handle, int priority)
{
    SfxVoice *v;
    uchar lock;

    if (*handle < 0) {
        return;
    }
    v = &g_SoundDriver.voices[*handle];
    lock = v->lock;
    v->lock = 1;
    if (v->owner == handle) {
        v->priority = priority;
    }
    v->lock = lock;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_SetPriority);
#endif

// Sets the pitch of the voice *handle, if it still has it.
#ifdef NON_MATCHING
// Handwritten, like the other functions that take a voice handle: it keeps its
// temporaries in $t registers and loads constants with both addiu and ori.
void SfxVoice_SetPitch(signed char *handle, ushort pitch)
{
    SfxVoice *v;
    uchar lock;

    if (*handle < 0) {
        return;
    }
    v = &g_SoundDriver.voices[*handle];
    lock = v->lock;
    v->lock = 1;
    if (v->owner == handle) {
        v->lock = 2;
        v->pitch = pitch;
        v->dirty |= 1;
    }
    v->lock = lock;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_SetPitch);
#endif

// Sets the volume pair (left, right) of the voice *handle, if it still has it.
#ifdef NON_MATCHING
// Handwritten, like the other functions that take a voice handle: it keeps its
// temporaries in $t registers and loads constants with both addiu and ori.
void SfxVoice_SetVolume(signed char *handle, short *vol)
{
    SfxVoice *v;
    uchar lock;

    if (*handle < 0) {
        return;
    }
    v = &g_SoundDriver.voices[*handle];
    lock = v->lock;
    v->lock = 1;
    if (v->owner == handle) {
        v->lock = 2;
        v->volL = vol[0];
        v->volR = vol[1];
        v->dirty |= 2;
    }
    v->lock = lock;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_SetVolume);
#endif

// Keys off the voice *handle, if it still has it.
#ifdef NON_MATCHING
// Handwritten, like the other functions that take a voice handle: it keeps its
// temporaries in $t registers and loads constants with both addiu and ori.
void SfxVoice_KeyOff(signed char *handle)
{
    SfxVoice *v;
    uchar lock;

    if (*handle < 0) {
        return;
    }
    v = &g_SoundDriver.voices[*handle];
    lock = v->lock;
    v->lock = 1;
    if (v->owner == handle) {
        v->keyOn = 0;
        v->keyOff = 1;
    }
    v->lock = lock;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_KeyOff);
#endif

// Gives up the voice *handle (it plays on, ownerless) and sets *handle to -1.
#ifdef NON_MATCHING
// Handwritten, like the other functions that take a voice handle: it keeps its
// temporaries in $t registers and loads constants with both addiu and ori.
void SfxVoice_Release(signed char *handle)
{
    SfxVoice *v;
    uchar lock;

    if (*handle < 0) {
        return;
    }
    v = &g_SoundDriver.voices[*handle];
    lock = v->lock;
    v->lock = 1;
    if (v->owner == handle) {
        *handle = -1;
        v->owner = 0;
    }
    v->lock = lock;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_Release);
#endif

// Sets f to go from from to to over ms milliseconds: step a tick, and the remainder
// spread Bresenham style (rate taken from acc, extra added when it runs out).
void FUN_8007a7cc(SeqFade *f, int from, int to, int ms)
{
    int delta;
    int neg;
    int ticks;
    int step;
    int extra;

    delta = to - from;
    if (delta == 0) {
        f->count = 0;
        return;
    }
    neg = 0;
    if (delta <= 0) {
        neg = 1;
        delta = -delta;
    }
    ticks = ms * g_SeqTickRate / 1000;
    if (ticks == 0) {
        ticks = 1;
    }
    step = delta / ticks;
    delta = delta % ticks;
    extra = delta != 0;
    if (neg) {
        step = -step;
        extra = -extra;
    }
    f->step = step;
    f->extra = extra;
    f->rate = delta;
    f->reload = ticks;
    f->acc = ticks >> 1;
    f->count = ticks;
}

// One tick of f: the amount to add, 0 once it has run out.
int SeqFade_Step(SeqFade *f)
{
    int v;

    if (f->count == 0) {
        return 0;
    }
    f->count--;
    v = f->step;
    if (f->extra != 0) {
        f->acc -= f->rate;
        // The game tests rate, not acc, so extra is never added (rate is never negative).
        if (f->rate < 0) {
            v += f->extra;
            f->acc += f->reload;
        }
    }
    return v;
}

void FUN_8007a8cc(int param_1, unsigned short param_2, unsigned int param_3)
{
    *(unsigned short *)(param_1 + 0x1c) = 0;
    FUN_8007a7cc((SeqFade *)(param_1 + 0x1c), *(unsigned short *)(param_1 + 4), param_2, param_3);
}

// Fades the master volume to vol over ms milliseconds.
void FUN_8007a900(unsigned short vol, int ms)
{
    g_Seq.masterFade.count = 0;
    FUN_8007a7cc(&g_Seq.masterFade, g_Seq.masterVol, vol, ms);
}

// Finds a voice for a sound of the given priority, from the one after the last
// found: a free one, or else the least important started one of at least that
// priority (keyed off and taken from its owner). Returns its index with the voice
// locked, or -1.
#ifdef NON_MATCHING
// Handwritten: it keeps the best voice so far in $t5-$t9, reads a voice's first four
// bytes in one load, and clears $t6 with xor.
int SfxVoice_Alloc(int priority)
{
    SoundDriver *d;
    SfxVoice *v;
    SfxVoice *best;
    signed char *owner;
    int idx;
    int bestIdx;
    int bestAge;
    int bestKeyed;
    int bestPri;
    int keyed;
    int n;

    d = &g_SoundDriver;
    idx = d->nextVoice;
    v = &d->voices[idx];
    for (n = 24; n != 0; n--) {
        if (idx == 23) {
            v = d->voices;
            idx = -1;
        } else {
            v++;
        }
        idx++;
        if (v->lock == 0 && v->state == 0) {
            v->lock = 1;
            if (v->state == 0) {
                v->priority = priority;
                d->nextVoice = idx;
                return idx;
            }
            v->lock = 0;
        }
    }
    best = 0;
    for (n = 24; n != 0; n--) {
        if (idx == 23) {
            v = d->voices;
            idx = -1;
        } else {
            v++;
        }
        idx++;
        if (v->lock != 0) {
            continue;
        }
        v->lock = 1;
        if (v->state == 0) {
            v->priority = priority;
            d->nextVoice = idx;
            return idx;
        }
        if (v->state == 1 && v->priority >= priority) {
            keyed = v->keyedOn;
            if (keyed == 0) {
                keyed = v->keyOn;
            }
            if (best == 0 || v->priority != bestPri || keyed < bestKeyed || v->age > bestAge) {
                if (best != 0) {
                    best->lock = 0;
                }
                bestIdx = idx;
                best = v;
                bestAge = v->age;
                bestKeyed = keyed;
                bestPri = v->priority;
                continue;
            }
        }
        v->lock = 0;
    }
    if (best == 0) {
        return -1;
    }
    owner = best->owner;
    if (owner != 0) {
        best->owner = 0;
        *owner = -1;
    }
    best->lock = 1;
    best->dirty = 0;
    best->keyOn = 0;
    best->keyOff = 1;
    d->nextVoice = bestIdx;
    return bestIdx;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_Alloc);
#endif

// Sets voice up to play params from the start, keyed on at the next vsync.
#ifdef NON_MATCHING
// Handwritten: it keeps its temporaries in $t registers and sets lock, dirty, keyOn
// and keyOff with one word.
void SfxVoice_Setup(int voice, SfxParams *params)
{
    SfxVoice *v;

    v = &g_SoundDriver.voices[voice];
    v->lock = 2;
    v->playing = 0;
    v->age = 0;
    v->elapsed = 0;
    v->volL = params->vol[0];
    v->volR = params->vol[1];
    v->pitch = params->pitch;
    v->addr = params->addr;
    v->adsr1 = params->adsr;
    v->adsr2 = params->adsr >> 16;
    v->duration = params->duration * g_SoundDriver.tickScale;
    v->reverb = params->flags & 1;
    g_SoundDriver.reverbDirty = 1;
    v->lock = 0;
    v->dirty = 7;
    v->keyOn = 1;
    v->keyOff = 0;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/seq", SfxVoice_Setup);
#endif
}
