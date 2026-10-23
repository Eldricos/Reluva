// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/ai_data.h"

#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace reluva::newworlds {

// The game being played on a map: its units, where they can go, and the
// player's selection and battle groups. Positions are in world units, x
// east and y north as the map viewer draws the map (a tile is 8192, a
// block 16384; a map position <a:b> is y = a / 8 blocks, x = b / 8). Only
// what recordings of play show is modelled (docs/formats/newworlds/play.md
// and play-tutorials.md); figures not yet measured in the game are named
// placeholders (vm-checklist.md).

// The ground units drive over: per tile, its movement class (the tile
// flags' bits 16-17, one value per ground model: 0 for holes, water_01
// and rock formations, which nothing in the recordings crosses; gmf.md),
// and the ground's height anywhere.
struct Terrain {
    int width = 0, height = 0; // in tiles
    std::vector<int> moveClass; // width x height, row by row from the south
    std::function<float(float x, float y)> heightAt;
};

// A tile can be entered if its class isn't 0 and the ground doesn't climb
// more than half this in a quarter tile on the way from its neighbour (a
// level is about 1840 units: a cliff between levels is a wall, a ramp
// isn't; F1's ramps up to the Federation colony climb up to 1800 a
// quarter tile).
constexpr float kMaxStep = 3600;
constexpr float kTile = 8192;

// What a unit is. speed: world units a second (placeholder until measured).
struct UnitKind {
    std::string name;  // as the selection panel names it ("GROUND SCOUT")
    std::string model; // "geo/fed/ffs.geo"
    float speed = 0;
};

// The side's ground scout: the four vehicles each recorded colony starts
// with are of one kind (F1, R1; which kind isn't known).
UnitKind groundScout(char side);

// Something on the map that can fight or be fought: a vehicle, or a
// building or turret (which doesn't move).
struct Unit {
    int id = 0;
    char side = 'F';   // its race (ai_data.h)
    int slot = 0;      // its side's slot, 0-6
    UnitKind kind;
    int type = 0;      // the game's type id (0: not known)
    std::string name;  // as the scripts name it ("taubat-hangar")
    std::string label; // the mission's name for it ("taubat-colony"), if any
    bool building = false;
    float x = 0, y = 0;
    float heading = 0; // radians anticlockwise from east
    std::vector<std::pair<float, float>> path; // waypoints still to reach
    bool selected = false;
    bool moving() const { return !path.empty(); }
    // Combat (exe.md#sides-and-combat): hit points, its weapon, what it's shooting at
    // (by id; 0 none) and whether it was told to (then it gives chase).
    float hp = 1, maxHp = 1;
    Weapon weapon;
    int target = 0;
    bool ordered = false;
    bool ai = false;     // run by a side's AI: it stops to fight what it meets
    int cooldown = 0, burstLeft = 0, burstGap = 0; // ticks
    bool firing = false; // stopped to shoot
    float turretYaw = 0; // from straight ahead, anticlockwise
    int lastAttacker = 0;
};

// What combat did in a step: a shot (from, to, its ends) or something
// destroyed (a copy of it as it was).
struct WorldEvent {
    enum Kind { Shot, Destroyed } kind = Shot;
    int from = 0, to = 0;
    int fromSlot = 0, toSlot = 0;
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    WeaponKind weapon = WeaponKind::None;
    Unit what;
};

class World {
public:
    explicit World(Terrain terrain);

    const Terrain& terrain() const { return terrain_; }
    bool passable(int tx, int ty) const;
    // Whether a unit can step from one tile to a neighbour (8 ways; a
    // diagonal needs both tiles beside it open).
    bool canStep(int tx, int ty, int dx, int dy) const;
    // A path of tile centres from one point to another (A*), the last one
    // the goal itself. The goal moves to the nearest open tile if it is
    // closed; if it can't be reached, the path goes as near as it can.
    std::vector<std::pair<float, float>> findPath(float x0, float y0, float x1, float y1) const;
    // The nearest spot a unit can drive away from: an open tile with at
    // least four ways out (not a pit, such as F1's frozen pools), where
    // no other vehicle stands if it has to look further.
    std::pair<float, float> openSpot(float x, float y) const;

    int addUnit(char side, const UnitKind& kind, float x, float y, float heading = 0);
    const std::vector<Unit>& units() const { return units_; }
    Unit* unit(int id);
    const Unit* unit(int id) const;
    void remove(int id);

    // Stances between side slots, each side's own towards each: 1 self or
    // ally, 2 neutral, 3 hostile. A side fires at those it's hostile to,
    // the player's units also at those hostile to the player; otherwise
    // only to fire back.
    int stance(int from, int to) const;
    void setStance(int from, int to, int posture);
    // Orders units to attack something: they give chase and fire.
    void attack(const std::vector<int>& ids, int target);
    // Units sent somewhere: they drop an ordered target.
    void moveTo(int id, float x, float y);

    // Selection, as the game's: a click selects one unit (or none), a box
    // the units inside it; right-click drops it.
    void select(const std::vector<int>& ids);
    void clearSelection();
    std::vector<int> selection() const;
    // Orders the selected units to a point: each finds its own path to a
    // spot round it, so they don't all pile onto one.
    void moveSelected(float x, float y);

    // Battle groups 1-9: Ctrl+N makes the selection group N, N selects it.
    void setGroup(int n);
    bool selectGroup(int n);
    // The middle of group N (or of the selection, for n = 0); false if empty.
    bool groupCentre(int n, float& x, float& y) const;

    // Moves time on; true if anything moved or fought. Combat runs at the
    // game's 20 ticks a second.
    bool advance(double seconds);
    // What combat did since last asked.
    std::vector<WorldEvent> takeEvents();

private:
    int tileOf(float v) const { return int(v / kTile); }
    void combatTick();
    void fire(Unit& u, Unit& t);
    // Whom a unit fires at unprovoked: those its side is hostile to (and,
    // for the player's own, those hostile to the player).
    bool hostile(const Unit& u, const Unit& v) const {
        return stance(u.slot, v.slot) == 3 || (!u.ai && stance(v.slot, u.slot) == 3);
    }
public:
    // Whom an AI side's units go for of themselves: those it's hostile to,
    // but a colony's AI (slots 0-2) only other colonies - the natives only
    // if they fire on it (the recordings show F1's Klingon attacks pass the
    // Taubat by, though the Klingons are hostile to every slot).
    bool prey(int from, int to) const { return stance(from, to) == 3 && (from > 2 || to <= 2); }

private:

    std::array<std::array<int, 7>, 7> stances_{};
    std::vector<WorldEvent> events_;
    double tickClock_ = 0;
    int tick_ = 0;

    Terrain terrain_;
    std::vector<Unit> units_;
    std::array<std::vector<int>, 10> groups_;
    int nextId_ = 1;
};

} // namespace reluva::newworlds
