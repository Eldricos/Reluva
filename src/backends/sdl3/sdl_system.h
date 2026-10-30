// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/system.h"

#include <SDL3/SDL.h>

#include <deque>
#include <functional>
#include <map>
#include <vector>

namespace reluva {

class SdlSystem : public System {
public:
    SdlSystem() = default;
    ~SdlSystem() override;
    SdlSystem(const SdlSystem&) = delete;
    SdlSystem& operator=(const SdlSystem&) = delete;

    Error init();

    // Used by the launcher, which draws with ImGui directly on SDL.
    SDL_Window* window() const { return window_; }
    SDL_Renderer* renderer() const { return renderer_; }
    // Drop the game framebuffer and return the window to launcher mode.
    void resetForLauncher();
    // True once the user closed the window during a game.
    bool quitRequested() const { return quitRequested_; }

    // The Reluva menu: Ctrl+F5 during a game runs `handler` from inside
    // pollEvent(), with the game's clock and sound paused; its answer
    // (Quit, ReturnToLauncher, or None to go on) becomes the next event.
    // Without one, Ctrl+F5 returns to the launcher.
    using MenuHandler = std::function<EventType()>;
    void setMenuHandler(MenuHandler handler) { menuHandler_ = std::move(handler); }
    // Runs `action` the same way at the game's next pollEvent() (a
    // savestate to load once the game is up).
    void queueMenuAction(MenuHandler action) { queuedAction_ = std::move(action); }

    // What the menu draws over: the game's last frame (null before the
    // first), and the presentation to put back afterwards.
    SDL_Texture* gameTexture() const { return gameW_ ? gameTexture_ : nullptr; }
    int gameWidth() const { return gameW_; }
    int gameHeight() const { return gameH_; }
    // The game's last frame as RGBA (for thumbnails), as it was when the
    // menu opened; empty before the game's first frame.
    const std::vector<uint8_t>& lastFrame() const { return lastFrame_; }
    void restoreGamePresentation();

    // System
    Error initGraphics(int width, int height, PixelFormat format) override;
    void setPalette(const Color* colors, int start, int count) override;
    void updateScreen(const Surface& surface) override;
    void setWindowTitle(const std::string& title) override;
    bool pollEvent(Event& event) override;
    void startTextInput() override;
    void stopTextInput() override;
    uint64_t getMillis() const override;
    void delayMillis(uint32_t ms) override;
    Error startAudio(AudioCallback callback) override;
    void stopAudio() override;
    int audioRate() const override { return kAudioRate; }

private:
    static constexpr int kAudioRate = 44100;
    static void SDLCALL audioThunk(void* userdata, SDL_AudioStream* stream, int additional, int total);

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* gameTexture_ = nullptr;
    int gameW_ = 0, gameH_ = 0;
    PixelFormat gameFormat_ = PixelFormat::CLUT8;
    uint32_t palette_[256] = {};          // RGBA32, byte order R,G,B,A
    std::vector<uint32_t> convertBuffer_; // CLUT8 -> RGBA32 staging
    bool quitRequested_ = false;
    EventType runPaused(const MenuHandler& handler);
    void captureFrame();
    void releaseHeld();
    void setMouse(float x, float y, Event& out);
    MenuHandler menuHandler_, queuedAction_;
    uint64_t pausedMs_ = 0; // time spent in the menu, left out of getMillis()
    bool haveFrame_ = false; // updateScreen() has shown one since initGraphics()
    std::vector<uint8_t> lastFrame_;

    // What the game was last told is held. The menu takes the events while
    // it's open, so what was let go meanwhile is released for the game
    // (from `pending_`) once it closes.
    std::map<SDL_Scancode, uint32_t> heldKeys_; // the key the game saw for each
    SDL_MouseButtonFlags heldButtons_ = 0;
    int mouseX_ = 0, mouseY_ = 0; // the pointer where the game last saw it
    bool mouseOutside_ = false;
    std::deque<Event> pending_;
    bool textInput_ = false; // the game's (the menu's ImGui switches it too)

    SDL_AudioStream* audioStream_ = nullptr;
    AudioCallback audioCallback_;
    std::vector<int16_t> audioBuffer_; // audio thread only
};

} // namespace reluva
