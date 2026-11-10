// SPDX-License-Identifier: GPL-2.0-or-later
// What the GPU's command decoding (gpu.cpp) shares with whatever draws the primitives: the
// software rasterizer (gpu_soft.cpp), or the hardware renderer (gpu_hw.cpp), which draws at a
// multiple of the PlayStation's resolution.
//
// VRAM here is always right for what the CPU puts there: uploads, fills and copies reach it
// whichever renderer draws. With the hardware renderer, what's drawn is only on the GPU.
#pragma once

#include <cstdint>

namespace gpu {

constexpr int kVramWidth = 1024;
constexpr int kVramHeight = 512;

extern std::uint16_t g_Vram[kVramHeight][kVramWidth];

// E1-E6 and GP1 state.
struct State {
    int pageX = 0, pageY = 0, depth = 0, transparency = 0;
    bool dither = false;
    int windowMaskX = 0, windowMaskY = 0, windowOffsetX = 0, windowOffsetY = 0;
    int clipX0 = 0, clipY0 = 0, clipX1 = 0, clipY1 = 0;
    int offsetX = 0, offsetY = 0;
    bool setMask = false, checkMask = false;
    int displayX = 0, displayY = 0;
    int rangeStart = 0, rangeEnd = 0;  // GP1(06), in GPU clocks
    std::uint32_t displayMode = 0;
    bool displayOn = false;
};

const State &state();

struct Vertex {
    int x, y, r, g, b, u, v;
};

struct Shader {
    bool textured, raw, semi, gouraud;
    int clutX, clutY, pageX, pageY, depth;
};

// The part of VRAM the screen shows.
struct Display {
    int x, y, width, height;
    bool on, truecolor;
};

Display display();

namespace soft {
void triangle(const Shader &s, Vertex a, Vertex b, Vertex c);
void line(const Shader &s, Vertex a, const Vertex &b);
void rect(const Shader &s, int x0, int y0, int w, int h, int r, int g, int b, int u0, int v0);
}  // namespace soft

// The hardware renderer. Draws go to it instead of the rasterizer; the rest is news of what
// VRAM already holds. State and the shader are as gpu::state() has them at the call.
class Backend {
public:
    virtual ~Backend() = default;
    virtual void triangle(const Shader &s, const Vertex &a, const Vertex &b, const Vertex &c) = 0;
    virtual void line(const Shader &s, const Vertex &a, const Vertex &b) = 0;
    virtual void rect(const Shader &s, int x, int y, int w, int h, int r, int g, int b, int u, int v) = 0;
    virtual void filled(int x, int y, int w, int h) = 0;
    virtual void copied(int sx, int sy, int dx, int dy, int w, int h) = 0;
    virtual void uploaded(int x, int y, int w, int h) = 0;
    // Before VRAM is read back: what was drawn there has to come down first.
    virtual void readingBack(int x, int y, int w, int h) = 0;
};

// nullptr for the software rasterizer.
void setBackend(Backend *backend);

}  // namespace gpu
