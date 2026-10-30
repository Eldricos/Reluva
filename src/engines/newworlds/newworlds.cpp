// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

// Star Trek: New Worlds engine (clean-room reimplementation).
//
// Everything in this engine must come from the game's data files, observed
// behaviour of the original, or documented reverse engineering of a legally
// owned copy. Never from leaked source code. See CONTRIBUTING.md.
//
// For now the engine is a data viewer: it opens every .cat archive in the
// game folder and shows the missions, models, textures, maps, sounds,
// strings or fonts (see viewers.h for the keys). Tab switches between them;
// Esc goes back to the launcher.

#include "engines/engine.h"
#include "engines/newworlds/movie_player.h"
#include "engines/newworlds/viewers.h"
#include "common/fs.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>

namespace reluva {

namespace {

using newworlds::CatArchive;
using newworlds::MapViewer;
using newworlds::MissionViewer;
using newworlds::ModelViewer;
using newworlds::SoundViewer;
using newworlds::FontViewer;
using newworlds::StringViewer;
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
        MapViewer maps(sys_, screen_, archives_);
        const std::vector<newworlds::Level> levels = readLevels();
        maps.setLevels(levels);
        // One mixer for every page, playing for as long as the engine runs;
        // without an audio device the pages show and don't play.
        audio::Mixer mixer(sys_.audioRate());
        const bool audioOut = !audio::startMixerOutput(sys_, mixer);
        struct AudioStop {
            System& sys;
            bool on;
            ~AudioStop() {
                if (on)
                    sys.stopAudio();
            }
        } audioStop{sys_, audioOut};
        audio::Mixer* const output = audioOut ? &mixer : nullptr;
        SoundViewer sounds(sys_, screen_, archives_, output, target_.path);
        StringViewer strings(sys_, screen_, archives_);
        FontViewer fonts(sys_, screen_, archives_);
        MissionViewer missions(sys_, screen_, archives_, levels, output, target_.path);
        missions_ = &missions;
        constexpr int kViewers = 7;
        const size_t counts[kViewers] = {missions.count(), models.count(),  textures.count(), maps.count(),
                                         sounds.count(),   strings.count(), fonts.count()};
        size_t total = 0;
        for (size_t c : counts)
            total += c;
        if (total == 0)
            return Error::fail("No missions, models, textures, maps, sounds, strings or fonts found in " +
                               fs::toUtf8(target_.path));
        // Viewer 0 shows missions, 1 models, 2 textures, 3 maps, 4 sounds, 5 strings, 6 fonts;
        // Tab moves to the next one that has something to show. They draw
        // into the screen, so it must exist before the first activate().
        int viewer = 0;
        while (counts[viewer] == 0)
            viewer = (viewer + 1) % kViewers;
        auto activate = [&] {
            if (viewer == 0)
                missions.activate();
            else if (viewer == 1)
                models.activate();
            else if (viewer == 2)
                textures.activate();
            else if (viewer == 3)
                maps.activate();
            else if (viewer == 4)
                sounds.activate();
            else if (viewer == 5)
                strings.activate();
            else
                fonts.activate();
        };
        // The game's movies (install.md#movies): the logos and the intro at
        // launch; a race's movie when its campaign is won; F5 (the intro)
        // and F6 (the side's movie) on the missions page.
        newworlds::MoviePlayer movies(sys_, screen_, output, target_.path);
        movies_ = &movies;
        struct Forget { // the pages are gone once run() returns
            NewWorldsEngine& e;
            ~Forget() { e.missions_ = nullptr, e.movies_ = nullptr; }
        } forget{*this};
        movies.play({"LOGOS.BIK", "STNW.BIK"});
        if (!movies.playing())
            activate();
        auto raceMovie = [](char side) {
            return side == 'K' ? "KLIWIN.BIK" : side == 'R' ? "ROMWIN.BIK" : "FEDWIN.BIK";
        };

        bool moviesOn = movies.playing();
        while (true) {
            Event ev;
            while (sys_.pollEvent(ev)) {
                if (ev.type == EventType::Quit || ev.type == EventType::ReturnToLauncher)
                    return Error::success();

                if (movies.playing()) {
                    movies.handleEvent(ev);
                    continue;
                }
                // While the missions page is playing, or a run (G) is under
                // way at any stage, Escape and Tab are the game's.
                const bool game = viewer == 0 && (missions.playing() || missions.inRun());
                if (ev.type == EventType::KeyDown && ev.key == key::Escape && !game)
                    return Error::success();
                if (ev.type == EventType::KeyDown && ev.key == key::Tab && !game) {
                    if (viewer == 0)
                        missions.deactivate();
                    else if (viewer == 4)
                        sounds.deactivate();
                    do
                        viewer = (viewer + 1) % kViewers;
                    while (counts[viewer] == 0);
                    activate();
                    continue;
                }
                if (viewer == 0 && !game && ev.type == EventType::KeyDown && !ev.repeat &&
                    (ev.key == key::F1 + 4 || ev.key == key::F1 + 5)) {
                    // The page stops (its voice clip and music) while the
                    // movie plays; it comes back when the movie ends.
                    missions.deactivate();
                    if (movies.play({ev.key == key::F1 + 4 ? "STNW.BIK" : raceMovie(missions.side())}) == 0)
                        activate(); // (no movie to play)
                    else
                        moviesOn = true;
                    continue;
                }
                if (viewer == 0)
                    missions.handleEvent(ev);
                else if (viewer == 1)
                    models.handleEvent(ev);
                else if (viewer == 2 && ev.type == EventType::KeyDown)
                    textures.handleKey(ev.key);
                else if (viewer == 3)
                    maps.handleEvent(ev);
                else if (viewer == 4 && ev.type == EventType::KeyDown)
                    sounds.handleKey(ev.key);
                else if (viewer == 5 && ev.type == EventType::KeyDown)
                    strings.handleKey(ev.key);
                else if (viewer == 6 && ev.type == EventType::KeyDown)
                    fonts.handleKey(ev.key);
            }
            // A savestate loaded (by the Reluva menu, inside pollEvent): its
            // mission is on the missions page.
            if (loaded_) {
                loaded_ = false;
                if (viewer != 0) {
                    if (viewer == 4)
                        sounds.deactivate();
                    viewer = 0;
                    activate();
                }
            }
            movies.update(sys_.getMillis());
            if (moviesOn && !movies.playing())
                activate(); // the page back on the screen (the movies ended or were skipped)
            moviesOn = movies.playing();
            if (moviesOn) {
                sys_.updateScreen(screen_);
                sys_.delayMillis(5);
                continue;
            }
            if (viewer == 0)
                missions.update(sys_.getMillis());
            if (const char won = missions.takeCampaignWon()) {
                // The race's movie, the page quiet under it (as F5/F6).
                missions.deactivate();
                moviesOn = movies.play({raceMovie(won)}) > 0;
                if (!moviesOn)
                    activate();
            }
            else if (viewer == 1)
                models.update(sys_.getMillis());
            else if (viewer == 3)
                maps.update(sys_.getMillis());
            else if (viewer == 4)
                sounds.update(sys_.getMillis());
            sys_.updateScreen(screen_);
            sys_.delayMillis(10);
        }
    }

    // Savestates (docs/savestates.md): a mission being played on the
    // missions page.
    bool canSaveState(std::string& why) const override {
        if (!missions_) {
            why = "The game isn't up yet.";
            return false;
        }
        if (movies_ && movies_->playing()) {
            why = "A movie is playing.";
            return false;
        }
        return missions_->canSave(why);
    }
    bool canLoadState(std::string& why) const override {
        if (!missions_) {
            why = "The game isn't up yet.";
            return false;
        }
        return true;
    }
    uint32_t stateVersion() const override { return kStateVersion; }
    std::string stateDescription() const override { return missions_ ? missions_->stateDescription() : ""; }
    Error syncState(save::Serializer& s) override {
        if (!missions_)
            return Error::fail("The game isn't up yet.");
        if (s.loading() && movies_)
            movies_->stop();
        Error e = missions_->syncState(s);
        if (s.loading()) {
            loaded_ = true;
            syncFailed_ = !e.ok;
        }
        return e;
    }
    // A load the menu found wrong after the mission took it (bytes left
    // over): the mission starts over. One that failed in syncState() has
    // already put the page right (the mission restarted, or none played)
    // and said so.
    bool loadFailed(std::string& done) override {
        if (missions_ && !syncFailed_ && missions_->restartPlay())
            done = "The mission starts over.";
        syncFailed_ = false;
        return true;
    }

private:
    // Goes up whenever a savestate's layout changes (any syncState in the
    // engine): older saves are then refused.
    static constexpr uint32_t kStateVersion = 9;

    // The main menu's mission list, if the game folder has one; a missing
    // or unreadable level.dat only loses the missions' names, a line in it
    // that isn't a mission only that line.
    std::vector<newworlds::Level> readLevels() const {
        std::vector<newworlds::Level> levels;
        const auto path = fs::findFile(target_.path, "level.dat");
        if (!path)
            return levels;
        std::ifstream in(*path, std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::vector<std::string> skipped;
        if (newworlds::parseLevelList(text, levels, &skipped))
            levels.clear();
        for (const std::string& why : skipped)
            std::fprintf(stderr, "New Worlds: %s, skipped\n", why.c_str());
        return levels;
    }

    Error openArchives() {
        std::vector<std::filesystem::path> paths;
        std::error_code ec;
        for (std::filesystem::directory_iterator it(target_.path, ec), end; !ec && it != end; it.increment(ec))
            if (fs::toLower(fs::toUtf8(it->path().extension())) == ".cat")
                paths.push_back(it->path());
        if (paths.empty())
            return Error::fail("No .cat archives in " + fs::toUtf8(target_.path));
        std::sort(paths.begin(), paths.end());

        // One that won't open (unreadable, or not the game's) is left out,
        // the rest shown.
        for (const auto& p : paths) {
            auto cat = std::make_unique<CatArchive>();
            if (Error err = cat->open(p)) {
                std::fprintf(stderr, "New Worlds: %s, skipped\n", err.message.c_str());
                continue;
            }
            archives_.push_back(std::move(cat));
        }
        if (archives_.empty())
            return Error::fail("No readable .cat archives in " + fs::toUtf8(target_.path));
        return Error::success();
    }

    System& sys_;
    GameTarget target_;
    MissionViewer* missions_ = nullptr;
    newworlds::MoviePlayer* movies_ = nullptr;
    bool loaded_ = false, syncFailed_ = false;
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
