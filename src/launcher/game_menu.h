// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/events.h"
#include "common/savestate.h"
#include "engines/engine.h"

#include <SDL3/SDL.h>

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace reluva {

class SdlSystem;

// A game's save folder: <root>/<target>.
std::filesystem::path saveFolder(const std::filesystem::path& root, const std::string& target);

// The list of a save folder's slots with their thumbnails, drawn with
// ImGui (the Reluva menu's and the launcher's).
class SlotBrowser {
public:
    explicit SlotBrowser(SDL_Renderer* renderer) : renderer_(renderer) {}
    ~SlotBrowser() { clear(); }
    SlotBrowser(const SlotBrowser&) = delete;
    SlotBrowser& operator=(const SlotBrowser&) = delete;

    void refresh(const std::filesystem::path& folder);
    void clear();
    const std::vector<save::Slot>& slots() const { return slots_; }
    // Draws the list (and `newSlot`, an empty slot to save into, first
    // when it's not 0); returns the slot picked by a double click.
    int draw(int& selected, int newSlot, float height);
    // Draws the selected slot's thumbnail and details.
    void drawDetails(int selected);

private:
    SDL_Texture* thumbnail(const save::Slot& slot);
    SDL_Renderer* renderer_;
    std::vector<save::Slot> slots_;
    std::map<int, SDL_Texture*> textures_;
};

// The Reluva menu (Ctrl+F5 in a game): resume, save, load, back to the
// launcher, quit. It saves and loads through the engine's syncState().
class GameMenu {
public:
    GameMenu(SdlSystem& system, std::filesystem::path saveRoot);

    void attach(Engine* engine, const GameTarget& target);
    void detach() { engine_ = nullptr; }

    // The menu, until closed: None to go on, else Quit or ReturnToLauncher.
    EventType run();
    // Loads a slot as the game starts (from the launcher); a failure is
    // shown in the menu.
    EventType loadAtStart(int slot);

    // Saves to / loads from a slot; the error says why not. A load that
    // fails once the engine has begun reading calls its loadFailed(), and
    // its error then goes on to say what the engine did about it (or that
    // it couldn't).
    Error save(int slot, const std::string& description);
    Error load(int slot);

private:
    enum class Page { Main, Save, Load };
    EventType loop(Page page, const std::string& problem);

    SdlSystem& sys_;
    std::filesystem::path root_;
    Engine* engine_ = nullptr;
    GameTarget target_;
    // A load failed part way and the engine couldn't put itself right: no
    // resuming or saving until a load works.
    bool broken_ = false;
};

} // namespace reluva
