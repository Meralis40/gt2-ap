// Resident code 8007c36c-8007d024 (cdaudio). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void CdAudio_ExecScript(CdAudio *cd, unsigned int *script);
int Sys_CriticalSection(int leave);
void *memset(void *dst, int c, unsigned int n);
int CdControlF(unsigned char com, unsigned char *param);
int CdGetSector(void *dst, int words);
void *CdSyncCallback(void (*func)(unsigned char status, unsigned char *result));
void *CdReadyCallback(void (*func)(unsigned char status, unsigned char *result));
void VSyncList_Add(VSyncCallback **list, VSyncCallback *node);
extern GfxIrqState g_GfxIrq;
extern CdAudio g_CdAudio;
extern unsigned char g_BcdToInt[];
extern unsigned char g_BcdTable[];
// Scripts: command words run by CdAudio_ExecScript (see there), ending with 0.
extern unsigned int *g_CdAudioScripts[];
extern unsigned int D_800903B4[];
extern unsigned int D_800903B8[];
extern unsigned int D_800903D4[];
unsigned int CdAudio_DefaultSectorFn(void);
void CdAudio_DefaultFn(void);
void CdAudio_VSync(void);
void CdAudio_OnSync(unsigned char status, unsigned char *result);
void CdAudio_OnReady(unsigned char status, unsigned char *result);
void CdAudio_ApplyVolume(CdAudio *cd);
void CdAudio_ClearPending(void *cd);
int CdAudio_QueueScript(CdAudio *cd, int index);
int CdAudio_RunScript_8007C4F8(CdAudio *cd, int index);
void CdAudio_SetMode(CdAudio *cd, unsigned int mode);
void CdAudio_SetScript(CdAudio *cd, unsigned int *script);
int CdAudio_RunQueued(CdAudio *cd);
void CdAudio_Reset(CdAudio *cd);
int CdAudio_OnReadSector(unsigned int sector, void *header);

// The bytes are read through an inline function (found with permuter); indexed
// directly, the compiler loads the table's entries in another order.
static inline unsigned char Cd_Byte(unsigned char *p, int i)
{
    return p[i];
}

// The sector number of a BCD minute, second and frame.
int Cd_BcdMsfToSector(unsigned char *pos)
{
    return (Cd_Byte(g_BcdToInt, Cd_Byte(pos, 0)) * 60 + Cd_Byte(g_BcdToInt, Cd_Byte(pos, 1))) * 75
           + Cd_Byte(g_BcdToInt, Cd_Byte(pos, 2)) - 150;
}

#ifdef NON_MATCHING
// The game divides with div; GCC turns a division by a constant into a
// multiplication unless it goes through inline functions, as here, but the
// registers and the order of the table loads still differ.
static inline int Cd_Div(int a, int b)
{
    return a / b;
}

static inline int Cd_Mod(int a, int b)
{
    return a % b;
}

// The BCD minute, second and frame of a sector number.
unsigned char *Cd_LbaToPos(int sector, unsigned char *pos)
{
    int frame;
    int second;

    sector += 150;
    frame = Cd_Mod(sector, 75);
    sector = Cd_Div(sector, 75);
    second = Cd_Mod(sector, 60);
    sector = Cd_Div(sector, 60);
    pos[2] = g_BcdTable[(unsigned char)frame];
    pos[1] = g_BcdTable[(unsigned char)second];
    pos[0] = g_BcdTable[(unsigned char)sector];
    return pos;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", Cd_LbaToPos);
#endif

unsigned int CdAudio_DefaultSectorFn(void)
{
    return 0;
}

void CdAudio_DefaultFn(void)
{
}

#ifdef NON_MATCHING
// The game loads &g_CdAudio into $a0 for memset before the prologue and keeps a copy
// in $s0; GCC loads it into $s0 directly (as Overlay_Load, docs/decomp.md).
void CdAudio_Init(void)
{
    CdAudio *cd = &g_CdAudio;

    memset(cd, 0, sizeof(CdAudio));
    cd->vsync.func = CdAudio_VSync;
    cd->vsync.priority = 0x30000;
    cd->onSector = (void *)CdAudio_DefaultSectorFn;
    cd->onIdle = (void *)CdAudio_DefaultFn;
    cd->masterVol = cd->volL = cd->volR = 0x8000;
    cd->fade = 0;
    CdAudio_ApplyVolume(cd);
    CdSyncCallback(CdAudio_OnSync);
    CdReadyCallback(CdAudio_OnReady);
    VSyncList_Add(&g_GfxIrq.vsyncCallbacks, &cd->vsync);
    CdAudio_RunScript_8007C4F8(cd, 1);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_Init);
#endif

#ifdef NON_MATCHING
// The game's version has no return: it sets up the arguments and falls through into
// CdAudio_RunScript_8007C4F8, the next function.
int CdAudio_RunScript(int index)
{
    return CdAudio_RunScript_8007C4F8(&g_CdAudio, index);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_RunScript);
#endif

int CdAudio_RunScript_8007C4F8(CdAudio *cd, int index)
{
    int leave;
    int ret;

    leave = Sys_CriticalSection(0);
    cd->unk165 = 0;
    ret = CdAudio_QueueScript(cd, index);
    Sys_CriticalSection(leave);
    return ret;
}

// The game keeps &g_CdAudio in $a0, as its other functions load it there first.
void CdAudio_WaitIdle(void)
{
    register CdAudio *cd asm("a0") = &g_CdAudio;

    while (*(volatile unsigned char *)&cd->state != 0) {
    }
}

#ifdef NON_MATCHING
// The game loads &g_CdAudio into $a0 before the prologue and copies it to $s0 in
// the call's delay slot; GCC loads it into $s0 directly.
void Music_Stop(void)
{
    CdAudio *cd = &g_CdAudio;

    CdAudio_ClearPending(cd);
    CdAudio_RunScript_8007C4F8(cd, 2);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", Music_Stop);
#endif

// The first sector of a track (1-based).
int FUN_8007c5a8(CdAudio *cd, int track)
{
    unsigned char pos[4];
    unsigned char *p = cd->trackPos[track - 1];

    pos[0] = p[0];
    pos[1] = p[1];
    pos[2] = 0;
    pos[3] = 0;
    return Cd_BcdMsfToSector(pos);
}

void CdAudio_ClearPending(void *cd)
{
    int leave;

    leave = Sys_CriticalSection(0);
    *(unsigned int *)((int)cd + 0x68) = 0;
    *(unsigned int *)((int)cd + 0x4c) = 0;
    Sys_CriticalSection(leave);
}

// Runs script index now, or queues it while a command is in flight. Returns 0 if
// it was dropped.
int CdAudio_QueueScript(CdAudio *cd, int index)
{
    unsigned int *script = g_CdAudioScripts[index];
    int ret = 0;

    if (cd->state != 3) {
        CdAudio_ExecScript(cd, script);
        ret = 1;
    } else if (cd->noQueue == 0) {
        cd->queuedScript = script;
        ret = 1;
    }
    return ret;
}

#ifdef NON_MATCHING
// The registers and the interleaving of the products and clamps differ.
// The CD audio volume: masterVol * fade, then the left and right volumes (1.15).
void CdAudio_ApplyVolume(CdAudio *cd)
{
    int vol;
    int left;
    int right;
    volatile short *spu;

    vol = (cd->masterVol * cd->fade) >> 15;
    left = (cd->volL * vol) >> 15;
    right = (cd->volR * vol) >> 15;
    spu = (volatile short *)0x1f801c00;
    if (left > 0x7fff) {
        left = 0x7fff;
    }
    if (left < -0x8000) {
        left = 0x8000;
    }
    if (right > 0x7fff) {
        right = 0x7fff;
    }
    if (right < -0x8000) {
        right = 0x8000;
    }
    spu[0xd8] = left;
    spu[0xd9] = right;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_ApplyVolume);
#endif

void CdAudio_SetMode(CdAudio *cd, unsigned int mode)
{
    unsigned char old = cd->modeParam;
    int changed = cd->speedChanged != 0;

    cd->modeParam = mode;
    if ((old ^ mode) & 0x80) {
        changed = 1;
    }
    cd->speedChanged = changed;
    cd->cddaBits = mode & 0x41;
}

// Runs a script: a list of command words ending with 0. A word >= 0 is a CdControlF
// command (its low 5 bits); the script goes on in CdAudio_OnSync when it completes.
// A negative word is an op: bits 27-30 select the group and the low 16 bits are its
// argument. When the script ends, the queued script runs, or onIdle is called.
void CdAudio_ExecScript(CdAudio *cd, unsigned int *script)
{
    unsigned int cmd;
    unsigned int arg;
    unsigned char com;
    unsigned char *param;
    unsigned char flag;
    void *fn;
    unsigned int *next;
    unsigned int sector;

    cd->lastCmd = 0;
    cd->retryScript = 0;
    cd->script = 0;
    if (script == 0) {
        goto end;
    }
    for (;;) {
        cd->state = 1;
        cmd = *script++;
        cd->command = cmd;
        if (cmd == 0) {
        end:
            script = cd->queuedScript;
            cd->state = 0;
            cd->sectorFn = 0;
            cd->queuedScript = 0;
            if (script == 0) {
                ((void (*)(void))cd->onIdle)();
                return;
            }
            continue;
        }
        if ((int)cmd >= 0) {
            com = cmd & 0x1f;
            cd->script = script;
            param = 0;
            if (com == 2) {
                param = cd->locParam;
            }
            if (com == 0xd) {
                param = cd->filterParam;
            }
            if (com == 0xe) {
                param = &cd->modeParam;
            }
            if (com == 0x14) {
                param = cd->cmd14Param;
            }
            cd->state = 3;
            cd->lastCmd = com;
            CdControlF(com, param);
            return;
        }
        arg = cmd & 0xffff;
        switch ((cmd >> 27) & 0xf) {
        case 0:
            switch (arg) {
            case 0:
                return;
            case 1:
                flag = cd->speedChanged;
                goto wait;
            case 2:
                script = (unsigned int *)*script;
                break;
            case 3:
                cmd = (signed char)cd->flag;
                next = (unsigned int *)*script++;
                if (cmd != 0) {
                    script = next;
                } else {
                    cd->unk164 = 1;
                }
                break;
            case 4:
                *(CdReq *)&cd->flag = *(CdReq *)&cd->reqFlag;
                fn = cd->sectorFn;
                cd->sectorFn = 0;
                cd->unk167 = 1;
                cd->unk164 = 0;
                cd->filterParam[0] = cd->xaFile;
                cd->filterParam[1] = cd->xaChannel;
                cd->savedSectorFn = fn;
                break;
            case 5:
                cd->unk165 = 1;
                cd->sectorFn = cd->savedSectorFn;
                break;
            case 6:
                cd->fadeStep = 0x2000;
                break;
            case 7:
                cd->halted = 1;
                cd->fadeStep = -0x2000;
                if (cd->fade == 0) {
                    break;
                }
                flag = cd->cddaBits;
            wait:
                if (flag != 0) {
                    cd->pollTimer = 4;
                    cd->retryScript = script;
                    return;
                }
                break;
            case 8:
                break;
            case 9:
                cd->noQueue = 0;
                break;
            }
            break;
        case 1:
            cd->unk15e = arg;
            break;
        case 2:
            CdAudio_SetMode(cd, arg);
            break;
        case 3:
            CdAudio_SetMode(cd, arg | cd->mode);
            break;
        case 4:
            sector = cd->curSector;
            if (arg == 0) {
                sector = cd->nextSector;
            }
            cd->curSector = sector;
            Cd_LbaToPos(sector, cd->locParam);
            break;
        }
    }
}

void CdAudio_SetScript(CdAudio *cd, unsigned int *script)
{
    cd->script = script;
    if (cd->state != 3) {
        CdAudio_ExecScript(cd, script);
    }
}

int CdAudio_RunQueued(CdAudio *cd)
{
    unsigned int *script = cd->queuedScript;

    if (cd->state != 3 && script != 0) {
        cd->queuedScript = 0;
        CdAudio_ExecScript(cd, script);
        return 1;
    }
    return 0;
}

void CdAudio_Reset(CdAudio *cd)
{
    cd->modeParam = 0;
    cd->lastCmd = 0;
    cd->noQueue = 1;
    cd->script = D_800903B8;
    cd->retryScript = D_800903B8 - 1;
    cd->queuedScript = 0;
    cd->state = 1;
    cd->playState = 0;
    cd->unk15e = 1;
    cd->unk168 = 0;
    cd->firstTrack = 0;
    cd->lastTrack = 0;
    cd->tocRead = 0;
    cd->cddaBits = 0;
}

#ifdef NON_MATCHING
// The game moves status and result to $a1 and $a2 and loads &g_CdAudio into $a0
// before the prologue, as if setting up a call, then copies them to saved registers.
// CdSyncCallback: a CdControlF command completed (status 2) or failed (5). The
// script goes on from the next word, or retries the command (one word back).
void CdAudio_OnSync(unsigned char status, unsigned char *result)
{
    CdAudio *cd = &g_CdAudio;
    unsigned int cmd = cd->command;
    unsigned int *script = cd->script;
    unsigned int res;
    unsigned int last;
    unsigned int prev;
    unsigned char track;
    unsigned char next;
    int index;
    int count;

    cd->state = 2;
    res = ((unsigned int *)result)[0];
    cd->result[1] = ((unsigned int *)result)[1];
    cd->result[0] = res;
    last = cd->lastCmd;
    if (status != 2) {
        if (status != 5) {
            return;
        }
        switch (last) {
        case 19:
        case 20:
            script = D_800903B4;
            goto next;
        case 16:
        case 17:
            if (res & 0x10) {
                goto reset;
            }
            script--;
            goto run;
        case 9:
            if (res & 0x10) {
                goto reset;
            }
            if (res & 2) {
                goto retry;
            }
            goto run;
        case 6:
        case 21:
        case 27:
            if ((res & 0x4001) == 0x4001) {
                cd->unk168 = 1;
            }
            // fall through
        default:
            if (res & 0x10) {
                goto reset;
            }
            script--;
            if (!(cmd & 0x8000)) {
            error:
                script = D_800903D4;
            }
            goto next;
        }
    reset:
        CdAudio_Reset(cd);
        return;
    }
    if ((cmd & 0x40) && !(res & 2)) {
        goto retry;
    }
    if ((cmd & 0x80) && (res & 0x40)) {
        goto retry;
    }
    if ((cmd & 0x100) && !(res & 2)) {
        goto error;
    }
    if ((cmd & 0x200) && !(res & 0x20)) {
        goto retry;
    }
    switch (last) {
    case 3:
        cd->playState = 1;
        cd->unk165 = 1;
        return;
    case 6:
    case 27:
        cd->speedChanged = 0;
        cd->halted = 0;
        cd->playState = 2;
        cd->stallCount = 0;
        break;
    case 9:
        cd->playState = 0;
        cd->halted = 1;
        cd->cddaBits = 0;
        break;
    case 16:
        prev = cd->curSector;
        cd->curSector = Cd_BcdMsfToSector(result);
        cd->locFile = result[4];
        cd->locChannel = result[5];
        if (prev == cd->curSector) {
            if (++cd->stallCount > 10) {
                goto reset;
            }
        }
        if (cd->curSector < cd->endSector) {
            script -= 2;
        }
        break;
    case 19:
        cd->cmd14Param[0] = result[1];
        cd->firstTrack = g_BcdToInt[result[1]];
        cd->lastTrack = g_BcdToInt[result[2]];
        goto again;
    case 20:
        track = cd->cmd14Param[0];
        count = cd->lastTrack;
        next = track + 1;
        if ((next & 0xf) >= 10) {
            next = track + 7;
        }
        index = g_BcdToInt[track];
        cd->cmd14Param[0] = next;
        cd->trackPos[index != 0 ? index - 1 : count][0] = result[1];
        cd->trackPos[index != 0 ? index - 1 : count][1] = result[2];
        if (index == 0) {
            cd->tocRead = 1;
            cmd &= ~0x20;
            break;
        }
        if (index > count) {
            break;
        }
        if (index == count) {
            cd->cmd14Param[0] = 0;
        }
        script--;
    again:
        cd->pollTimer = 0;
        goto next;
    }
    if (CdAudio_RunQueued(cd)) {
        return;
    }
    goto next;
retry:
    script--;
run:
next:
    cd->retryScript = script;
    if (!(cmd & 0x20)) {
        CdAudio_ExecScript(cd, script);
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_OnSync);
#endif

#ifdef NON_MATCHING
// Only the prologue differs: the game moves status and result to $a1 and $a2 and
// loads &g_CdAudio into $a0 first, then copies all three to saved registers.
// CdReadyCallback: a sector is ready (status 1). The sector function returns a
// script index to run, or 0; at the end sector the script goes on.
void CdAudio_OnReady(unsigned char status, unsigned char *result)
{
    CdAudio *cd = &g_CdAudio;
    unsigned int (*fn)(unsigned int, void *);
    unsigned int header[3];
    unsigned char *pos;
    unsigned int index;
    unsigned int res;

    res = ((unsigned int *)result)[0];
    cd->result[1] = ((unsigned int *)result)[1];
    cd->result[0] = res;
    if (status == 5 && (res & 0x10)) {
        CdAudio_Reset(cd);
        return;
    }
    if (CdAudio_RunQueued(cd)) {
        return;
    }
    if (status != 1) {
        return;
    }
    fn = (unsigned int (*)(unsigned int, void *))cd->sectorFn;
    if (cd->playState != 1) {
        CdGetSector(header, 3);
        pos = (unsigned char *)header;
    } else {
        fn = 0;
        if (result[4] & 0x80) {
            return;
        }
        pos = result + 3;
    }
    if (cd->halted) {
        return;
    }
    cd->pollTimer = 8;
    cd->curSector = Cd_BcdMsfToSector(pos);
    index = 0;
    if (fn != 0) {
        index = fn(cd->curSector, header);
    }
    if (index != 0) {
        CdAudio_QueueScript(cd, index);
        return;
    }
    if (cd->curSector >= cd->endSector) {
        CdAudio_SetScript(cd, cd->script);
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_OnReady);
#endif

#ifdef NON_MATCHING
// The game loads &g_CdAudio into $a0 and copies it to $s1; GCC loads $s1 directly,
// which also changes the fade code's registers and scheduling.
// The vblank callback: steps the fade, and every 8 vblanks retries the script's
// last command if it hasn't completed.
void CdAudio_VSync(void)
{
    CdAudio *cd = &g_CdAudio;
    int fade;
    short timer;
    unsigned int *retry;

    if (cd->fadeStep != 0) {
        fade = cd->fade + cd->fadeStep;
        if (fade < 0) {
            fade = 0;
        }
        if (fade > 0x7fff) {
            fade = 0x8000;
        }
        cd->fade = fade;
        if ((fade & 0x7fff) == 0) {
            cd->fadeStep = 0;
        }
        CdAudio_ApplyVolume(cd);
    }
    timer = cd->pollTimer;
    retry = cd->retryScript;
    if (timer == 0) {
        timer = 8;
        if (retry != 0) {
            CdAudio_ExecScript(cd, retry);
        }
    }
    cd->pollTimer = timer - 1;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdAudio_VSync);
#endif

#ifdef NON_MATCHING
// The game's version is a jump to CdGetSector, a tail call GCC 2.8 doesn't make.
int CdGetSector_Thunk(void *dst, int words)
{
    return CdGetSector(dst, words);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/cdaudio", CdGetSector_Thunk);
#endif

// Sets up a data read of size bytes from sector start into dst (double speed).
// reportErrors is copied to the request flag.
void CdAudio_SetupRead(void *dst, unsigned int start, unsigned int size, int reportErrors)
{
    unsigned int sectors = (size + 0x7ff) >> 11;
    int flag = reportErrors != 0;

    g_CdAudio.reqFlag = flag;
    g_CdAudio.reqMode = 0x80;
    g_CdAudio.reqStart = start;
    g_CdAudio.reqEnd = start + sectors;
    g_CdAudio.onSector = (void *)CdAudio_OnReadSector;
    g_CdAudio.reqDst = dst;
    g_CdAudio.reqSkip = 0;
    g_CdAudio.reqSize = size;
}
}
