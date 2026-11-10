// SPDX-License-Identifier: GPL-2.0-or-later
// The hardware renderer. VRAM is a texture N times the console's in each direction, which the
// primitives are drawn into by the rules of the software rasterizer (shaders/vram.hlsl).
// Textures and CLUTs are sampled from a second texture, VRAM at its own size as the CPU has
// it: the game uploads its textures, and never draws into what it then samples (that's
// checked, and logged once if it happens).
//
// What the game sends during a frame is only recorded; submit() plays it to the graphics card
// in order, uploads and copies between the draws.
#include "gpu_hw.h"

#include "gpu_backend.h"
#include "rt.h"
#include "shaders/gen/vram.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace gpu::hw {
namespace {

constexpr int kFlagTextured = 4;
constexpr int kFlagRaw = 8;
constexpr int kFlagDither = 16;
constexpr int kFlagCopy = 32;

constexpr int kNoBlend = 4;  // after the four semi-transparency modes
constexpr int kPipelines = 5;

struct GpuVertex {
    float x, y;
    float r, g, b, a;
    float u, v;
    std::int32_t clutX, clutY, pageX, pageY;
    std::int32_t flags;
};

// The fragment shader's uniforms, as vram.hlsl has them.
struct Uniforms {
    std::int32_t window[4];
    std::int32_t scale, pass, setMask, unused;
};

struct Scissor {
    int x0, y0, x1, y1;  // inclusive, in VRAM pixels

    bool operator==(const Scissor &) const = default;
};

enum class Kind { Draw, Upload, Copy };

struct Command {
    Kind kind;
    // Draw: the vertices, and what they're drawn with. Semi-transparent ones (blend 0-3)
    // take two passes: their unblended texels, then the blended ones.
    int first, count, blend;
    Scissor scissor;
    int window[4];
    bool setMask;
    // Upload: a rectangle of VRAM (inside it), from the words at offset in g_Uploads. Copy:
    // from sx, sy.
    int x, y, w, h, sx, sy;
    std::size_t offset;
};

SDL_Renderer *g_Renderer;
SDL_GPUDevice *g_Device;
SDL_GPUGraphicsPipeline *g_Pipelines[kPipelines];
SDL_GPUSampler *g_Sampler;
SDL_GPUTexture *g_Target;   // VRAM at scale
SDL_GPUTexture *g_Scratch;  // for copies within VRAM
SDL_GPUTexture *g_Native;   // VRAM at 1x, R16_UNORM
SDL_Texture *g_Shown;       // g_Target, for the SDL renderer
SDL_GPUBuffer *g_VertexBuffer;
std::size_t g_VertexBufferSize;
SDL_GPUTransferBuffer *g_Transfer;
std::size_t g_TransferSize;
int g_Scale = 1;
bool g_Dither = true;
bool g_Active;

std::vector<GpuVertex> g_Vertices;
std::vector<Command> g_Commands;
std::vector<std::uint16_t> g_Uploads;

// What was drawn since it was last uploaded, in 16 by 16 blocks: for the check that textures
// don't come from there.
constexpr int kBlock = 16;
bool g_Drawn[kVramHeight / kBlock][kVramWidth / kBlock];
bool g_ToldSampling, g_ToldMask, g_ToldReadBack;

void markBlocks(int x0, int y0, int x1, int y1, bool drawn)
{
    x0 = std::clamp(x0, 0, kVramWidth - 1) / kBlock;
    x1 = std::clamp(x1, 0, kVramWidth - 1) / kBlock;
    y0 = std::clamp(y0, 0, kVramHeight - 1) / kBlock;
    y1 = std::clamp(y1, 0, kVramHeight - 1) / kBlock;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            g_Drawn[y][x] = drawn;
        }
    }
}

bool anyDrawn(int x0, int y0, int x1, int y1)
{
    x0 = std::clamp(x0, 0, kVramWidth - 1) / kBlock;
    x1 = std::clamp(x1, 0, kVramWidth - 1) / kBlock;
    y0 = std::clamp(y0, 0, kVramHeight - 1) / kBlock;
    y1 = std::clamp(y1, 0, kVramHeight - 1) / kBlock;
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            if (g_Drawn[y][x]) {
                return true;
            }
        }
    }
    return false;
}

// Logs, once, a texture or CLUT read from where the game drew: the renderer would be wrong
// there, since it samples VRAM as the CPU has it.
void checkSampling(const Shader &s, const Vertex *v, int count)
{
    if (!s.textured || g_ToldSampling) {
        return;
    }
    int u0 = 255, u1 = 0, v0 = 255, v1 = 0;
    for (int i = 0; i < count; i++) {
        u0 = std::min(u0, v[i].u & 0xff);
        u1 = std::max(u1, v[i].u & 0xff);
        v0 = std::min(v0, v[i].v & 0xff);
        v1 = std::max(v1, v[i].v & 0xff);
    }
    int shift = s.depth == 0 ? 2 : s.depth == 1 ? 1 : 0;
    bool texels = anyDrawn(s.pageX + (u0 >> shift), s.pageY + v0, s.pageX + (u1 >> shift), s.pageY + v1);
    bool clut = s.depth < 2 && anyDrawn(s.clutX, s.clutY, s.clutX + (s.depth == 0 ? 15 : 255), s.clutY);
    if (texels || clut) {
        g_ToldSampling = true;
        Rt_Log("hardware renderer: a %s at %d,%d comes from what was drawn there; it'll look wrong",
               texels ? "texture" : "CLUT", texels ? s.pageX : s.clutX, texels ? s.pageY : s.clutY);
    }
}

void checkMask()
{
    const State &g = gpu::state();
    if ((g.checkMask || g.setMask) && !g_ToldMask) {
        g_ToldMask = true;
        Rt_Log("hardware renderer: the mask bit is in use, and not supported");
    }
}

Scissor currentScissor()
{
    const State &g = gpu::state();
    return {g.clipX0, g.clipY0, g.clipX1, g.clipY1};
}

// The draw command these vertices go in: the last one, if nothing they're drawn with changed.
void addDraw(int blend, const GpuVertex *vertices, int count, const Scissor &scissor, bool setMask)
{
    const State &g = gpu::state();
    int window[4] = {g.windowMaskX, g.windowMaskY, g.windowOffsetX, g.windowOffsetY};

    if (g_Commands.empty() || g_Commands.back().kind != Kind::Draw || g_Commands.back().blend != blend ||
        !(g_Commands.back().scissor == scissor) || g_Commands.back().setMask != setMask ||
        std::memcmp(g_Commands.back().window, window, sizeof window) != 0) {
        Command c{};
        c.kind = Kind::Draw;
        c.first = static_cast<int>(g_Vertices.size());
        c.blend = blend;
        c.scissor = scissor;
        std::memcpy(c.window, window, sizeof window);
        c.setMask = setMask;
        g_Commands.push_back(c);
    }
    g_Vertices.insert(g_Vertices.end(), vertices, vertices + count);
    g_Commands.back().count += count;
}

int flagsFor(const Shader &s)
{
    const State &g = gpu::state();
    int flags = s.depth & 3;
    bool dither = g_Dither && g.dither && (s.textured ? !s.raw : s.gouraud);

    flags |= s.textured ? kFlagTextured : 0;
    flags |= s.raw ? kFlagRaw : 0;
    flags |= dither ? kFlagDither : 0;
    return flags;
}

GpuVertex vertexFor(const Shader &s, int flags, float x, float y, int r, int g, int b, float u, float v)
{
    return {x, y, static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), 0, u, v,
            s.clutX, s.clutY, s.pageX, s.pageY, flags};
}

// Corners: top left, top right, bottom left, bottom right.
void addQuad(const GpuVertex corners[4], const Scissor &scissor, bool setMask, int blend)
{
    GpuVertex six[6] = {corners[0], corners[1], corners[2], corners[1], corners[3], corners[2]};
    addDraw(blend, six, 6, scissor, setMask);
}

int blendFor(const Shader &s)
{
    return s.semi ? gpu::state().transparency : kNoBlend;
}

void markDrawn(int x0, int y0, int x1, int y1, const Scissor &clip)
{
    markBlocks(std::max(x0, clip.x0), std::max(y0, clip.y0), std::min(x1, clip.x1), std::min(y1, clip.y1), true);
}

// VRAM as the CPU has it, from x, y, w by h, which can wrap, to the 1x texture; and to the
// scaled one too, if blit.
void addUpload(int x, int y, int w, int h, bool blit)
{
    for (int top = 0; top < h;) {
        int row = (y + top) & (kVramHeight - 1);
        int rows = std::min(h - top, kVramHeight - row);
        for (int left = 0; left < w;) {
            int column = (x + left) & (kVramWidth - 1);
            int columns = std::min(w - left, kVramWidth - column);
            Command c{};
            c.kind = Kind::Upload;
            c.x = column;
            c.y = row;
            c.w = columns;
            c.h = rows;
            c.offset = g_Uploads.size();
            for (int i = 0; i < rows; i++) {
                const std::uint16_t *from = &g_Vram[row + i][column];
                g_Uploads.insert(g_Uploads.end(), from, from + columns);
            }
            g_Commands.push_back(c);
            markBlocks(column, row, column + columns - 1, row + rows - 1, false);
            if (blit) {
                Shader none{};
                float fx0 = static_cast<float>(column), fy0 = static_cast<float>(row);
                float fx1 = fx0 + static_cast<float>(columns), fy1 = fy0 + static_cast<float>(rows);
                GpuVertex corners[4] = {
                    vertexFor(none, kFlagCopy, fx0, fy0, 0, 0, 0, fx0, fy0),
                    vertexFor(none, kFlagCopy, fx1, fy0, 0, 0, 0, fx1, fy0),
                    vertexFor(none, kFlagCopy, fx0, fy1, 0, 0, 0, fx0, fy1),
                    vertexFor(none, kFlagCopy, fx1, fy1, 0, 0, 0, fx1, fy1),
                };
                addQuad(corners, {0, 0, kVramWidth - 1, kVramHeight - 1}, false, kNoBlend);
            }
            left += columns;
        }
        top += rows;
    }
}

class HardwareBackend final : public Backend {
public:
    void triangle(const Shader &s, const Vertex &a, const Vertex &b, const Vertex &c) override
    {
        int minX = std::min({a.x, b.x, c.x}), maxX = std::max({a.x, b.x, c.x});
        int minY = std::min({a.y, b.y, c.y}), maxY = std::max({a.y, b.y, c.y});
        // As the rasterizer does: too big to draw.
        if (maxX - minX >= kVramWidth || maxY - minY >= kVramHeight) {
            return;
        }
        Vertex v[3] = {a, b, c};
        checkSampling(s, v, 3);
        checkMask();
        int flags = flagsFor(s);
        GpuVertex out[3];
        for (int i = 0; i < 3; i++) {
            out[i] = vertexFor(s, flags, static_cast<float>(v[i].x), static_cast<float>(v[i].y), v[i].r, v[i].g,
                               v[i].b, static_cast<float>(v[i].u), static_cast<float>(v[i].v));
        }
        Scissor clip = currentScissor();
        addDraw(blendFor(s), out, 3, clip, gpu::state().setMask);
        markDrawn(minX, minY, maxX, maxY, clip);
    }

    // A line one dot thick, as a quad.
    void line(const Shader &s, const Vertex &a, const Vertex &b) override
    {
        int dx = b.x - a.x, dy = b.y - a.y;
        if (std::max(std::abs(dx), std::abs(dy)) >= kVramWidth) {
            return;
        }
        checkMask();
        int flags = flagsFor(s);
        bool across = std::abs(dx) >= std::abs(dy);
        float ax = static_cast<float>(a.x), ay = static_cast<float>(a.y);
        float bx = static_cast<float>(b.x), by = static_cast<float>(b.y);
        // Out to the far side of the last dot, and one dot wide.
        if (across) {
            (bx >= ax ? bx : ax) += 1;
        } else {
            (by >= ay ? by : ay) += 1;
        }
        float ox = across ? 0.0f : 1.0f, oy = across ? 1.0f : 0.0f;
        GpuVertex corners[4] = {
            vertexFor(s, flags, ax, ay, a.r, a.g, a.b, 0, 0),
            vertexFor(s, flags, bx, by, b.r, b.g, b.b, 0, 0),
            vertexFor(s, flags, ax + ox, ay + oy, a.r, a.g, a.b, 0, 0),
            vertexFor(s, flags, bx + ox, by + oy, b.r, b.g, b.b, 0, 0),
        };
        Scissor clip = currentScissor();
        addQuad(corners, clip, gpu::state().setMask, blendFor(s));
        markDrawn(std::min(a.x, b.x), std::min(a.y, b.y), std::max(a.x, b.x), std::max(a.y, b.y), clip);
    }

    void rect(const Shader &s, int x, int y, int w, int h, int r, int g, int b, int u, int v) override
    {
        if (w <= 0 || h <= 0) {
            return;
        }
        Vertex corners[2] = {{x, y, r, g, b, u, v}, {x + w - 1, y + h - 1, r, g, b, u + w - 1, v + h - 1}};
        checkSampling(s, corners, 2);
        checkMask();
        int flags = flagsFor(s);
        float x0 = static_cast<float>(x), y0 = static_cast<float>(y);
        float x1 = static_cast<float>(x + w), y1 = static_cast<float>(y + h);
        float u0 = static_cast<float>(u), v0 = static_cast<float>(v);
        float u1 = static_cast<float>(u + w), v1 = static_cast<float>(v + h);
        GpuVertex quad[4] = {
            vertexFor(s, flags, x0, y0, r, g, b, u0, v0),
            vertexFor(s, flags, x1, y0, r, g, b, u1, v0),
            vertexFor(s, flags, x0, y1, r, g, b, u0, v1),
            vertexFor(s, flags, x1, y1, r, g, b, u1, v1),
        };
        Scissor clip = currentScissor();
        addQuad(quad, clip, gpu::state().setMask, blendFor(s));
        markDrawn(x, y, x + w - 1, y + h - 1, clip);
    }

    void filled(int x, int y, int w, int h) override
    {
        addUpload(x, y, w, h, true);
    }

    void copied(int sx, int sy, int dx, int dy, int w, int h) override
    {
        // VRAM at 1x has the copy already; the scaled one copies what was drawn too. A copy
        // that wraps around VRAM's edge stops at it.
        addUpload(dx, dy, w, h, false);
        w = std::min({w, kVramWidth - sx, kVramWidth - dx});
        h = std::min({h, kVramHeight - sy, kVramHeight - dy});
        if (w > 0 && h > 0) {
            Command c{};
            c.kind = Kind::Copy;
            c.sx = sx;
            c.sy = sy;
            c.x = dx;
            c.y = dy;
            c.w = w;
            c.h = h;
            g_Commands.push_back(c);
            if (anyDrawn(sx, sy, sx + w - 1, sy + h - 1)) {
                markBlocks(dx, dy, dx + w - 1, dy + h - 1, true);
            }
        }
    }

    void uploaded(int x, int y, int w, int h) override
    {
        addUpload(x, y, w, h, true);
    }

    void readingBack(int, int, int, int) override
    {
        if (!g_ToldReadBack) {
            g_ToldReadBack = true;
            Rt_Log("hardware renderer: the game reads VRAM back, which has only what the CPU put there");
        }
    }
} g_Backend;

SDL_GPUShader *makeShader(SDL_GPUShaderStage stage, SDL_GPUShaderFormat format)
{
    bool vertex = stage == SDL_GPU_SHADERSTAGE_VERTEX;
    SDL_GPUShaderCreateInfo info{};

    if (format == SDL_GPU_SHADERFORMAT_SPIRV) {
        info.code = vertex ? k_vram_vertex_spirv : k_vram_fragment_spirv;
        info.code_size = vertex ? sizeof k_vram_vertex_spirv : sizeof k_vram_fragment_spirv;
    } else {
        info.code = vertex ? k_vram_vertex_dxil : k_vram_fragment_dxil;
        info.code_size = vertex ? sizeof k_vram_vertex_dxil : sizeof k_vram_fragment_dxil;
    }
    info.entrypoint = vertex ? "VertexMain" : "FragmentMain";
    info.format = format;
    info.stage = stage;
    info.num_samplers = vertex ? 0 : 1;
    info.num_uniform_buffers = vertex ? 0 : 1;
    return SDL_CreateGPUShader(g_Device, &info);
}

SDL_GPUColorTargetBlendState blendState(int blend)
{
    SDL_GPUColorTargetBlendState state{};

    if (blend == kNoBlend) {
        return state;
    }
    state.enable_blend = true;
    state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    switch (blend) {
    case 0:  // B/2 + F/2, by a constant of 0.5
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
        break;
    case 1:  // B + F
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        break;
    case 2:  // B - F
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        state.color_blend_op = SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
        break;
    default:  // B + F/4, by a constant of 0.25
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        break;
    }
    return state;
}

bool makePipelines(SDL_GPUShaderFormat format)
{
    SDL_GPUShader *vertex = makeShader(SDL_GPU_SHADERSTAGE_VERTEX, format);
    SDL_GPUShader *fragment = makeShader(SDL_GPU_SHADERSTAGE_FRAGMENT, format);
    bool made = vertex != nullptr && fragment != nullptr;

    SDL_GPUVertexBufferDescription buffer{0, sizeof(GpuVertex), SDL_GPU_VERTEXINPUTRATE_VERTEX, 0};
    SDL_GPUVertexAttribute attributes[] = {
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(GpuVertex, x)},
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(GpuVertex, r)},
        {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(GpuVertex, u)},
        {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_INT4, offsetof(GpuVertex, clutX)},
        {4, 0, SDL_GPU_VERTEXELEMENTFORMAT_INT, offsetof(GpuVertex, flags)},
    };
    for (int blend = 0; made && blend < kPipelines; blend++) {
        SDL_GPUColorTargetDescription target{SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, blendState(blend)};
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = vertex;
        info.fragment_shader = fragment;
        info.vertex_input_state.vertex_buffer_descriptions = &buffer;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attributes;
        info.vertex_input_state.num_vertex_attributes = SDL_arraysize(attributes);
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        g_Pipelines[blend] = SDL_CreateGPUGraphicsPipeline(g_Device, &info);
        made = g_Pipelines[blend] != nullptr;
    }
    if (vertex != nullptr) {
        SDL_ReleaseGPUShader(g_Device, vertex);
    }
    if (fragment != nullptr) {
        SDL_ReleaseGPUShader(g_Device, fragment);
    }
    return made;
}

SDL_GPUTexture *makeTexture(SDL_GPUTextureFormat format, int width, int height, SDL_GPUTextureUsageFlags usage)
{
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = format;
    info.usage = usage;
    info.width = static_cast<Uint32>(width);
    info.height = static_cast<Uint32>(height);
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    return SDL_CreateGPUTexture(g_Device, &info);
}

constexpr SDL_GPUTextureUsageFlags kTargetUsage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

// The scaled textures, and the renderer's view of g_Target.
bool makeTargets(int scale, SDL_GPUTexture **target, SDL_GPUTexture **scratch, SDL_Texture **shown)
{
    *target = makeTexture(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, kVramWidth * scale, kVramHeight * scale, kTargetUsage);
    *scratch = makeTexture(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, kVramWidth * scale, kVramHeight * scale, kTargetUsage);
    *shown = nullptr;
    if (*target == nullptr || *scratch == nullptr) {
        return false;
    }
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_TEXTURE_CREATE_GPU_TEXTURE_POINTER, *target);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_ABGR8888);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_STATIC);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, kVramWidth * scale);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, kVramHeight * scale);
    *shown = SDL_CreateTextureWithProperties(g_Renderer, props);
    SDL_DestroyProperties(props);
    if (*shown == nullptr) {
        return false;
    }
    SDL_SetTextureBlendMode(*shown, SDL_BLENDMODE_NONE);
    SDL_SetTextureScaleMode(*shown, SDL_SCALEMODE_NEAREST);
    return true;
}

void releaseTargets(SDL_GPUTexture *target, SDL_GPUTexture *scratch, SDL_Texture *shown)
{
    if (shown != nullptr) {
        SDL_DestroyTexture(shown);
    }
    if (target != nullptr) {
        SDL_ReleaseGPUTexture(g_Device, target);
    }
    if (scratch != nullptr) {
        SDL_ReleaseGPUTexture(g_Device, scratch);
    }
}

void releaseAll()
{
    releaseTargets(g_Target, g_Scratch, g_Shown);
    g_Target = g_Scratch = nullptr;
    g_Shown = nullptr;
    for (SDL_GPUGraphicsPipeline *&pipeline : g_Pipelines) {
        if (pipeline != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(g_Device, pipeline);
            pipeline = nullptr;
        }
    }
    if (g_Native != nullptr) {
        SDL_ReleaseGPUTexture(g_Device, g_Native);
        g_Native = nullptr;
    }
    if (g_Sampler != nullptr) {
        SDL_ReleaseGPUSampler(g_Device, g_Sampler);
        g_Sampler = nullptr;
    }
    if (g_VertexBuffer != nullptr) {
        SDL_ReleaseGPUBuffer(g_Device, g_VertexBuffer);
        g_VertexBuffer = nullptr;
        g_VertexBufferSize = 0;
    }
    if (g_Transfer != nullptr) {
        SDL_ReleaseGPUTransferBuffer(g_Device, g_Transfer);
        g_Transfer = nullptr;
        g_TransferSize = 0;
    }
}

bool reserve(std::size_t vertexBytes, std::size_t transferBytes)
{
    if (vertexBytes > g_VertexBufferSize) {
        if (g_VertexBuffer != nullptr) {
            SDL_ReleaseGPUBuffer(g_Device, g_VertexBuffer);
        }
        g_VertexBufferSize = std::max(vertexBytes, g_VertexBufferSize * 2);
        SDL_GPUBufferCreateInfo info{SDL_GPU_BUFFERUSAGE_VERTEX, static_cast<Uint32>(g_VertexBufferSize), 0};
        g_VertexBuffer = SDL_CreateGPUBuffer(g_Device, &info);
    }
    if (transferBytes > g_TransferSize) {
        if (g_Transfer != nullptr) {
            SDL_ReleaseGPUTransferBuffer(g_Device, g_Transfer);
        }
        g_TransferSize = std::max(transferBytes, g_TransferSize * 2);
        SDL_GPUTransferBufferCreateInfo info{SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, static_cast<Uint32>(g_TransferSize), 0};
        g_Transfer = SDL_CreateGPUTransferBuffer(g_Device, &info);
    }
    return (vertexBytes == 0 || g_VertexBuffer != nullptr) && g_Transfer != nullptr;
}

SDL_GPUTextureRegion region(SDL_GPUTexture *texture, int x, int y, int w, int h, int scale)
{
    SDL_GPUTextureRegion r{};
    r.texture = texture;
    r.x = static_cast<Uint32>(x * scale);
    r.y = static_cast<Uint32>(y * scale);
    r.w = static_cast<Uint32>(w * scale);
    r.h = static_cast<Uint32>(h * scale);
    r.d = 1;
    return r;
}

SDL_GPUTextureLocation location(SDL_GPUTexture *texture, int x, int y, int scale)
{
    SDL_GPUTextureLocation l{};
    l.texture = texture;
    l.x = static_cast<Uint32>(x * scale);
    l.y = static_cast<Uint32>(y * scale);
    return l;
}

// Plays the recorded commands into this command buffer.
void record(SDL_GPUCommandBuffer *commands)
{
    std::size_t vertexBytes = g_Vertices.size() * sizeof(GpuVertex);
    std::size_t uploadBytes = g_Uploads.size() * sizeof(std::uint16_t);
    if (vertexBytes + uploadBytes == 0 || !reserve(vertexBytes, vertexBytes + uploadBytes)) {
        return;
    }
    auto *mapped = static_cast<unsigned char *>(SDL_MapGPUTransferBuffer(g_Device, g_Transfer, true));
    if (mapped == nullptr) {
        return;
    }
    if (vertexBytes != 0) {
        std::memcpy(mapped, g_Vertices.data(), vertexBytes);
    }
    if (uploadBytes != 0) {
        std::memcpy(mapped + vertexBytes, g_Uploads.data(), uploadBytes);
    }
    SDL_UnmapGPUTransferBuffer(g_Device, g_Transfer);

    if (vertexBytes != 0) {
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
        SDL_GPUTransferBufferLocation from{g_Transfer, 0};
        SDL_GPUBufferRegion to{g_VertexBuffer, 0, static_cast<Uint32>(vertexBytes)};
        SDL_UploadToGPUBuffer(copy, &from, &to, false);
        SDL_EndGPUCopyPass(copy);
    }

    SDL_GPURenderPass *pass = nullptr;
    int bound = -1;
    auto endPass = [&] {
        if (pass != nullptr) {
            SDL_EndGPURenderPass(pass);
            pass = nullptr;
            bound = -1;
        }
    };
    for (std::size_t i = 0; i < g_Commands.size(); i++) {
        const Command &c = g_Commands[i];
        switch (c.kind) {
        case Kind::Upload: {
            endPass();
            SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
            for (; i < g_Commands.size() && g_Commands[i].kind == Kind::Upload; i++) {
                const Command &u = g_Commands[i];
                SDL_GPUTextureTransferInfo from{};
                from.transfer_buffer = g_Transfer;
                from.offset = static_cast<Uint32>(vertexBytes + u.offset * sizeof(std::uint16_t));
                from.pixels_per_row = static_cast<Uint32>(u.w);
                from.rows_per_layer = static_cast<Uint32>(u.h);
                SDL_GPUTextureRegion to = region(g_Native, u.x, u.y, u.w, u.h, 1);
                SDL_UploadToGPUTexture(copy, &from, &to, false);
            }
            i--;
            SDL_EndGPUCopyPass(copy);
            break;
        }
        case Kind::Copy: {
            endPass();
            SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
            SDL_GPUTextureLocation from = location(g_Target, c.sx, c.sy, g_Scale);
            SDL_GPUTextureLocation scratch = location(g_Scratch, c.sx, c.sy, g_Scale);
            Uint32 w = static_cast<Uint32>(c.w * g_Scale), h = static_cast<Uint32>(c.h * g_Scale);
            SDL_CopyGPUTextureToTexture(copy, &from, &scratch, w, h, 1, false);
            SDL_EndGPUCopyPass(copy);
            copy = SDL_BeginGPUCopyPass(commands);
            SDL_GPUTextureLocation to = location(g_Target, c.x, c.y, g_Scale);
            SDL_CopyGPUTextureToTexture(copy, &scratch, &to, w, h, 1, false);
            SDL_EndGPUCopyPass(copy);
            break;
        }
        case Kind::Draw: {
            if (c.count == 0) {
                break;
            }
            if (pass == nullptr) {
                SDL_GPUColorTargetInfo target{};
                target.texture = g_Target;
                target.load_op = SDL_GPU_LOADOP_LOAD;
                target.store_op = SDL_GPU_STOREOP_STORE;
                pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
                SDL_GPUViewport viewport{0, 0, static_cast<float>(kVramWidth * g_Scale),
                                         static_cast<float>(kVramHeight * g_Scale), 0, 1};
                SDL_SetGPUViewport(pass, &viewport);
                SDL_GPUBufferBinding vertices{g_VertexBuffer, 0};
                SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
                SDL_GPUTextureSamplerBinding sampler{g_Native, g_Sampler};
                SDL_BindGPUFragmentSamplers(pass, 0, &sampler, 1);
            }
            SDL_Rect scissor{c.scissor.x0 * g_Scale, c.scissor.y0 * g_Scale, (c.scissor.x1 - c.scissor.x0 + 1) * g_Scale,
                             (c.scissor.y1 - c.scissor.y0 + 1) * g_Scale};
            if (scissor.w <= 0 || scissor.h <= 0) {
                break;
            }
            SDL_SetGPUScissor(pass, &scissor);
            Uniforms u{};
            std::memcpy(u.window, c.window, sizeof u.window);
            u.scale = g_Scale;
            u.setMask = c.setMask ? 1 : 0;
            // Semi-transparent: the unblended texels first, then the blended ones.
            for (int step = c.blend == kNoBlend ? 0 : 1; step <= (c.blend == kNoBlend ? 0 : 2); step++) {
                int pipeline = step == 1 ? kNoBlend : c.blend;
                if (pipeline != bound) {
                    SDL_BindGPUGraphicsPipeline(pass, g_Pipelines[pipeline]);
                    bound = pipeline;
                }
                if (step == 2) {
                    float k = c.blend == 0 ? 0.5f : 0.25f;
                    SDL_SetGPUBlendConstants(pass, SDL_FColor{k, k, k, k});
                }
                u.pass = step;
                SDL_PushGPUFragmentUniformData(commands, 0, &u, sizeof u);
                SDL_DrawGPUPrimitives(pass, static_cast<Uint32>(c.count), 1, static_cast<Uint32>(c.first), 0);
            }
            break;
        }
        }
    }
    endPass();
}

void clearRecorded()
{
    g_Vertices.clear();
    g_Commands.clear();
    g_Uploads.clear();
}

// Brings the scaled VRAM down to 1x, one dot per N by N, into the CPU's VRAM.
bool download()
{
    SDL_GPUTexture *small = makeTexture(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, kVramWidth, kVramHeight, kTargetUsage);
    SDL_GPUTransferBufferCreateInfo info{SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, kVramWidth * kVramHeight * 4, 0};
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(g_Device, &info);
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(g_Device);
    bool done = false;

    if (small != nullptr && transfer != nullptr && commands != nullptr) {
        record(commands);
        SDL_GPUBlitInfo blit{};
        blit.source.texture = g_Target;
        blit.source.w = static_cast<Uint32>(kVramWidth * g_Scale);
        blit.source.h = static_cast<Uint32>(kVramHeight * g_Scale);
        blit.destination.texture = small;
        blit.destination.w = kVramWidth;
        blit.destination.h = kVramHeight;
        blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
        blit.filter = SDL_GPU_FILTER_NEAREST;
        SDL_BlitGPUTexture(commands, &blit);
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
        SDL_GPUTextureRegion from = region(small, 0, 0, kVramWidth, kVramHeight, 1);
        SDL_GPUTextureTransferInfo to{};
        to.transfer_buffer = transfer;
        SDL_DownloadFromGPUTexture(copy, &from, &to);
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
        commands = nullptr;
        if (fence != nullptr && SDL_WaitForGPUFences(g_Device, true, &fence, 1)) {
            const auto *rgba = static_cast<const unsigned char *>(SDL_MapGPUTransferBuffer(g_Device, transfer, false));
            if (rgba != nullptr) {
                for (int y = 0; y < kVramHeight; y++) {
                    for (int x = 0; x < kVramWidth; x++) {
                        const unsigned char *p = rgba + (y * kVramWidth + x) * 4;
                        auto five = [](unsigned char c) { return (c * 31 + 127) / 255; };
                        g_Vram[y][x] = static_cast<std::uint16_t>(five(p[0]) | five(p[1]) << 5 | five(p[2]) << 10 |
                                                                  (p[3] >= 128 ? 0x8000 : 0));
                    }
                }
                SDL_UnmapGPUTransferBuffer(g_Device, transfer);
                done = true;
            }
        }
        if (fence != nullptr) {
            SDL_ReleaseGPUFence(g_Device, fence);
        }
    }
    if (commands != nullptr) {
        SDL_CancelGPUCommandBuffer(commands);
    }
    if (transfer != nullptr) {
        SDL_ReleaseGPUTransferBuffer(g_Device, transfer);
    }
    if (small != nullptr) {
        SDL_ReleaseGPUTexture(g_Device, small);
    }
    clearRecorded();
    return done;
}

}  // namespace

bool open(SDL_Renderer *renderer, int scale)
{
    if (g_Active) {
        return true;
    }
    g_Renderer = renderer;
    g_Device = SDL_GetGPURendererDevice(renderer);
    if (g_Device == nullptr) {
        return false;
    }
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(g_Device);
    SDL_GPUShaderFormat format = (formats & SDL_GPU_SHADERFORMAT_DXIL) != 0    ? SDL_GPU_SHADERFORMAT_DXIL
                                 : (formats & SDL_GPU_SHADERFORMAT_SPIRV) != 0 ? SDL_GPU_SHADERFORMAT_SPIRV
                                                                               : SDL_GPU_SHADERFORMAT_INVALID;
    g_Scale = std::clamp(scale, 1, kMaxScale);
    SDL_GPUSamplerCreateInfo sampler{};
    sampler.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    bool made = format != SDL_GPU_SHADERFORMAT_INVALID && makePipelines(format);
    if (made) {
        g_Sampler = SDL_CreateGPUSampler(g_Device, &sampler);
        g_Native = makeTexture(SDL_GPU_TEXTUREFORMAT_R16_UNORM, kVramWidth, kVramHeight, SDL_GPU_TEXTUREUSAGE_SAMPLER);
        made = g_Sampler != nullptr && g_Native != nullptr && makeTargets(g_Scale, &g_Target, &g_Scratch, &g_Shown);
    }
    if (!made) {
        Rt_Log("hardware renderer: %s", SDL_GetError());
        releaseAll();
        g_Device = nullptr;
        return false;
    }
    std::memset(g_Drawn, 0, sizeof g_Drawn);
    clearRecorded();
    addUpload(0, 0, kVramWidth, kVramHeight, true);
    setBackend(&g_Backend);
    g_Active = true;
    Rt_Log("hardware renderer: %s, %dx", SDL_GetGPUDeviceDriver(g_Device), g_Scale);
    return true;
}

void close()
{
    if (!g_Active) {
        return;
    }
    if (!download()) {
        Rt_Log("hardware renderer: VRAM didn't come back: %s", SDL_GetError());
    }
    setBackend(nullptr);
    releaseAll();
    g_Device = nullptr;
    g_Active = false;
}

bool active()
{
    return g_Active;
}

void setScale(int scale)
{
    scale = std::clamp(scale, 1, kMaxScale);
    if (!g_Active) {
        g_Scale = scale;
        return;
    }
    if (scale == g_Scale) {
        return;
    }
    SDL_GPUTexture *target, *scratch;
    SDL_Texture *shown;
    if (!makeTargets(scale, &target, &scratch, &shown)) {
        Rt_Log("hardware renderer: can't draw at %dx: %s", scale, SDL_GetError());
        releaseTargets(target, scratch, shown);
        return;
    }
    // What's drawn so far, at the new size.
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(g_Device);
    record(commands);
    clearRecorded();
    SDL_GPUBlitInfo blit{};
    blit.source.texture = g_Target;
    blit.source.w = static_cast<Uint32>(kVramWidth * g_Scale);
    blit.source.h = static_cast<Uint32>(kVramHeight * g_Scale);
    blit.destination.texture = target;
    blit.destination.w = static_cast<Uint32>(kVramWidth * scale);
    blit.destination.h = static_cast<Uint32>(kVramHeight * scale);
    blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
    blit.filter = SDL_GPU_FILTER_NEAREST;
    SDL_BlitGPUTexture(commands, &blit);
    SDL_SubmitGPUCommandBuffer(commands);
    releaseTargets(g_Target, g_Scratch, g_Shown);
    g_Target = target;
    g_Scratch = scratch;
    g_Shown = shown;
    g_Scale = scale;
}

int scale()
{
    return g_Scale;
}

void setDither(bool on)
{
    g_Dither = on;
}

SDL_Texture *submit()
{
    if (!g_Active) {
        return nullptr;
    }
    if (!g_Commands.empty()) {
        SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(g_Device);
        if (commands != nullptr) {
            record(commands);
            SDL_SubmitGPUCommandBuffer(commands);
        }
        clearRecorded();
    }
    return g_Shown;
}

bool readDisplay(std::vector<unsigned int> &pixels, int &width, int &height)
{
    if (!g_Active) {
        return false;
    }
    Display d = display();
    width = d.width * g_Scale;
    height = d.height * g_Scale;
    pixels.assign(static_cast<std::size_t>(width) * height, 0);
    if (!d.on || d.truecolor) {
        return false;
    }
    int x = std::min(d.x, kVramWidth - d.width), y = std::min(d.y, kVramHeight - d.height);
    SDL_GPUTransferBufferCreateInfo info{SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, static_cast<Uint32>(width * height * 4), 0};
    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(g_Device, &info);
    SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(g_Device);
    bool done = false;
    if (transfer != nullptr && commands != nullptr) {
        record(commands);
        clearRecorded();
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
        SDL_GPUTextureRegion from = region(g_Target, x, y, d.width, d.height, g_Scale);
        SDL_GPUTextureTransferInfo to{};
        to.transfer_buffer = transfer;
        SDL_DownloadFromGPUTexture(copy, &from, &to);
        SDL_EndGPUCopyPass(copy);
        SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
        commands = nullptr;
        if (fence != nullptr && SDL_WaitForGPUFences(g_Device, true, &fence, 1)) {
            const auto *rgba = static_cast<const unsigned char *>(SDL_MapGPUTransferBuffer(g_Device, transfer, false));
            if (rgba != nullptr) {
                auto eight = [](unsigned char c) { return static_cast<unsigned int>((c * 31 + 127) / 255) << 3; };
                for (std::size_t i = 0; i < pixels.size(); i++) {
                    const unsigned char *p = rgba + i * 4;
                    pixels[i] = eight(p[0]) | eight(p[1]) << 8 | eight(p[2]) << 16;
                }
                SDL_UnmapGPUTransferBuffer(g_Device, transfer);
                done = true;
            }
        }
        if (fence != nullptr) {
            SDL_ReleaseGPUFence(g_Device, fence);
        }
    }
    if (commands != nullptr) {
        SDL_CancelGPUCommandBuffer(commands);
    }
    if (transfer != nullptr) {
        SDL_ReleaseGPUTransferBuffer(g_Device, transfer);
    }
    return done;
}

}  // namespace gpu::hw
