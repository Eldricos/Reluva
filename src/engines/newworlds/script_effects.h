// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/ai_sides.h"
#include "engines/newworlds/economy.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/world.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace reluva::newworlds {

// The game's script effects, carried out in play
// (docs/formats/newworlds/exe.md#script-effects): stock given, taken and
// topped up; a group damaged, beamed up, driven along a route; units
// spawned; meteor and electrical storms over a zone; the disease. And the
// events the game's area, build-and-hold and disease monitors give:
// inside / outside <who> <zone>, beamed-up <group>, standing <what>
// <seconds>, infection <percent>, cured.
class ScriptEffects {
public:
    ScriptEffects(int playerSlot, World& world, AiSides* ai, Economy* economy, const MissionScript* script);

    // A script output: an effect (verb and its arguments), or stock
    // (give / take <material> <amount>).
    void apply(const std::string& verb, const std::string& args);

    std::vector<ScriptEvent> advance(double seconds);

    // What to draw: meteors falling and storm flashes, by world position
    // and age (seconds); kind 0 a meteor, 1 a flash, 2 a beam (spawn or
    // beam-up).
    struct Spark {
        float x, y;
        int kind;
        double age = 0;
    };
    const std::vector<Spark>& sparks() const { return sparks_; }
    int infection() const { return int(infection_); } // percent

private:
    struct Storm {
        std::string zone;
        double every = 0, clock = 0; // meteors: seconds between; 0 a storm's flashes
        bool meteors = false;
    };
    struct Route {
        std::vector<std::pair<float, float>> points;
        size_t next = 0;
        bool loop = false;
    };
    std::array<float, 4> zoneRect(const std::string& zone) const; // world x0, y0, x1, y1
    int slotNamed(const std::string& side) const;
    void tick(std::vector<ScriptEvent>& out);
    void zones(std::vector<ScriptEvent>& out);

    int player_;
    World& world_;
    AiSides* ai_;
    Economy* economy_;
    const MissionScript* script_;
    Stock topUp_{};
    bool toppingUp_ = false;
    std::vector<Storm> storms_;
    std::map<int, Route> routes_; // by unit
    std::vector<Spark> sparks_;
    std::vector<std::string> beaming_; // groups beamed up, to report
    std::map<std::string, std::set<std::string>> inside_; // per zone: who was inside
    bool zonesSeen_ = false;
    std::map<std::string, double> standing_; // what: seconds standing
    double infection_ = 0;
    int infected_ = 0, crewAtStart_ = 0;
    int infecting_ = 0; // 1 spreading, -1 being cured, 0 neither
    double clock_ = 0, second_ = 0, topUpClock_ = 0;
    uint32_t random_ = 2024;
};

} // namespace reluva::newworlds
