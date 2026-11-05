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

PENDING(DecDCTin)
PENDING(DecDCTout)

// Set-up calls with no effect on PC (yet).
#define NOTHING(name) \
    int name() \
    { \
        return 0; \
    }

NOTHING(ResetCallback)
// No pad connected yet: port/hw/pad, for the menus.
NOTHING(PadGetState)
NOTHING(PadInfoAct)
NOTHING(PadSetAct)
NOTHING(PadSetActAlign)
NOTHING(PadStartCom)
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
NOTHING(DecDCToutCallback)
NOTHING(EnableEvent)
NOTHING(CloseEvent)
NOTHING(GPU_cw)

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

// The BIOS Kanji ROM isn't on the disc. Its glyphs read as blank for now; only memory card
// screens use them. The game indexes from the space's address, in both directions.
static unsigned short s_BlankKanji[0x80000];

void *Krom2RawAdd(unsigned int)
{
    return &s_BlankKanji[0x40000];
}

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
