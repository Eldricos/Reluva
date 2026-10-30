// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/savestate.h"
#include "engines/newworlds/ai_sides.h"
#include "engines/newworlds/colony.h"
#include "engines/newworlds/economy.h"
#include "engines/newworlds/field_work.h"
#include "engines/newworlds/fog.h"
#include "engines/newworlds/ground.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/model_textures.h"
#include "engines/newworlds/repairs.h"
#include "engines/newworlds/research.h"
#include "engines/newworlds/script_effects.h"
#include "engines/newworlds/world.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace reluva::newworlds {

// A mission being played: the one simulation both the missions page's
// play mode and the headless runs use. Its map's terrain; the player's
// starting colony and units as the mission places them; the other sides
// with their AI and economies; the fog, repairs, field work, research,
// the script's effects and the mission script itself; and the rules that
// join them (officers' skills into efficiency, vehicles out of the yards,
// buildings razed, the scripts' sightings and monitors). The screen only
// draws it and passes on the player's orders.
//
// For finding what gets stuck it also watches for units that can't get
// anywhere, AI groups that never gather or move on, builds whose spot
// never clears and shots that never tell.
class MissionSim {
public:
    // A savestate's part (docs/savestates.md): what play changes, saved or
    // loaded; the rest is made again as the mission starts.
    void syncState(save::Serializer& s);
    // A mission on its map: the terrain, the ground's deposits, the map's
    // size in blocks, and the player's starting colony (its hub first; map
    // positions in eighths of a block, as the scripts and placements
    // write them).
    MissionSim(std::string_view mission, char side, Terrain terrain, ColonyMap map, int blocksWide, int blocksHigh,
               std::vector<ColonyBuilding> colony);
    // Headless: the colony as the mission places it (else its hub where
    // the recordings put it).
    MissionSim(std::string_view mission, char side, const Ground& ground);
    // The starting colony the mission places for a side, its hub first;
    // empty if it places none.
    static std::vector<ColonyBuilding> placedColony(std::string_view mission, char side);

    // Objects' sizes to shots from their models (exe.md#sides-and-combat):
    // each model's bounds radius, read from the archives; 0.9 of it for a
    // vehicle (half for types 117, 261, 264), times its type's scale for a
    // building. Without this, fixed sizes. All of it is read here (the
    // docks too), so the archives needn't outlive the sim.
    void useModels(const Archives& archives);

    // Moves time on in the game's fixed ticks of 50 ms (world, AI,
    // economy, field work, effects, script, all a tick at a time), the
    // part of a tick left over carried to the next call: the same run
    // whatever steps it's given (the screen's frames, play-check's
    // seconds).
    void advance(double seconds);
    double clock() const { return clock_; }
    // How far time is into the next tick (0 to 1): the screen draws what
    // moves that far between where the last tick found it and where it is.
    double tickShare() const;

    World& world() { return *world_; }
    const World& world() const { return *world_; }
    Economy& economy() { return *economy_; }
    AiSides& ai() { return *ai_; }
    Fog& fog() { return *fog_; }
    Repairs& repairs() { return *repairs_; }
    FieldWork& fieldWork() { return *fieldWork_; }
    Research& research() { return *research_; }
    ScriptEffects& effects() { return *effects_; }
    MissionRun* run() { return run_.get(); }
    int playerSlot() const { return player_; }
    char side() const { return side_; }

    // The colony's buildings in the order they were put up: what each is,
    // its world object (-1 once destroyed, -2 a site cancelled) and its
    // structure in the economy.
    const std::vector<ColonyBuilding>& colony() const { return colony_; }
    const std::vector<int>& colonyObjects() const { return colonyObjects_; }
    const std::vector<int>& structureIds() const { return structureIds_; }
    // The bees in the hub, by kind ("construction-bee"), and all of them.
    const std::map<std::string, int>& beesHoused() const { return beesHoused_; }
    int housed() const { return housed_; }
    // The colony's transports (exe.md#transports): in the hub, and all
    // (those out ferrying people too).
    int transportsHoused() const { return garage_; }
    int transports() const { return garage_ + int(shuttles_.size()); }

    // The player's orders that touch the rules. A building ordered at a
    // map position (its middle): the cost taken, its site put up; its
    // index in colony(), or -1 if the economy refuses it.
    int placeBuilding(const std::string& name, const std::string& model, int a, int b);
    // Where a building ordered near a world point would stand (its
    // footprint's middle, as a map position) and whether it can go there
    // (exe.md#building-placement); false if the side has no such building.
    bool placementSpot(const std::string& name, float x, float y, int& a, int& b, bool& fits) const;
    // An upgrade, ordered on its building's spot (x, y), shown at a, b.
    int placeUpgrade(const std::string& name, const std::string& model, int a, int b, float x, float y);
    // DEMOLISH (exe.md#demolishing): half its cost back, its officers to
    // the hub, its people out; then it's destroyed.
    bool raze(int unit);
    // A site or upgrade still going up cancelled (its structure id).
    bool cancelBuild(int structure);
    bool razing(int unit) const;
    // ALLY (exe.md#the-tricorder): the player's posture towards a race
    // (every slot of it): 1 friendly, 2 neutral, 3 hostile. Set neutral,
    // it turns hostile at that race's first shot at the player.
    void setPosture(int slot, int posture);

    // The script driven by hand (a testing aid): an event fired, or its
    // timers moved on.
    void fireScript(const ScriptEvent& e);
    void runTimers(double seconds);

    // What the script said since last asked, with the event that made it
    // (the screen shows the messages, plays the voices, ends the mission);
    // what combat did (shots, hits, losses).
    struct Said {
        ScriptEvent fired;
        std::vector<MissionRun::Output> out;
        friend void syncState(save::Serializer& s, Said& v) { s(v.fired)(v.out); }
    };
    std::vector<Said> takeSaid();
    std::vector<WorldEvent> takeEvents();
    // What the colony says (exe.md#sounds): its officers' reports by
    // number ("002": construction delayed, ...) and "attack" (one of its
    // finished buildings hit; at most once in 30 seconds).
    std::vector<std::string> takeVoices();
    // The player's objects with an alert (exe.md#cameras: hit, or a
    // report: everyone beamed down, a mine exhausted), for Space; an
    // alert lasts 41 ticks.
    bool alerted(int unit) const;

    // What looks wrong now, one line each: "stuck: ..." (units that can't
    // get anywhere, groups that never move on, builds whose spot never
    // clears, groups whose side has a yard but gets no units) or "waits:
    // ..." (a group whose side has no way to get its units: the game's
    // waits too). And what happened, as it did.
    std::vector<std::string> problems() const;
    const std::vector<std::string>& log() const { return log_; }
    bool won() const { return run_ && run_->won(); }
    bool lost() const { return lost_; }

private:
    void step();
    void deliver(const ScriptEvent& fired, const std::vector<MissionRun::Output>& out);
    void spawnColonyObject(size_t index);
    void rollOut();
    void runTransports(double dt);
    void sightings(std::vector<ScriptEvent>& happened);
    void watchUnits();
    void note(const std::string& line);

    std::string mission_;
    char side_;
    int player_;
    int blocksWide_, blocksHigh_;
    ColonyMap ground_; // the deposits, for mines' spots
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
    std::vector<ColonyBuilding> colony_;
    std::vector<int> colonyObjects_, structureIds_;
    std::map<std::string, int> beesHoused_;
    int housed_ = 0, rallied_ = 0;
    // The transports: those in the hub, and each one out on a ride: its
    // unit, the ride, the leg (0 to the pick-up, 1 loading, 2 to the
    // drop, 3 unloading, 4 home, 5 idle with no hub to go home to),
    // seconds left at a stop, tries to get where it's going.
    int garage_ = 0;
    struct Shuttle {
        int unit, ride, leg = 0;
        double wait = 0;
        int tries = 0;
        float tx = 0, ty = 0;
        friend void syncState(save::Serializer& s, Shuttle& v) {
            s(v.unit)(v.ride)(v.leg)(v.wait)(v.tries)(v.tx)(v.ty);
        }
    };
    std::vector<Shuttle> shuttles_;
    std::vector<int> razing_;
    std::set<int> touchyNeutral_;
    std::vector<Said> said_;
    std::vector<WorldEvent> events_;
    std::vector<std::string> voices_;
    double lastAttackVoice_ = -1e9;
    std::map<int, int> alerts_; // unit: the world tick its alert was set
    std::map<std::string, int> lastCounts_;
    std::set<std::string> seenSites_, scannedSites_;
    std::set<std::pair<std::string, std::string>> approached_;
    // Each moving unit's destination, how far it was from it, and since
    // when: to tell one that's stuck.
    struct Track {
        float x, y, left;
        double since;
        friend void syncState(save::Serializer& s, Track& v) { s(v.x)(v.y)(v.left)(v.since); }
    };
    std::map<int, Track> tracks_;
    // Each firing unit's target, its hit points then, and since when.
    struct Shooting {
        int target;
        float hp;
        double since;
        friend void syncState(save::Serializer& s, Shooting& v) { s(v.target)(v.hp)(v.since); }
    };
    std::map<int, Shooting> shooting_;
    std::set<int> stuck_, placedBadly_, harmless_;
    std::vector<std::string> log_;
    double clock_ = 0;
    int ticks_ = 0;       // ticks run
    double pending_ = 0;  // seconds passed short of a tick
    bool lost_ = false;
};

// A vehicle's voice (exe.md#sounds): the recording for its type, one of
// its two voices (0, 1) and one of a group's two lines (0, 1); group 0
// on being selected, 1 on taking an order, 2 refusing one. Empty if the
// type has no voices.
std::string unitLine(int type, int group, int voice, int line);

// A building's own sound (exe.md#sounds), played when it's double-clicked:
// its race's build archive ("fedbuild.cat") and the clip's stem in it
// ("fcomstn"); empty for types without one (every vehicle, the natives).
std::pair<std::string, std::string> buildingSound(int type);

// A vehicle's start-up sound (exe.md#sounds), played as it sets off on a
// path: its archive ("fedactor.cat") and stem ("fpvstart"); empty for
// types without one (bees, buildings).
std::pair<std::string, std::string> driveSound(int type);

// Where a world point is, as the scripts write it ("<row>:<col>" in
// eighths of a block).
std::string mapPosition(float x, float y);

} // namespace reluva::newworlds
