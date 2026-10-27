// Resident code 80083b0c-80085858 (inflate). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
void Inflate_Run(InflateState *state);
int longjmp(jmpbuf *env, int value);
int setjmp(jmpbuf *__env);
extern jmpbuf g_GzipCallerCtx;
extern int g_GzipChunkSize;
extern jmpbuf g_GzipInflateCtx;
extern GzipStream *g_GzipStream;

void GzipStream_Main(void)
{
    Inflate_Run(&g_GzipStream->state);
    g_GzipStream = 0;
    longjmp(&g_GzipCallerCtx, 2);
}

InflateState *GzipStream_Yield(InflateState *state)
{
    int iVar1;

    iVar1 = setjmp(&g_GzipInflateCtx);
    if (iVar1 == 0) {
        longjmp(&g_GzipCallerCtx, 1);
    }
    return state;
}

InflateState *GzipStream_Flush(InflateState *state)
{
    int iVar1;

    iVar1 = setjmp(&g_GzipInflateCtx);
    if (iVar1 == 0) {
        longjmp(&g_GzipCallerCtx, 1);
    }
    state->outEnd = state->out + g_GzipChunkSize;
    return state;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", GzipStream_Begin);

INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", GzipStream_Continue);

INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", Lz_Decompress);

INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", Inflate_BuildHuft);

INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", Inflate_Codes);

INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", Inflate_Run);
}
