// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/config.h"
#include "common/detection.h"
#include "engines/engine.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace reluva {

class SdlSystem;
class SlotBrowser;

// A configured game, as its section of the config describes it.
GameTarget targetFromConfig(const Config& config, const std::string& name);

// The game list UI, drawn with Dear ImGui on the SDL renderer.
class Launcher {
public:
    Launcher(SdlSystem& system, Config& config, std::filesystem::path saveRoot);
    ~Launcher();
    Launcher(const Launcher&) = delete;
    Launcher& operator=(const Launcher&) = delete;

    // Show the launcher until the user starts a game (returned) or quits.
    std::optional<GameTarget> run();

    // Shown in the status bar next time the launcher appears.
    void setStatus(std::string message) { status_ = std::move(message); }

private:
    void drawUi(std::optional<GameTarget>& launch, bool& quit);
    void handlePickedFolder();
    void addGame(const DetectedGame& game, const std::string& path);
    GameTarget targetFromConfig(const std::string& name) const { return reluva::targetFromConfig(config_, name); }

    // The folder dialog answers on another thread, perhaps after the
    // launcher is gone: it writes to this, which it shares.
    struct PickState {
        std::mutex mutex;
        std::optional<std::string> folder; // UTF-8
        std::optional<std::string> error;
        std::atomic<bool> inProgress{false};
    };
    static void pickFolderCallback(void* userdata, const char* const* files, int filter);

    void drawLoad(std::optional<GameTarget>& launch);

    SdlSystem& sys_;
    Config& config_;
    std::filesystem::path saveRoot_;
    std::unique_ptr<SlotBrowser> loadBrowser_; // the Load window's list, while it's open
    int loadSelected_ = 0;
    std::string selected_;
    std::string status_;

    std::shared_ptr<PickState> pick_ = std::make_shared<PickState>();

    // Folder (UTF-8) with several matches, waiting for the user to choose one.
    std::string pendingPath_;
    std::vector<DetectedGame> pendingChoices_;
};

} // namespace reluva
