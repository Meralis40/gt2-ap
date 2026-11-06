// SPDX-License-Identifier: GPL-2.0-or-later
// The window and the controls, through SDL3. The picture is the PlayStation's 4:3 frame,
// whatever its resolution; the keyboard and the first gamepad are controller 1.
#include "ap.h"
#include "ap_host.h"
#include "host.h"
#include "hw.h"
#include "rt.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kWindowScale = 2;
constexpr int kAudioRate = 44100;

// Active-low bits of a digital controller, in the order the game reads them.
enum PadBit : std::uint16_t {
    kSelect = 1 << 0, kStart = 1 << 3, kUp = 1 << 4, kRight = 1 << 5, kDown = 1 << 6, kLeft = 1 << 7,
    kL2 = 1 << 8, kR2 = 1 << 9, kL1 = 1 << 10, kR1 = 1 << 11, kTriangle = 1 << 12, kCircle = 1 << 13,
    kCross = 1 << 14, kSquare = 1 << 15,
};

struct KeyBinding {
    SDL_Scancode key;
    std::uint16_t button;
};

constexpr KeyBinding kKeys[] = {
    {SDL_SCANCODE_UP, kUp},       {SDL_SCANCODE_RIGHT, kRight},  {SDL_SCANCODE_DOWN, kDown},
    {SDL_SCANCODE_LEFT, kLeft},   {SDL_SCANCODE_RETURN, kStart}, {SDL_SCANCODE_BACKSPACE, kSelect},
    {SDL_SCANCODE_X, kCross},     {SDL_SCANCODE_C, kCircle},     {SDL_SCANCODE_S, kSquare},
    {SDL_SCANCODE_D, kTriangle},  {SDL_SCANCODE_Q, kL1},         {SDL_SCANCODE_E, kR1},
    {SDL_SCANCODE_1, kL2},        {SDL_SCANCODE_3, kR2},
};

struct GamepadBinding {
    SDL_GamepadButton pad;
    std::uint16_t button;
};

constexpr GamepadBinding kGamepad[] = {
    {SDL_GAMEPAD_BUTTON_DPAD_UP, kUp},      {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, kRight},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, kDown},  {SDL_GAMEPAD_BUTTON_DPAD_LEFT, kLeft},
    {SDL_GAMEPAD_BUTTON_START, kStart},     {SDL_GAMEPAD_BUTTON_BACK, kSelect},
    {SDL_GAMEPAD_BUTTON_SOUTH, kCross},     {SDL_GAMEPAD_BUTTON_EAST, kCircle},
    {SDL_GAMEPAD_BUTTON_WEST, kSquare},     {SDL_GAMEPAD_BUTTON_NORTH, kTriangle},
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, kL1}, {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, kR1},
};

SDL_Window *g_Window;
SDL_Renderer *g_Renderer;
SDL_Texture *g_Texture;
SDL_Gamepad *g_Pad;
SDL_AudioStream *g_Audio;
std::vector<unsigned int> g_Pixels(kFrameWidth * kFrameHeight);
bool g_TextMode;

void readControls()
{
    const bool *keys = SDL_GetKeyboardState(nullptr);
    std::uint16_t held = 0;

    for (const KeyBinding &binding : kKeys) {
        // The letters are for typing on the connection page.
        bool typing = binding.key != SDL_SCANCODE_UP && binding.key != SDL_SCANCODE_DOWN &&
                      binding.key != SDL_SCANCODE_RETURN;
        if (keys[binding.key] && !(g_TextMode && typing)) {
            held |= binding.button;
        }
    }
    if (g_Pad != nullptr) {
        for (const GamepadBinding &binding : kGamepad) {
            if (SDL_GetGamepadButton(g_Pad, binding.pad)) {
                held |= binding.button;
            }
        }
        if (SDL_GetGamepadAxis(g_Pad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 16384) {
            held |= kL2;
        }
        if (SDL_GetGamepadAxis(g_Pad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 16384) {
            held |= kR2;
        }
    }
    Hw_PadSetButtons(held);
}

// The largest 4:3 picture that fits the window.
SDL_FRect pictureArea()
{
    int width, height;

    SDL_GetCurrentRenderOutputSize(g_Renderer, &width, &height);
    float w = static_cast<float>(width), h = static_cast<float>(height);
    if (w * 3 > h * 4) {
        w = h * 4 / 3;
    } else {
        h = w * 3 / 4;
    }
    return {(width - w) / 2, (height - h) / 2, w, h};
}

void onKey(const SDL_KeyboardEvent &key)
{
    if (!g_TextMode) {
        return;
    }
    if (key.scancode == SDL_SCANCODE_BACKSPACE) {
        Ap_PageBackspace();
    } else if (key.scancode == SDL_SCANCODE_V && (key.mod & SDL_KMOD_CTRL) != 0 && !key.repeat) {
        char *text = SDL_GetClipboardText();
        Ap_PageText(text);
        SDL_free(text);
    }
}

// What the session has to say, over the picture, in plain ASCII.
void drawToasts()
{
    constexpr float kScale = 2;
    constexpr float kLineHeight = 12;
    int width, height;

    SDL_GetCurrentRenderOutputSize(g_Renderer, &width, &height);
    SDL_SetRenderScale(g_Renderer, kScale, kScale);
    for (int i = 0;; i++) {
        std::string line = Ap_ToastLine(i);
        if (line.empty()) {
            break;
        }
        for (char &c : line) {
            if (c < 0x20 || c >= 0x7f) {
                c = '?';
            }
        }
        float y = kLineHeight * (i + 1);
        SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, 255);
        SDL_RenderDebugText(g_Renderer, 9, y + 1, line.c_str());
        SDL_SetRenderDrawColor(g_Renderer, 255, 255, 160, 255);
        SDL_RenderDebugText(g_Renderer, 8, y, line.c_str());
    }
    SDL_SetRenderScale(g_Renderer, 1, 1);
}

}  // namespace

extern "C" void Host_TextMode(int on)
{
    if (g_Window == nullptr) {
        return;
    }
    g_TextMode = on != 0;
    if (on) {
        SDL_StartTextInput(g_Window);
    } else {
        SDL_StopTextInput(g_Window);
    }
}

void Host_Open()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        Rt_Fatal("SDL_Init: %s", SDL_GetError());
    }
    if (!SDL_CreateWindowAndRenderer("Gran Turismo 2", kFrameWidth * kWindowScale, kFrameHeight * kWindowScale,
                                     SDL_WINDOW_RESIZABLE, &g_Window, &g_Renderer)) {
        Rt_Fatal("SDL_CreateWindowAndRenderer: %s", SDL_GetError());
    }
    g_Texture = SDL_CreateTexture(g_Renderer, SDL_PIXELFORMAT_XBGR8888, SDL_TEXTUREACCESS_STREAMING, kFrameWidth,
                                  kFrameHeight);
    if (g_Texture == nullptr) {
        Rt_Fatal("SDL_CreateTexture: %s", SDL_GetError());
    }
    SDL_SetTextureScaleMode(g_Texture, SDL_SCALEMODE_NEAREST);

    const SDL_AudioSpec spec = {SDL_AUDIO_S16LE, 2, kAudioRate};
    g_Audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (g_Audio == nullptr) {
        Rt_Log("no sound: %s", SDL_GetError());
    } else {
        SDL_ResumeAudioStreamDevice(g_Audio);
    }
}

void Host_Audio(const short *samples, int count)
{
    constexpr int kBytesPerFrame = 2 * sizeof(short);
    constexpr int kMaxQueued = kAudioRate / 10 * kBytesPerFrame;  // what's more is late: let it catch up

    if (g_Audio != nullptr && SDL_GetAudioStreamQueued(g_Audio) < kMaxQueued) {
        SDL_PutAudioStreamData(g_Audio, samples, count * kBytesPerFrame);
    }
}

void Host_Frame()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            std::exit(0);
        case SDL_EVENT_TEXT_INPUT:
            if (g_TextMode) {
                Ap_PageText(event.text.text);
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            onKey(event.key);
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
            if (g_Pad == nullptr) {
                g_Pad = SDL_OpenGamepad(event.gdevice.which);
            }
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (g_Pad != nullptr && SDL_GetGamepadID(g_Pad) == event.gdevice.which) {
                SDL_CloseGamepad(g_Pad);
                g_Pad = nullptr;
            }
            break;
        default:
            break;
        }
    }
    readControls();

    int width, height;
    Hw_GpuFrame(g_Pixels.data(), &width, &height);
    SDL_Rect updated{0, 0, width, height};
    SDL_UpdateTexture(g_Texture, &updated, g_Pixels.data(), width * 4);
    SDL_FRect source{0, 0, static_cast<float>(width), static_cast<float>(height)};
    SDL_FRect target = pictureArea();
    SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_Renderer);
    SDL_RenderTexture(g_Renderer, g_Texture, &source, &target);
    drawToasts();
    SDL_RenderPresent(g_Renderer);
}
