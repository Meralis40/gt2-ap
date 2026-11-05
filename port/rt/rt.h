// SPDX-License-Identifier: GPL-2.0-or-later
// The runtime's interface to the game code. Plain C names: game sources include this.
#ifndef RT_H
#define RT_H

#ifdef __cplusplus
extern "C" {
#endif

void Rt_Log(const char *format, ...);
[[noreturn]] void Rt_Fatal(const char *format, ...);
void Rt_Unimplemented(const char *what);

// Copies overlay index to its guest address and patches its code pointers.
void Rt_LoadOverlay(int index);
// Leaves the current overlay and runs entry(a, b, c, d) from the runtime's loop.
[[noreturn]] void Rt_RunEntry(void *entry, int a, int b, int c, int d);

#ifdef __cplusplus
}

// Test runs: stops after maxVblanks without waiting for real time, writing a frame to
// dumpDir every dumpEvery vblanks. 0 for no limit, no frames.
void Rt_SetRunLimits(unsigned int maxVblanks, unsigned int dumpEvery, const char *dumpDir);
#endif

#endif
