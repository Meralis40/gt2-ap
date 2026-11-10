// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>
#include <vector>

// Opens the window. Test runs (--vblanks) don't, and never call Host_Frame. hardware false
// keeps to the software renderer.
void Host_Open(bool hardware);
// Test runs with --renderer hardware: the hardware renderer, with no window, at this scale.
void Host_OpenOffscreen(int scale);
// The frame for --dump, each dot 0xbbggrr: the hardware renderer's when it runs, at its scale.
void Host_DumpFrame(std::vector<unsigned int> &pixels, int &width, int &height);
// Reads the controls and shows the current frame. Runs once a vblank.
void Host_Frame();
// Queues interleaved stereo samples at 44100 Hz. Does nothing without a window.
void Host_Audio(const short *samples, int count);
// With the window open: asks for the disc image. Returns its path in UTF-8, empty if cancelled.
std::string Host_PickDisc();
void Host_Alert(const std::string &text);
