// SPDX-License-Identifier: GPL-2.0-or-later
// The GPU, in software: GP0 and GP1 commands, 1 MB of VRAM and the rasterizer. The game
// builds the same packets it sends a real GPU; this walks its ordering tables and draws them.
#include "hw.h"
#include "rt.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace {

constexpr int kVramWidth = 1024;
constexpr int kVramHeight = 512;
constexpr std::uint32_t kEndOfList = 0xffffff;
constexpr std::uint32_t kGuestRamMask = 0x1fffff;

std::uint16_t g_Vram[kVramHeight][kVramWidth];

// E1-E6 and GP1 state.
struct State {
    int pageX = 0, pageY = 0, depth = 0, transparency = 0;
    bool dither = false;
    int windowMaskX = 0, windowMaskY = 0, windowOffsetX = 0, windowOffsetY = 0;
    int clipX0 = 0, clipY0 = 0, clipX1 = 0, clipY1 = 0;
    int offsetX = 0, offsetY = 0;
    bool setMask = false, checkMask = false;
    int displayX = 0, displayY = 0;
    std::uint32_t displayMode = 0;
    bool displayOn = false;
} g_State;

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

struct Vertex {
    int x, y, r, g, b, u, v;
};

struct Shader {
    bool textured, raw, semi, gouraud;
    int clutX, clutY, pageX, pageY, depth;
};

constexpr int kDither[4][4] = {{-4, 0, -3, 1}, {2, -2, 3, -1}, {-3, 1, -4, 0}, {3, -1, 2, -2}};

std::uint16_t texel(const Shader &s, int u, int v)
{
    const State &g = g_State;

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
    const State &state = g_State;
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
    const State &g = g_State;
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
    const State &g = g_State;
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
    const State &state = g_State;
    int startX = std::max(x0, state.clipX0), endX = std::min(x0 + w - 1, state.clipX1);
    int startY = std::max(y0, state.clipY0), endY = std::min(y0 + h - 1, state.clipY1);

    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            shade(s, x, y, r, g, b, u0 + x - x0, v0 + y - y0);
        }
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
        if (gouraud && i > 0) {
            color = readColor({}, g_Words[word]);
        }
        word++;
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
        w = h = 1 << (size - 1) << 3 >> 3;
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
}

int commandLength(std::uint32_t cmd)
{
    switch (cmd >> 5) {
    case 1: {
        int count = cmd & 8 ? 4 : 3;
        return 1 + count * (cmd & 4 ? 2 : 1) + (cmd & 0x10 ? count - 1 + 0 : 0) + (cmd & 0x10 ? 0 : 0);
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
        if (cmd & 8) {
            Rt_Unimplemented("GP0 polyline");
        }
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

extern "C" {

void Hw_Gp0(unsigned int word)
{
    if (g_Upload.active) {
        uploadWord(word);
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
        g_Upload.active = false;
        g_Download.active = false;
        break;
    case 0x01:
        g_WordCount = 0;
        g_Upload.active = false;
        break;
    case 0x03:
        g.displayOn = !(word & 1);
        break;
    case 0x05:
        g.displayX = word & 0x3fe;
        g.displayY = (word >> 10) & 0x1ff;
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
    std::uint32_t address = reinterpret_cast<std::uintptr_t>(list) & 0xffffff;

    while ((address & 0x800000) == 0) {
        if (address > kGuestRamMask) {
            Rt_Fatal("GPU list link %06x is outside guest RAM", address);
        }
        auto *packet = reinterpret_cast<const std::uint32_t *>(static_cast<std::uintptr_t>(address | 0x80000000u));
        std::uint32_t tag = packet[0];
        for (std::uint32_t i = 1; i <= tag >> 24; i++) {
            Hw_Gp0(packet[i]);
        }
        address = tag & 0xffffff;
    }
}

void Hw_GpuFrame(unsigned int *pixels, int *width, int *height)
{
    static const int kWidths[4] = {256, 320, 512, 640};
    const State &g = g_State;
    int w = g.displayMode & 0x40 ? 368 : kWidths[g.displayMode & 3];
    int h = g.displayMode & 4 ? 480 : 240;
    bool truecolor = g.displayMode & 0x10;

    *width = w;
    *height = h;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned int rgb = 0;
            int row = (g.displayY + y) & (kVramHeight - 1);
            if (!g.displayOn) {
                rgb = 0;
            } else if (truecolor) {
                const auto *bytes = reinterpret_cast<const std::uint8_t *>(g_Vram[row]);
                int at = (g.displayX & 0x3fe) * 2 + x * 3;
                rgb = bytes[at] | bytes[at + 1] << 8 | bytes[at + 2] << 16;
            } else {
                std::uint16_t p = g_Vram[row][(g.displayX + x) & (kVramWidth - 1)];
                rgb = (p & 31) << 3 | ((p >> 5) & 31) << 11 | ((p >> 10) & 31) << 19;
            }
            pixels[y * w + x] = rgb;
        }
    }
}

}  // extern "C"
