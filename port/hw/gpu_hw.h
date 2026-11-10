// SPDX-License-Identifier: GPL-2.0-or-later
// The hardware renderer: the GPU's primitives drawn by the graphics card, through SDL_GPU, at
// 1 to 8 times the PlayStation's resolution. The window's SDL renderer shows the result.
#pragma once

#include <SDL3/SDL.h>

#include <vector>

namespace gpu::hw {

constexpr int kMaxScale = 8;

// Takes over drawing from the software rasterizer, with VRAM as it stands. False if this
// renderer can't run here: the software one goes on.
bool open(SDL_Renderer *renderer, int scale);
// Back to the software rasterizer, with what was drawn brought down into VRAM.
void close();
bool active();

void setScale(int scale);
int scale();
void setDither(bool on);

// Sends what was drawn since the last call to the graphics card. Returns VRAM at scale(),
// for the renderer to show the display area of.
SDL_Texture *submit();

// Test runs: the display area as drawn, at scale(), each dot 0xbbggrr with 5-bit channels
// as the software renderer writes them. False if it didn't come back.
bool readDisplay(std::vector<unsigned int> &pixels, int &width, int &height);

}  // namespace gpu::hw
