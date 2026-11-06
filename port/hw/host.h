// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Opens the window. Test runs (--vblanks) don't, and never call Host_Frame.
void Host_Open();
// Reads the controls and shows the current frame. Runs once a vblank.
void Host_Frame();
// Queues interleaved stereo samples at 44100 Hz. Does nothing without a window.
void Host_Audio(const short *samples, int count);
