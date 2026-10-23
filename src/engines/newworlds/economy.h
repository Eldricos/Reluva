// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#pragma once

#include "engines/newworlds/mission_script.h"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace reluva::newworlds {

// A colony's economy while playing: its stock, buildings going up, power,
// mines and the resource processor. The rules and figures are the game's
// own, as studied in its executable (docs/formats/newworlds/exe.md): a
// simulation of 20 ticks a second; costs and prerequisites per side; power
// buffers charged from a shared supply; mines working the ground round them
// and a processor refining each ore into one material. Where the
// executable's figure wasn't found (construction speed, how fast the bees
// carry ore) the training missions' timings stand in (play-vm-tutorials.md).

// The colony's six materials, in the NEW WORLDS panel's order.
enum Material { Dilithium, Polycomp, Thermoconc, Transalumin, Nitrium, Duranium, kMaterials };
using Stock = std::array<int, kMaterials>;
const char* materialName(int m); // "DILITHIUM", ...
// The stock counts dilithium to trans al in thousands of the game's units
// and nitrium and duranium in hundreds (so a tonne of ore raises nitrium
// ten times as much as polycomp).
int unitsPerMaterial(int m);

// The six raw ores, by deposit kind 1-6 (gmf.md#resource-deposits); the
// processor makes ore kind k into material k - 1.
constexpr int kOres = 6;
const char* oreName(int kind); // "dilithium", "talgonite", ... (kind 1-6)

constexpr double kTicksPerSecond = 20;

struct BuildingType {
    std::string id;    // the mission scripts' name: "construction-yard"
    std::string panel; // the selection panel's: "CONSTRUCTION YARD"
    std::array<Stock, 3> cost{}; // per side (Federation, Klingon, Romulan), in stock units
    // What must stand first, per side: buildings and upgrades by id.
    std::array<std::vector<std::string>, 3> needs;
    std::string upgradeOf;  // an upgrade: the building it's fitted to (no placing)
    bool onDeposit = false; // a mining station: stands on a deposit
    bool orderable = true;  // offered by the build menu
    // Power, in the game's units a tick: a generator's output at full
    // efficiency (per side), or a consumer's buffer (capacity, intake a
    // tick) and its standing use a tick.
    std::array<float, 3> output{};
    float bufferCap = 0, intake = 0, demand = 0;
    int capacity = 0;         // colony capacity it adds once built
    double buildSeconds = 40; // order to finished
    const Stock& costFor(char side) const;
    const std::vector<std::string>& needsFor(char side) const;
};
const std::vector<BuildingType>& buildingTypes();
const BuildingType* buildingType(std::string_view id);

// The ground the colony stands on, by block: its deposit kind (0 none) and
// the amount its map cell holds (50-300; each unit a mining take of a
// tonne).
struct ColonyMap {
    std::function<int(int bx, int by)> depositKind;
    std::function<int(int bx, int by)> depositAmount;
};

struct ColonyStructure {
    int id = 0;
    std::string type;
    float x = 0, y = 0;  // its middle, world units
    float progress = 1;  // 0 when ordered, 1 built
    int blockX = -1, blockY = -1; // a mine's block
    bool exhausted = false;
    float stored = 0;    // its power buffer
};

class Economy {
public:
    Economy(char side, Stock start, int capacity, ColonyMap map);

    // The stock as the panel shows it.
    Stock stock() const;
    // COLONY SIZE: the colony's buildings and vehicles, and the most it
    // may have (50 at the start, raised by housing, never past 90).
    int colonySize() const;
    int capacity() const { return capacity_; }
    void setVehicles(int n) { vehicles_ = n; }
    // Power: what generators give a tick, what buildings ask a tick, the
    // share granted (DEMAND MET), and the reserve (0-1).
    float powerSupply() const;
    float powerDemand() const { return requested_; }
    float demandMet() const { return met_; }
    float reserve() const { return float(reserve_ / kReserveCap); }
    const std::vector<ColonyStructure>& structures() const { return structures_; }
    // Raw ore waiting at the processor, in tonnes, by ore kind - 1.
    std::array<float, kOres> oreStock() const;

    // A building already standing (the starting colony).
    int addBuilt(const std::string& type, float x, float y);
    // Whether the menu offers a type now (what it needs stands; an upgrade
    // not fitted yet), and whether the stock can pay for it.
    bool available(const BuildingType& t) const;
    bool affordable(const BuildingType& t) const;
    // Orders a building at a spot (its middle; for an upgrade, where its
    // building stands). The cost is taken at once; without enough
    // materials nothing is spent, 0 is returned and the colony is told
    // (short-resources). tx, ty: the tile under its middle.
    int order(const std::string& type, float x, float y, int tx, int ty);
    bool has(const std::string& type) const; // built
    // A building destroyed: it no longer counts, works or draws power.
    void remove(int id);

    // Moves time on: construction, power, mining and refining. Returns what
    // happened, as the mission scripts' events (built <type>, power-low,
    // short-resources, mine-exhausted at its position).
    std::vector<ScriptEvent> advance(double seconds);

private:
    static constexpr double kReserveCap = 640000;
    void tick(std::vector<ScriptEvent>& out);
    int sideIndex() const;
    bool mineTake(ColonyStructure& mine); // one mining take; false when dry

    char side_;
    std::array<int64_t, kMaterials> units_{}; // the stock in the game's units
    int capacity_, vehicles_ = 0;
    ColonyMap map_;
    std::vector<ColonyStructure> structures_;
    std::array<int64_t, kOres> ore_{}; // raw ore at the processor, game units
    std::map<std::pair<int, int>, int> dug_; // blocks' amounts taken so far
    std::vector<std::pair<int, double>> mineCarry_; // per mine id: tonnes owed by the bees
    int oreCycle_ = 0, refineSlot_ = 0;
    double tickClock_ = 0, clock_ = 0, lastPowerWarning_ = -1e9;
    double reserve_ = kReserveCap;
    float requested_ = 0, met_ = 1;
    std::vector<ScriptEvent> pendingShort_;
    int nextId_ = 1;
};

} // namespace reluva::newworlds
