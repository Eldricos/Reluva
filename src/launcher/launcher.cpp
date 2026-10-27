// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "launcher/launcher.h"

#include "backends/sdl3/sdl_system.h"
#include "engines/registry.h"
#include "launcher/game_menu.h"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

namespace reluva {

Launcher::Launcher(SdlSystem& system, Config& config, std::filesystem::path saveRoot)
    : sys_(system), config_(config), saveRoot_(std::move(saveRoot)) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr; // our layout is fixed; don't litter imgui.ini

    float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (scale <= 0)
        scale = 1.0f;
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(scale);
    ImGui::GetStyle().FontScaleDpi = scale;

    ImGui_ImplSDL3_InitForSDLRenderer(sys_.window(), sys_.renderer());
    ImGui_ImplSDLRenderer3_Init(sys_.renderer());

    selected_ = config_.get(Config::kGlobalSection, "last_target");
}

Launcher::~Launcher() {
    loadBrowser_.reset();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

std::optional<GameTarget> Launcher::run() {
    sys_.resetForLauncher();
    SDL_Renderer* renderer = sys_.renderer();

    std::optional<GameTarget> launch;
    bool quit = false;
    while (!launch && !quit) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL3_ProcessEvent(&ev);
            if (ev.type == SDL_EVENT_QUIT)
                quit = true;
        }
        if (SDL_GetWindowFlags(sys_.window()) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(16);
            continue;
        }

        handlePickedFolder();

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        drawUi(launch, quit);
        ImGui::Render();

        ImGuiIO& io = ImGui::GetIO();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 20, 22, 28, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);

    if (launch) {
        config_.set(Config::kGlobalSection, "last_target", launch->target);
        config_.save();
    }
    return launch;
}

void Launcher::drawUi(std::optional<GameTarget>& launch, bool& quit) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("Reluva", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::TextUnformatted("Reluva");
    ImGui::SameLine();
    ImGui::TextDisabled("v0.1 - classic game engine reimplementations");
    ImGui::Separator();

    const float buttonsWidth = 200.0f * ImGui::GetStyle().FontScaleDpi;
    const float footer = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;

    // Game list
    ImGui::BeginChild("games", ImVec2(-buttonsWidth, -footer), ImGuiChildFlags_Borders);
    const auto targets = config_.targets();
    if (targets.empty())
        ImGui::TextDisabled("No games yet. Use \"Add Game...\" and pick a game's install folder.");
    for (const std::string& name : targets) {
        std::string label = config_.get(name, "description", name) + "##" + name;
        if (ImGui::Selectable(label.c_str(), selected_ == name, ImGuiSelectableFlags_AllowDoubleClick)) {
            selected_ = name;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                launch = targetFromConfig(name);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", config_.get(name, "path").c_str());
    }
    ImGui::EndChild();

    // Buttons
    ImGui::SameLine();
    ImGui::BeginGroup();
    const ImVec2 bsize(-1, 0);
    const bool haveSelection = config_.hasSection(selected_) && selected_ != Config::kGlobalSection;

    ImGui::BeginDisabled(!haveSelection);
    if (ImGui::Button("Start", bsize))
        launch = targetFromConfig(selected_);
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!haveSelection);
    if (ImGui::Button("Load...", bsize)) {
        loadBrowser_ = std::make_unique<SlotBrowser>(sys_.renderer());
        loadBrowser_->refresh(saveFolder(saveRoot_, selected_));
        loadSelected_ = loadBrowser_->slots().empty() ? 0 : loadBrowser_->slots().back().number;
        int64_t newest = -1;
        for (const save::Slot& s : loadBrowser_->slots())
            if (s.error.empty() && s.header.created > newest)
                newest = s.header.created, loadSelected_ = s.number;
        ImGui::OpenPopup("Load");
    }
    ImGui::EndDisabled();
    drawLoad(launch);

    ImGui::BeginDisabled(pickInProgress_);
    if (ImGui::Button("Add Game...", bsize)) {
        pickInProgress_ = true;
        SDL_ShowOpenFolderDialog(pickFolderCallback, this, sys_.window(), nullptr, false);
    }
    ImGui::EndDisabled();

    ImGui::BeginDisabled(!haveSelection);
    if (ImGui::Button("Remove", bsize)) {
        config_.removeSection(selected_);
        config_.save();
        status_ = "Removed " + selected_;
        selected_.clear();
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    if (ImGui::Button("Run Testbed", bsize))
        launch = GameTarget{"testbed", "testbed", "testbed", "Testbed", {}, 0};

    ImGui::Spacing();
    if (ImGui::Button("Quit", bsize))
        quit = true;
    ImGui::EndGroup();

    // Status bar
    ImGui::Separator();
    ImGui::TextUnformatted(status_.empty() ? "Ready." : status_.c_str());

    // Several games matched one folder: let the user pick.
    if (!pendingChoices_.empty()) {
        ImGui::OpenPopup("Choose game");
        if (ImGui::BeginPopupModal("Choose game", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Several games match this folder:");
            for (const DetectedGame& g : pendingChoices_) {
                std::string label = g.description + " (" + g.language + ", " + g.platform + ")";
                if (ImGui::Selectable(label.c_str())) {
                    addGame(g, pendingPath_);
                    pendingChoices_.clear();
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
            if (ImGui::Button("Cancel")) {
                pendingChoices_.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

// The selected game's saves: one to start the game with.
void Launcher::drawLoad(std::optional<GameTarget>& launch) {
    const float scale = ImGui::GetStyle().FontScaleDpi;
    ImGui::SetNextWindowSize(ImVec2(560 * scale, 360 * scale));
    if (!ImGui::BeginPopupModal("Load", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
        if (loadBrowser_ && !ImGui::IsPopupOpen("Load"))
            loadBrowser_.reset();
        return;
    }
    if (!loadBrowser_) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }
    ImGui::TextUnformatted(config_.get(selected_, "description", selected_).c_str());
    ImGui::BeginGroup();
    const int picked = loadBrowser_->draw(loadSelected_, 0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 1.5f);
    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::BeginGroup();
    loadBrowser_->drawDetails(loadSelected_);
    ImGui::EndGroup();
    bool readable = false;
    for (const save::Slot& s : loadBrowser_->slots())
        if (s.number == loadSelected_)
            readable = s.error.empty();
    ImGui::BeginDisabled(!readable);
    if (ImGui::Button("Load") || (picked && readable)) {
        GameTarget t = targetFromConfig(selected_);
        t.loadSlot = loadSelected_;
        launch = t;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
        ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
}

void SDLCALL Launcher::pickFolderCallback(void* userdata, const char* const* files, int) {
    auto* self = static_cast<Launcher*>(userdata);
    {
        std::lock_guard lock(self->pickMutex_);
        if (!files)
            self->pickError_ = std::string("Folder dialog failed: ") + SDL_GetError();
        else if (files[0])
            self->pickedFolder_ = files[0];
    }
    self->pickInProgress_ = false;
}

void Launcher::handlePickedFolder() {
    std::optional<std::string> folder, error;
    {
        std::lock_guard lock(pickMutex_);
        folder.swap(pickedFolder_);
        error.swap(pickError_);
    }
    if (error)
        status_ = *error;
    if (!folder)
        return;

    auto found = detectGames(*folder);
    if (found.empty()) {
        status_ = "No supported game found in " + *folder;
    } else if (found.size() == 1) {
        addGame(found.front(), *folder);
    } else {
        pendingPath_ = *folder;
        pendingChoices_ = std::move(found);
    }
}

void Launcher::addGame(const DetectedGame& game, const std::string& path) {
    std::string name = game.gameId;
    for (int n = 2; config_.hasSection(name); ++n)
        name = game.gameId + "-" + std::to_string(n);

    config_.set(name, "engine", game.engineId);
    config_.set(name, "gameid", game.gameId);
    config_.set(name, "description", game.description);
    config_.set(name, "language", game.language);
    config_.set(name, "platform", game.platform);
    config_.set(name, "path", path);
    if (Error err = config_.save())
        status_ = err.message;
    else
        status_ = "Added " + game.description;
    selected_ = name;
}

GameTarget Launcher::targetFromConfig(const std::string& name) const {
    return GameTarget{name, config_.get(name, "engine"), config_.get(name, "gameid"),
                      config_.get(name, "description", name), config_.get(name, "path"), 0};
}

} // namespace reluva
