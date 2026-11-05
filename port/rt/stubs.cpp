// SPDX-License-Identifier: GPL-2.0-or-later
// The PsyQ and BIOS functions the game calls, until each has its real implementation
// in port/hw. A stub stops with its name, so the next one to write is the one the game
// reached.
#include "rt.h"

#include <cstring>
#if !defined(_WIN32)
#include <strings.h>
#endif

#define PENDING(name) \
    int name() \
    { \
        Rt_Unimplemented(#name); \
        return 0; \
    }

extern "C" {

PENDING(CdControlF)
PENDING(CdGetSector)
PENDING(DecDCTin)
PENDING(DecDCTout)
PENDING(Krom2RawAdd)
PENDING(PadGetState)
PENDING(PadInfoAct)
PENDING(PadSetAct)
PENDING(PadSetActAlign)
PENDING(PadStartCom)
PENDING(_card_clear)
PENDING(_card_info)
PENDING(_card_read)
PENDING(_card_write)

// Set-up calls with no effect on PC (yet).
#define NOTHING(name) \
    int name() \
    { \
        return 0; \
    }

NOTHING(ResetCallback)
NOTHING(SetIntrMask)
NOTHING(EnterCriticalSection)
NOTHING(ExitCriticalSection)
NOTHING(FlushCache)
NOTHING(CdInit)
NOTHING(InitCARD)
NOTHING(StartCARD)
NOTHING(_bu_init)
NOTHING(PadInitDirect)
NOTHING(DecDCTReset)
NOTHING(InitGeom)
NOTHING(DMACallback)
NOTHING(CdReadyCallback)
NOTHING(CdSyncCallback)
NOTHING(DecDCToutCallback)
NOTHING(CheckCallback)
NOTHING(OpenEvent)
NOTHING(EnableEvent)
NOTHING(CloseEvent)
NOTHING(GPU_cw)

// The I/O page. GPUSTAT reads as ready, until the GPU runtime owns it.
unsigned char g_HwRegs[0x1000];

static void (*s_VSyncCallback)(void);

void *VSyncCallback_Psyq(void (*func)(void))
{
    void (*previous)(void) = s_VSyncCallback;

    s_VSyncCallback = func;
    return (void *)previous;
}

// One vblank: where the game waits, the host gets its turn.
void Rt_Pump(void)
{
    *(unsigned int *)(g_HwRegs + 0x814) = 0x1c000000;
    if (s_VSyncCallback != 0) {
        s_VSyncCallback();
    }
}

// GTE: port/hw/gte, for the race.
PENDING(gte_GetCtrlReg)
PENDING(gte_GetDataReg)
PENDING(gte_LoadDataReg)
PENDING(gte_LoadDataRegAt)
PENDING(gte_Lzc)
PENDING(gte_SetCtrlReg)
PENDING(gte_SetDataReg)
PENDING(gte_StoreDataReg)
PENDING(gte_dpcs)
PENDING(gte_dpct)
PENDING(gte_gpf)
PENDING(gte_gpl)
PENDING(gte_intpl)
PENDING(gte_mvmva)
PENDING(gte_nclip)
PENDING(gte_rtps)
PENDING(gte_rtpt)
PENDING(gte_sqr)
PENDING(ot_AddPrim)
PENDING(ot_AddPrims)

// Loops written as plain functions over guest vtables: these become real classes.
PENDING(RaceMenuLoop__Execute)
PENDING(RaceMenuLoop__RaceMenuLoop)
PENDING(RaceMenuLoop__RaceMenuLoop_dtor)
PENDING(RaceMenuLoop____tf)
PENDING(RaceViewLoop__RaceViewLoop)
PENDING(RaceViewLoop__RaceViewLoop_dtor)
PENDING(RaceViewLoop__Start)
PENDING(RaceViewLoop____tf)
PENDING(__rtti_si)
PENDING(psxViewLoopDoubleBuffer__Leave)
PENDING(psxViewLoopDoubleBuffer__SetVideoMode)
PENDING(psxViewLoopDoubleBuffer____tf)
PENDING(psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer)
PENDING(psxViewLoopDoubleBuffer__psxViewLoopDoubleBuffer_dtor)
PENDING(psxViewLoop__BeginFrame)
PENDING(psxViewLoop__Draw)
PENDING(psxViewLoop__Enter)
PENDING(psxViewLoop__Update)

// libc names with no equivalent in the host's C library.
void bcopy(const void *src, void *dst, unsigned int size)
{
    std::memmove(dst, src, size);
}

#if defined(_WIN32)
int strncasecmp(const char *a, const char *b, unsigned int size)
{
    return _strnicmp(a, b, size);
}
#endif

}  // extern "C"
