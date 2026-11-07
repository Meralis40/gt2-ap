// SPDX-License-Identifier: GPL-2.0-or-later
// The window and the controls, through SDL3. The picture is the PlayStation's 4:3 frame,
// whatever its resolution; the keyboard and the first gamepad are controller 1, a digital
// controller or, with a gamepad, a DualShock. Tab or the right stick's click pauses the game
// on the tracker, which is drawn here over the picture, not in the game's menus: it has to
// open anywhere, races included.
#include "ap.h"
#include "ap_host.h"
#include "host.h"
#include "hw.h"
#include "rt.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

constexpr int kFrameWidth = 640;
constexpr int kFrameHeight = 480;
constexpr int kWindowScale = 2;
constexpr int kAudioRate = 44100;

// A gamepad's stick rests near the centre: nothing happens up to here, of 32767.
constexpr int kStickDeadZone = 4000;
constexpr int kStickRest = 0x80;
constexpr int kStickFarLow = 0, kStickFarHigh = 0xff;
// Pad_ScaleStick ignores everything between these, for worn sticks.
constexpr int kGameDeadZoneLow = 0x5b, kGameDeadZoneHigh = 0xa4;
constexpr int kStickTravel = kGameDeadZoneLow;

enum StickByte { kRightX, kRightY, kLeftX, kLeftY, kStickBytes };

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
bool g_TrackerOpen;

// A stick past the gamepad's dead zone starts just past the game's, or the first third of its
// travel would do nothing.
int stickByte(SDL_GamepadAxis axis)
{
    int value = SDL_GetGamepadAxis(g_Pad, axis);
    int amount = std::min(std::abs(value), SDL_JOYSTICK_AXIS_MAX);

    if (amount <= kStickDeadZone) {
        return kStickRest;
    }
    int travel = (amount - kStickDeadZone) * kStickTravel / (SDL_JOYSTICK_AXIS_MAX - kStickDeadZone);
    return value < 0 ? kGameDeadZoneLow - travel : kGameDeadZoneHigh + travel;
}

// A DualShock's map steers with the stick alone, so the d-pad and the arrow keys push it.
void readSticks(std::uint16_t held)
{
    unsigned char sticks[kStickBytes];

    sticks[kRightX] = static_cast<unsigned char>(stickByte(SDL_GAMEPAD_AXIS_RIGHTX));
    sticks[kRightY] = static_cast<unsigned char>(stickByte(SDL_GAMEPAD_AXIS_RIGHTY));
    int x = stickByte(SDL_GAMEPAD_AXIS_LEFTX);
    int y = stickByte(SDL_GAMEPAD_AXIS_LEFTY);
    if (x == kStickRest && (held & (kLeft | kRight)) != 0) {
        x = (held & kLeft) != 0 ? kStickFarLow : kStickFarHigh;
    }
    if (y == kStickRest && (held & (kUp | kDown)) != 0) {
        y = (held & kUp) != 0 ? kStickFarLow : kStickFarHigh;
    }
    sticks[kLeftX] = static_cast<unsigned char>(x);
    sticks[kLeftY] = static_cast<unsigned char>(y);
    Hw_PadSetSticks(1, sticks);
}

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
        readSticks(held);
    } else {
        Hw_PadSetSticks(0, nullptr);
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

// Only in a session: nothing is tracked without one.
void toggleTracker()
{
    g_TrackerOpen = !g_TrackerOpen && Ap_Active();
}

void onKey(const SDL_KeyboardEvent &key)
{
    if (!g_TextMode) {
        if (key.repeat) {
            return;
        }
        if (key.scancode == SDL_SCANCODE_TAB) {
            toggleTracker();
        } else if (key.scancode == SDL_SCANCODE_ESCAPE) {
            g_TrackerOpen = false;
        }
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

void onGamepadButton(const SDL_GamepadButtonEvent &button)
{
    if (g_Pad == nullptr || SDL_GetGamepadID(g_Pad) != button.which) {
        return;
    }
    if (button.button == SDL_GAMEPAD_BUTTON_RIGHT_STICK) {
        toggleTracker();
    }
}

struct TrackerText {
    float originX, originY;

    void line(float x, float y, const std::string &text) const
    {
        SDL_RenderDebugText(g_Renderer, originX + x, originY + y, text.c_str());
    }
};

void trackerColour(const ApTrackerEntry &entry)
{
    if (entry.have >= entry.total) {
        SDL_SetRenderDrawColor(g_Renderer, 120, 255, 120, 255);
    } else if (entry.have > 0) {
        SDL_SetRenderDrawColor(g_Renderer, 255, 220, 100, 255);
    } else {
        SDL_SetRenderDrawColor(g_Renderer, 130, 130, 130, 255);
    }
}

// Separates the thousands, as the game writes prices.
std::string withCommas(long long value)
{
    std::string text = std::to_string(value);
    for (int at = static_cast<int>(text.size()) - 3; at > 0; at -= 3) {
        text.insert(static_cast<std::size_t>(at), ",");
    }
    return text;
}

// In the PlayStation's 640 by 480, so that it fits the picture at any window size.
void drawTracker(const SDL_FRect &area)
{
    constexpr float kMargin = 12, kColumnX = 24, kManufacturerStep = 140, kPartsX = 312;
    constexpr float kTitleY = 22, kStatsY = 38, kStatStep = 12, kHeadingY = 72, kListY = 88, kRowStep = 15;
    constexpr float kHintY = 452;
    constexpr int kManufacturerRows = 18;
    ApTracker tracker = Ap_Tracker();
    float scale = std::min(area.w / kFrameWidth, area.h / kFrameHeight);
    if (scale >= 1) {
        scale = SDL_floorf(scale);  // the font is blurred at anything else
    }
    float width = kFrameWidth * scale, height = kFrameHeight * scale;
    TrackerText text{(area.x + (area.w - width) / 2) / scale, (area.y + (area.h - height) / 2) / scale};

    SDL_SetRenderScale(g_Renderer, scale, scale);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, 225);
    SDL_FRect panel{text.originX + kMargin, text.originY + kMargin, kFrameWidth - 2 * kMargin,
                    kFrameHeight - 2 * kMargin};
    SDL_RenderFillRect(g_Renderer, &panel);
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_NONE);

    SDL_SetRenderDrawColor(g_Renderer, 255, 255, 160, 255);
    text.line(kColumnX, kTitleY, "ARCHIPELAGO TRACKER - PAUSED");
    SDL_SetRenderDrawColor(g_Renderer, 255, 255, 255, 255);
    std::string stats = "Checks " + std::to_string(tracker.checks) + "/" + std::to_string(tracker.checksTotal) +
                        "    GT World League " + std::to_string(tracker.goalRaces) + "/" +
                        std::to_string(tracker.goalRacesTotal) + "    Credits received " +
                        withCommas(tracker.credits) + " Cr";
    text.line(kColumnX, kStatsY, stats);
    if (tracker.trophyRule) {
        text.line(kColumnX, kStatsY + kStatStep,
                  "Trophies " + std::to_string(tracker.trophies) + "    Pro leagues need " +
                      std::to_string(tracker.proNeeded) + "    GT World League needs " +
                      std::to_string(tracker.worldNeeded));
    }

    int manufacturers = 0, partsHeld = 0, partsTotal = 0;
    for (std::size_t i = 0; i < tracker.manufacturers.size(); i++) {
        const ApTrackerEntry &entry = tracker.manufacturers[i];
        int row = static_cast<int>(i) % kManufacturerRows;
        int column = static_cast<int>(i) / kManufacturerRows;
        manufacturers += entry.have;
        trackerColour(entry);
        text.line(kColumnX + column * kManufacturerStep, kListY + row * kRowStep,
                  (entry.have > 0 ? "[x] " : "[ ] ") + entry.name);
    }
    for (std::size_t i = 0; i < tracker.parts.size(); i++) {
        const ApTrackerEntry &entry = tracker.parts[i];
        std::string mark = entry.total > 1 ? std::to_string(entry.have) + "/" + std::to_string(entry.total)
                                           : (entry.have > 0 ? " x " : "   ");
        partsHeld += entry.have;
        partsTotal += entry.total;
        trackerColour(entry);
        text.line(kPartsX, kListY + static_cast<float>(i) * kRowStep, "[" + mark + "] " + entry.name);
    }
    SDL_SetRenderDrawColor(g_Renderer, 255, 255, 160, 255);
    text.line(kColumnX, kHeadingY,
              "MANUFACTURERS " + std::to_string(manufacturers) + "/" + std::to_string(tracker.manufacturers.size()));
    text.line(kPartsX, kHeadingY, "TUNING PARTS " + std::to_string(partsHeld) + "/" + std::to_string(partsTotal));
    SDL_SetRenderDrawColor(g_Renderer, 200, 200, 200, 255);
    text.line(kColumnX, kHintY, "Tab, Esc or right stick click: back to the game");
    SDL_SetRenderScale(g_Renderer, 1, 1);
}

void handleEvents()
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
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            onGamepadButton(event.gbutton);
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
}

void present(const SDL_FRect &source)
{
    SDL_FRect target = pictureArea();

    SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_Renderer);
    SDL_RenderTexture(g_Renderer, g_Texture, &source, &target);
    if (g_TrackerOpen) {
        drawTracker(target);
    } else {
        drawToasts();
    }
    SDL_RenderPresent(g_Renderer);
}

struct DiscPick {
    std::atomic<bool> done{false};
    bool failed = false;
    std::string path, error;
};

// On a thread of SDL's, on some systems.
void SDLCALL onDiscPicked(void *data, const char *const *files, int)
{
    auto *pick = static_cast<DiscPick *>(data);

    if (files == nullptr) {
        pick->failed = true;
        pick->error = SDL_GetError();
    } else if (*files != nullptr) {
        pick->path = *files;
    }
    pick->done = true;
}

}  // namespace

std::string Host_PickDisc()
{
    static const SDL_DialogFileFilter kFilters[] = {{"Disc image (.cue, .bin, .iso)", "cue;bin;iso"}};
    DiscPick pick;
    SDL_PropertiesID properties = SDL_CreateProperties();

    SDL_SetPointerProperty(properties, SDL_PROP_FILE_DIALOG_FILTERS_POINTER, const_cast<SDL_DialogFileFilter *>(kFilters));
    SDL_SetNumberProperty(properties, SDL_PROP_FILE_DIALOG_NFILTERS_NUMBER, 1);
    SDL_SetPointerProperty(properties, SDL_PROP_FILE_DIALOG_WINDOW_POINTER, g_Window);
    SDL_SetStringProperty(properties, SDL_PROP_FILE_DIALOG_TITLE_STRING, "Your Gran Turismo 2 Simulation disc image");
    SDL_ShowFileDialogWithProperties(SDL_FILEDIALOG_OPENFILE, onDiscPicked, &pick, properties);
    SDL_DestroyProperties(properties);
    while (!pick.done) {
        handleEvents();
        SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, 255);
        SDL_RenderClear(g_Renderer);
        SDL_RenderPresent(g_Renderer);
        SDL_Delay(16);
    }
    if (pick.failed) {
        Host_Alert("This system can't show a file dialog (" + pick.error +
                   ").\n\nGive the disc image on the command line: gt2ap <disc.cue>");
        std::exit(2);
    }
    return pick.path;
}

void Host_Alert(const std::string &text)
{
    Rt_Log("%s", text.c_str());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Gran Turismo 2", text.c_str(), g_Window);
}

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
    handleEvents();

    int width, height;
    Hw_GpuFrame(g_Pixels.data(), &width, &height);
    SDL_Rect updated{0, 0, width, height};
    SDL_UpdateTexture(g_Texture, &updated, g_Pixels.data(), width * 4);
    SDL_FRect source{0, 0, static_cast<float>(width), static_cast<float>(height)};
    present(source);

    // The game waits here, on no vblank, so a recording doesn't see the pause.
    while (g_TrackerOpen) {
        constexpr int kPauseFrameMs = 16;
        SDL_Delay(kPauseFrameMs);
        Rt_StillAlive();
        Ap_Poll();
        handleEvents();
        present(source);
    }
    readControls();
}
