// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace reluva::newworlds {

// What the game's executable says about every side in a mission, not
// only the player's (docs/formats/newworlds/exe.md#sides-and-combat): the
// seven side slots (0 Federation, 1 Klingon, 2 Romulan, 3-5 the planet's
// natives and rivals), their stances towards each other, what each places
// and the orders its AI carries out, and the object types' combat figures.

// A side's race: 'F', 'K', 'R'; the natives 'T' Taubat, 'M' Metar,
// 'H' Hubrin, 'O' Orion, 'E' rebel Taubat (K2), 'V' rival Romulans (R1).
const char* raceName(char race); // "federation", ..., "enemy-taubat", "rival"

enum class WeaponKind : uint8_t {
    None,
    Burst,     // a magazine of shots (Federation, Romulan, most natives)
    Single,    // one heavy shot (Klingon disruptors, Hubrin)
    Artillery, // a lobbed shell
};

struct Weapon {
    WeaponKind kind = WeaponKind::None;
    int damage = 0;    // a shot's, at the crew's lowest skill
    float range = 0;   // world units
    int reload = 0;    // ticks (20 a second) between bursts or shots
    int magazine = 0;  // shots a burst
    int guns = 0;      // weapons the type carries
};

struct CombatType {
    int id;
    const char* name;  // as the scripts count it: "taubat-hangar", "phaser-tank", "colony-hub"
    const char* model; // "geo/bad/tw.geo"; empty if none is known
    char race;
    bool building;     // a building or turret: it doesn't move
    int hp;
    float scale;       // the game's model scale
    float pace;        // path steps a tick (0: it doesn't move)
    Weapon weapon;
    float scan;        // scan range (0: none)
};
const CombatType* combatType(int id);

struct SideInfo {
    const char* mission;
    int slot;
    char race;
    std::array<int8_t, 7> posture; // towards each slot: 1 self or ally, 2 neutral, 3 hostile
};
const SideInfo* sideInfo(std::string_view mission, int slot);
std::vector<const SideInfo*> slotsOf(std::string_view mission);

struct SidePlacement {
    const char* mission;
    int slot;
    int type;
    int a, b;          // map position: eighths of a block, row first
    const char* label; // the game's name for it, as the scripts write it ("taubat-colony")
    int forWhom;       // 3 always; 1 only if the side is a player's, 2 only if not
    const char* model; // in place of the type's (empty: the type's)
};
std::vector<const SidePlacement*> sidePlacements(std::string_view mission, int slot);

// An order a side's AI carries out. Task: 1 patrol (round the waypoints),
// 4 guard (the first waypoint), 5-8 assault (along the waypoints, fighting
// what it meets), 9 or 3 build (a building of the type after the delay).
// Named orders wait for the script (ai <side> <name>); unnamed ones run
// from the start.
struct NpcOrder {
    const char* mission;
    int slot;
    const char* name;  // "initial-assault-one"; empty for none
    int task;
    int type;          // the units' type, or the building's
    int count;
    int delay;         // seconds, for a build
    int ways;
    int way[4][2];     // map positions (eighths of a block, row first)
    // Where the game beams its units in (an attack group spawner); -1 if
    // it sends the side's own.
    int spawn[2];
};
std::vector<const NpcOrder*> npcOrders(std::string_view mission);

} // namespace reluva::newworlds
