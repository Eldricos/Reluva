// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "common/savestate.h"
#include "engines/newworlds/ai_data.h"
#include "engines/newworlds/fog.h"
#include "engines/newworlds/mission_script.h"
#include "engines/newworlds/world.h"

#include <array>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace reluva::newworlds {

// The player's field work: scans, captures and officers, as the game's
// executable does them (docs/formats/newworlds/exe.md#scans-and-captures).
//
// A scan: a ground scout or science vehicle drives into sensor range of
// its target (its type's scan range plus the target's size) and scans it
// for as long as it stays there; nothing ends it but another order or the
// target's loss. It is strong when an officer aboard has the scan's
// speciality (science 4, medical 3, engineering 1; a command officer for
// engineering at a quarter less) at a skill of at least a half.
//
// A capture: an armed APC fires on a building until it's down to 80 % of
// its hit points, drives up to it and sends its away team in; 16.8 s
// later the building is held by the APC's side (its hit points full; its
// owner unchanged) for as long as the APC keeps that order.
//
// The mission's watches on these (its raids) turn them into the scripts'
// events, by the group's label, the target's names and the sites near it:
// scan-started, scanned and scan-lost <kind> <name> (kind science,
// medical, engineering, strong-science, ...); scanned-nothing <kind> for
// a scan no handler wants; entered, captured, capture-lost and vacated
// <name>, and held <name> <seconds> each second it's held.
//
// The other sides' watches run too (exe.md#rival-watches). The computer's
// sides never capture or scan actively, so theirs succeed only when the
// group is destroyed (by anyone) or their own buildings and vehicles mark
// its block scanned: captured-by <race> <name> (a capture or destroy
// watch) or scanned-by <race> <name>. One side's success tells the others'
// watches on the group: the player's, if under way, is beaten <name>;
// otherwise its later success is futile <name> instead. And captured
// <name> <percent>: the share of a colony the player's teams hold (small
// buildings 1, large 3, of the colony's size at the start).
class FieldWork {
public:
    // A savestate's part (docs/savestates.md): what play changes, saved or
    // loaded; the rest is made again as the mission starts.
    void syncState(save::Serializer& s);
    enum Kind { Science, Medical, Engineering };

    FieldWork(std::string_view mission, int playerSlot, World& world,
              std::map<std::string, std::pair<int, int>> sites = {});

    static bool canScan(const Unit& u);    // ground scouts, science vehicles
    static bool canCapture(const Unit& u); // armed APCs
    static bool canCarry(const Unit& u);   // officers: science vehicles, APCs

    // Orders (false if the unit or target won't do): a scan of a target
    // that isn't the player's own, a capture of another side's building.
    bool scan(int unit, int target, Kind kind);
    bool capture(int unit, int target);
    // Another order for the unit: a scan stops, a capture ends (the
    // building let go, its hit points as they were).
    void cancel(int unit);

    // The side's officers, and where each is: in a building or vehicle
    // (by its world id; 0 in orbit, before they're placed or with no hub
    // to go to; -1 dead).
    struct OfficerAt {
        const Officer* officer;
        int in;
        float skill; // now (training raises it)
    };
    const std::vector<OfficerAt>& officers() const { return officers_; }
    // Places the officers in the buildings they start in; one whose
    // building isn't there goes where the officer before went (the first
    // to the hub); call once the colony's buildings are in the world.
    void placeOfficers();
    // Takes aboard the officers of the nearest of the player's buildings
    // in reach (the game's 1414 units, from its walls), up to 16; or puts
    // them all down into it. Returns how many moved.
    int board(int vehicle);
    int disembark(int vehicle);
    // The officer chosen on the STAFF panel moved into one of the
    // colony's buildings, or a vehicle that carries officers (16 at most);
    // false if it can't take them.
    bool moveOfficer(size_t officer, int into);
    // The building or vehicle was destroyed (razed too): the officers
    // still in it die with it (exe.md#crew-and-efficiency), and can't be
    // moved again.
    void destroyed(int unit);
    std::vector<const Officer*> aboard(int vehicle) const;
    bool strong(int unit, Kind kind) const;
    // The best skill at a speciality among the officers in a building or
    // vehicle (0 none); a command officer counts for engineering (1) and
    // security (2) at a quarter less, at least a quarter.
    float skill(int unit, int speciality) const;

    // Officer training (exe.md#research): a course for each officer the
    // mission lets train who isn't yet at the top (1.0), made at the
    // start. The player starts one; the side's 1000 training points a tick
    // (its own, not research's) are shared out in whole points between the
    // courses running, 150000 each. A finished course raises the officer a
    // band (0.25, to 1.0 at most); it isn't offered again.
    struct Course {
        size_t officer; // in officers()
        int done = 0;
        bool running = false, finished = false;
        friend void syncState(save::Serializer& s, Course& v) { s(v.officer)(v.done)(v.running)(v.finished); }
    };
    static constexpr int kCoursePoints = 150000, kTrainingPerTick = 1000;
    const std::vector<Course>& courses() const { return courses_; }
    bool train(size_t course);

    // The building's holder (a side slot), or -1.
    int heldBy(int building) const;
    // What a unit is doing, for its panel: "SCANNING", "CAPTURING", ...;
    // empty if none of these.
    std::string status(int unit) const;
    // Scans being made now, for drawing: scanner, target.
    std::vector<std::pair<int, int>> scanning() const;

    // Moves time on (the game's 20 ticks a second). handled: whether the
    // script waits for an event (for the "nothing found" reply).
    std::vector<ScriptEvent> advance(double seconds, const std::function<bool(const ScriptEvent&)>& handled);

    static const char* kindName(Kind kind, bool strong); // "science", "strong-medical"
    // The fog, for the watches on a group's block being scanned (the kind
    // "cell": any scanner whose fog class marks it).
    void setFog(const Fog* fog) { fog_ = fog; }

private:
    struct ScanOrder {
        int target;
        Kind kind = Science;
        int ticks = 0; // in range, scanning, in a row
        friend void syncState(save::Serializer& s, ScanOrder& v) {
            s(v.target);
            save::syncEnum(s, v.kind, Engineering);
            s(v.ticks);
        }
    };
    struct CaptureOrder {
        int target;
        int state = 0; // 0 weaken it, 1 drive up, 2 the team going in, 3 held
        int timer = 0;
        float savedHp = 0;
        friend void syncState(save::Serializer& s, CaptureOrder& v) { s(v.target)(v.state)(v.timer)(v.savedHp); }
    };
    struct Hold {
        int slot;
        int apc;
        double seconds = 0;
        friend void syncState(save::Serializer& s, Hold& v) { s(v.slot)(v.apc)(v.seconds); }
    };
    // A raid's progress: how long its condition has held, and whether it
    // has started (its start sent) or succeeded.
    struct Watch {
        const Raid* raid = nullptr;
        // For a scan no raid covers: the target and the condition.
        int target = 0;
        int condition = 0;
        int start = 0, hold = 0; // its own times (script ticks), without a raid
        int ticks = 0;
        bool started = false, done = false;
        int side = 0;           // whose watch: the player's, or a rival's
        bool futile = false;    // another side got there first: a success now counts for nothing
        bool seen = false;      // its group has been on the map (for "all destroyed")
        std::vector<std::string> names;
    };
    void tick(std::vector<ScriptEvent>& out, const std::function<bool(const ScriptEvent&)>& handled);
    void release(int building);
    std::vector<std::string> namesOf(const Unit& u) const;
    float reach(const Unit& scanner, const Unit& target) const;
    bool conditionMet(const Watch& w) const;
    void succeeded(const Watch& w, std::vector<ScriptEvent>& out);
    void shares(std::vector<ScriptEvent>& out);

    std::string mission_;
    int player_;
    World& world_;
    std::map<std::string, std::pair<int, int>> sites_;
    std::map<int, ScanOrder> scans_;
    std::map<int, CaptureOrder> captures_;
    std::map<int, Hold> holds_;
    std::vector<OfficerAt> officers_;
    std::vector<Course> courses_;
    // Per target: the scan conditions (4-9) it met this tick and the last.
    std::map<int, std::array<int, 10>> scanned_;
    std::map<int, int> session_; // per target, its scan's ticks in a row (the nothing reply)
    std::vector<Watch> watches_;
    std::vector<ScriptEvent> pending_;
    std::map<int, std::vector<std::string>> heldNames_;
    std::map<int, char> races_;           // each slot's race
    std::map<int, int> colonyBase_;       // per slot: its colony's size at the start (small + 3 x large)
    std::map<std::string, int> shared_;   // per label: the share of its side's colony the player holds, last sent
    double clock_ = 0;
    int tick_ = 0;
    const Fog* fog_ = nullptr;
};

// An officer's title, by race ('F', 'K', 'R') and the mission's rank
// number (1-8; the Federation's Ens. to Adm., 8 a doctor).
const char* rankTitle(char race, int rank);
// A speciality's name (1 ENGINEERING ... 9 PERSONNEL).
const char* specialityName(int speciality);
// The word for a skill, by its quarter: UNTRAINED, BASIC, SKILLED,
// HIGHLY SKILLED, EXPERT (1.0).
const char* skillWord(float skill);

} // namespace reluva::newworlds
