// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/ai_sides.h"
#include "engines/newworlds/economy.h"
#include "engines/newworlds/field_work.h"
#include "engines/newworlds/fog.h"
#include "engines/newworlds/ground.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/repairs.h"
#include "engines/newworlds/research.h"
#include "engines/newworlds/script_effects.h"
#include "engines/newworlds/world.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// A mission played without a screen: its map's terrain, the player's
// starting colony (left to itself: nobody gives it orders), the other
// sides with their AI and economies, the fog, repairs, field work,
// research, the script's effects and the mission script itself. For
// finding what gets stuck: units that can't get anywhere, AI groups that
// never gather or never move on, builds whose spot never clears.
class MissionSim {
public:
    MissionSim(std::string_view mission, char side, const Ground& ground);

    // Moves time on, in steps of a tenth of a second.
    void advance(double seconds);
    double clock() const { return clock_; }

    // What looks wrong now, one line each: "stuck: ..." (units that can't
    // get anywhere, groups that never move on, builds whose spot never
    // clears, groups whose side has a yard but gets no units) or "waits:
    // ..." (a group whose side has no way to get its units: the game's
    // waits too). And what happened, as it did.
    std::vector<std::string> problems() const;
    const std::vector<std::string>& log() const { return log_; }
    bool won() const { return run_ && run_->won(); }
    bool lost() const { return lost_; }

    World& world() { return *world_; }
    AiSides& ai() { return *ai_; }

private:
    void step(double dt);
    void deliver(const std::vector<MissionRun::Output>& out);
    void watchUnits();
    void note(const std::string& line);

    std::string mission_;
    char side_;
    int player_;
    std::unique_ptr<World> world_;
    std::unique_ptr<Economy> economy_;
    std::unique_ptr<AiSides> ai_;
    std::unique_ptr<Fog> fog_;
    std::unique_ptr<Repairs> repairs_;
    std::unique_ptr<FieldWork> fieldWork_;
    std::unique_ptr<Research> research_;
    std::unique_ptr<ScriptEffects> effects_;
    std::unique_ptr<MissionRun> run_;
    const MissionScript* script_ = nullptr;
    std::map<int, int> structureOf_; // the player's buildings: unit id, structure
    std::map<std::string, int> lastCounts_;
    std::set<std::string> seenSites_, scannedSites_;
    // Each moving unit's destination, how far it was from it, and since
    // when: to tell one that's stuck.
    struct Track {
        float x, y, left;
        double since;
    };
    std::map<int, Track> tracks_;
    // Each firing unit's target, its hit points then, and since when.
    struct Shooting {
        int target;
        float hp;
        double since;
    };
    std::map<int, Shooting> shooting_;
    std::set<int> stuck_, placedBadly_, harmless_;
    std::vector<std::string> log_;
    double clock_ = 0, fogClock_ = 0;
    bool lost_ = false;
};

// Where a world point is, as the scripts write it ("<row>:<col>" in
// eighths of a block).
std::string mapPosition(float x, float y);

} // namespace reluva::newworlds
