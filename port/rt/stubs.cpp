// SPDX-License-Identifier: GPL-2.0-or-later
// The PsyQ and BIOS functions the game calls, until each has its real implementation
// in port/hw. A stub stops with its name, so the next one to write is the one the game
// reached.
#include "memmap.h"
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
NOTHING(SetIntrMask)
NOTHING(Psx_EnterCriticalSection)  // the BIOS call; the name is also Win32's
NOTHING(ExitCriticalSection)
NOTHING(FlushCache)
NOTHING(CdInit)
NOTHING(InitCARD)
NOTHING(StartCARD)
NOTHING(_bu_init)
NOTHING(DecDCTReset)
NOTHING(DecDCToutCallback)
NOTHING(EnableEvent)
NOTHING(CloseEvent)
NOTHING(GPU_cw)

// The BIOS Kanji ROM isn't on the disc. Its glyphs read as blank for now; only memory card
// screens use them. The game indexes from the space's address, in both directions, and keeps
// it in guest data, so the blank ROM is in guest memory.
void *Krom2RawAdd(unsigned int)
{
    static unsigned short *blank;

    if (!blank) {
        blank = static_cast<unsigned short *>(rt::guestAlloc(0x80000 * sizeof(unsigned short)));
    }
    return blank + 0x40000;
}

// libc names with no equivalent in the host's C library.
void Game_bcopy(const void *src, void *dst, std::size_t size)
{
    std::memmove(dst, src, size);
}

#if defined(_WIN32)
int strncasecmp(const char *a, const char *b, std::size_t size)
{
    return _strnicmp(a, b, size);
}
#endif

}  // extern "C"
