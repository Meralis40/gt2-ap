// M0 spike: real decomp types, laid out by the original and the host compiler.
#include "common.h"
#include "viewloop.h"
#include "ovl0.h"
#ifdef __clang__
#define OFF(T, f) __builtin_offsetof(T, f)
#else
#define OFF(T, f) ((int)&((T *)0)->f)
#endif
int layout[] = {
    0x4c41594f,
    sizeof(GameState), OFF(GameState, licenses),
    sizeof(GameSaveFile), sizeof(LicenseTest), sizeof(Car),
    sizeof(OtTag), sizeof(psxViewLoop), OFF(psxViewLoop, prevLoop), OFF(psxViewLoop, vsync),
    sizeof(psxViewLoopDoubleBuffer), sizeof(RaceMenuLoop),
};
