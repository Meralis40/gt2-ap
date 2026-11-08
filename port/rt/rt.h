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

// Copies overlay index to its guest address; its functions then answer for its addresses.
void Rt_LoadOverlay(int index);
// Leaves the current overlay and runs entry(a, b, c, d) from the runtime's loop.
[[noreturn]] void Rt_RunEntry(void *entry, int a, int b, int c, int d);

#ifdef __cplusplus
}

// Test runs: stops after maxVblanks without waiting for real time, writing a frame to
// dumpDir every dumpEvery vblanks. 0 for no limit, no frames.
void Rt_StartWatchdog();
// A wait that's meant to last, like the paused tracker: tells the watchdog so.
void Rt_StillAlive();
// Test runs: the buttons held from vblank on (hw/host.cpp lists the bits).
void Rt_AddInput(unsigned int vblank, unsigned short buttons);
void Rt_SetRunLimits(unsigned int maxVblanks, unsigned int dumpEvery, const char *dumpDir);

// Recordings, in rt/record.cpp. Replaying uses the recording's memory card and leaves the
// real one alone; at the end the controls come back, or the run stops if stopAtEnd.
void Rt_ReplayOpen(const char *path, bool stopAtEnd);
void Rt_RecordOpen(const char *path);
// Plays and records this vblank's buttons, once the host and scripts have set them.
void Rt_RecordVblank();
#endif

#endif
