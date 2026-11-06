// SPDX-License-Identifier: GPL-2.0-or-later
// The emulated hardware's interface to the runtime. The game's side is game/include/hw.h.
#ifndef PORT_HW_H
#define PORT_HW_H

#ifdef __cplusplus
extern "C" {
#endif

// On or off for the connection page: the keyboard types, and stops being the controller.
void Host_TextMode(int on);

void Hw_Gp0(unsigned int word);
void Hw_Gp1(unsigned int word);
unsigned int Hw_GpuRead(void);
void Hw_GpuDrawList(const void *list);
// pixels has room for 640 * 480; each is 0xbbggrr.
void Hw_GpuFrame(unsigned int *pixels, int *width, int *height);

// Runs one pending CD event. Returns 0 if there was none.
int Hw_CdPump(void);
void Hw_CdVblank(void);
void Hw_CardPump(void);
void Hw_PadSetButtons(unsigned short held);
unsigned short Hw_PadButtons(void);
void Hw_PadVblank(void);

#ifdef __cplusplus
}

#include <filesystem>
#include <string>

constexpr int kHwCardSize = 128 * 1024;

void Hw_CdOpen(const std::filesystem::path &cue);
// The card as it stands.
const unsigned char *Hw_CardImage();
// Runs from this card instead of the file, which then stays untouched.
void Hw_CardUseImage(const unsigned char *image);
// Swaps in the card in this file, as if the player had changed cards.
void Hw_CardUseFile(const std::string &path);
#endif

#endif
