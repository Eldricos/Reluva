// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/economy.h"

#include <algorithm>
#include <cmath>

namespace reluva::newworlds {

namespace {

// What the executable doesn't fix (exe.md#still-open), from the training
// missions' timings:
// - the bees carry about 2 t of ore a second from each mine (one mine fed
//   the processor some 1.1 t of its own ore and 0.2 t of each other a
//   second in C7);
constexpr double kMineTonnesPerSecond = 2;
// - warnings repeat once a minute while they hold (the game's own 60 s).
constexpr double kWarnEvery = 60;

// The game's rules (exe.md):
// - a mine digs the nearest ore within 3 blocks of its own, a tonne a
//   take, each take using one unit of the cell's amount;
constexpr int kMineReach = 3;
constexpr int64_t kTonne = 1000;
// - the processor turns to one of its 16 store slots a tick and refines up
//   to a quarter tonne of that ore into its material;
constexpr int kRefineSlots = 16;
constexpr int64_t kRefinePerVisit = 250;
// - the power warning comes when less than three quarters of the asked
//   power is granted;
constexpr float kPowerLow = 0.75f;
// - generators and the hub run at 1.2 efficiency when whole, powered and
//   staffed (the panel's 120 %);
constexpr float kGeneratorEfficiency = 1.2f;
// - the colony's capacity never passes 90.
constexpr int kCapacityCap = 90;

constexpr int sideOf(char side) { return side == 'K' ? 1 : side == 'R' ? 2 : 0; }

} // namespace

const char* materialName(int m) {
    static const char* const kNames[] = {"DILITHIUM", "POLYCOMP", "THERMOCONC", "TRANSALUMIN", "NITRIUM", "DURANIUM"};
    return m >= 0 && m < kMaterials ? kNames[m] : "";
}

int unitsPerMaterial(int m) { return m >= Nitrium ? 100 : 1000; }

const char* oreName(int kind) {
    static const char* const kNames[] = {"dilithium", "talgonite", "silicon", "kelbonite", "magnesite", "dolamide"};
    return kind >= 1 && kind <= kOres ? kNames[kind - 1] : "";
}

const Stock& BuildingType::costFor(char side) const { return cost[size_t(sideOf(side))]; }
const std::vector<std::string>& BuildingType::needsFor(char side) const { return needs[size_t(sideOf(side))]; }

const std::vector<BuildingType>& buildingTypes() {
    // Costs and prerequisites per side as the executable registers them
    // (exe.md#costs-and-prerequisites; stock units: dilithium, polycomp,
    // thermoconc, trans al, nitrium, duranium). Power figures are the
    // Federation's (the other sides' differ a little; exe.md#buildings).
    // Build times as the training missions timed them, order to finished
    // (play-vm-tutorials.md#build-times): the game builds with its bees,
    // at a pace the executable doesn't fix as a number; others 40 s.
    static const std::vector<BuildingType> kTypes = [] {
        std::vector<BuildingType> t;
        auto add = [&](const char* id, const char* panel, std::array<Stock, 3> cost,
                       std::array<std::vector<std::string>, 3> needs, double seconds) -> BuildingType& {
            BuildingType b;
            b.id = id, b.panel = panel, b.cost = cost, b.needs = std::move(needs), b.buildSeconds = seconds;
            t.push_back(std::move(b));
            return t.back();
        };
        auto consumer = [](BuildingType& b, float cap, float intake, float demand) {
            b.bufferCap = cap, b.intake = intake, b.demand = demand;
        };
        const std::vector<std::string> yard{"construction-yard"};
        BuildingType& hub = add("colony-hub", "COLONY HUB", {}, {}, 0);
        hub.orderable = false, hub.output = {500, 500, 500};
        consumer(add("construction-yard", "CONSTRUCTION YARD",
                     {Stock{0, 12, 89, 5, 50, 0}, Stock{0, 12, 89, 50, 30, 0}, Stock{0, 12, 89, 3, 500, 0}},
                     {{{}, {}, {"colony-hub"}}}, 45),
                 34000, 550, 85);
        consumer(add("vehicle-yard", "VEHICLE YARD",
                     {Stock{0, 10, 90, 5, 0, 0}, Stock{0, 10, 90, 5, 0, 0}, Stock{0, 10, 90, 2, 0, 0}},
                     {yard, yard, yard}, 65),
                 34000, 550, 60);
        add("power-generator", "POWER GENERATOR",
            {Stock{35, 1, 35, 0, 800, 0}, Stock{35, 1, 30, 0, 850, 0}, Stock{35, 1, 30, 0, 850, 0}}, {yard, yard, yard},
            30)
            .output = {500, 500, 1000};
        add("advanced-generator", "ADVANCED GENERATOR",
            {Stock{50, 2, 40, 0, 850, 0}, Stock{50, 2, 37, 0, 870, 0}, Stock{50, 2, 37, 0, 870, 0}},
            {{{"construction-yard-advanced", "colony-hub-upgrade"},
              {"construction-yard-advanced"},
              {"construction-yard-upgrade", "colony-hub-advanced"}}},
            40)
            .output = {1500, 1500, 3000};
        consumer(add("resource-processor", "RESOURCE PROCESSOR",
                     {Stock{0, 4, 30, 2, 0, 0}, Stock{0, 4, 30, 3, 0, 0}, Stock{0, 4, 30, 2, 0, 0}}, {yard, yard, yard},
                     25),
                 24000, 300, 60);
        BuildingType& mine = add("mining-station", "MINING STATION",
                                 {Stock{0, 14, 80, 10, 0, 140}, Stock{0, 14, 80, 50, 0, 140}, Stock{0, 14, 80, 4, 0, 140}},
                                 {{{"resource-processor"}, {"resource-processor", "construction-yard"}, {"resource-processor"}}},
                                 60);
        mine.onDeposit = true;
        consumer(mine, 24000, 550, 110);
        consumer(add("science-station", "SCIENCE STATION",
                     {Stock{0, 4, 53, 17, 120, 0}, Stock{0, 4, 53, 32, 120, 0}, Stock{0, 4, 53, 22, 120, 0}},
                     {yard, yard, yard}, 75),
                 34000, 550, 440);
        const std::vector<std::string> improvedYard{"construction-yard-upgrade"};
        consumer(add("transporter-pad", "TRANSPORTER PAD",
                     {Stock{0, 6, 32, 2, 0, 0}, Stock{0, 6, 32, 2, 0, 0}, Stock{0, 6, 32, 2, 0, 0}},
                     {improvedYard, improvedYard, improvedYard}, 40),
                 44000, 300, 60);
        consumer(add("sickbay", "SICKBAY",
                     {Stock{0, 8, 48, 20, 0, 0}, Stock{0, 8, 48, 26, 0, 0}, Stock{0, 8, 48, 36, 0, 0}}, {yard, yard, yard},
                     40),
                 44000, 550, 440);
        const std::vector<std::string> housing{"construction-yard-upgrade", "transporter-pad"};
        BuildingType& accommodation = add("accommodation", "ACCOMMODATION",
                                          {Stock{0, 20, 50, 36, 0, 0}, Stock{0, 16, 60, 40, 0, 0}, Stock{0, 16, 60, 42, 0, 0}},
                                          {housing, housing, housing}, 40);
        accommodation.capacity = 5;
        consumer(accommodation, 34000, 300, 240);
        BuildingType& farm = add("farm", "FARM",
                                 {Stock{0, 18, 75, 40, 0, 0}, Stock{0, 18, 75, 38, 0, 0}, Stock{0, 18, 75, 36, 0, 0}},
                                 {{{"construction-yard", "accommodation"},
                                   {"construction-yard-upgrade", "transporter-pad", "accommodation"},
                                   {"construction-yard-upgrade", "transporter-pad", "accommodation"}}},
                                 40);
        farm.capacity = 10;
        consumer(farm, 34000, 425, 340);
        consumer(add("security-center", "SECURITY CENTER",
                     {Stock{0, 10, 100, 10, 1200, 0}, Stock{0, 10, 100, 100, 120, 0}, Stock{0, 10, 100, 27, 1200, 0}},
                     {{{"construction-yard-upgrade", "colony-hub-upgrade"},
                       {"construction-yard-upgrade"},
                       {"construction-yard-upgrade", "colony-hub-upgrade"}}},
                     83),
                 34000, 425, 340);
        // Upgrades: fitted to their building, no placing. An advanced one
        // follows the improved one.
        auto upgrade = [&](const char* id, const char* panel, const char* of, Stock f, Stock k, Stock r,
                           const char* after) {
            BuildingType& u = add(id, panel, {f, k, r}, {}, 40);
            u.upgradeOf = of;
            for (auto& n : u.needs) {
                n.push_back(of);
                if (after)
                    n.push_back(after);
            }
        };
        upgrade("colony-hub-upgrade", "IMPROVED HUB", "colony-hub", Stock{0, 4, 40, 0, 0, 1000},
                Stock{0, 4, 40, 0, 0, 1000}, Stock{0, 4, 40, 0, 0, 1000}, nullptr);
        upgrade("colony-hub-advanced", "ADVANCED HUB", "colony-hub", Stock{0, 50, 30, 20, 0, 0},
                Stock{0, 50, 30, 20, 0, 0}, Stock{0, 50, 30, 20, 0, 0}, "colony-hub-upgrade");
        upgrade("construction-yard-upgrade", "IMPROVED YARD", "construction-yard", Stock{0, 2, 30, 4, 0, 0},
                Stock{0, 2, 30, 2, 0, 0}, Stock{0, 2, 30, 2, 0, 0}, nullptr);
        upgrade("construction-yard-advanced", "ADVANCED YARD", "construction-yard", Stock{0, 20, 10, 6, 0, 0},
                Stock{0, 20, 10, 2, 0, 0}, Stock{0, 2, 30, 2, 0, 0}, "construction-yard-upgrade");
        upgrade("vehicle-yard-upgrade", "EXTENDED YARD", "vehicle-yard", Stock{0, 1, 50, 3, 0, 0},
                Stock{0, 1, 50, 3, 0, 0}, Stock{0, 1, 50, 3, 0, 0}, nullptr);
        upgrade("vehicle-yard-advanced", "SPECIALIST YARD", "vehicle-yard", Stock{0, 1, 50, 3, 0, 0},
                Stock{0, 1, 50, 3, 0, 0}, Stock{0, 1, 50, 3, 0, 0}, "vehicle-yard-upgrade");
        return t;
    }();
    return kTypes;
}

const BuildingType* buildingType(std::string_view id) {
    for (const BuildingType& t : buildingTypes())
        if (t.id == id)
            return &t;
    return nullptr;
}

Economy::Economy(char side, Stock start, int capacity, ColonyMap map)
    : side_(side), capacity_(std::min(capacity, kCapacityCap)), map_(std::move(map)) {
    for (int m = 0; m < kMaterials; ++m)
        units_[size_t(m)] = int64_t(start[size_t(m)]) * unitsPerMaterial(m);
}

int Economy::sideIndex() const { return sideOf(side_); }

Stock Economy::stock() const {
    Stock s{};
    for (int m = 0; m < kMaterials; ++m)
        s[size_t(m)] = int(units_[size_t(m)] / unitsPerMaterial(m));
    return s;
}

std::array<float, kOres> Economy::oreStock() const {
    std::array<float, kOres> t{};
    for (int k = 0; k < kOres; ++k)
        t[size_t(k)] = float(ore_[size_t(k)]) / float(kTonne);
    return t;
}

int Economy::colonySize() const {
    int n = vehicles_;
    for (const ColonyStructure& s : structures_)
        if (const BuildingType* t = buildingType(s.type); t && t->upgradeOf.empty())
            ++n;
    return n;
}

bool Economy::has(const std::string& type) const {
    return std::any_of(structures_.begin(), structures_.end(),
                       [&](const ColonyStructure& s) { return s.type == type && s.progress >= 1; });
}

bool Economy::available(const BuildingType& t) const {
    if (!t.orderable)
        return false;
    for (const std::string& n : t.needsFor(side_))
        if (!has(n))
            return false;
    // An upgrade once only.
    return t.upgradeOf.empty() || std::none_of(structures_.begin(), structures_.end(),
                                               [&](const ColonyStructure& s) { return s.type == t.id; });
}

bool Economy::affordable(const BuildingType& t) const {
    const Stock& c = t.costFor(side_);
    for (int m = 0; m < kMaterials; ++m)
        if (units_[size_t(m)] < int64_t(c[size_t(m)]) * unitsPerMaterial(m))
            return false;
    return true;
}

float Economy::powerSupply() const {
    float p = 0;
    for (const ColonyStructure& s : structures_)
        if (const BuildingType* t = buildingType(s.type); t && s.progress >= 1)
            p += t->output[size_t(sideIndex())] * kGeneratorEfficiency;
    return p;
}

int Economy::addBuilt(const std::string& type, float x, float y) {
    ColonyStructure s;
    s.id = nextId_++;
    s.type = type;
    s.x = x, s.y = y;
    if (const BuildingType* t = buildingType(type)) {
        s.stored = t->bufferCap;
        capacity_ = std::min(kCapacityCap, capacity_ + t->capacity);
    }
    structures_.push_back(s);
    return s.id;
}

int Economy::order(const std::string& type, float x, float y, int tx, int ty) {
    const BuildingType* t = buildingType(type);
    if (!t || !available(*t))
        return 0;
    if (!affordable(*t)) {
        pendingShort_.push_back({"short-resources", {}});
        return 0;
    }
    ColonyStructure s;
    s.id = nextId_++;
    s.type = type;
    s.x = x, s.y = y;
    s.progress = 0;
    if (t->onDeposit) {
        s.blockX = tx / 2, s.blockY = ty / 2;
        if (!map_.depositKind || map_.depositKind(s.blockX, s.blockY) == 0)
            return 0;
    }
    const Stock& c = t->costFor(side_);
    for (int m = 0; m < kMaterials; ++m)
        units_[size_t(m)] -= int64_t(c[size_t(m)]) * unitsPerMaterial(m);
    structures_.push_back(s);
    return s.id;
}

bool Economy::mineTake(ColonyStructure& mine) {
    // The nearest ore within reach, ring by ring from its own block.
    for (int r = 0; r <= kMineReach; ++r)
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != r)
                    continue;
                const int bx = mine.blockX + dx, by = mine.blockY + dy;
                const int kind = map_.depositKind ? map_.depositKind(bx, by) : 0;
                if (kind == 0)
                    continue;
                int& dug = dug_[{bx, by}];
                const int held = map_.depositAmount ? map_.depositAmount(bx, by) : 0;
                if (dug >= held)
                    continue;
                ++dug;
                // Every eight takes (counted for the whole colony) give two
                // tonnes of the cell's own ore and one of each kind.
                const int step = oreCycle_++ % 8;
                const int got = step < 2 ? kind : step - 1;
                ore_[size_t(got - 1)] += kTonne;
                return true;
            }
    return false;
}

void Economy::tick(std::vector<ScriptEvent>& out) {
    clock_ += 1 / kTicksPerSecond;
    // Construction, at each type's pace.
    for (ColonyStructure& s : structures_) {
        if (s.progress >= 1)
            continue;
        const BuildingType* t = buildingType(s.type);
        s.progress = float(std::min(1.0, s.progress + 1 / (std::max(1.0, t->buildSeconds) * kTicksPerSecond)));
        if (s.progress >= 1) {
            s.stored = t->bufferCap;
            capacity_ = std::min(kCapacityCap, capacity_ + t->capacity);
            out.push_back({"built", {s.type}});
        }
    }
    // Power: each built consumer asks to fill its buffer, at most its
    // intake a tick; the supply (with the reserve behind it) is shared out
    // in proportion; then each uses its standing demand from its buffer.
    float asked = 0;
    for (ColonyStructure& s : structures_)
        if (const BuildingType* t = buildingType(s.type); t && s.progress >= 1 && t->intake > 0)
            asked += std::min(t->intake, t->bufferCap - s.stored + t->demand);
    const double supply = powerSupply();
    double give = supply;
    if (asked > supply) {
        const double draw = std::min(reserve_, double(asked) - supply);
        reserve_ -= draw;
        give += draw;
    } else {
        reserve_ = std::min(kReserveCap, reserve_ + (supply - asked));
    }
    requested_ = asked;
    met_ = asked > 0 ? float(std::min(1.0, give / asked)) : 1;
    for (ColonyStructure& s : structures_)
        if (const BuildingType* t = buildingType(s.type); t && s.progress >= 1 && t->intake > 0) {
            const float want = std::min(t->intake, t->bufferCap - s.stored + t->demand);
            s.stored = std::clamp(s.stored + want * met_ - t->demand, 0.0f, t->bufferCap);
        }
    if (met_ < kPowerLow && clock_ - lastPowerWarning_ >= kWarnEvery) {
        out.push_back({"power-low", {}});
        lastPowerWarning_ = clock_;
    }
    // Mining, for as long as there's a processor to take the ore: the bees
    // bring it at their pace.
    const bool processor = has("resource-processor");
    for (ColonyStructure& s : structures_) {
        if (s.type != "mining-station" || s.progress < 1 || s.exhausted || !processor)
            continue;
        auto it = std::find_if(mineCarry_.begin(), mineCarry_.end(), [&](const auto& c) { return c.first == s.id; });
        if (it == mineCarry_.end()) {
            mineCarry_.push_back({s.id, 0});
            it = mineCarry_.end() - 1;
        }
        it->second += kMineTonnesPerSecond / kTicksPerSecond;
        while (it->second >= 1) {
            it->second -= 1;
            if (!mineTake(s)) {
                s.exhausted = true;
                out.push_back({"mine-exhausted", {}, s.blockY * 8 + 4, s.blockX * 8 + 4});
                break;
            }
        }
    }
    // Refining: one store slot a tick (ores 1-6 in the first six), a
    // quarter tonne at a time, one for one into the ore's material.
    if (processor) {
        const int slot = refineSlot_++ % kRefineSlots;
        if (slot < kOres) {
            const int64_t use = std::min(kRefinePerVisit, ore_[size_t(slot)]);
            ore_[size_t(slot)] -= use;
            units_[size_t(slot)] += use;
        }
    }
}

std::vector<ScriptEvent> Economy::advance(double seconds) {
    std::vector<ScriptEvent> out = std::move(pendingShort_);
    pendingShort_.clear();
    tickClock_ += seconds * kTicksPerSecond;
    while (tickClock_ >= 1) {
        tickClock_ -= 1;
        tick(out);
    }
    return out;
}

void Economy::remove(int id) {
    structures_.erase(std::remove_if(structures_.begin(), structures_.end(),
                                     [&](const ColonyStructure& c) { return c.id == id; }),
                      structures_.end());
}

} // namespace reluva::newworlds
