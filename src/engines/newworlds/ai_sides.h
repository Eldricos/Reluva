// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/ai_data.h"
#include "engines/newworlds/economy.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/world.h"

#include <map>
#include <memory>
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

// Where the n-th of the units a mission places on one spot stands: round
// the spot ring by ring (eight to a ring), clear of a building standing on
// it (the game parks a colony's vehicles round its hub).
std::pair<float, float> parkingSpot(int n, float x, float y, bool building);
bool isBee(const CombatType& type);
// A main hub's type (the colonies' and the natives' bases).
bool isMainHub(int type);
// A starting vehicle the game lines up in its side's rally group (one
// that fights, scouts or scans; not a bee or an unarmed APC).
bool joinsRallyGroup(const CombatType& type);
// A side's vehicle rally point (exe.md#orders): the n-th of its 16 parking
// places, a 4 x 4 grid 4096 apart from 2560 inside the corner of the
// block its hub stands on. New armed vehicles drive there, and so do
// those sent back to it.
std::pair<float, float> rallySpot(float hubX, float hubY, int n);

// The names the scripts know an object by, for the player playing slot
// playerSlot: its own side's by type ("colony-hub", "phaser-tank"), other
// colonies' with their race ("klingon-colony-hub", "klingon-hub"), the
// natives' as they are ("taubat-hangar"); and its label, if it has one.
std::vector<std::string> scriptNames(const Unit& u, int playerSlot);

// The sides a mission's AI plays (every slot but the player's): what they
// place, their stances, the orders the game gives them and those the script
// sends (exe.md#sides-and-combat, exe.md#ai-colonies).
//
// An order waits for its time (mission seconds from the start). Its units
// are beamed in where the game's attack group puts them, or are the side's
// own idle ones of the type, nearest first; those it lacks are asked of the
// side's vehicle yards, and the group sets off once it has them all. Guards
// hold their spot and fight what comes near; patrols go round their
// waypoints; assaults follow theirs, fighting what they meet, then go for
// the nearest enemy.
//
// The other colonies (slots 0-2) each run an economy as the player's does,
// except that what they build and make costs them nothing. A build order
// puts a site up on its spot once the spot is clear; a named one (task 9)
// builds again whenever its building is gone. Orders the game marks as
// also for a human side run for the player too, with the player's units.
class AiSides {
public:
    AiSides(std::string_view mission, int playerSlot, World& world);

    // Places every AI side's objects and starts the orders that run from
    // the start; the stances between all sides as the mission sets them.
    void start();
    // The ground, for the AI colonies' mining stations (before start).
    void setGround(ColonyMap ground) { ground_ = std::move(ground); }
    // Whether a side sees a block now (its build orders wait for it).
    void setSeen(std::function<bool(int slot, int bx, int by)> seen) { seen_ = std::move(seen); }
    // An AI colony's economy; nullptr for the player's slot and natives.
    const Economy* colony(int slot) const;
    // A defensive turret's shot paid for from its colony's power (true if
    // it has no colony to pay).
    bool payShot(const Unit& turret, float power);
    // A script's ai action: "<order>..." or "abort <order>...", for the
    // side named ("klingon", "metar"). False if the mission lacks any of them.
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
    // colony's hub, count <name> <n> when the number left of a kind
    // changes (other than the player's buildings, which the colony counts)
    // or of a side's group ([<race>-]small-buildings, large-buildings,
    // fighting-units), and colony-size [<race>] <percent> as a side's
    // colony grows or shrinks against its start.
    std::vector<ScriptEvent> observe(const std::vector<WorldEvent>& events);

    // Running orders, for tests and the title: by name ("" for unnamed).
    std::vector<std::string> running() const;
    // Each order not yet over, as a headless run reports it.
    struct OrderStatus {
        const NpcOrder* order;
        bool started, gathered, hunting;
        int units, awaited; // its units now, those asked of the yards
        int leg;
        double legSeconds;  // since its leg began (since it started, while gathering)
        int site;           // a build's building, once put up (its unit id)
        int blocker;        // a build's: what stands on its spot instead (unit id; 0 none)
        std::vector<int> ids; // its units
    };
    std::vector<OrderStatus> status() const;

private:
    struct Running {
        const NpcOrder* order;
        std::vector<int> units;
        int leg = 0;
        double clock = 0, legClock = 0; // since it started, since its leg began
        bool started = false, gathered = false, hunting = false, done = false;
        int awaited = 0; // units asked of the yards and not yet out
        int site = 0;    // a build order's building, once put up
    };
    struct Colony {
        std::unique_ptr<Economy> economy;
        std::map<int, int> structureOf; // unit id: its structure in the economy
    };
    void queue(const NpcOrder& o);
    void fire(Running& r);
    void gather(Running& r);
    void build(Running& r);
    void colonies(double seconds);
    void steer(Running& r);
    std::pair<float, float> wayPoint(const NpcOrder& o, int leg) const;
    std::pair<float, float> openSpot(float x, float y) const;
    bool home(int slot, float& x, float& y) const;

    std::string mission_;
    int player_;
    World& world_;
    std::map<int, char> races_;
    std::vector<Running> running_;
    std::map<int, Colony> colonies_;
    std::map<int, double> nativeSites_; // a building going up without an economy: seconds put in
    std::map<int, int> rallied_;         // per slot: vehicles sent to its rally point so far
    std::map<int, int> colonyBase_, colonyShare_, colonyTotal_; // per slot: colony size at the start, its shares last told
    ColonyMap ground_;
    std::function<bool(int slot, int bx, int by)> seen_;
    std::map<std::string, int> counts_;
    double clock_ = 0;
};

} // namespace reluva::newworlds
