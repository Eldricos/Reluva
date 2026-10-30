// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "backends/sdl3/sdl_system.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace reluva {

namespace {

uint32_t packRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const uint8_t bytes[4] = {r, g, b, a};
    uint32_t v;
    std::memcpy(&v, bytes, 4);
    return v;
}

uint32_t translateKey(SDL_Keycode k) {
    if (k >= 'A' && k <= 'Z')
        return k - 'A' + 'a';
    if (k < 128)
        return k; // SDL keycodes match ASCII for printable keys
    switch (k) {
    case SDLK_UP: return key::Up;
    case SDLK_DOWN: return key::Down;
    case SDLK_LEFT: return key::Left;
    case SDLK_RIGHT: return key::Right;
    case SDLK_INSERT: return key::Insert;
    case SDLK_HOME: return key::Home;
    case SDLK_END: return key::End;
    case SDLK_PAGEUP: return key::PageUp;
    case SDLK_PAGEDOWN: return key::PageDown;
    case SDLK_PAUSE: return key::Pause;
    case SDLK_LSHIFT: return key::LShift;
    case SDLK_RSHIFT: return key::RShift;
    case SDLK_LCTRL: return key::LCtrl;
    case SDLK_RCTRL: return key::RCtrl;
    case SDLK_LALT: return key::LAlt;
    case SDLK_RALT: return key::RAlt;
    default: break;
    }
    if (k >= SDLK_F1 && k <= SDLK_F12)
        return key::F1 + (k - SDLK_F1);
    // The keypad's keys keep their own codes with NumLock off too (SDL3
    // reports KP_8, not Up), so engines can use them as keys of their own.
    if (k >= SDLK_KP_1 && k <= SDLK_KP_9)
        return key::Kp0 + 1 + (k - SDLK_KP_1);
    if (k == SDLK_KP_0)
        return key::Kp0;
    if (k == SDLK_KP_PERIOD)
        return key::KpPeriod;
    if (k == SDLK_KP_ENTER)
        return key::KpEnter;
    return key::Unknown;
}

uint16_t translateMods(SDL_Keymod m) {
    uint16_t out = 0;
    if (m & SDL_KMOD_SHIFT) out |= keymod::Shift;
    if (m & SDL_KMOD_CTRL) out |= keymod::Ctrl;
    if (m & SDL_KMOD_ALT) out |= keymod::Alt;
    return out;
}

MouseButton translateButton(uint8_t b) {
    switch (b) {
    case SDL_BUTTON_LEFT: return MouseButton::Left;
    case SDL_BUTTON_RIGHT: return MouseButton::Right;
    case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
    default: return MouseButton::None;
    }
}

} // namespace

SdlSystem::~SdlSystem() {
    stopAudio();
    if (gameTexture_)
        SDL_DestroyTexture(gameTexture_);
    if (renderer_)
        SDL_DestroyRenderer(renderer_);
    if (window_)
        SDL_DestroyWindow(window_);
    SDL_Quit();
}

Error SdlSystem::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        return Error::fail(std::string("SDL_Init failed: ") + SDL_GetError());
    // No sound card shouldn't stop anyone playing; startAudio() reports it.
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        SDL_Log("Audio unavailable: %s", SDL_GetError());

    window_ = SDL_CreateWindow("Reluva", 1280, 800,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window_)
        return Error::fail(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_)
        return Error::fail(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    SDL_SetRenderVSync(renderer_, 1);

    SDL_Log("Video driver: %s, renderer: %s", SDL_GetCurrentVideoDriver(),
            SDL_GetRendererName(renderer_));
    return Error::success();
}

void SdlSystem::resetForLauncher() {
    stopAudio();
    if (gameTexture_) {
        SDL_DestroyTexture(gameTexture_);
        gameTexture_ = nullptr;
    }
    gameW_ = gameH_ = 0;
    haveFrame_ = false;
    lastFrame_.clear();
    heldKeys_.clear();
    heldButtons_ = 0;
    pending_.clear();
    textInput_ = false;
    SDL_StopTextInput(window_); // the launcher's ImGui starts it as it needs
    menuHandler_ = queuedAction_ = nullptr;
    SDL_SetRenderLogicalPresentation(renderer_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_SetWindowTitle(window_, "Reluva");
}

Error SdlSystem::initGraphics(int width, int height, PixelFormat format) {
    if (gameTexture_)
        SDL_DestroyTexture(gameTexture_);
    gameTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                     SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!gameTexture_)
        return Error::fail(std::string("SDL_CreateTexture failed: ") + SDL_GetError());
    SDL_SetTextureScaleMode(gameTexture_, SDL_SCALEMODE_NEAREST);

    gameW_ = width;
    gameH_ = height;
    gameFormat_ = format;
    haveFrame_ = false;
    textInput_ ? SDL_StartTextInput(window_) : SDL_StopTextInput(window_);
    convertBuffer_.assign(static_cast<size_t>(width) * height, 0);
    for (int i = 0; i < 256; ++i)
        palette_[i] = packRgba(0, 0, 0, 255);

    // Letterbox the game framebuffer into the window at any size, and map
    // mouse coordinates back into framebuffer pixels.
    SDL_SetRenderLogicalPresentation(renderer_, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    return Error::success();
}

void SdlSystem::restoreGamePresentation() {
    SDL_SetRenderScale(renderer_, 1.0f, 1.0f);
    if (gameW_)
        SDL_SetRenderLogicalPresentation(renderer_, gameW_, gameH_, SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

EventType SdlSystem::runPaused(const MenuHandler& handler) {
    const uint64_t start = SDL_GetTicks();
    if (audioStream_)
        SDL_PauseAudioStreamDevice(audioStream_);
    captureFrame();
    const EventType result = handler();
    if (audioStream_)
        SDL_ResumeAudioStreamDevice(audioStream_);
    restoreGamePresentation();
    textInput_ ? SDL_StartTextInput(window_) : SDL_StopTextInput(window_);
    releaseHeld();
    pausedMs_ += SDL_GetTicks() - start;
    return result;
}

// The game texture read back (taken as the menu opens, not every frame).
void SdlSystem::captureFrame() {
    lastFrame_.clear();
    if (!gameTexture_ || !haveFrame_)
        return;
    const size_t rowBytes = size_t(gameW_) * 4;
    if (gameFormat_ == PixelFormat::CLUT8) { // already converted, in convertBuffer_
        lastFrame_.resize(rowBytes * gameH_);
        std::memcpy(lastFrame_.data(), convertBuffer_.data(), lastFrame_.size());
        return;
    }
    SDL_Texture* target =
        SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, gameW_, gameH_);
    if (!target)
        return;
    SDL_Texture* previous = SDL_GetRenderTarget(renderer_);
    SDL_BlendMode blend = SDL_BLENDMODE_NONE;
    SDL_GetTextureBlendMode(gameTexture_, &blend);
    SDL_SetRenderTarget(renderer_, target);
    SDL_SetTextureBlendMode(gameTexture_, SDL_BLENDMODE_NONE);
    SDL_RenderTexture(renderer_, gameTexture_, nullptr, nullptr);
    SDL_SetTextureBlendMode(gameTexture_, blend);
    SDL_Surface* read = SDL_RenderReadPixels(renderer_, nullptr);
    SDL_SetRenderTarget(renderer_, previous);
    SDL_DestroyTexture(target);
    SDL_Surface* rgba = read ? SDL_ConvertSurface(read, SDL_PIXELFORMAT_RGBA32) : nullptr;
    if (rgba && rgba->w == gameW_ && rgba->h == gameH_) {
        lastFrame_.resize(rowBytes * gameH_);
        const auto* pixels = static_cast<const uint8_t*>(rgba->pixels);
        for (int y = 0; y < gameH_; ++y)
            std::memcpy(lastFrame_.data() + rowBytes * y, pixels + size_t(y) * rgba->pitch, rowBytes);
    }
    SDL_DestroySurface(rgba);
    SDL_DestroySurface(read);
}

// After the menu: a release for each key and button the game saw pressed
// that isn't held any more (one still held keeps its own release for
// later), and the pointer where it is now.
void SdlSystem::releaseHeld() {
    const bool* keys = SDL_GetKeyboardState(nullptr);
    float wx = 0, wy = 0;
    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&wx, &wy);
    Event e;
    e.mods = translateMods(SDL_GetModState());
    for (auto it = heldKeys_.begin(); it != heldKeys_.end();) {
        if (keys[it->first]) {
            ++it;
            continue;
        }
        e.type = EventType::KeyUp;
        e.key = it->second;
        pending_.push_back(e);
        it = heldKeys_.erase(it);
    }
    e.key = key::Unknown;
    e.mouseX = mouseX_, e.mouseY = mouseY_, e.outside = mouseOutside_;
    for (uint8_t b = SDL_BUTTON_LEFT; b <= SDL_BUTTON_X2; ++b)
        if ((heldButtons_ & SDL_BUTTON_MASK(b)) && !(buttons & SDL_BUTTON_MASK(b))) {
            e.type = EventType::MouseUp;
            e.button = translateButton(b);
            e.clicks = 1;
            pending_.push_back(e);
            heldButtons_ &= ~SDL_BUTTON_MASK(b);
        }
    float x = 0, y = 0;
    if (gameW_ && SDL_RenderCoordinatesFromWindow(renderer_, wx, wy, &x, &y)) {
        Event move;
        move.type = EventType::MouseMove;
        move.mods = e.mods;
        setMouse(x, y, move);
        if (move.mouseX != e.mouseX || move.mouseY != e.mouseY || move.outside != e.outside)
            pending_.push_back(move);
    }
}

// Render coordinates to framebuffer pixels, clamped into the frame.
void SdlSystem::setMouse(float x, float y, Event& out) {
    const int px = int(std::floor(x)), py = int(std::floor(y));
    out.mouseX = px, out.mouseY = py;
    if (gameW_ > 0 && gameH_ > 0) {
        out.outside = px < 0 || py < 0 || px >= gameW_ || py >= gameH_;
        out.mouseX = std::clamp(px, 0, gameW_ - 1);
        out.mouseY = std::clamp(py, 0, gameH_ - 1);
    }
    mouseX_ = out.mouseX, mouseY_ = out.mouseY, mouseOutside_ = out.outside;
}

void SdlSystem::setPalette(const Color* colors, int start, int count) {
    if (!colors)
        return;
    for (int i = std::max(0, -start); i < count && start + i < 256; ++i)
        palette_[start + i] = packRgba(colors[i].r, colors[i].g, colors[i].b, 255);
}

void SdlSystem::updateScreen(const Surface& surface) {
    if (!gameTexture_ || surface.w != gameW_ || surface.h != gameH_ || surface.format != gameFormat_)
        return;

    if (surface.format == PixelFormat::CLUT8) {
        for (int y = 0; y < surface.h; ++y) {
            const uint8_t* src = surface.row(y);
            uint32_t* dst = convertBuffer_.data() + static_cast<size_t>(y) * gameW_;
            for (int x = 0; x < surface.w; ++x)
                dst[x] = palette_[src[x]];
        }
        SDL_UpdateTexture(gameTexture_, nullptr, convertBuffer_.data(), gameW_ * 4);
    } else {
        SDL_UpdateTexture(gameTexture_, nullptr, surface.pixels.data(), surface.pitch);
    }
    haveFrame_ = true;

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderTexture(renderer_, gameTexture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

void SdlSystem::setWindowTitle(const std::string& title) { SDL_SetWindowTitle(window_, title.c_str()); }

void SdlSystem::startTextInput() {
    textInput_ = true;
    SDL_StartTextInput(window_);
}

void SdlSystem::stopTextInput() {
    textInput_ = false;
    SDL_StopTextInput(window_);
}

bool SdlSystem::pollEvent(Event& out) {
    if (!pending_.empty()) {
        out = std::move(pending_.front());
        pending_.pop_front();
        return true;
    }
    if (queuedAction_) {
        const MenuHandler action = std::move(queuedAction_);
        queuedAction_ = nullptr;
        if (const EventType t = runPaused(action); t != EventType::None) {
            out = Event{};
            out.type = t;
            return true;
        }
        if (!pending_.empty())
            return pollEvent(out);
    }
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        SDL_ConvertEventToRenderCoordinates(renderer_, &ev);
        out = Event{};
        switch (ev.type) {
        case SDL_EVENT_QUIT:
            quitRequested_ = true;
            out.type = EventType::Quit;
            return true;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            // Ctrl+F5 opens the Reluva menu (else returns to the launcher).
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_F5 && (ev.key.mod & SDL_KMOD_CTRL)) {
                if (ev.key.repeat)
                    continue;
                const EventType t = menuHandler_ ? runPaused(menuHandler_) : EventType::ReturnToLauncher;
                if (t == EventType::None) {
                    if (!pending_.empty())
                        return pollEvent(out); // the releases the menu took
                    continue;
                }
                if (t == EventType::Quit)
                    quitRequested_ = true;
                out.type = t;
                return true;
            }
            // Alt+Enter toggles fullscreen.
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_RETURN && (ev.key.mod & SDL_KMOD_ALT)) {
                bool fs = SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN;
                SDL_SetWindowFullscreen(window_, !fs);
                continue;
            }
            out.type = ev.type == SDL_EVENT_KEY_DOWN ? EventType::KeyDown : EventType::KeyUp;
            out.key = translateKey(ev.key.key);
            out.mods = translateMods(ev.key.mod);
            out.repeat = ev.key.repeat;
            if (ev.type == SDL_EVENT_KEY_DOWN)
                heldKeys_[ev.key.scancode] = out.key;
            else
                heldKeys_.erase(ev.key.scancode);
            return true;
        case SDL_EVENT_TEXT_INPUT:
            if (!textInput_ || !ev.text.text)
                continue; // the menu's, or the launcher's
            out.type = EventType::TextInput;
            out.text = ev.text.text;
            return true;
        case SDL_EVENT_MOUSE_MOTION:
            out.type = EventType::MouseMove;
            setMouse(ev.motion.x, ev.motion.y, out);
            out.mods = translateMods(SDL_GetModState());
            return true;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            out.type = ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? EventType::MouseDown : EventType::MouseUp;
            out.button = translateButton(ev.button.button);
            setMouse(ev.button.x, ev.button.y, out);
            out.clicks = ev.button.clicks;
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                heldButtons_ |= SDL_BUTTON_MASK(ev.button.button);
            else
                heldButtons_ &= ~SDL_BUTTON_MASK(ev.button.button);
            out.mods = translateMods(SDL_GetModState());
            return true;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            out.type = ev.type == SDL_EVENT_WINDOW_FOCUS_LOST ? EventType::FocusLost : EventType::FocusGained;
            return true;
        case SDL_EVENT_MOUSE_WHEEL:
            out.type = EventType::MouseWheel;
            out.wheelY = ev.wheel.integer_y;
            setMouse(ev.wheel.mouse_x, ev.wheel.mouse_y, out);
            return true;
        default:
            break; // ignore everything else
        }
    }
    return false;
}

uint64_t SdlSystem::getMillis() const { return SDL_GetTicks() - pausedMs_; }

void SdlSystem::delayMillis(uint32_t ms) { SDL_Delay(ms); }

void SDLCALL SdlSystem::audioThunk(void* userdata, SDL_AudioStream* stream, int additional, int) {
    auto* self = static_cast<SdlSystem*>(userdata);
    const int frames = additional / (2 * static_cast<int>(sizeof(int16_t)));
    if (frames <= 0)
        return;
    self->audioBuffer_.resize(static_cast<size_t>(frames) * 2);
    self->audioCallback_(self->audioBuffer_.data(), frames);
    SDL_PutAudioStreamData(stream, self->audioBuffer_.data(), frames * 4);
}

Error SdlSystem::startAudio(AudioCallback callback) {
    stopAudio();
    audioCallback_ = std::move(callback);
    const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, kAudioRate};
    audioStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audioThunk, this);
    if (!audioStream_)
        return Error::fail(std::string("Cannot open audio: ") + SDL_GetError());
    SDL_ResumeAudioStreamDevice(audioStream_);
    return Error::success();
}

void SdlSystem::stopAudio() {
    if (audioStream_) {
        SDL_DestroyAudioStream(audioStream_); // stops the callback before returning
        audioStream_ = nullptr;
    }
    audioCallback_ = nullptr;
}

} // namespace reluva
