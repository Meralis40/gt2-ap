// SPDX-License-Identifier: GPL-2.0-or-later
// The emulated hardware's interface to the runtime. The game's side is game/include/hw.h.
#ifndef PORT_HW_H
#define PORT_HW_H

#ifdef __cplusplus
extern "C" {
#endif

void Hw_Gp0(unsigned int word);
void Hw_Gp1(unsigned int word);
unsigned int Hw_GpuRead(void);
void Hw_GpuDrawList(const void *list);
// pixels has room for 640 * 480; each is 0xbbggrr.
void Hw_GpuFrame(unsigned int *pixels, int *width, int *height);

// Runs one pending CD event. Returns 0 if there was none.
int Hw_CdPump(void);
void Hw_CardPump(void);

#ifdef __cplusplus
}

#include <filesystem>

void Hw_CdOpen(const std::filesystem::path &cue);
#endif

#endif
