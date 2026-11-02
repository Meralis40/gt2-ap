// OVL2 80016cc0-800173d4 (arclogo): Car maker logos.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {


INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclogo", ArcLogo_Upload);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclogo", ArcLogo_UploadPending);

// Sets the page and buffers and resets.
void ArcLogo_Init(ArcLogo *logo, short vramPage, unsigned char *archive, void *timBuf)
{
    logo->vramPage = vramPage;
    logo->archive = archive;
    logo->timBuf = timBuf;
    logo->cdBusy = '\0';
    logo->cancel = '\0';
    ArcLogo_Reset(logo);
}

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclogo", ArcLogo_Reset);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclogo", ArcLogo_Request);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclogo", ArcLogo_CancelRead);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclogo", ArcLogo_Update);

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arclogo", ArcLogo_Draw);
}
