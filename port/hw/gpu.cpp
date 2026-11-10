// SPDX-License-Identifier: GPL-2.0-or-later
// The GPU: GP0 and GP1 commands and 1 MB of VRAM. The game builds the same packets it sends a
// real GPU; this walks its ordering tables and hands the primitives to the software
// rasterizer (gpu_soft.cpp) or to the hardware renderer (gpu_hw.cpp).
#include "gpu_backend.h"
#include "gptr.h"
#include "hw.h"
#include "rt.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace gpu {

std::uint16_t g_Vram[kVramHeight][kVramWidth];

namespace {

constexpr std::uint32_t kEndOfList = 0xffffff;
constexpr std::uint32_t kGuestRamMask = 0x1fffff;

State g_State;
Backend *g_Backend;

// The command being gathered from GP0.
std::uint32_t g_Words[64];
int g_WordCount;
int g_WordsNeeded;

// A transfer to or from VRAM.
struct Transfer {
    int x, y, w, h, column, row;
    bool active;
} g_Upload, g_Download;

int signExtend11(std::uint32_t value)
{
    return static_cast<std::int32_t>(value << 21) >> 21;
}

void drawTriangle(const Shader &s, const Vertex &a, const Vertex &b, const Vertex &c)
{
    if (g_Backend != nullptr) {
        g_Backend->triangle(s, a, b, c);
    } else {
        soft::triangle(s, a, b, c);
    }
}

void drawLine(const Shader &s, const Vertex &a, const Vertex &b)
{
    if (g_Backend != nullptr) {
        g_Backend->line(s, a, b);
    } else {
        soft::line(s, a, b);
    }
}

void drawRect(const Shader &s, int x0, int y0, int w, int h, int r, int g, int b, int u0, int v0)
{
    if (g_Backend != nullptr) {
        g_Backend->rect(s, x0, y0, w, h, r, g, b, u0, v0);
    } else {
        soft::rect(s, x0, y0, w, h, r, g, b, u0, v0);
    }
}

// An upload is news for the hardware renderer once it's all in VRAM.
void finishUpload()
{
    if (g_Backend != nullptr) {
        g_Backend->uploaded(g_Upload.x, g_Upload.y, g_Upload.w, g_Upload.h);
    }
}

void setTexturePage(std::uint32_t value)
{
    g_State.pageX = (value & 0xf) * 64;
    g_State.pageY = ((value >> 4) & 1) * 256;
    g_State.transparency = (value >> 5) & 3;
    g_State.depth = (value >> 7) & 3;
    g_State.dither = (value >> 9) & 1;
}

Vertex readColor(Vertex v, std::uint32_t word)
{
    v.r = word & 0xff;
    v.g = (word >> 8) & 0xff;
    v.b = (word >> 16) & 0xff;
    return v;
}

Vertex readPosition(Vertex v, std::uint32_t word)
{
    v.x = signExtend11(word) + g_State.offsetX;
    v.y = signExtend11(word >> 16) + g_State.offsetY;
    return v;
}

void runPolygon()
{
    std::uint32_t cmd = g_Words[0] >> 24;
    bool gouraud = cmd & 0x10, quad = cmd & 8, textured = cmd & 4;
    int count = quad ? 4 : 3;
    Vertex v[4] = {};
    int word = 0;
    Shader s{};

    s.textured = textured;
    s.raw = textured && (cmd & 1);
    s.semi = cmd & 2;
    s.gouraud = gouraud;
    Vertex color = readColor({}, g_Words[0]);
    for (int i = 0; i < count; i++) {
        if (i == 0) {
            word = 1;
        } else if (gouraud) {
            color = readColor({}, g_Words[word++]);
        }
        v[i] = readPosition(color, g_Words[word++]);
        if (textured) {
            std::uint32_t uv = g_Words[word++];
            v[i].u = uv & 0xff;
            v[i].v = (uv >> 8) & 0xff;
            if (i == 0) {
                s.clutX = ((uv >> 16) & 0x3f) * 16;
                s.clutY = (uv >> 22) & 0x1ff;
            } else if (i == 1) {
                setTexturePage(uv >> 16);
            }
        }
    }
    s.pageX = g_State.pageX;
    s.pageY = g_State.pageY;
    s.depth = g_State.depth;
    drawTriangle(s, v[0], v[1], v[2]);
    if (quad) {
        drawTriangle(s, v[1], v[2], v[3]);
    }
}

struct Polyline {
    Shader shader;
    Vertex last;
    std::uint32_t color;
    bool haveColor;
    bool gouraud;
    bool active;
} g_Polyline;

constexpr std::uint32_t kPolylineEndMask = 0xf000f000;
constexpr std::uint32_t kPolylineEnd = 0x50005000;

void runLine()
{
    std::uint32_t cmd = g_Words[0] >> 24;
    bool gouraud = cmd & 0x10;
    Shader s{};
    int word = 1;

    s.semi = cmd & 2;
    s.gouraud = gouraud;
    Vertex from = readPosition(readColor({}, g_Words[0]), g_Words[word++]);
    Vertex to = gouraud ? readColor({}, g_Words[word++]) : from;
    to = readPosition(to, g_Words[word++]);
    drawLine(s, from, to);
    if (cmd & 8) {
        g_Polyline = {s, to, 0, false, gouraud, true};
    }
}

// The further points of a polyline, up to its terminator. In a shaded one, each point
// starts with its colour, and that's where the terminator goes.
void continuePolyline(std::uint32_t word)
{
    Polyline &line = g_Polyline;

    if (!line.haveColor && (word & kPolylineEndMask) == kPolylineEnd) {
        line.active = false;
        return;
    }
    if (line.gouraud && !line.haveColor) {
        line.color = word;
        line.haveColor = true;
        return;
    }
    Vertex to = readPosition(line.gouraud ? readColor({}, line.color) : line.last, word);
    drawLine(line.shader, line.last, to);
    line.last = to;
    line.haveColor = false;
}

void runRect()
{
    std::uint32_t cmd = g_Words[0] >> 24;
    bool textured = cmd & 4;
    int size = (cmd >> 3) & 3;
    int word = 1;
    Shader s{};

    s.textured = textured;
    s.raw = textured && (cmd & 1);
    s.semi = cmd & 2;
    s.pageX = g_State.pageX;
    s.pageY = g_State.pageY;
    s.depth = g_State.depth;
    Vertex v = readPosition(readColor({}, g_Words[0]), g_Words[word++]);
    if (textured) {
        std::uint32_t uv = g_Words[word++];
        v.u = uv & 0xff;
        v.v = (uv >> 8) & 0xff;
        s.clutX = ((uv >> 16) & 0x3f) * 16;
        s.clutY = (uv >> 22) & 0x1ff;
    }
    int w, h;
    if (size == 0) {
        w = g_Words[word] & 0x3ff;
        h = (g_Words[word] >> 16) & 0x1ff;
    } else {
        w = h = size == 1 ? 1 : size == 2 ? 8 : 16;
    }
    drawRect(s, v.x, v.y, w, h, v.r, v.g, v.b, v.u, v.v);
}

void fillRect()
{
    std::uint32_t c = g_Words[0];
    std::uint16_t color = (c >> 3 & 31) | (c >> 11 & 31) << 5 | (c >> 19 & 31) << 10;
    int x0 = g_Words[1] & 0x3f0, y0 = (g_Words[1] >> 16) & 0x1ff;
    int w = ((g_Words[2] & 0x3ff) + 0xf) & ~0xf, h = (g_Words[2] >> 16) & 0x1ff;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            g_Vram[(y0 + y) & (kVramHeight - 1)][(x0 + x) & (kVramWidth - 1)] = color;
        }
    }
    if (g_Backend != nullptr) {
        g_Backend->filled(x0, y0, w, h);
    }
}

void copyRect()
{
    int sx = g_Words[1] & 0x3ff, sy = (g_Words[1] >> 16) & 0x1ff;
    int dx = g_Words[2] & 0x3ff, dy = (g_Words[2] >> 16) & 0x1ff;
    int w = g_Words[3] & 0x3ff, h = (g_Words[3] >> 16) & 0x1ff;

    w = w != 0 ? w : kVramWidth;
    h = h != 0 ? h : kVramHeight;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            g_Vram[(dy + y) & (kVramHeight - 1)][(dx + x) & (kVramWidth - 1)] =
                g_Vram[(sy + y) & (kVramHeight - 1)][(sx + x) & (kVramWidth - 1)];
        }
    }
    if (g_Backend != nullptr) {
        g_Backend->copied(sx, sy, dx, dy, w, h);
    }
}

Transfer startTransfer()
{
    Transfer t{};

    t.x = g_Words[1] & 0x3ff;
    t.y = (g_Words[1] >> 16) & 0x1ff;
    t.w = g_Words[2] & 0x3ff;
    t.h = (g_Words[2] >> 16) & 0x1ff;
    t.w = t.w != 0 ? t.w : kVramWidth;
    t.h = t.h != 0 ? t.h : kVramHeight;
    t.active = true;
    return t;
}

std::uint16_t *transferPixel(Transfer &t)
{
    std::uint16_t *pixel = &g_Vram[(t.y + t.row) & (kVramHeight - 1)][(t.x + t.column) & (kVramWidth - 1)];

    if (++t.column == t.w) {
        t.column = 0;
        if (++t.row == t.h) {
            t.active = false;
        }
    }
    return pixel;
}

void uploadWord(std::uint32_t word)
{
    *transferPixel(g_Upload) = static_cast<std::uint16_t>(word);
    if (g_Upload.active) {
        *transferPixel(g_Upload) = static_cast<std::uint16_t>(word >> 16);
    }
    if (!g_Upload.active) {
        finishUpload();
    }
}

int commandLength(std::uint32_t cmd)
{
    switch (cmd >> 5) {
    case 1: {
        int count = cmd & 8 ? 4 : 3;
        return 1 + count * (cmd & 4 ? 2 : 1) + (cmd & 0x10 ? count - 1 : 0);
    }
    case 2:
        return cmd & 0x10 ? 4 : 3;
    case 3:
        return 2 + (cmd & 4 ? 1 : 0) + ((cmd >> 3) & 3 ? 0 : 1);
    case 4:
        return 4;
    case 5:
    case 6:
        return 3;
    default:
        return cmd == 2 ? 3 : 1;
    }
}

void runCommand()
{
    std::uint32_t word = g_Words[0];
    std::uint32_t cmd = word >> 24;

    switch (cmd >> 5) {
    case 1:
        runPolygon();
        return;
    case 2:
        runLine();
        return;
    case 3:
        runRect();
        return;
    case 4:
        copyRect();
        return;
    case 5:
        g_Upload = startTransfer();
        return;
    case 6:
        g_Download = startTransfer();
        if (g_Backend != nullptr) {
            g_Backend->readingBack(g_Download.x, g_Download.y, g_Download.w, g_Download.h);
        }
        return;
    default:
        break;
    }
    switch (cmd) {
    case 0x02:
        fillRect();
        break;
    case 0xe1:
        setTexturePage(word);
        break;
    case 0xe2:
        g_State.windowMaskX = word & 31;
        g_State.windowMaskY = (word >> 5) & 31;
        g_State.windowOffsetX = (word >> 10) & 31;
        g_State.windowOffsetY = (word >> 15) & 31;
        break;
    case 0xe3:
        g_State.clipX0 = word & 0x3ff;
        g_State.clipY0 = (word >> 10) & 0x1ff;
        break;
    case 0xe4:
        g_State.clipX1 = word & 0x3ff;
        g_State.clipY1 = (word >> 10) & 0x1ff;
        break;
    case 0xe5:
        g_State.offsetX = signExtend11(word);
        g_State.offsetY = signExtend11(word >> 11);
        break;
    case 0xe6:
        g_State.setMask = word & 1;
        g_State.checkMask = word & 2;
        break;
    default:
        break;
    }
}

}  // namespace

const State &state()
{
    return g_State;
}

void setBackend(Backend *backend)
{
    g_Backend = backend;
}

Display display()
{
    static const int kWidths[4] = {256, 320, 512, 640};
    static const int kClocks[4] = {10, 8, 5, 4};
    const State &g = g_State;
    int w = g.displayMode & 0x40 ? 368 : kWidths[g.displayMode & 3];
    int clocks = g.displayMode & 0x40 ? 7 : kClocks[g.displayMode & 3];
    // The screen shows the display range, which can be narrower than the mode: the game
    // draws 352 dots in the 368 dot mode, and what's in VRAM beside them was never visible.
    if (g.rangeEnd > g.rangeStart) {
        w = std::min(w, ((g.rangeEnd - g.rangeStart) / clocks + 2) & ~3);
    }
    int h = g.displayMode & 4 ? 480 : 240;
    return {g.displayX, g.displayY, w, h, g.displayOn, (g.displayMode & 0x10) != 0};
}

}  // namespace gpu

using namespace gpu;

extern "C" {

void Hw_Gp0(unsigned int word)
{
    if (g_Upload.active) {
        uploadWord(word);
        return;
    }
    if (g_Polyline.active) {
        continuePolyline(word);
        return;
    }
    if (g_WordCount == 0) {
        g_WordsNeeded = commandLength(word >> 24);
    }
    g_Words[g_WordCount++] = word;
    if (g_WordCount == g_WordsNeeded) {
        g_WordCount = 0;
        runCommand();
    }
}

void Hw_Gp1(unsigned int word)
{
    State &g = g_State;

    switch (word >> 24) {
    case 0x00:
        g = State{};
        g_WordCount = 0;
        g_Polyline.active = false;
        g_Upload.active = false;
        g_Download.active = false;
        break;
    case 0x01:
        g_WordCount = 0;
        if (g_Upload.active) {
            g_Upload.active = false;
            finishUpload();
        }
        break;
    case 0x03:
        g.displayOn = !(word & 1);
        break;
    case 0x05:
        g.displayX = word & 0x3fe;
        g.displayY = (word >> 10) & 0x1ff;
        break;
    case 0x06:
        g.rangeStart = word & 0xfff;
        g.rangeEnd = (word >> 12) & 0xfff;
        break;
    case 0x08:
        g.displayMode = word & 0xff;
        break;
    default:
        break;
    }
}

unsigned int Hw_GpuRead(void)
{
    unsigned int word = 0;

    if (g_Download.active) {
        word = *transferPixel(g_Download);
    }
    if (g_Download.active) {
        word |= static_cast<unsigned int>(*transferPixel(g_Download)) << 16;
    }
    return word;
}

// DMA down a linked list. Each tag is a word count and the 24-bit address of the next one.
void Hw_GpuDrawList(const void *list)
{
    std::uint32_t address = rt::to_guest(list) & 0xffffff;

    while ((address & 0x800000) == 0) {
        if (address > kGuestRamMask) {
            Rt_Fatal("GPU list link %06x is outside guest RAM", address);
        }
        auto *packet = rt::guest<const std::uint32_t>(address | 0x80000000u);
        std::uint32_t tag = packet[0];
        for (std::uint32_t i = 1; i <= tag >> 24; i++) {
            Hw_Gp0(packet[i]);
        }
        address = tag & 0xffffff;
    }
}

void Hw_GpuFrame(unsigned int *pixels, int *width, int *height)
{
    Display d = display();
    int w = d.width, h = d.height;

    *width = w;
    *height = h;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned int rgb = 0;
            int row = (d.y + y) & (kVramHeight - 1);
            if (!d.on) {
                rgb = 0;
            } else if (d.truecolor) {
                const auto *bytes = reinterpret_cast<const std::uint8_t *>(g_Vram[row]);
                int at = (d.x & 0x3fe) * 2 + x * 3;
                rgb = bytes[at] | bytes[at + 1] << 8 | bytes[at + 2] << 16;
            } else {
                std::uint16_t p = g_Vram[row][(d.x + x) & (kVramWidth - 1)];
                rgb = (p & 31) << 3 | ((p >> 5) & 31) << 11 | ((p >> 10) & 31) << 19;
            }
            pixels[y * w + x] = rgb;
        }
    }
}

}  // extern "C"
