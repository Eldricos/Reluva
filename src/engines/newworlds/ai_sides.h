// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/ai_data.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/world.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// A unit or building of a type put on the map for a side: its hit points,
// weapon and speed from the type (the local player's own vehicles and
// buildings are tougher: the game gives the Federation 1.2 times the hit
// points, the Romulans 1.3; exe.md#sides-and-combat). Returns its id.
int spawnObject(World& world, int slot, char race, const CombatType& type, float x, float y, const std::string& label,
                const std::string& model = {}, bool player = false);

// The names the scripts know an object by, for the player playing slot
// playerSlot: its own side's by type ("colony-hub", "phaser-tank"), other
// colonies' with their race ("klingon-colony-hub", "klingon-hub"), the
// natives' as they are ("taubat-hangar"); and its label, if it has one.
std::vector<std::string> scriptNames(const Unit& u, int playerSlot);

// The sides a mission's AI plays (every slot but the player's): what they
// place, their stances, the orders the game gives them from the start and
// those the script sends (exe.md#sides-and-combat).
//
// An order's units are beamed in where the game's attack group puts them,
// or are the side's own (idle ones of the type, nearest first); any it
// lacks come out of the side's first building, standing in for the game's
// own production, which isn't modelled. Guards hold their spot and fight
// what comes near; patrols go round their waypoints; assaults follow
// theirs, fighting what they meet, then go for the nearest enemy.
class AiSides {
public:
    AiSides(std::string_view mission, int playerSlot, World& world);

    // Places every AI side's objects and starts the orders that run from
    // the start; the stances between all sides as the mission sets them.
    void start();
    // A script's ai action: "<order>" or "abort <order>", for the side
    // named ("klingon", "metar"). False if the mission has no such order.
    bool order(const std::string& side, const std::string& text);
    // A script's stance action (hostile, neutral, allied): the side named
    // takes that stance towards the player (as the game's posture switch).
    void stance(const std::string& side, const std::string& verb);
    // The slots a script's side name means: those of that race, or placing
    // its objects (R2's Orion tanks share the Taubat slot).
    std::vector<int> slotsNamed(const std::string& side) const;
    char raceOf(int slot) const;

    void advance(double seconds);

    // What combat did, as the scripts' events: destroyed <name> for each
    // name of each object destroyed, hub-destroyed <race> for another
    // colony's hub, and count <name> <n> when the number left of a kind
    // changes (other than the player's buildings, which the colony counts).
    std::vector<ScriptEvent> observe(const std::vector<WorldEvent>& events);

    // Running orders, for tests and the title: by name ("" for unnamed).
    std::vector<std::string> running() const;

private:
    struct Running {
        const NpcOrder* order;
        std::vector<int> units;
        int leg = 0;
        double clock = 0, legClock = 0; // since it started, since its leg began
        bool hunting = false, done = false;
    };
    void begin(const NpcOrder& o);
    void steer(Running& r);
    std::pair<float, float> wayPoint(const NpcOrder& o, int leg) const;
    std::pair<float, float> openSpot(float x, float y) const;
    bool home(int slot, float& x, float& y) const;

    std::string mission_;
    int player_;
    World& world_;
    std::map<int, char> races_;
    std::vector<Running> running_;
    std::map<std::string, int> counts_;
    double clock_ = 0;
};

} // namespace reluva::newworlds
