// Resident code 8005d9bc-8005db90 (overlay): loads the overlays from their gzip pack and
// runs their entries from a setjmp point.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern void *g_OverlayDefaultEntries;
extern char g_MenuContext[];    // start of the overlays' .bss, and the CD read buffer
extern char g_MainInitialized[]; // end of the overlays' .bss
extern jmpbuf g_OverlayJmpBuf;
extern int g_OverlayArgs[4];
extern int g_OvlLba;
extern int g_OvlSize;

extern OverlayPack g_OverlayFile;

void Overlay_ClearBss(void)
{
    memset(g_MenuContext, 0, g_MainInitialized - g_MenuContext);
}

// Runs each overlay entry Overlay_StartAt jumps back here with.
void Overlay_Dispatch(void)
{
    void (*entry)(int, int, int, int) = (void (*)(int, int, int, int))setjmp(&g_OverlayJmpBuf);

    if (entry != 0) {
        entry(g_OverlayArgs[0], g_OverlayArgs[1], g_OverlayArgs[2], g_OverlayArgs[3]);
    }
}

void Overlay_Start(int index)
{
    Overlay_StartAt(index, (&g_OverlayDefaultEntries)[index], 0, 0, 0, 0);
}

#define NEXT_INT(ap) (*(int *)(ap += 4, ap - 4))

// Loads overlay index and runs entry with the four ints after it, from Overlay_Dispatch.
// Variadic, as the game stores $a1-$a3 to their home slots and reads the ints from there.
void Overlay_StartAt(int index, void *entry, ...)
{
    char *ap = (char *)__builtin_next_arg(entry); // va_start, va_arg written out

    g_OverlayArgs[0] = NEXT_INT(ap);
    g_OverlayArgs[1] = NEXT_INT(ap);
    g_OverlayArgs[2] = NEXT_INT(ap);
    g_OverlayArgs[3] = NEXT_INT(ap);
    Overlay_Load(index);
    longjmp(&g_OverlayJmpBuf, (int)entry);
}

// Inflates overlay index from the pack, read from the CD first if it isn't in memory.
// Not matching: the game copies index through $a1 and the pack and entry pointers through
// $a0 before keeping them in $s0 and $s1, so three moves more, with the index copy and the
// pack's address ahead of the stack adjustment. Inline helpers, references and other
// orders didn't give it.
#ifdef NON_MATCHING
void Overlay_Load(int index)
{
    OverlayPack *pack = &g_OverlayFile;
    OverlayEntry *e;

    Spu_WaitVoicesSilent();
    Sfx_StopAllIfReady();
    e = &pack->entries[index];
    if (pack->data != 0) {
        Gzip_Inflate(pack->data + e->offset, (void *)Overlay_OpenFile);
    } else {
        Cd_ReadSync(g_MenuContext, g_OvlLba, g_OvlSize);
        Gzip_Inflate(g_MenuContext + e->offset, (void *)Overlay_OpenFile);
    }
    FlushCache();
    Overlay_ClearBss();
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/overlay", Overlay_Load);
#endif
}
