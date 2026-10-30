// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/detection.h"
#include "common/savestate.h"
#include "common/system.h"
#include "common/types.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace reluva {

// A game the user has added, as stored in the config file.
struct GameTarget {
    std::string target;   // config section name, unique per install
    std::string engineId;
    std::string gameId;
    std::string description;
    std::filesystem::path path; // game data directory
    int loadSlot = 0;            // a savestate to load once the game is up (0 none)
    std::string language;        // the detected variant's ("en", "de", ...; empty if unknown)
    std::string platform;        // ("windows", ...)
};

// A running game. Created by its MetaEngine, run once, then destroyed.
class Engine {
public:
    virtual ~Engine() = default;

    // Runs the game until the player quits or asks to return to the launcher.
    virtual Error run() = 0;

    // Savestates (docs/savestates.md). An engine that has them says when
    // it can save and load (`why` explains a no), and lists its whole
    // state in syncState(), for saving or loading as the Serializer says.
    // Both are called by the Reluva menu from inside System::pollEvent(),
    // between the engine's frames. stateVersion() goes up whenever the
    // state's layout changes: older saves are then refused, not misread.
    virtual bool canSaveState(std::string& why) const {
        why = "This game has no savestates yet.";
        return false;
    }
    virtual bool canLoadState(std::string& why) const { return canSaveState(why); }
    virtual uint32_t stateVersion() const { return 0; }
    // A description for a new save ("F1: Power Corrupts").
    virtual std::string stateDescription() const { return {}; }
    virtual Error syncState(save::Serializer& s) {
        (void)s;
        return Error::fail("This game has no savestates.");
    }
    // A load that failed once syncState() had begun reading (its error,
    // a damaged save, bytes left over): the state may be half-read, so the
    // engine puts itself somewhere sound (its menu, the mission from its
    // start) and returns true, saying in `done` what it did if the error
    // doesn't already ("The mission starts over."; the menu shows it after
    // the error), or returns false if it can't, and the menu then offers
    // only another load, the launcher or quitting.
    virtual bool loadFailed(std::string& done) {
        (void)done;
        return false;
    }
};

// Static description of an engine: its name, how to recognise its games,
// and how to create an instance.
class MetaEngine {
public:
    virtual ~MetaEngine() = default;

    virtual const char* id() const = 0;
    virtual const char* name() const = 0;

    virtual std::vector<DetectedGame> detect(const std::filesystem::path& dir) const = 0;
    virtual std::unique_ptr<Engine> create(System& system, const GameTarget& target) const = 0;
};

} // namespace reluva
