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
PENDING(CdInit)
PENDING(CdReadyCallback)
PENDING(CdSyncCallback)
PENDING(CheckCallback)
PENDING(CloseEvent)
PENDING(DMACallback)
PENDING(DecDCTReset)
PENDING(DecDCTin)
PENDING(DecDCTout)
PENDING(DecDCToutCallback)
PENDING(EnableEvent)
PENDING(EnterCriticalSection)
PENDING(ExitCriticalSection)
PENDING(FlushCache)
PENDING(GPU_cw)
PENDING(InitCARD)
PENDING(InitGeom)
PENDING(Krom2RawAdd)
PENDING(OpenEvent)
PENDING(PadGetState)
PENDING(PadInfoAct)
PENDING(PadInitDirect)
PENDING(PadSetAct)
PENDING(PadSetActAlign)
PENDING(PadStartCom)
PENDING(ResetCallback)
PENDING(SetIntrMask)
PENDING(StartCARD)
PENDING(VSyncCallback_Psyq)
PENDING(_bu_init)
PENDING(_card_clear)
PENDING(_card_info)
PENDING(_card_read)
PENDING(_card_write)

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
