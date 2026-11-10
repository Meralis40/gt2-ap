// SPDX-License-Identifier: GPL-2.0-or-later
// The software rasterizer: draws into VRAM the way the console does, pixel for pixel.
#include "gpu_backend.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace gpu {
namespace {

constexpr int kDither[4][4] = {{-4, 0, -3, 1}, {2, -2, 3, -1}, {-3, 1, -4, 0}, {3, -1, 2, -2}};

std::uint16_t texel(const Shader &s, int u, int v)
{
    const State &g = state();

    u = (u & 0xff & ~(g.windowMaskX * 8)) | ((g.windowOffsetX & g.windowMaskX) * 8);
    v = (v & 0xff & ~(g.windowMaskY * 8)) | ((g.windowOffsetY & g.windowMaskY) * 8);
    int y = (s.pageY + v) & (kVramHeight - 1);
    switch (s.depth) {
    case 0: {
        int index = (g_Vram[y][(s.pageX + u / 4) & (kVramWidth - 1)] >> ((u & 3) * 4)) & 0xf;
        return g_Vram[s.clutY][(s.clutX + index) & (kVramWidth - 1)];
    }
    case 1: {
        int index = (g_Vram[y][(s.pageX + u / 2) & (kVramWidth - 1)] >> ((u & 1) * 8)) & 0xff;
        return g_Vram[s.clutY][(s.clutX + index) & (kVramWidth - 1)];
    }
    default:
        return g_Vram[y][(s.pageX + u) & (kVramWidth - 1)];
    }
}

int clamp5(int value)
{
    return std::clamp(value, 0, 31);
}

// Draws one pixel: r, g and b are 8-bit vertex colours, u and v are used if textured.
void shade(const Shader &s, int x, int y, int r, int g, int b, int u, int v)
{
    const State &state = gpu::state();
    std::uint16_t *pixel = &g_Vram[y][x];

    if (state.checkMask && (*pixel & 0x8000)) {
        return;
    }
    bool blend = s.semi;
    std::uint16_t stp = 0;
    int red, green, blue;
    if (s.textured) {
        std::uint16_t t = texel(s, u, v);
        if (t == 0) {
            return;
        }
        stp = t & 0x8000;
        blend = s.semi && stp != 0;
        int tr = t & 31, tg = (t >> 5) & 31, tb = (t >> 10) & 31;
        if (s.raw) {
            red = tr;
            green = tg;
            blue = tb;
        } else {
            int d = state.dither ? kDither[y & 3][x & 3] : 0;
            red = clamp5((((tr << 3) * r >> 7) + d) >> 3);
            green = clamp5((((tg << 3) * g >> 7) + d) >> 3);
            blue = clamp5((((tb << 3) * b >> 7) + d) >> 3);
        }
    } else {
        int d = state.dither && s.gouraud ? kDither[y & 3][x & 3] : 0;
        red = clamp5((r + d) >> 3);
        green = clamp5((g + d) >> 3);
        blue = clamp5((b + d) >> 3);
    }
    if (blend) {
        int br = *pixel & 31, bg = (*pixel >> 5) & 31, bb = (*pixel >> 10) & 31;
        switch (state.transparency) {
        case 0:
            red = (br + red) >> 1;
            green = (bg + green) >> 1;
            blue = (bb + blue) >> 1;
            break;
        case 1:
            red = clamp5(br + red);
            green = clamp5(bg + green);
            blue = clamp5(bb + blue);
            break;
        case 2:
            red = clamp5(br - red);
            green = clamp5(bg - green);
            blue = clamp5(bb - blue);
            break;
        default:
            red = clamp5(br + (red >> 2));
            green = clamp5(bg + (green >> 2));
            blue = clamp5(bb + (blue >> 2));
            break;
        }
    }
    *pixel = static_cast<std::uint16_t>(red | green << 5 | blue << 10 | stp | (state.setMask ? 0x8000 : 0));
}

std::int64_t edge(const Vertex &a, const Vertex &b, int x, int y)
{
    return static_cast<std::int64_t>(b.x - a.x) * (y - a.y) - static_cast<std::int64_t>(b.y - a.y) * (x - a.x);
}

// A top or left edge owns its pixels; a bottom or right one doesn't.
bool ownsEdge(const Vertex &a, const Vertex &b)
{
    return b.y < a.y || (b.y == a.y && b.x > a.x);
}

void drawTriangle(const Shader &s, Vertex a, Vertex b, Vertex c)
{
    std::int64_t area = edge(a, b, c.x, c.y);
    if (area == 0) {
        return;
    }
    if (area < 0) {
        std::swap(b, c);
        area = -area;
    }
    int minX = std::min({a.x, b.x, c.x}), maxX = std::max({a.x, b.x, c.x});
    int minY = std::min({a.y, b.y, c.y}), maxY = std::max({a.y, b.y, c.y});
    if (maxX - minX >= kVramWidth || maxY - minY >= kVramHeight) {
        return;
    }
    const State &g = state();
    minX = std::max(minX, g.clipX0);
    maxX = std::min(maxX, g.clipX1);
    minY = std::max(minY, g.clipY0);
    maxY = std::min(maxY, g.clipY1);
    bool ownsAB = ownsEdge(a, b), ownsBC = ownsEdge(b, c), ownsCA = ownsEdge(c, a);
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            std::int64_t wc = edge(a, b, x, y), wa = edge(b, c, x, y), wb = edge(c, a, x, y);
            if (wa < 0 || wb < 0 || wc < 0 || (wa == 0 && !ownsBC) || (wb == 0 && !ownsCA) || (wc == 0 && !ownsAB)) {
                continue;
            }
            auto mix = [&](int va, int vb, int vc) {
                return static_cast<int>((wa * va + wb * vb + wc * vc) / area);
            };
            shade(s, x, y, mix(a.r, b.r, c.r), mix(a.g, b.g, c.g), mix(a.b, b.b, c.b), mix(a.u, b.u, c.u),
                  mix(a.v, b.v, c.v));
        }
    }
}

void drawLine(const Shader &s, Vertex a, const Vertex &b)
{
    const State &g = state();
    int dx = b.x - a.x, dy = b.y - a.y;
    int steps = std::max(std::abs(dx), std::abs(dy));

    if (steps >= kVramWidth) {
        return;
    }
    for (int i = 0; i <= steps; i++) {
        int t = steps != 0 ? i : 0, n = steps != 0 ? steps : 1;
        int x = a.x + dx * t / n, y = a.y + dy * t / n;
        if (x >= g.clipX0 && x <= g.clipX1 && y >= g.clipY0 && y <= g.clipY1) {
            shade(s, x, y, a.r + (b.r - a.r) * t / n, a.g + (b.g - a.g) * t / n, a.b + (b.b - a.b) * t / n, 0, 0);
        }
    }
}

void drawRect(const Shader &s, int x0, int y0, int w, int h, int r, int g, int b, int u0, int v0)
{
    const State &state = gpu::state();
    int startX = std::max(x0, state.clipX0), endX = std::min(x0 + w - 1, state.clipX1);
    int startY = std::max(y0, state.clipY0), endY = std::min(y0 + h - 1, state.clipY1);

    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            shade(s, x, y, r, g, b, u0 + x - x0, v0 + y - y0);
        }
    }
}

}  // namespace

namespace soft {

void triangle(const Shader &s, Vertex a, Vertex b, Vertex c)
{
    drawTriangle(s, a, b, c);
}

void line(const Shader &s, Vertex a, const Vertex &b)
{
    drawLine(s, a, b);
}

void rect(const Shader &s, int x0, int y0, int w, int h, int r, int g, int b, int u0, int v0)
{
    drawRect(s, x0, y0, w, h, r, g, b, u0, v0);
}

}  // namespace soft
}  // namespace gpu
