// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/economy.h"

#include <algorithm>
#include <set>
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
// - the colony's capacity never passes 90.
constexpr int kCapacityCap = 90;
// Vehicle production (exe.md#vehicle-production): a job is its build value
// x 300 power units; each tick a running yard takes up to 300 from its
// buffer, times its efficiency; it starts above 0.35 and stops below 0.2.
// Then the vehicle rolls out: a placeholder from the recordings' scout,
// whose value is 0 yet takes about 8 s.
constexpr double kWorkPerValue = 300;
constexpr float kYardDraw = 300, kYardStart = 0.35f, kYardStop = 0.2f;
constexpr double kRollout = 8;
// How fast a buffer's averaged fill follows it, a tick (a placeholder: how
// the game keeps its average wasn't found).
constexpr float kFillAverage = 0.05f;

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
        // The science station's labs: add-ons bought like upgrades, each
        // one side's (exe.md#vehicle-production; the Romulans' 308 and 309
        // aren't named in the executable's tables). The training missions
        // timed a lab at 21-35 s.
        auto lab = [&](const char* id, const char* panel, std::array<bool, 3> sides, Stock f, Stock k, Stock r) {
            upgrade(id, panel, "science-station", f, k, r, nullptr);
            t.back().sides = sides;
            t.back().buildSeconds = 28;
        };
        const Stock fedLab{0, 10, 15, 7, 0, 0}, kliLab{0, 10, 15, 4, 0, 0}, romLab{0, 10, 15, 5, 0, 0};
        lab("phaser-coil-lab", "PHASER COIL LAB", {true, false, false}, fedLab, kliLab, romLab);
        lab("away-team-lab", "AWAY TEAM LAB", {true, false, false}, fedLab, kliLab, romLab);
        lab("sensor-lab", "SENSOR LAB", {true, true, false}, fedLab, kliLab, romLab);
        lab("energy-lab", "ENERGY LAB", {true, false, true}, fedLab, kliLab, romLab);
        lab("shield-buffer-lab", "SHIELD BUFFER LAB", {true, true, true}, fedLab, kliLab, romLab);
        lab("tracking-systems", "TRACKING SYSTEMS", {false, true, false}, fedLab, kliLab, romLab);
        lab("disruptor-matrix", "DISRUPTOR MATRIX", {false, true, false}, fedLab, kliLab, romLab);
        lab("special-weapons", "SPECIAL WEAPONS", {false, true, false}, fedLab, kliLab, romLab);
        lab("trooper-center", "TROOPER CENTER", {false, true, false}, fedLab, kliLab, romLab);
        lab("infiltration-lab", "INFILTRATION LAB", {false, false, true}, fedLab, kliLab, romLab);
        lab("lab-308", "LAB (308)", {false, false, true}, fedLab, kliLab, romLab);
        lab("lab-309", "LAB (309)", {false, false, true}, fedLab, kliLab, romLab);
        // The officers each wants, and whether its crew counts.
        for (BuildingType& b : t) {
            const std::string& id = b.upgradeOf.empty() ? b.id : b.upgradeOf;
            b.speciality = id == "construction-yard" || id == "vehicle-yard"            ? 1
                           : id == "security-center"                                    ? 2
                           : id == "sickbay"                                            ? 3
                           : id == "science-station"                                    ? 4
                           : id == "mining-station" || id == "resource-processor"       ? 6
                           : id == "colony-hub"                                         ? 7
                           : id == "storage-facility"                                   ? 8
                           : id == "accommodation"                                      ? 9
                                                                                        : 0;
            b.crewless = !(id == "construction-yard" || id == "vehicle-yard" || id == "security-center" ||
                           id == "sickbay" || id == "science-station" || id == "mining-station" ||
                           id == "resource-processor" || id == "storage-facility");
        }
        return t;
    }();
    return kTypes;
}

// Each side's vehicles as the executable registers them: costs (stock
// units), what they need, build values (exe.md#vehicle-production).
const std::vector<VehicleType>& vehicleTypes() {
    static const std::vector<VehicleType> kVehicles = [] {
        const std::string vy = "vehicle-yard", ext = "vehicle-yard-upgrade", spec = "vehicle-yard-advanced",
                          sec = "security-center", sci = "science-station", hub3 = "colony-hub-advanced",
                          dw = "defensive-weapon", da = "defensive-artillery";
        return std::vector<VehicleType>{
            {13, 'F', "CARGO BEE", Stock{1, 1, 0, 1, 0, 0}, {vy}, 128},
            {14, 'F', "CONSTRUCTION BEE", Stock{1, 1, 0, 1, 0, 0}, {vy}, 0},
            {12, 'F', "WORK BEE", Stock{0, 0, 0, 1, 0, 0}, {vy}, 0},
            {97, 'F', "GROUND SCOUT", Stock{3, 1, 0, 1, 0, 0}, {vy}, 0},
            {55, 'F', "TRANSPORT", Stock{5, 10, 0, 2, 0, 0}, {vy, "away-team-lab"}, 0},
            {56, 'F', "ARMED TRANSPORT", Stock{4, 3, 0, 1, 0, 30}, {vy}, 240},
            {62, 'F', "SCIENCE VEHICLE", Stock{8, 14, 0, 2, 0, 80}, {vy, "sensor-lab"}, 640},
            {8, 'F', "PHASER TANK", Stock{15, 18, 0, 2, 0, 100}, {ext, "phaser-coil-lab"}, 640},
            {47, 'F', "PHOTON ARTILLERY", Stock{10, 16, 0, 0, 100, 80}, {sec, ext}, 640},
            {9, 'F', "ADV. ARTILLERY", Stock{12, 18, 0, 0, 150, 90}, {vy, spec, sec, sci}, 640},
            {154, 'F', "MOBILE SHIELD", Stock{16, 20, 0, 2, 0, 160}, {spec, dw, da, "energy-lab"}, 640},
            {61, 'K', "CARGO BEE", Stock{1, 2, 0, 1, 0, 0}, {vy}, 0},
            {46, 'K', "CONSTRUCTION BEE", Stock{1, 2, 0, 1, 0, 0}, {vy}, 0},
            {11, 'K', "WORK BEE", Stock{0, 0, 0, 1, 0, 0}, {vy}, 0},
            {37, 'K', "GROUND SCOUT", Stock{3, 1, 0, 1, 0, 0}, {vy}, 128},
            {88, 'K', "TRANSPORT", Stock{5, 10, 0, 2, 0, 0}, {vy, "trooper-center"}, 240},
            {40, 'K', "ARMED TRANSPORT", Stock{4, 3, 0, 1, 0, 30}, {vy}, 240},
            {95, 'K', "SCIENCE VEHICLE", Stock{8, 14, 0, 2, 0, 80}, {vy, "sensor-lab"}, 640},
            {10, 'K', "DISRUPTOR TANK", Stock{15, 18, 0, 2, 0, 100}, {vy, "disruptor-matrix", ext}, 640},
            {36, 'K', "PHOTON ARTILLERY", Stock{10, 16, 0, 0, 100, 80}, {vy, sec, ext}, 640},
            {228, 'K', "DISRUPTOR BATTERY", Stock{18, 26, 0, 2, 0, 120}, {hub3, dw, da, "special-weapons"}, 640},
            {229, 'K', "CLOAKED DISRUPTOR TANK", Stock{20, 20, 0, 4, 0, 80}, {hub3, dw, da, "disruptor-matrix"}, 640},
            {90, 'R', "CARGO BEE", Stock{1, 2, 0, 1, 0, 0}, {vy}, 128},
            {91, 'R', "CONSTRUCTION BEE", Stock{1, 2, 0, 1, 0, 0}, {vy}, 0},
            {92, 'R', "WORK BEE", Stock{0, 0, 0, 1, 0, 0}, {vy}, 0},
            {98, 'R', "GROUND SCOUT", Stock{3, 1, 0, 1, 0, 0}, {vy}, 0},
            {89, 'R', "TRANSPORT", Stock{5, 10, 0, 2, 0, 0}, {vy, "infiltration-lab"}, 0},
            {64, 'R', "ARMED TRANSPORT", Stock{5, 5, 0, 1, 0, 30}, {vy}, 240},
            {63, 'R', "SCIENCE VEHICLE", Stock{8, 14, 0, 2, 0, 80}, {vy, sci, "energy-lab"}, 640},
            {148, 'R', "TANK", Stock{15, 18, 0, 2, 0, 100}, {vy, sci, "lab-308"}, 640},
            {87, 'R', "PHOTON ARTILLERY", Stock{10, 16, 0, 0, 100, 80}, {ext, sec}, 640},
            {35, 'R', "CLOAKED TANK", Stock{18, 18, 0, 2, 0, 120}, {ext, "lab-309"}, 640},
            {230, 'R', "CLOAKED APC", Stock{5, 7, 0, 1, 0, 40}, {hub3, dw, da, "energy-lab"}, 240},
        };
    }();
    return kVehicles;
}

const VehicleType* vehicleType(int type) {
    for (const VehicleType& v : vehicleTypes())
        if (v.type == type)
            return &v;
    return nullptr;
}

bool VehicleType::bee() const { return std::string_view(panel).find("BEE") != std::string_view::npos; }

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
    if (!t.orderable || !t.sides[size_t(sideIndex())])
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
            p += t->output[size_t(sideIndex())] * efficiency(s);
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

std::vector<const VehicleType*> Economy::vehicleMenu() const {
    std::vector<const VehicleType*> out;
    for (const VehicleType& v : vehicleTypes())
        if (v.side == side_ && vehicleAvailable(v))
            out.push_back(&v);
    return out;
}

bool Economy::vehicleAvailable(const VehicleType& v) const {
    return v.side == side_ && std::all_of(v.needs.begin(), v.needs.end(), [&](const std::string& n) { return has(n); });
}

bool Economy::orderVehicle(const VehicleType& v) {
    if (!vehicleAvailable(v))
        return false;
    // The yard's button is off while the colony is at its size limit.
    if (colonySize() + int(jobs_.size()) >= capacity_)
        return false;
    for (int m = 0; m < kMaterials; ++m)
        if (units_[size_t(m)] < int64_t(v.cost[size_t(m)]) * unitsPerMaterial(m)) {
            pendingShort_.push_back({"short-resources", {}});
            return false;
        }
    // The yard with the least work queued.
    int yard = 0;
    double least = 1e30;
    for (const ColonyStructure& s : structures_)
        if (s.type == "vehicle-yard" && s.progress >= 1) {
            double queued = 0;
            for (const VehicleJob& j : jobs_)
                if (j.yard == s.id)
                    queued += j.work + 1;
            if (queued < least)
                least = queued, yard = s.id;
        }
    if (!yard)
        return false;
    for (int m = 0; m < kMaterials; ++m)
        units_[size_t(m)] -= int64_t(v.cost[size_t(m)]) * unitsPerMaterial(m);
    const double work = double(v.value) * kWorkPerValue;
    jobs_.push_back({v.type, yard, work, work, -1});
    return true;
}

std::vector<std::pair<int, int>> Economy::takeVehicles() {
    std::vector<std::pair<int, int>> out;
    out.swap(finished_);
    return out;
}

void Economy::setOfficer(int structure, float skill) { officers_[structure] = skill; }

float Economy::crewShare(const ColonyStructure& s) const {
    (void)s;
    return 1; // crew: to come
}

void Economy::setHealth(int structure, float share) {
    for (ColonyStructure& s : structures_)
        if (s.id == structure)
            s.health = std::clamp(share, 0.0f, 1.0f);
}

float Economy::efficiency(const ColonyStructure& s) const {
    const BuildingType* t = buildingType(s.type);
    if (!t)
        return 0;
    // Health: 0-0.5 in tenths below 30 %, then 0.55-0.95 in twentieths, whole
    // from 75 %.
    const int step = std::clamp(int(s.health * 20), 0, 20);
    const float health = step >= 15 ? 1.0f : step <= 5 ? 0.1f * float(step) : 0.5f + 0.05f * float(step - 5);
    // The buffer's fill, its average weighted four to one (whole above 99 %).
    float fill = 1;
    if (t->bufferCap > 0) {
        fill = (4 * s.fill + s.stored / t->bufferCap) / 5;
        if (fill > 0.99f)
            fill = 1;
    }
    float officer = 1;
    if (t->speciality) {
        auto it = officers_.find(s.id);
        officer = it == officers_.end() ? 0 : it->second;
    }
    return 0.4f * (health + fill + officer) * crewShare(s);
}

void Economy::produce(std::vector<ScriptEvent>& out) {
    std::set<int> busy;
    for (VehicleJob& j : jobs_) {
        if (busy.count(j.yard))
            continue; // one at a time per yard
        busy.insert(j.yard);
        auto s = std::find_if(structures_.begin(), structures_.end(), [&](const ColonyStructure& c) { return c.id == j.yard; });
        if (s == structures_.end())
            continue;
        if (j.work > 0) {
            // Running above 0.35, stopped below 0.2 (and between, as it was).
            const float eff = efficiency(*s);
            bool& running = yardRunning_[s->id];
            running = eff > kYardStart || (running && eff >= kYardStop);
            if (!running)
                continue;
            const float draw = std::min(kYardDraw, s->stored);
            s->stored -= draw;
            j.work -= double(draw * eff);
            continue;
        }
        if (j.rollout < 0)
            j.rollout = kRollout;
        j.rollout -= 1 / kTicksPerSecond;
        if (j.rollout <= 0) {
            finished_.push_back({j.type, j.yard});
            if (const VehicleType* v = vehicleType(j.type))
                out.push_back({"vehicle-built", {v->panel}});
            j.type = -1;
        }
    }
    jobs_.erase(std::remove_if(jobs_.begin(), jobs_.end(), [](const VehicleJob& j) { return j.type < 0; }), jobs_.end());
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
            s.fill += (s.stored / t->bufferCap - s.fill) * kFillAverage;
        }
    if (met_ < kPowerLow && clock_ - lastPowerWarning_ >= kWarnEvery) {
        out.push_back({"power-low", {}});
        lastPowerWarning_ = clock_;
    }
    produce(out);
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
