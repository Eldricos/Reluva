// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Star Trek: New Worlds engine (clean-room reimplementation).
//
// Everything in this engine must come from the game's data files, observed
// behaviour of the original, or documented reverse engineering of a legally
// owned copy. Never from leaked source code. See CONTRIBUTING.md.
//
// For now the engine is a data viewer: it opens every .cat archive in the
// game folder and shows the models or the textures (see viewers.h for the
// keys). Tab switches between them; Esc goes back to the launcher.

#include "engines/engine.h"
#include "engines/newworlds/viewers.h"
#include "common/fs.h"

#include <algorithm>
#include <memory>

namespace reluva {

namespace {

using newworlds::CatArchive;
using newworlds::ModelViewer;
using newworlds::TextureViewer;

// Detection table. Built with `reluva --hash`: each entry needs the size and
// the MD5 of the first kDetectionHashBytes bytes.
//
// Detection keys on glblgame.cat: patches tend to replace the executable but
// not the large archives, so the executable is only required to exist.
const std::vector<GameVariant> kVariants = {
    {"newworlds", "Star Trek: New Worlds", "en", "windows",
     {{"glblgame.cat", "9b5c322aef388f8e58e72b81bd9a91ad", 6883984},
      {"STNW.exe", "", -1}}},
};

constexpr int kScreenW = 640;
constexpr int kScreenH = 480;

class NewWorldsEngine : public Engine {
public:
    NewWorldsEngine(System& system, GameTarget target) : sys_(system), target_(std::move(target)) {}

    Error run() override {
        if (Error err = openArchives())
            return err;
        if (Error err = sys_.initGraphics(kScreenW, kScreenH, PixelFormat::RGBA32))
            return err;
        screen_.create(kScreenW, kScreenH, PixelFormat::RGBA32);
        ModelViewer models(sys_, screen_, archives_);
        TextureViewer textures(sys_, screen_, archives_);
        if (models.count() == 0 && textures.count() == 0)
            return Error::fail("No models or textures found in " + target_.path.string());
        bool showModels = models.count() > 0;
        if (showModels)
            models.activate(); // the viewers draw into the screen, so it must exist
        else
            textures.activate();

        while (true) {
            Event ev;
            while (sys_.pollEvent(ev)) {
                if (ev.type == EventType::Quit || ev.type == EventType::ReturnToLauncher)
                    return Error::success();
                if (ev.type == EventType::KeyDown && ev.key == key::Escape)
                    return Error::success();
                if (ev.type == EventType::KeyDown && ev.key == key::Tab) {
                    if ((showModels ? textures.count() : models.count()) > 0) {
                        showModels = !showModels;
                        if (showModels)
                            models.activate();
                        else
                            textures.activate();
                    }
                    continue;
                }
                if (showModels)
                    models.handleEvent(ev);
                else if (ev.type == EventType::KeyDown)
                    textures.handleKey(ev.key);
            }
            if (showModels)
                models.update(sys_.getMillis());
            sys_.updateScreen(screen_);
            sys_.delayMillis(10);
        }
    }

private:
    Error openArchives() {
        std::vector<std::filesystem::path> paths;
        std::error_code ec;
        for (std::filesystem::directory_iterator it(target_.path, ec), end; !ec && it != end; it.increment(ec))
            if (fs::toLower(it->path().extension().string()) == ".cat")
                paths.push_back(it->path());
        if (paths.empty())
            return Error::fail("No .cat archives in " + target_.path.string());
        std::sort(paths.begin(), paths.end());

        for (const auto& p : paths) {
            auto cat = std::make_unique<CatArchive>();
            if (Error err = cat->open(p))
                return err;
            archives_.push_back(std::move(cat));
        }
        return Error::success();
    }

    System& sys_;
    GameTarget target_;
    Surface screen_;
    newworlds::Archives archives_;
};

class NewWorldsMetaEngine : public MetaEngine {
public:
    const char* id() const override { return "newworlds"; }
    const char* name() const override { return "New Worlds"; }

    std::vector<DetectedGame> detect(const std::filesystem::path& dir) const override {
        return detectFromTable(id(), kVariants, dir);
    }

    std::unique_ptr<Engine> create(System& system, const GameTarget& target) const override {
        return std::make_unique<NewWorldsEngine>(system, target);
    }
};

} // namespace

const MetaEngine& newWorldsMetaEngine() {
    static const NewWorldsMetaEngine instance;
    return instance;
}

} // namespace reluva
