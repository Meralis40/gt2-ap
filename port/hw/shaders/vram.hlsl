// SPDX-License-Identifier: GPL-2.0-or-later
// The hardware renderer's one shader pair: draws the GPU's primitives into VRAM at N times
// its size, by the same rules as the software rasterizer (gpu_soft.cpp). Colours stay 5-bit,
// stored as x/31 in RGBA8, with the mask bit in alpha. Textures and CLUTs come from a copy
// of VRAM as the CPU has it, at its own size, in R16_UNORM: VRAM's 16-bit words as they are.
//
// Resources follow SDL_GPU's layout: vertex uniforms in space1, fragment textures and samplers
// in space2, fragment uniforms in space3 (sets for SPIR-V).

struct VertexIn {
    [[vk::location(0)]] float2 position : TEXCOORD0;  // in VRAM pixels
    [[vk::location(1)]] float4 color : TEXCOORD1;     // 0-255
    [[vk::location(2)]] float2 uv : TEXCOORD2;        // in texels
    [[vk::location(3)]] int4 page : TEXCOORD3;        // CLUT x, CLUT y, page x, page y
    [[vk::location(4)]] int flags : TEXCOORD4;        // see kFlag*
};

struct FragmentIn {
    float4 position : SV_Position;
    [[vk::location(0)]] noperspective float4 color : TEXCOORD0;
    [[vk::location(1)]] noperspective float2 uv : TEXCOORD1;
    [[vk::location(2)]] nointerpolation int4 page : TEXCOORD2;
    [[vk::location(3)]] nointerpolation int flags : TEXCOORD3;
};

static const int kFlagDepthMask = 3;  // 4-bit, 8-bit, 15-bit texels
static const int kFlagTextured = 4;
static const int kFlagRaw = 8;        // texels as they are, not by the vertex colour
static const int kFlagDither = 16;
static const int kFlagCopy = 32;      // the VRAM copy's words, at uv, as they are

FragmentIn VertexMain(VertexIn input)
{
    FragmentIn output;
    output.position = float4(input.position.x / 512.0 - 1.0, 1.0 - input.position.y / 256.0, 0.0, 1.0);
    output.color = input.color;
    output.uv = input.uv;
    output.page = input.page;
    output.flags = input.flags;
    return output;
}

[[vk::combinedImageSampler]] [[vk::binding(0, 2)]]
Texture2D<float> g_Vram : register(t0, space2);
[[vk::combinedImageSampler]] [[vk::binding(0, 2)]]
SamplerState g_VramSampler : register(s0, space2);

[[vk::binding(0, 3)]]
cbuffer Draw : register(b0, space3)
{
    int4 g_Window;  // E2: mask x, mask y, offset x, offset y
    int g_Scale;
    // 0 draws everything, 1 only what isn't blended, 2 only what is: a textured semi-
    // transparent primitive is blended where its texels have bit 15 set.
    int g_Pass;
    int g_SetMask;
    int g_Unused;
};

static const int kDither[4][4] = {{-4, 0, -3, 1}, {2, -2, 3, -1}, {-3, 1, -4, 0}, {3, -1, 2, -2}};

uint vramWord(int x, int y)
{
    float2 at = (float2(x & 1023, y & 511) + 0.5) / float2(1024.0, 512.0);
    return (uint)round(g_Vram.SampleLevel(g_VramSampler, at, 0) * 65535.0);
}

uint texel(int depth, int4 page, int u, int v)
{
    u = (u & 0xff & ~(g_Window.x * 8)) | ((g_Window.z & g_Window.x) * 8);
    v = (v & 0xff & ~(g_Window.y * 8)) | ((g_Window.w & g_Window.y) * 8);
    int y = page.w + v;
    if (depth == 0) {
        uint index = (vramWord(page.z + u / 4, y) >> ((u & 3) * 4)) & 0xf;
        return vramWord(page.x + (int)index, page.y);
    }
    if (depth == 1) {
        uint index = (vramWord(page.z + u / 2, y) >> ((u & 1) * 8)) & 0xff;
        return vramWord(page.x + (int)index, page.y);
    }
    return vramWord(page.z + u, y);
}

int3 channels(uint word)
{
    return int3(word & 31, (word >> 5) & 31, (word >> 10) & 31);
}

float4 FragmentMain(FragmentIn input) : SV_Target0
{
    int flags = input.flags;
    int2 dot = int2(input.position.xy) / g_Scale;
    int d = (flags & kFlagDither) != 0 ? kDither[dot.y & 3][dot.x & 3] : 0;
    int3 rgb = int3(floor(input.color.rgb));
    int3 colour;
    uint stp = 0;
    bool textured = (flags & kFlagTextured) != 0;

    if ((flags & kFlagCopy) != 0) {
        uint word = vramWord(int(floor(input.uv.x)), int(floor(input.uv.y)));
        return float4(float3(channels(word)) / 31.0, (word >> 15) != 0 ? 1.0 : 0.0);
    }
    if (textured) {
        uint t = texel(flags & kFlagDepthMask, input.page, int(floor(input.uv.x)), int(floor(input.uv.y)));
        if (t == 0) {
            discard;
        }
        stp = t >> 15;
        int3 tc = channels(t);
        if ((flags & kFlagRaw) != 0) {
            colour = tc;
        } else {
            colour = clamp(((((tc << 3) * rgb) >> 7) + d) >> 3, 0, 31);
        }
    } else {
        colour = clamp((rgb + d) >> 3, 0, 31);
    }
    bool blended = !textured || stp != 0;
    if ((g_Pass == 1 && blended) || (g_Pass == 2 && !blended)) {
        discard;
    }
    return float4(float3(colour) / 31.0, (stp != 0 || g_SetMask != 0) ? 1.0 : 0.0);
}
