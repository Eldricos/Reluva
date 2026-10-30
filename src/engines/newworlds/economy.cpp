// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/economy.h"

#include <algorithm>
#include <array>
#include <numeric>
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
// The processor's plants, by the material each doubles.
constexpr const char* kPlants[] = {"dilithium-plant",   "polycomp-plant", "thermoconc-plant",
                                   "transalumin-plant", "nitrium-plant",  "duranium-plant"};
// - the power warning comes when less than three quarters of the asked
//   power is granted;
constexpr float kPowerLow = 0.75f;
// - generators and the hub run at 1.2 efficiency when whole, powered and
//   staffed (the panel's 120 %);
// - the colony's capacity never passes 90.
constexpr int kCapacityCap = 90;
// Bees (exe.md#bees): their flying speeds (units a tick, as the game's
// start-up rewrite sets them: construction bees 1000, cargo bees 600), the
// shortest flight, how far a site's spots are from its middle, and the
// most at a site. A cargo bee's stay at a dock is Reluva's (not found).
constexpr float kBeeSpeed = 1000, kCargoSpeed = 600;
constexpr int kShortestFlight = 10;
constexpr float kBeeSpot = 2000;
constexpr int kBeesASite = 4;
constexpr int kDockTicks = 20;
// How often the processor sends its goods to storage (ticks).
constexpr int kShipEvery = 32;
// Pods (exe.md#bees): how many a mine lets wait before it stops; a
// pad's places round its building (the game lays out 80), how far out;
// most coming to one building; how long a worker bee waits to load; a
// worker bee's speed.
constexpr int kPodsWaiting = 4, kPadPlaces = 16, kMostIncoming = 17;
constexpr float kWorkerSpeed = 400;
// The bees' stops (exe.md#bees), in ticks: a worker bee setting a pod down
// on its yard (a tick and a 30-tick lift); a cargo bee taking each pod up
// (a tick, a 50-tick climb) and setting each down (a tick, a 30-tick lift,
// a 50-tick climb); its checks while it waits for pods (every 16 ticks; it
// goes for one after 64).
constexpr int kWorkerSetDown = 1 + 30, kCargoTakeUp = 1 + 50, kCargoSetDown = 1 + 30 + 50;
constexpr int kCargoCheck = 16, kCargoChecks = 64;
// Shortage lines repeat at most every 600 ms (exe.md#stores).
constexpr double kShortageEvery = 0.6;
// Vehicle production (exe.md#vehicle-production): a job is its build value
// x 300 power units; each tick a running yard takes up to 300 from its
// buffer, times its efficiency; it starts above 0.35 and stops below 0.2.
// Then the vehicle rolls out through the yard's dock (exe.md#docking), by
// its dock legs: a tank about 9.5 s from the Federation's yard, 4 s from
// the Klingons', 7.3 s from the Romulans'; a scout, quicker, about two
// thirds of that.
constexpr double kWorkPerValue = 300;
constexpr float kYardDraw = 300, kYardStart = 0.35f, kYardStop = 0.2f;
double rolloutSeconds(char side, int type) {
    const double tank = side == 'K' ? 4 : side == 'R' ? 7.3 : 9.5;
    return type == 97 || type == 37 || type == 98 ? tank * 6.5 / 9.5 : tank;
}
// People (exe.md#crew-and-efficiency): a race's roster, an accommodation
// block's batches, life support's use a person a tick and its rate of
// exchange with power, a farm's output. Crew walk from the hub at 19 units
// a tick, 380 a second (about 43 s a block; exe.md#bees); how long a
// beamed-down crew takes from the pad is a placeholder (C8's yard was
// crewed about 20 s after the "not enough crew" message).
constexpr int kRoster = 150, kBatch = 10;
constexpr double kLifePerPerson = 16, kLifePerPower = 16.384, kFarmLife = 2048;
constexpr double kCrewWalk = 20, kCrewSpeed = 380;
double walkSeconds(const ColonyStructure& from, const ColonyStructure& to) {
    return std::hypot(double(to.x - from.x), double(to.y - from.y)) / kCrewSpeed;
}
// The least a razed building stands evacuating (Reluva's: the game's is
// a few ticks, its parts let go in turn).
constexpr int kEvacTicks = 5;

constexpr int sideOf(char side) { return side == 'K' ? 1 : side == 'R' ? 2 : 0; }

} // namespace

const char* materialName(int m) {
    static const char* const kNames[] = {"DILITHIUM", "POLYCOMP", "THERMOCONC", "TRANSALUMIN", "NITRIUM", "DURANIUM"};
    return m >= 0 && m < kMaterials ? kNames[m] : "";
}

int unitsPerMaterial(int m) { return m >= Nitrium ? 100 : 1000; }

int materialNamed(std::string_view name) {
    static const char* const kPrefixes[] = {"dilith", "polycomp", "thermoc", "trans", "nitri", "duran"};
    for (int m = 0; m < kMaterials; ++m)
        if (name.rfind(kPrefixes[m], 0) == 0)
            return m;
    return -1;
}

void Economy::give(int material, int amount) {
    if (material < 0 || material >= kMaterials)
        return;
    Units u{};
    u[size_t(material)] = int64_t(std::abs(amount)) * unitsPerMaterial(material);
    if (amount >= 0) {
        deposit(u);
    } else {
        u[size_t(material)] = std::min(u[size_t(material)], total(material));
        take(u);
    }
}

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
    // (play-vm-tutorials.md#build-times). The game's bees build a model's
    // tiles, one each a cycle, in four sections at once (exe.md#bees), so
    // the rest follow from their models' tiles, against those timed: the
    // defences and shield generators, and the other races' generators and
    // yards (its largest section's tiles x 0.70 s, + 5.5 s + 1.4 s for each
    // section before it).
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
        // The Klingons' and Romulans' times as their Let's Plays timed them
        // (play-kli-lp.md, play-rom-lp.md).
        t.back().kliSeconds = 32, t.back().romSeconds = 70;
        consumer(add("vehicle-yard", "VEHICLE YARD",
                     {Stock{0, 10, 90, 5, 0, 0}, Stock{0, 10, 90, 5, 0, 0}, Stock{0, 10, 90, 2, 0, 0}},
                     {yard, yard, yard}, 65),
                 34000, 550, 60);
        t.back().kliSeconds = 42, t.back().romSeconds = 62;
        add("power-generator", "POWER GENERATOR",
            {Stock{35, 1, 35, 0, 800, 0}, Stock{35, 1, 30, 0, 850, 0}, Stock{35, 1, 30, 0, 850, 0}}, {yard, yard, yard},
            30)
            .output = {500, 500, 1000};
        t.back().kliSeconds = 23, t.back().romSeconds = 36;
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
        // The storage facility: never in the build menu (the missions place
        // it), a store of the colony's materials (exe.md#stores), with its
        // power buffer, its one crew and its officer (storage, 8).
        BuildingType& storage = add("storage-facility", "STORAGE FACILITY", {}, {}, 40);
        storage.orderable = false;
        consumer(storage, 44000, 425, 340);
        const std::vector<std::string> improvedYard{"construction-yard-upgrade"};
        consumer(add("transporter-pad", "TRANSPORTER PAD",
                     {Stock{0, 6, 32, 2, 0, 0}, Stock{0, 6, 32, 2, 0, 0}, Stock{0, 6, 32, 2, 0, 0}},
                     {improvedYard, improvedYard, improvedYard}, 40),
                 44000, 300, 60);
        consumer(add("sickbay", "SICKBAY",
                     {Stock{0, 8, 48, 20, 0, 0}, Stock{0, 8, 48, 26, 0, 0}, Stock{0, 8, 48, 36, 0, 0}}, {yard, yard, yard},
                     40),
                 44000, 550, 440);
        t.back().kliSeconds = 61, t.back().romSeconds = 90;
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
        t.back().kliSeconds = 70;
        // Defences: the shield generator (its field halves the colony's
        // damage; exe.md#shields) and the two turrets, by their models.
        consumer(add("shield-generator", "SHIELD GENERATOR",
                     {Stock{20, 2, 40, 0, 0, 800}, Stock{20, 2, 20, 0, 0, 800}, Stock{20, 2, 20, 0, 0, 800}},
                     {{{"construction-yard-upgrade", "shield-buffer-lab", "security-center"},
                       {"construction-yard-upgrade", "security-center", "shield-buffer-lab"},
                       {"construction-yard-upgrade", "science-station", "shield-buffer-lab"}}},
                     27),
                 64000, 550, 640);
        t.back().kliSeconds = 22, t.back().romSeconds = 48;
        consumer(add("defensive-weapon", "DEFENSIVE WEAPON",
                     {Stock{0, 0, 92, 12, 0, 500}, Stock{0, 0, 92, 12, 0, 500}, Stock{0, 0, 92, 12, 0, 500}},
                     {{{"construction-yard-upgrade", "colony-hub-upgrade", "shield-generator"},
                       {"construction-yard-upgrade", "shield-generator", "disruptor-matrix", "security-center"},
                       {"construction-yard-upgrade", "security-center", "shield-generator"}}},
                     22),
                 64000, 550, 160);
        t.back().kliSeconds = 24, t.back().romSeconds = 34;
        consumer(add("defensive-artillery", "DEFENSIVE ARTILLERY",
                     {Stock{0, 0, 93, 13, 250, 500}, Stock{0, 0, 93, 11, 250, 500}, Stock{0, 0, 93, 11, 250, 500}},
                     {{{"construction-yard-advanced", "colony-hub-advanced", "security-center"},
                       {"construction-yard-advanced", "shield-generator", "tracking-systems"},
                       {"construction-yard-upgrade", "colony-hub-advanced", "security-center", "energy-lab"}}},
                     31),
                 64000, 550, 160);
        t.back().romSeconds = 27;
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
        t.back().kliSeconds = 46;
        upgrade("colony-hub-advanced", "ADVANCED HUB", "colony-hub", Stock{0, 50, 30, 20, 0, 0},
                Stock{0, 50, 30, 20, 0, 0}, Stock{0, 50, 30, 20, 0, 0}, "colony-hub-upgrade");
        t.back().kliSeconds = 45;
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
        // The resource processor's plants (exe.md#upgrades-and-add-ons):
        // one a material, each doubling what the processor makes of it;
        // the Federation's are its types 166 and 167 by turns, the others'
        // one type each. Built by bees on the processor (times by their
        // models' tiles).
        auto plant = [&](const char* id, const char* panel, const std::string& fedNeed, const std::string& kliNeed,
                         const std::string& romNeed) {
            BuildingType& p = add(id, panel, {Stock{0, 2, 12, 10, 0, 0}, Stock{0, 2, 12, 2, 0, 0}, Stock{0, 2, 12, 2, 0, 0}},
                                  {{{"resource-processor", fedNeed}, {"resource-processor", kliNeed},
                                    {"resource-processor", romNeed}}},
                                  12);
            p.upgradeOf = "resource-processor";
            p.romSeconds = 19;
        };
        plant("dilithium-plant", "DILITHIUM PLANT", "construction-yard", "construction-yard", "construction-yard");
        plant("polycomp-plant", "POLYCOMP PLANT", "construction-yard", "construction-yard", "construction-yard");
        plant("thermoconc-plant", "THERMOCONC PLANT", "construction-yard-upgrade", "construction-yard-upgrade",
              "construction-yard-upgrade");
        plant("transalumin-plant", "TRANS AL PLANT", "construction-yard-upgrade", "construction-yard-upgrade",
              "construction-yard-upgrade");
        plant("nitrium-plant", "NITRIUM PLANT", "construction-yard-advanced", "construction-yard-advanced",
              "construction-yard-upgrade");
        plant("duranium-plant", "DURANIUM PLANT", "construction-yard-advanced", "construction-yard-advanced",
              "construction-yard-upgrade");
        // The sickbay's wards: each raises the colony's capacity by 5 as
        // it's ordered, and does nothing else (the sickbay heals no one).
        // Five a sickbay, the Klingons' four; each needs a hub or yard
        // upgrade (the Romulans' one yard upgrade serves for both).
        auto ward = [&](const char* id, const char* panel, std::array<bool, 3> sides,
                        std::array<std::vector<std::string>, 3> needs) {
            for (auto& n : needs)
                n.insert(n.begin(), "sickbay");
            BuildingType& w = add(id, panel, {Stock{0, 2, 25, 6, 0, 0}, Stock{0, 2, 25, 4, 0, 0}, Stock{0, 2, 25, 5, 0, 0}},
                                  std::move(needs), 18);
            w.upgradeOf = "sickbay", w.sides = sides, w.capacity = 5;
            w.kliSeconds = 25, w.romSeconds = 24;
        };
        ward("surgical-ward", "SURGICAL WARD", {true, true, true},
             {{{"colony-hub-upgrade"}, {"colony-hub-upgrade"}, {"colony-hub-upgrade"}}});
        ward("intensive-care", "INTENSIVE CARE", {true, true, true},
             {{{"construction-yard-upgrade"}, {"construction-yard-upgrade"}, {"construction-yard-upgrade"}}});
        ward("null-g-ward", "NULL-G WARD", {true, true, true},
             {{{"colony-hub-advanced"}, {"colony-hub-advanced"}, {"colony-hub-advanced"}}});
        ward("medical-lab", "MEDICAL LAB", {true, true, true},
             {{{"construction-yard-advanced"}, {"construction-yard-advanced"}, {"construction-yard-upgrade"}}});
        ward("training-ward", "TRAINING WARD", {true, false, true},
             {{{"colony-hub-advanced", "construction-yard-advanced"},
               {},
               {"colony-hub-advanced", "construction-yard-upgrade"}}});
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
            // Upgrades bring no people of their own.
            if (!b.upgradeOf.empty())
                continue;
            if (!b.crewless || id == "farm")
                b.required = 1, b.crewSlots = 1;
            if (id == "vehicle-yard")
                b.crewSlots = 3;
            if (id == "accommodation")
                b.required = 8, b.crewSlots = 8, b.poolSlots = 200;
            if (id == "colony-hub")
                b.poolSlots = 150;
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

// Who is served first when power runs short (after the hub's life
// support, class 0): 1 the vehicle yard, landing pad (the transporter
// pad), sickbay, shields and defences; 2 the storage facility (the
// Klingons' and Romulans' processor instead) and the security center; 3
// the rest.
int Economy::powerClass(std::string_view type) const {
    for (std::string_view t : {"vehicle-yard", "transporter-pad", "sickbay", "shield-generator", "defensive-weapon",
                               "defensive-artillery"})
        if (type.rfind(t, 0) == 0)
            return 1;
    if (type.rfind("security-center", 0) == 0 ||
        type.rfind(side_ == 'F' ? "storage-facility" : "resource-processor", 0) == 0)
        return 2;
    return 3;
}

const BuildingType* buildingType(std::string_view id) {
    for (const BuildingType& t : buildingTypes())
        if (t.id == id)
            return &t;
    return nullptr;
}

Economy::Economy(char side, Stock start, int capacity, ColonyMap map)
    : side_(side), capacity_(std::min(capacity, kCapacityCap)), map_(std::move(map)) {
    stores_.push_back({0, false, unitsOf(start)});
}

Economy::Units Economy::unitsOf(const Stock& s) {
    Units u{};
    for (int m = 0; m < kMaterials; ++m)
        u[size_t(m)] = int64_t(s[size_t(m)]) * unitsPerMaterial(m);
    return u;
}

int64_t Economy::total(int m) const {
    int64_t n = 0;
    for (const Store& st : stores_)
        if (!st.gone)
            n += st.units[size_t(m)];
    return n;
}

bool Economy::pays(const Units& cost) const {
    for (int m = 0; m < kMaterials; ++m)
        if (total(m) < cost[size_t(m)])
            return false;
    return true;
}

void Economy::take(Units cost) {
    for (Store& st : stores_)
        for (int m = 0; m < kMaterials && !st.gone; ++m) {
            const int64_t t = std::min(cost[size_t(m)], st.units[size_t(m)]);
            st.units[size_t(m)] -= t, cost[size_t(m)] -= t;
        }
}

void Economy::startingYardStock() {
    Units u{};
    for (int m = 0; m < kMaterials; ++m)
        if (m != Thermoconc)
            u[size_t(m)] = 160000;
    deposit(u);
}

void Economy::deposit(const Units& u) {
    for (Store& st : stores_)
        if (!st.gone) {
            for (int m = 0; m < kMaterials; ++m)
                st.units[size_t(m)] += u[size_t(m)];
            return;
        }
}

// A storage facility standing: the first takes over the mission's stock
// (and is the yards' store from then on); later ones start empty.
void Economy::addStore(int structure) {
    for (Store& st : stores_)
        if (st.structure == 0) {
            st.structure = structure;
            return;
        }
    stores_.push_back({structure, false, {}});
}

Economy::Store* Economy::yardStore() {
    return stores_.empty() ? nullptr : &stores_.front();
}

Stock Economy::storeOf(int structure) const {
    Stock s{};
    for (const Store& st : stores_)
        if (st.structure == structure && !st.gone)
            for (int m = 0; m < kMaterials; ++m)
                s[size_t(m)] = int(st.units[size_t(m)] / unitsPerMaterial(m));
    return s;
}

int Economy::sideIndex() const { return sideOf(side_); }

Stock Economy::stock() const {
    Stock s{};
    for (int m = 0; m < kMaterials; ++m)
        s[size_t(m)] = int(total(m) / unitsPerMaterial(m));
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
        if (const BuildingType* t = buildingType(s.type); !t || t->upgradeOf.empty())
            ++n; // (the storage facility, not in the build menu, too)
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
    return pays(unitsOf(t.costFor(side_)));
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
    if (type == "storage-facility")
        addStore(s.id);
    if (const BuildingType* t = buildingType(type)) {
        s.stored = t->bufferCap;
        // A mine digs round its own block (16384 units a block), as one
        // ordered in play does.
        if (t->onDeposit)
            s.blockX = int(std::floor(x / 16384)), s.blockY = int(std::floor(y / 16384));
        // Housing standing at the start counts twice: the briefing's scene
        // makes the colony's own a first time and never takes it off
        // (exe.md#the-economy).
        raiseCapacity(2 * t->capacity);
        // The hub takes in everyone still in orbit; accommodation standing
        // at the start takes people straight from the hub.
        if (type == "colony-hub") {
            s.pool = std::min(t->poolSlots, orbit_);
            orbit_ -= s.pool;
        } else if (type == "accommodation") {
            for (ColonyStructure& h : structures_)
                if (h.type == "colony-hub") {
                    const int m = std::min(h.pool, t->poolSlots);
                    h.pool -= m, s.pool += m;
                }
        }
    }
    structures_.push_back(s);
    // A building standing at the start has its crew at once (no rides).
    const bool transports = transports_;
    transports_ = false;
    requestCrew(structures_.back());
    transports_ = transports;
    for (auto& c : structures_.back().coming)
        c.second = 0;
    structures_.back().crew += std::accumulate(structures_.back().coming.begin(), structures_.back().coming.end(), 0,
                                               [](int n, const auto& c) { return n + c.first; });
    structures_.back().coming.clear();
    return s.id;
}

int Economy::order(const std::string& type, float x, float y, int tx, int ty) {
    const BuildingType* t = buildingType(type);
    // (An AI's build order may name an advanced building outright: in the
    // game those are types of their own, the AI's 189 an advanced mine.)
    if (!t || (free_ ? !t->sides[size_t(sideIndex())] : !available(*t)))
        return 0;
    // The colony's queue holds 10 at most (exe.md#the-selection-panel).
    if (!free_ && pendingBuilds() >= 10)
        return 0;
    // The build buttons work only while COLONY SIZE is below the limit
    // (exe.md#the-economy; no report is named); upgrades and add-ons don't
    // count in it.
    if (!free_ && t->upgradeOf.empty() && colonySize() >= capacity_)
        return 0;
    if (!free_ && !affordable(*t)) {
        // Engineering's report: construction (002) or an upgrade (003) delayed.
        shortage(t->upgradeOf.empty() ? 2 : 3);
        return 0;
    }
    ColonyStructure s;
    s.id = nextId_++;
    s.type = type;
    s.x = x, s.y = y;
    s.progress = 0;
    if (t->onDeposit) {
        s.blockX = tx / 2, s.blockY = ty / 2;
        // (An AI's build order skips this: its mines' spots aren't all on
        // a deposit as Reluva reads the map; they dig what's in reach.)
        if (!free_ && (!map_.depositKind || map_.depositKind(s.blockX, s.blockY) == 0))
            return 0;
    }
    if (!free_)
        take(unitsOf(t->costFor(side_)));
    // Housing raises the capacity as soon as its site is placed.
    raiseCapacity(t->capacity);
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
    if (free_ ? v.side != side_ : !vehicleAvailable(v))
        return false;
    // The yard's button is off while the colony is at its size limit.
    if (!free_ && colonySize() + int(jobs_.size()) >= capacity_)
        return false;
    // Paid from the first storage facility's store alone.
    Store* store = yardStore();
    for (int m = 0; m < kMaterials && !free_; ++m)
        if (!store || store->units[size_t(m)] < int64_t(v.cost[size_t(m)]) * unitsPerMaterial(m)) {
            shortage(1); // vehicle manufacture delayed
            return false;
        }
    // The yard with the least work queued (not one being razed).
    int yard = 0;
    double least = 1e30;
    for (const ColonyStructure& s : structures_)
        if (s.type == "vehicle-yard" && s.progress >= 1 && !s.evacuating) {
            double queued = 0;
            for (const VehicleJob& j : jobs_)
                if (j.yard == s.id)
                    queued += j.work + 1;
            if (queued < least)
                least = queued, yard = s.id;
        }
    if (!yard)
        return false;
    for (int m = 0; m < kMaterials && !free_; ++m)
        store->units[size_t(m)] -= int64_t(v.cost[size_t(m)]) * unitsPerMaterial(m);
    for (const ColonyStructure& s : structures_)
        if (s.id == yard && s.crew < 1)
            pendingShort_.push_back({"short-crew", {}}), pendingShort_.push_back({"officer", {"001"}});
    const double work = double(v.value) * kWorkPerValue;
    jobs_.push_back({v.type, yard, work, work, -1});
    return true;
}

// A yard's jobs handed on: those not yet rolling out (and, the yard lost,
// those too) start again at the back of another of the side's
// finished yards' queue (what was made of them went with the yard, so
// from nothing; Reluva's choice, the game's not traced); with none, they
// are dropped and their cost goes back to the yards' store, as a deleted
// order's (exe.md#stores). A worker bee's dropped, the buildings without
// one ask again.
void Economy::handOffJobs(int yard, bool rollingOut) {
    std::vector<VehicleJob> moved;
    for (auto j = jobs_.begin(); j != jobs_.end();) {
        if (j->yard != yard || (!rollingOut && j->rollout >= 0)) {
            ++j;
            continue;
        }
        int to = 0;
        double least = 1e30;
        for (const ColonyStructure& s : structures_)
            if (s.type == "vehicle-yard" && s.id != yard && s.progress >= 1 && !s.evacuating) {
                double queued = 0;
                for (const VehicleJob& o : jobs_)
                    if (o.yard == s.id)
                        queued += o.work + 1;
                if (queued < least)
                    least = queued, to = s.id;
            }
        if (to) {
            moved.push_back({j->type, to, j->total, j->total, -1});
        } else if (const VehicleType* v = vehicleType(j->type)) {
            if (Store* st = free_ ? nullptr : yardStore())
                for (int m = 0; m < kMaterials; ++m)
                    st->units[size_t(m)] += int64_t(v->cost[size_t(m)]) * unitsPerMaterial(m);
            if (v->type == 12 || v->type == 11 || v->type == 92)
                for (auto& [id, d] : depots_)
                    d.asked = false;
        }
        j = jobs_.erase(j);
    }
    jobs_.insert(jobs_.end(), moved.begin(), moved.end());
}

void Economy::shortage(int line) {
    double& last = lastShortage_[size_t(line - 1)];
    if (clock_ - last < kShortageEvery)
        return;
    last = clock_;
    pendingShort_.push_back({"short-resources", {}});
    pendingShort_.push_back({"officer", {"00" + std::to_string(line)}});
}

std::vector<std::pair<int, int>> Economy::takeVehicles() {
    std::vector<std::pair<int, int>> out;
    out.swap(finished_);
    return out;
}

void Economy::setOfficer(int structure, float skill) { officers_[structure] = skill; }

float Economy::crewShare(const ColonyStructure& s) const {
    const BuildingType* t = buildingType(s.type);
    if (!t || t->crewless || t->crewSlots == 0)
        return 1;
    // Below the crew it needs it doesn't work at all.
    if (s.crew < t->required)
        return 0;
    return std::min(1.0f, float(s.crew) / float(t->crewSlots));
}

void Economy::setOfficers(int n) {
    officersDown_ = n;
    orbit_ = std::max(0, kRoster - n);
}

int Economy::pool() const {
    int n = 0;
    for (const ColonyStructure& s : structures_)
        n += s.pool;
    return n;
}

int Economy::crew() const {
    int n = 0;
    for (const ColonyStructure& s : structures_) {
        n += s.crew;
        for (const auto& c : s.coming)
            n += std::abs(c.first); // on their way, to work or to a pool
    }
    return n;
}

int Economy::people() const { return pool() + crew() + walking() + officersDown_; }

int Economy::walking() const {
    int n = 0;
    for (const auto& w : walking_)
        n += w.first;
    for (const Ride& r : rides_)
        n += r.listed ? 0 : r.people;
    return n;
}

ColonyStructure* Economy::structureById(int id) {
    for (ColonyStructure& s : structures_)
        if (s.id == id)
            return &s;
    return nullptr;
}

const ColonyStructure* Economy::pad() const {
    for (const ColonyStructure& s : structures_)
        if (s.type == "transporter-pad" && s.progress >= 1 && !s.evacuating)
            return &s;
    return nullptr;
}

// People to carry, ten to a transport.
void Economy::addRides(int from, int to, int people, bool toPool, bool listed, double wait) {
    for (int left = people; left > 0; left -= kBatch) {
        const int n = std::min(left, kBatch);
        rides_.push_back({nextRide_++, from, to, n, toPool, listed, false, wait});
        if (ColonyStructure* t = listed ? structureById(to) : nullptr)
            t->coming.push_back({toPool ? -n : n, kRiding});
    }
}

void Economy::rideLoaded(int ride) {
    for (Ride& r : rides_)
        if (r.id == ride)
            r.loaded = true;
}

int Economy::endRide(int ride, bool delivered) {
    const auto it = std::find_if(rides_.begin(), rides_.end(), [&](const Ride& r) { return r.id == ride; });
    if (it == rides_.end())
        return 0;
    const Ride r = *it;
    rides_.erase(it);
    ColonyStructure* to = structureById(r.to);
    if (to && r.listed) {
        const int signedPeople = r.toPool ? -r.people : r.people;
        for (auto c = to->coming.begin(); c != to->coming.end(); ++c)
            if (c->second >= kRiding / 2 && c->first == signedPeople) {
                to->coming.erase(c);
                break;
            }
    }
    if (delivered) {
        if (to && !to->evacuating)
            (r.toPool ? to->pool : to->crew) += r.people;
        else if (ColonyStructure* h = structureById(hub() ? hub()->id : -1))
            h->pool += r.people;
        else
            orbit_ += r.people;
    }
    if (to && to->progress >= 1 && !to->evacuating)
        requestCrew(*to);
    return r.people;
}

void Economy::rideDelivered(int ride) {
    endRide(ride, true);
    retryCrew();
}

void Economy::rideLost(int ride) { dead_ += endRide(ride, false); }

void Economy::loseOne() {
    for (bool pools : {true, false})
        for (ColonyStructure& s : structures_)
            if (int& n = pools ? s.pool : s.crew; n > 0) {
                --n, ++dead_;
                if (!pools)
                    requestCrew(s);
                return;
            }
}

// A building's request for its crew (its slots less those it has or has
// coming): all from the first hub or accommodation with enough in its pool,
// else beamed down from orbit by a transporter pad, else it waits.
void Economy::requestCrew(ColonyStructure& s) {
    const BuildingType* t = buildingType(s.type);
    if (!t || t->crewSlots == 0)
        return;
    int have = s.crew;
    for (const auto& c : s.coming)
        have += std::max(0, c.first);
    const int slots = s.target < 0 ? t->crewSlots : std::min(s.target, t->crewSlots);
    const int need = slots - have;
    if (need <= 0) {
        s.wanted = 0;
        return;
    }
    s.wanted = need;
    for (ColonyStructure& from : structures_)
        if (from.progress >= 1 && !from.evacuating && from.pool >= need && from.id != s.id) {
            from.pool -= need;
            // From the hub's own pool they walk; from a block they ride.
            if (transports_ && from.type != "colony-hub")
                addRides(from.id, s.id, need, false, true);
            else
                s.coming.push_back({need, walkSeconds(from, s)});
            s.wanted = 0;
            return;
        }
    if (has("transporter-pad") && orbit_ >= need) {
        orbit_ -= need;
        s.beaming = true;
        // Beamed down to the pad, then (with transports) carried on.
        if (const ColonyStructure* p = transports_ ? pad() : nullptr; p && p->id != s.id)
            addRides(p->id, s.id, need, false, true, kCrewWalk);
        else
            s.coming.push_back({need, kCrewWalk});
        s.wanted = 0;
    }
}

void Economy::setCrewTarget(int structure, int n) {
    for (ColonyStructure& s : structures_) {
        if (s.id != structure)
            continue;
        const BuildingType* t = buildingType(s.type);
        if (!t || t->crewSlots == 0)
            return;
        s.target = std::clamp(n, 0, t->crewSlots);
        // Those over it go back to the hub's pool (with transports, riding
        // to a hub or block with room for ten).
        if (s.crew > s.target && transports_) {
            for (ColonyStructure& h : structures_) {
                const BuildingType* ht = buildingType(h.type);
                if (h.progress >= 1 && !h.evacuating && ht && ht->poolSlots - h.pool >= kBatch &&
                    (h.type == "colony-hub" || h.type == "accommodation")) {
                    addRides(s.id, h.id, s.crew - s.target, true, false);
                    s.crew = s.target;
                    break;
                }
            }
        }
        if (s.crew > s.target)
            for (ColonyStructure& h : structures_)
                if (h.type == "colony-hub" && h.progress >= 1) {
                    h.pool += s.crew - s.target;
                    s.crew = s.target;
                    break;
                }
        if (s.progress >= 1)
            requestCrew(s);
        return;
    }
}

// The requests still waiting are tried again when a pool has taken
// people in (the game has no timer for them).
void Economy::retryCrew() {
    for (ColonyStructure& s : structures_)
        if (s.wanted > 0 && s.progress >= 1)
            requestCrew(s);
}

// People arriving; an accommodation block filling its pool ten at a time,
// from the hub's pool or, once that's empty, beamed down.
void Economy::staff(ColonyStructure& s, std::vector<ScriptEvent>& out) {
    bool arrived = false;
    for (auto& c : s.coming)
        if ((c.second -= 1 / kTicksPerSecond) <= 0) {
            const BuildingType* t = buildingType(s.type);
            if (t && t->poolSlots > 0 && s.type == "accommodation" && c.first < 0)
                s.pool += -c.first;
            else
                s.crew += c.first;
            arrived = true;
        }
    s.coming.erase(std::remove_if(s.coming.begin(), s.coming.end(), [](const auto& c) { return c.second <= 0; }),
                   s.coming.end());
    // Its people all beamed down: the building reports it (an alert).
    if (s.beaming && s.coming.empty()) {
        s.beaming = false;
        out.push_back({"beamed-down", {std::to_string(s.id)}});
    }
    const BuildingType* t = buildingType(s.type);
    if (t && s.type == "accommodation" && s.progress >= 1 && !s.evacuating &&
        std::none_of(s.coming.begin(), s.coming.end(), [](const auto& c) { return c.first < 0; })) {
        int hubPool = 0;
        ColonyStructure* hub = nullptr;
        for (ColonyStructure& h : structures_)
            if (h.type == "colony-hub" && h.progress >= 1)
                hubPool += h.pool, hub = hub ? hub : &h;
        const int room = t->poolSlots - s.pool;
        const int m = std::min({kBatch, room, hubPool > 0 ? hubPool : (has("transporter-pad") ? orbit_ : 0)});
        if (m > 0) {
            if (hubPool > 0) {
                hub->pool -= m;
                s.coming.push_back({-m, walkSeconds(*hub, s)}); // negative: for its pool
            } else {
                orbit_ -= m;
                if (const ColonyStructure* p = transports_ ? pad() : nullptr)
                    addRides(p->id, s.id, m, true, true, kCrewWalk);
                else
                    s.coming.push_back({-m, kCrewWalk});
            }
        }
    }
    if (arrived)
        retryCrew();
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
    // The buffer's fill: (4 x its average + now) / (4 x capacity), whole
    // above 99 %.
    float fill = 1;
    if (t->bufferCap > 0) {
        fill = (4 * s.fill + s.stored / t->bufferCap) / 4;
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
        // No crew, no work (the colony told, as the game's, when it's
        // ordered); a vehicle rolling out goes on without (a razed yard's
        // last, its crew gone).
        if (s->crew < 1 && j.rollout < 0)
            continue;
        if (j.work > 0) {
            // Running above 0.35, stopped below 0.2 (and between, as it was).
            const float eff = efficiency(*s);
            bool& running = yardRunning_.try_emplace(s->id, true).first->second;
            const bool was = running;
            running = eff > kYardStart || (running && eff >= kYardStop);
            // The yard's reports: suspended (006), back on line (008).
            if (was != running)
                out.push_back({"officer", {running ? "008" : "006"}});
            if (!running)
                continue;
            const float draw = std::min(kYardDraw, s->stored);
            s->stored -= draw;
            j.work -= double(draw * eff);
            continue;
        }
        if (j.rollout < 0)
            j.rollout = rolloutSeconds(side_, j.type);
        j.rollout -= 1 / kTicksPerSecond;
        if (j.rollout <= 0) {
            finished_.push_back({j.type, j.yard});
            if (const VehicleType* v = vehicleType(j.type))
                out.push_back({"vehicle-built", {v->panel}});
            out.push_back({"officer", {"005"}}); // a vehicle finished
            j.type = -1;
        }
    }
    jobs_.erase(std::remove_if(jobs_.begin(), jobs_.end(), [](const VehicleJob& j) { return j.type < 0; }), jobs_.end());
}

bool Economy::mineTake(ColonyStructure& mine, std::array<int64_t, kOres>& into) {
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
                into[size_t(got - 1)] += kTonne;
                return true;
            }
    return false;
}

void Economy::tick(std::vector<ScriptEvent>& out) {
    clock_ += 1 / kTicksPerSecond;
    // Construction, at each type's pace (with bees, as many quarters of it
    // as there are at work).
    if (beesModelled_)
        beeTick();
    for (ColonyStructure& s : structures_) {
        if (s.progress >= 1)
            continue;
        const BuildingType* t = buildingType(s.type);
        const double share = beesModelled_ ? double(beesAt(s.id)) / kBeesASite : 1;
        s.progress = float(std::min(1.0, s.progress + share / (std::max(1.0, t->secondsFor(side_)) * kTicksPerSecond)));
        if (s.progress >= 1) {
            if (s.type == "storage-facility")
                addStore(s.id);
            for (Bee& b : bees_)
                if (!b.cargo && !b.worker && b.site == s.id)
                    b.site = 0, b.working = false;
            s.stored = t->bufferCap;
            out.push_back({"built", {s.type}});
            if (s.type == "colony-hub")
                out.push_back({"officer", {"004"}}); // a hub finished: Engineering's line
            requestCrew(s);
        }
    }
    for (ColonyStructure& s : structures_) {
        staff(s, out);
        s.evacTicks += s.evacuating;
    }
    for (Ride& r : rides_)
        r.wait = std::max(0.0, r.wait - 1 / kTicksPerSecond);
    // People walking back from a razed building reach the hub's pool (with
    // no hub, the roster in orbit).
    for (auto& w : walking_)
        if ((w.second -= 1 / kTicksPerSecond) <= 0) {
            ColonyStructure* h = nullptr;
            for (ColonyStructure& c : structures_)
                if (c.type == "colony-hub" && c.progress >= 1)
                    h = h ? h : &c;
            if (h)
                h->pool += w.first;
            else
                orbit_ += w.first;
        }
    const bool arrived = std::any_of(walking_.begin(), walking_.end(), [](const auto& w) { return w.second <= 0; });
    walking_.erase(std::remove_if(walking_.begin(), walking_.end(), [](const auto& w) { return w.second <= 0; }),
                   walking_.end());
    if (arrived)
        retryCrew();
    // Power: each built consumer asks to fill its buffer, at most its
    // intake a tick; the supply (with the reserve behind it) is shared out
    // class by class (exe.md#the-economy), each class in proportion; then
    // each uses its standing demand from its buffer.
    std::array<float, 4> asked{};
    for (ColonyStructure& s : structures_)
        if (const BuildingType* t = buildingType(s.type); t && s.progress >= 1 && t->intake > 0)
            asked[size_t(powerClass(s.type))] += std::min(t->intake, t->bufferCap - s.stored + t->demand);
    const float askedAll = asked[1] + asked[2] + asked[3];
    // Life support first: everyone on the planet uses 16 a tick; farms add
    // to it; the hub buys back what's used with power, before any other
    // building (its request divided by its efficiency).
    lifeUsed_ = kLifePerPerson * people();
    life_ -= lifeUsed_;
    lifeProduced_ = 0;
    float hubEff = 0;
    for (const ColonyStructure& s : structures_)
        if (s.progress >= 1) {
            if (s.type == "farm")
                lifeProduced_ += kFarmLife * efficiency(s);
            else if (s.type == "colony-hub")
                hubEff = std::max(hubEff, efficiency(s));
        }
    life_ = std::clamp(life_ + lifeProduced_, 0.0, kLifeCap);
    lifeMet_ = 1;
    lifeDraw_ = 0;
    double supply = powerSupply();
    const double produced = supply;
    if (hubEff > 0) {
        // (What it's granted buys back as much less as its request was
        // more: the whole request fills the pool.)
        const double ask = (kLifeCap - life_) / kLifePerPower / hubEff;
        const double grant = std::min(ask, supply + reserve_);
        lifeMet_ = ask > 0 ? float(grant / ask) : 1;
        life_ = std::min(kLifeCap, life_ + grant * kLifePerPower * hubEff);
        supply -= grant;
        if (supply < 0) {
            reserve_ += supply;
            supply = 0;
        }
        lifeDraw_ = float(grant);
    }
    double give = supply;
    if (askedAll > supply) {
        const double draw = std::min(reserve_, double(askedAll) - supply);
        reserve_ -= draw;
        give += draw;
    } else {
        reserve_ = std::min(kReserveCap, reserve_ + (supply - askedAll));
    }
    // Class 1 first, then 2, then 3, each getting what's left.
    std::array<float, 4> classMet{1, 1, 1, 1};
    double left = give;
    for (size_t c = 1; c < 4; ++c)
        if (asked[c] > 0) {
            const double g = std::min(double(asked[c]), left);
            classMet[c] = float(g / asked[c]);
            left -= g;
        }
    requested_ = askedAll;
    met_ = askedAll > 0 ? float(std::min(1.0, give / askedAll)) : 1;
    // What the colony drew of what it made (the hub panel's consumption).
    consumption_ = produced > 0 ? float(std::min(1.0, (lifeDraw_ + std::min(double(askedAll), supply)) / produced)) : 0;
    for (ColonyStructure& s : structures_)
        if (const BuildingType* t = buildingType(s.type); t && s.progress >= 1 && t->intake > 0) {
            const float want = std::min(t->intake, t->bufferCap - s.stored + t->demand);
            const float got = classMet[size_t(powerClass(s.type))];
            s.stored = std::clamp(s.stored + want * got - t->demand, 0.0f, t->bufferCap);
            s.fill = (s.stored / t->bufferCap + s.fill) * 0.5f; // the game's average, kept at each refill
            s.supply = (got + s.supply) * 0.5f;                   // and of the share granted
        }
    if (met_ < kPowerLow && clock_ - lastPowerWarning_ >= kWarnEvery) {
        out.push_back({"power-low", {}});
        lastPowerWarning_ = clock_;
    }
    produce(out);
    // Mining, for as long as there's a processor to take the ore: the bees
    // bring it at their pace. Neither a mine nor the processor works
    // without its crew (exe.md#crew-and-efficiency).
    const bool processor = has("resource-processor");
    const bool refining = std::any_of(structures_.begin(), structures_.end(), [&](const ColonyStructure& s) {
        return s.type == "resource-processor" && s.progress >= 1 && crewShare(s) > 0;
    });
    for (ColonyStructure& s : structures_) {
        if (s.type != "mining-station" || s.progress < 1 || s.exhausted || crewShare(s) == 0)
            continue;
        if (beesModelled_) {
            // Into its own store, packed into pods; it stops while four
            // wait inside it for its worker bee (those set down on its
            // yard, or on their way, don't count).
            if (std::count_if(pods_.begin(), pods_.end(), [&](const Pod& p) {
                    return p.origin == s.id && p.state == Pod::Queued;
                }) >= kPodsWaiting)
                continue;
            auto it = std::find_if(mineCarry_.begin(), mineCarry_.end(), [&](const auto& c) { return c.first == s.id; });
            if (it == mineCarry_.end()) {
                mineCarry_.push_back({s.id, 0});
                it = mineCarry_.end() - 1;
            }
            it->second += kMineTonnesPerSecond / kTicksPerSecond;
            Depot& d = depots_[s.id];
            while (it->second >= 1) {
                it->second -= 1;
                if (!mineTake(s, d.dug)) {
                    s.exhausted = true;
                    out.push_back({"mine-exhausted", {}, s.blockY * 8 + 4, s.blockX * 8 + 4});
                    break;
                }
            }
            pack(s.id, d.dug, false);
            continue;
        }
        if (!processor)
            continue;
        auto it = std::find_if(mineCarry_.begin(), mineCarry_.end(), [&](const auto& c) { return c.first == s.id; });
        if (it == mineCarry_.end()) {
            mineCarry_.push_back({s.id, 0});
            it = mineCarry_.end() - 1;
        }
        it->second += kMineTonnesPerSecond / kTicksPerSecond;
        while (it->second >= 1) {
            it->second -= 1;
            if (!mineTake(s, ore_)) {
                s.exhausted = true;
                out.push_back({"mine-exhausted", {}, s.blockY * 8 + 4, s.blockX * 8 + 4});
                break;
            }
        }
    }
    // Refining: one store slot a tick (ores 1-6 in the first six), a
    // quarter tonne at a time, one for one into the ore's material.
    if (refining) {
        const int slot = refineSlot_++ % kRefineSlots;
        if (slot < kOres) {
            const int64_t use = std::min(kRefinePerVisit, ore_[size_t(slot)]);
            ore_[size_t(slot)] -= use;
            // A plant for the material, standing, doubles what it makes.
            refined_[size_t(slot)] += has(kPlants[size_t(slot)]) ? 2 * use : use;
        }
        // Every 32 ticks its goods go to storage: packed into pods for the
        // bees to carry, or straight there.
        if (refineSlot_ % kShipEvery == 0) {
            if (beesModelled_) {
                for (const ColonyStructure& c : structures_)
                    if (c.type == "resource-processor" && c.progress >= 1) {
                        pack(c.id, refined_, true);
                        break;
                    }
            } else {
                deposit(refined_);
                refined_ = {};
            }
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

// The capacity: what housing adds past 90 is banked, and refills it when
// housing is lost.
void Economy::raiseCapacity(int n) {
    capacity_ += n;
    if (capacity_ > kCapacityCap)
        capBank_ += capacity_ - kCapacityCap, capacity_ = kCapacityCap;
}

void Economy::lowerCapacity(int n) {
    capacity_ -= n;
    const int refill = std::min(capBank_, std::max(0, kCapacityCap - capacity_));
    capacity_ += refill, capBank_ -= refill;
}

void Economy::remove(int id) {
    // Its upgrades and add-ons (the plants, wards, labs) go with it.
    if (const ColonyStructure* host = structure(id)) {
        const std::string type = host->type;
        const float x = host->x, y = host->y;
        std::vector<int> fitted;
        for (const ColonyStructure& c : structures_)
            if (const BuildingType* t = buildingType(c.type); t && t->upgradeOf == type && c.x == x && c.y == y)
                fitted.push_back(c.id);
        for (int f : fitted)
            remove(f);
    }
    // What it held goes with it: a storage facility's store (the first
    // stays the yards'), a processor's ore and goods.
    for (auto it = stores_.begin(); it != stores_.end();)
        if (it->structure == id && it != stores_.begin())
            it = stores_.erase(it);
        else
            (it->structure == id ? it->gone = true : false), ++it;
    // The bees docked inside it go with it (exe.md#bees): a hub's, home in
    // it, and its own worker bee waiting in its dock (a yard's too). Those
    // out on the map, or still on their way in, are let go where they are.
    // None is made again in its place (the building that would ask for
    // its worker bee is gone; a cargo bee asked for again is a free one):
    // the colony has one fewer of each (takeLostBees). A lost worker bee comes off those the
    // starting buildings were given free while any are left.
    if (const ColonyStructure* self = structure(id)) {
        const ColonyStructure* h = hub();
        const bool isHub = h && h->id == id;
        const float x = self->x, y = self->y;
        for (auto it = bees_.begin(); it != bees_.end();) {
            const bool inside = isHub ? it->home
                                      : it->worker && it->site == id && it->flight == 0 && it->dock == 0 &&
                                            it->pods.empty() && std::hypot(it->x - x, it->y - y) < 1;
            if (!inside) {
                if (isHub && it->docking)
                    it->docking = false, it->dock = 0; // (no dock to finish going in through)
                ++it;
                continue;
            }
            const int kind = it->worker ? 2 : it->cargo ? 1 : 0;
            if (kind == 2 && freeWorkers_ > 0)
                --freeWorkers_;
            else
                ++lostBees_[size_t(kind)];
            it = bees_.erase(it);
        }
    }
    // Its pods waiting go with it; those bound for it look again. Its
    // bees out let go of what they carry.
    for (Bee& b : bees_)
        if ((b.cargo || b.worker) && b.site == id)
            releaseBee(b, id);
    depots_.erase(id);
    pods_.erase(std::remove_if(pods_.begin(), pods_.end(), [&](const Pod& p) { return p.origin == id && p.state != Pod::Aboard; }),
                pods_.end());
    for (Pod& p : pods_)
        if (p.dest == id)
            p.dest = 0;
    // Its vehicle jobs go to another yard, or are given up.
    handOffJobs(id, true);
    // Its people waiting for a ride out die with it; rides to it set their
    // people down at the hub instead.
    std::vector<int> aboard;
    for (Ride& r : rides_)
        if (r.from == id && !r.loaded)
            aboard.push_back(r.id);
        else if (r.to == id)
            r.listed = false, r.toPool = true, r.to = hub() && hub()->id != id ? hub()->id : -1;
    for (int r : aboard)
        dead_ += endRide(r, false);
    for (const ColonyStructure& c : structures_)
        if (c.id == id) {
            // The ore and goods at the processor go with the last (Reluva
            // keeps them colony-wide, as its refining).
            if (c.type == "resource-processor" &&
                std::none_of(structures_.begin(), structures_.end(), [&](const ColonyStructure& o) {
                    return o.type == "resource-processor" && o.id != id && o.progress >= 1;
                }))
                ore_ = {}, refined_ = {};
            // Its people die with it (not those razing let go); those on
            // their way to it turn back to the hub's pool (those riding,
            // above).
            dead_ += c.crew + c.pool;
            for (const auto& w : c.coming)
                if (w.second < kRiding / 2)
                    walking_.push_back({std::abs(w.first), kCrewWalk});
            if (const BuildingType* t = buildingType(c.type))
                // (A ward takes its 5 only if it was finished.)
                if (t->upgradeOf.empty() || c.progress >= 1)
                    lowerCapacity(t->capacity);
        }
    structures_.erase(std::remove_if(structures_.begin(), structures_.end(),
                                     [&](const ColonyStructure& c) { return c.id == id; }),
                      structures_.end());
}

void Economy::setBees(int construction, int cargo, int worker) {
    const bool first = !beesModelled_;
    beesModelled_ = true;
    // 0 construction, 1 cargo, 2 worker bees.
    auto kindOf = [](const Bee& b) { return b.worker ? 2 : b.cargo ? 1 : 0; };
    for (int kind = 0; kind < 3; ++kind) {
        // (The worker bees the starting buildings were given free come on
        // top of those the colony asks for.)
        // (Less those lost with a building that the caller hasn't taken
        // off its counts yet.)
        const int want = std::max(0, (kind == 0 ? construction : kind == 1 ? cargo : worker + freeWorkers_) -
                                         lostBees_[size_t(kind)]);
        int have = 0;
        for (const Bee& b : bees_)
            have += kindOf(b) == kind && !(b.worker && first);
        for (; have < want; ++have) {
            Bee b;
            b.id = nextBee_++;
            b.cargo = kind == 1, b.worker = kind == 2;
            if (const ColonyStructure* h = hub())
                b.x = h->x, b.y = h->y;
            bees_.push_back(b);
        }
        // Fewer (lost): those at home go first.
        for (bool atHome : {true, false})
            for (auto it = bees_.begin(); have > want && it != bees_.end();)
                if (kindOf(*it) == kind && it->home == atHome)
                    it = bees_.erase(it), --have;
                else
                    ++it;
    }
    // The buildings standing at the start get a worker bee each, free: the
    // mines, processors and storage facilities, and the yards too (the
    // game makes them the same kind of agent; exe.md#the-economy).
    if (first)
        for (const ColonyStructure& s : structures_)
            if (s.progress >= 1 && (s.type == "mining-station" || s.type == "resource-processor" ||
                                    s.type == "storage-facility" || s.type == "vehicle-yard" ||
                                    s.type == "construction-yard")) {
                Bee b;
                b.id = nextBee_++;
                b.worker = true, b.home = false;
                b.x = s.x, b.y = s.y, b.site = s.id;
                bees_.push_back(b);
                ++freeWorkers_;
                if (s.type != "vehicle-yard" && s.type != "construction-yard")
                    depots_[s.id].worker = b.id;
            }
}

std::array<int, 3> Economy::takeLostBees() {
    return std::exchange(lostBees_, {});
}

Economy::Bee* Economy::beeById(int id) {
    for (Bee& b : bees_)
        if (b.id == id && id)
            return &b;
    return nullptr;
}

std::array<float, kOres> Economy::mineStock(int structure) const {
    std::array<float, kOres> out{};
    if (auto it = depots_.find(structure); it != depots_.end())
        for (int k = 0; k < kOres; ++k)
            out[size_t(k)] = float(it->second.dug[size_t(k)]) / float(kTonne);
    return out;
}

// A store packed into pods once it holds a pod's worth: stock / size + 1
// of them, the stock shared evenly, each material too.
void Economy::pack(int structure, std::array<int64_t, kMaterials>& stock, bool refined) {
    int64_t total = 0;
    for (int64_t v : stock)
        total += v;
    if (total < podSize())
        return;
    const int64_t n = total / podSize() + 1;
    std::array<int64_t, kMaterials> left = stock;
    for (int64_t k = 0; k < n; ++k) {
        Pod p;
        p.id = nextPod_++;
        p.refined = refined, p.origin = structure;
        for (int m = 0; m < kMaterials; ++m) {
            p.load[size_t(m)] = k + 1 < n ? stock[size_t(m)] / n : left[size_t(m)];
            left[size_t(m)] -= p.load[size_t(m)];
        }
        pods_.push_back(p);
    }
    stock = {};
}

// Who takes a pod: ore a processor, goods a storage facility; built,
// crewed, with its worker bee, fewer than 17 coming.
bool Economy::takes(const ColonyStructure& s, const Pod& p) const {
    if (s.type != (p.refined ? "storage-facility" : "resource-processor") || s.progress < 1 || s.evacuating)
        return false;
    const BuildingType* t = buildingType(s.type);
    if (t && t->crewSlots > 0 && s.crew < 1)
        return false;
    auto d = depots_.find(s.id);
    if (d == depots_.end() || !d->second.worker)
        return false;
    return std::count_if(pods_.begin(), pods_.end(), [&](const Pod& o) { return o.dest == s.id; }) < kMostIncoming;
}

// A pod at its destination: its contents into the building's store.
void Economy::unload(const Pod& p) {
    if (!p.refined) {
        for (int k = 0; k < kOres; ++k)
            ore_[size_t(k)] += p.load[size_t(k)];
        return;
    }
    for (Store& st : stores_)
        if (st.structure == p.dest && !st.gone) {
            for (int m = 0; m < kMaterials; ++m)
                st.units[size_t(m)] += p.load[size_t(m)];
            return;
        }
    deposit(p.load);
}

const Economy::DockPoints& Economy::dockOf(const ColonyStructure& s) const {
    auto [it, fresh] = dockCache_.try_emplace(s.type);
    if (fresh && docks_)
        it->second = docks_(s.type);
    return it->second;
}

namespace {
// A dock leg's ticks: its length over the speed, at least 10.
int leg(const std::array<float, 3>& a, const std::array<float, 3>& b, float speed, bool level) {
    const float d = level ? std::hypot(b[0] - a[0], b[1] - a[1])
                          : std::hypot(std::hypot(b[0] - a[0], b[1] - a[1]), b[2] - a[2]);
    return std::max(10, int(d / speed));
}
} // namespace

// In: a tick's wait for its slot, ARRIVAL to CAPTURE, the lock leg (three
// times as long; at a level dock the heights don't count), then 5 ticks.
// Out: a tick, the release leg (three times), RELEASE to DEPARTURE.
int Economy::dockIn(const ColonyStructure& s, float speed) const {
    const DockPoints& d = dockOf(s);
    if (!d.known)
        return kDockTicks;
    return 1 + leg(d.arrival, d.capture, speed, false) + 3 * leg(d.capture, d.lock, speed, !d.vertical) + 5;
}

int Economy::dockOut(const ColonyStructure& s, float speed, bool toDeparture) const {
    const DockPoints& d = dockOf(s);
    if (!d.known)
        return kDockTicks;
    return 1 + 3 * leg(d.lock, d.release, speed, !d.vertical) + (toDeparture ? leg(d.release, d.departure, speed, false) : 0);
}

const ColonyStructure* Economy::structure(int id) const {
    for (const ColonyStructure& s : structures_)
        if (s.id == id)
            return &s;
    return nullptr;
}

int Economy::dockTurn(int structure, int ticks) {
    int& free = dockFree_[structure];
    const int start = std::max(beeClock_, free);
    free = start + ticks;
    return free - beeClock_;
}

std::pair<float, float> Economy::padPlace(const ColonyStructure& s, int slot) const {
    // The quarter block the dock's RELEASE point is in (the building's
    // middle without one), and the L round its outer corner: 256 in from
    // the edges, 992 apart; places 0-8 along one edge to the corner, 9-15
    // along the other.
    const DockPoints& d = dockOf(s);
    const float px = s.x + (d.known ? d.release[0] : 0), py = s.y + (d.known ? d.release[1] : 0);
    constexpr float kBlockSize = 16384, kQuarter = 8192;
    const float bx = std::floor(px / kBlockSize) * kBlockSize, by = std::floor(py / kBlockSize) * kBlockSize;
    const bool east = px - bx >= kQuarter, north = py - by >= kQuarter;
    const float ox = bx + (east ? kQuarter : 0), oy = by + (north ? kQuarter : 0);
    const int i = std::clamp(slot, 0, kPadPlaces - 1);
    const float along = i <= 8 ? 992.0f * float(i) : 1248.0f + 992.0f * float(i - 9);
    float x = 0, y = 0;
    if (!east && !north) // the outer edges: west and south
        std::tie(x, y) = i <= 8 ? std::pair{256.0f, 8192 - along} : std::pair{1248.0f + 992.0f * float(i - 9), 256.0f};
    else if (east && !north) // south and east
        std::tie(x, y) = i <= 8 ? std::pair{along, 256.0f} : std::pair{7936.0f, 1248.0f + 992.0f * float(i - 9)};
    else if (east && north) // east and north
        std::tie(x, y) = i <= 8 ? std::pair{7936.0f, along} : std::pair{6944.0f - 992.0f * float(i - 9), 7936.0f};
    else // north and west
        std::tie(x, y) = i <= 8 ? std::pair{8192 - along, 7936.0f} : std::pair{256.0f, 6944.0f - 992.0f * float(i - 9)};
    return {ox + x, oy + y};
}

// The pods' tick: each building's worker bee (given one, or asked of
// the yard), flying its pods out to its pad; the pods offered to whoever
// takes them; cargo bees for buildings with more than one coming,
// bringing two a trip.
void Economy::podTick() {
    auto find = [&](int id) -> const ColonyStructure* {
        for (const ColonyStructure& s : structures_)
            if (s.id == id)
                return &s;
        return nullptr;
    };
    auto at = [](const Bee& b, float x, float y) { return std::hypot(b.x - x, b.y - y) < 1; };
    auto flyWorker = [&](Bee& b, float x, float y) {
        fly(b, x, y);
        b.flight = std::max(10, int(std::hypot(x - b.fromX, y - b.fromY) / kWorkerSpeed));
    };
    // Worker bees for the buildings that handle pods (not one being razed:
    // the game's searches pass it over, exe.md#demolishing).
    for (const ColonyStructure& s : structures_) {
        if (s.progress < 1 || s.evacuating ||
            (s.type != "mining-station" && s.type != "resource-processor" && s.type != "storage-facility"))
            continue;
        Depot& d = depots_[s.id];
        if (const Bee* w = beeById(d.worker); !w || w->site != s.id)
            d.worker = 0;
        if (d.worker)
            continue;
        for (Bee& b : bees_)
            if (b.worker && !b.site && b.flight == 0) {
                b.site = s.id, d.worker = b.id;
                d.asked = false; // (so it asks again if this one's lost)
                flyWorker(b, s.x, s.y);
                break;
            }
        // None spare: one asked of the vehicle yard (the game's builds it;
        // each refusal reported, at most every 600 ms).
        if (!d.worker && !d.asked && !free_)
            for (const VehicleType& v : vehicleTypes())
                if (v.side == side_ && (v.type == 12 || v.type == 11 || v.type == 92) && vehicleAvailable(v) && orderVehicle(v)) {
                    d.asked = true;
                    break;
                }
    }
    // A building's yard: its places taken (by pods standing there or being
    // set down there), and the pods on their way to it.
    auto yardUsed = [&](int id) {
        std::array<bool, kPadPlaces> used{};
        for (const Pod& p : pods_)
            if (p.pad == id && p.slot >= 0)
                used[size_t(p.slot)] = true;
        return used;
    };
    auto freePlace = [&](int id) { // the highest free place (the game fills them from the far end), or -1
        const auto used = yardUsed(id);
        for (int k = kPadPlaces - 1; k >= 0; --k)
            if (!used[size_t(k)])
                return k;
        return -1;
    };
    auto freeCount = [&](int id) {
        const auto used = yardUsed(id);
        return int(std::count(used.begin(), used.end(), false));
    };
    auto podById = [&](int id) -> Pod* {
        for (Pod& q : pods_)
            if (q.id == id)
                return &q;
        return nullptr;
    };
    // Each worker bee (exe.md#bees): docked in its building, it takes in a
    // pod set down on its yard (out of the dock, to the pod, a tick to take
    // it, back and in: its contents go into the store), or takes one of its
    // own out (with two places free more than are coming): out of the dock,
    // to the highest free place, a tick and a 30-tick lift to set it down,
    // back and in.
    for (Bee& w : bees_) {
        if (!w.worker || !w.site || w.flight || w.dock)
            continue;
        const ColonyStructure* s = find(w.site);
        if (!s) {
            w.site = 0, w.inside = false;
            continue;
        }
        if (w.fetching) { // at the pod it went for
            Pod* p = podById(w.fetching);
            w.fetching = 0;
            if (p && p->state == Pod::Parked && at(w, p->x, p->y)) {
                p->state = Pod::Aboard, p->inbound = true, p->pad = 0, p->slot = -1;
                w.pods = {p->id};
                w.dock = 1;
            }
            flyWorker(w, s->x, s->y);
            continue;
        }
        if (!w.pods.empty()) {
            Pod* p = podById(w.pods[0]);
            if (!p) {
                w.pods.clear();
            } else if (p->inbound) {
                if (at(w, s->x, s->y)) { // in through the dock with it
                    w.dock = dockTurn(s->id, dockIn(*s, kWorkerSpeed));
                    w.inside = true;
                    w.pods.clear();
                    for (auto it = pods_.begin(); it != pods_.end(); ++it)
                        if (it->id == p->id) {
                            unload(*it);
                            pods_.erase(it);
                            break;
                        }
                } else {
                    flyWorker(w, s->x, s->y);
                }
            } else if (at(w, p->x, p->y)) { // set down on its place
                p->state = Pod::Parked;
                w.pods.clear();
                w.dock = kWorkerSetDown;
                flyWorker(w, s->x, s->y);
            }
            continue;
        }
        if (!at(w, s->x, s->y)) {
            flyWorker(w, s->x, s->y);
            continue;
        }
        if (!w.inside) {
            w.dock = dockTurn(s->id, dockIn(*s, kWorkerSpeed));
            w.inside = true;
            continue;
        }
        // A pod for it on its yard first.
        Pod* in = nullptr;
        for (Pod& p : pods_)
            if (p.state == Pod::Parked && p.pad == s->id && p.dest == s->id) {
                in = &p;
                break;
            }
        if (in) {
            w.inside = false;
            w.fetching = in->id;
            flyWorker(w, in->x, in->y);
            w.dock = dockTurn(s->id, dockOut(*s, kWorkerSpeed));
            continue;
        }
        const int coming = int(std::count_if(pods_.begin(), pods_.end(),
                                             [&](const Pod& p) { return p.dest == s->id && p.pad != s->id; }));
        if (freeCount(s->id) - coming < 2)
            continue;
        for (Pod& p : pods_)
            if (p.origin == s->id && p.state == Pod::Queued) {
                p.state = Pod::Aboard;
                p.pad = s->id, p.slot = freePlace(s->id);
                std::tie(p.x, p.y) = padPlace(*s, p.slot);
                w.pods = {p.id};
                w.inside = false;
                flyWorker(w, p.x, p.y);
                w.dock = dockTurn(s->id, dockOut(*s, kWorkerSpeed));
                break;
            }
    }
    // The pods offered: the first building in the list that takes each.
    for (Pod& p : pods_) {
        if (p.dest && !find(p.dest))
            p.dest = 0;
        if (!p.dest && p.state != Pod::Aboard)
            for (const ColonyStructure& s : structures_)
                if (takes(s, p)) {
                    p.dest = s.id;
                    break;
                }
    }
    // A building with more than one coming asks for a cargo bee.
    for (const ColonyStructure& s : structures_) {
        auto dit = depots_.find(s.id);
        if (dit == depots_.end())
            continue;
        Depot& d = dit->second;
        if (const Bee* c = beeById(d.cargo); !c || c->site != s.id)
            d.cargo = 0;
        if (d.cargo || std::count_if(pods_.begin(), pods_.end(), [&](const Pod& p) { return p.dest == s.id; }) < 2)
            continue;
        for (Bee& b : bees_)
            if (b.cargo && !b.site && b.dock == 0) {
                b.site = s.id, d.cargo = b.id;
                break;
            }
    }
    // Each cargo bee (exe.md#bees): it waits (checking every 16 ticks)
    // until its building's yard has room and pods enough for its bays (the
    // Federation's two, the others' one) are ready (after 64 checks it goes
    // for one); flies to them, takes up to its bays' worth (a
    // tick each, a 50-tick climb), flies to its building's yard and sets
    // each down (a tick, a 30-tick lift, a 50-tick climb), for the
    // building's worker bee to take in. It never docks there.
    for (Bee& b : bees_) {
        if (!b.cargo || !b.site || b.flight || b.dock)
            continue;
        const ColonyStructure* s = find(b.site);
        if (!s) {
            b.site = 0;
            continue;
        }
        if (!b.pods.empty()) {
            const auto [cx, cy] = padPlace(*s, 8);
            if (!at(b, cx, cy)) {
                fly(b, cx, cy);
                continue;
            }
            if (freeCount(s->id) < int(b.pods.size())) {
                b.dock = kCargoCheck;
                continue;
            }
            for (int id : b.pods)
                if (Pod* p = podById(id)) {
                    p->state = Pod::Parked, p->pad = s->id, p->slot = freePlace(s->id);
                    std::tie(p->x, p->y) = padPlace(*s, p->slot);
                }
            b.dock = kCargoSetDown * int(b.pods.size());
            b.pods.clear();
            continue;
        }
        std::vector<Pod*> ready;
        for (Pod& p : pods_)
            if (p.dest == s->id && p.state == Pod::Parked && p.pad != s->id)
                ready.push_back(&p);
        if (ready.empty()) {
            // Nothing ready: free again, unless more are on their way.
            if (std::none_of(pods_.begin(), pods_.end(), [&](const Pod& p) { return p.dest == s->id && p.pad != s->id; }))
                b.site = 0, depots_[s->id].cargo = 0, b.retries = 0;
            continue;
        }
        const int origin = ready[0]->pad;
        const bool full = std::count_if(ready.begin(), ready.end(), [&](const Pod* p) { return p->pad == origin; }) >= cargoBays();
        if (freeCount(s->id) < cargoBays() || (!full && b.retries < kCargoChecks)) {
            ++b.retries;
            b.dock = kCargoCheck;
            continue;
        }
        if (!at(b, ready[0]->x, ready[0]->y)) {
            fly(b, ready[0]->x, ready[0]->y);
            continue;
        }
        for (Pod* p : ready)
            if (int(b.pods.size()) < cargoBays() && p->pad == origin) {
                p->state = Pod::Aboard, p->pad = 0, p->slot = -1;
                b.pods.push_back(p->id);
            }
        b.retries = 0;
        b.dock = kCargoTakeUp * int(b.pods.size());
    }
    // Pods aboard move with their bee.
    for (const Bee& b : bees_)
        for (int id : b.pods)
            for (Pod& p : pods_)
                if (p.id == id)
                    p.x = b.x, p.y = b.y;
}

int Economy::beesAt(int site) const {
    return int(std::count_if(bees_.begin(), bees_.end(),
                             [&](const Bee& b) { return !b.cargo && !b.worker && b.site == site && b.working; }));
}

const ColonyStructure* Economy::hub() const {
    for (const ColonyStructure& s : structures_)
        if (s.type == "colony-hub" && s.progress >= 1)
            return &s;
    return nullptr;
}

void Economy::fly(Bee& b, float x, float y) {
    const ColonyStructure* h = b.home ? hub() : nullptr;
    if (h)
        b.x = h->x, b.y = h->y; // out of the hub
    b.home = false, b.working = false, b.docking = false;
    b.fromX = b.x, b.fromY = b.y, b.toX = x, b.toY = y;
    b.flown = 0;
    b.flight = std::max(kShortestFlight, int(std::hypot(x - b.x, y - b.y) / (b.cargo ? kCargoSpeed : kBeeSpeed)));
    if (x != b.x || y != b.y)
        b.heading = std::atan2(y - b.y, x - b.x);
    // Out through the hub's dock first, in its turn.
    if (h)
        b.dock = dockTurn(h->id, dockOut(*h, b.worker ? kWorkerSpeed : b.cargo ? kCargoSpeed : kBeeSpeed));
}

// A tick of the bees: flights on, then each idle one given work: a site
// short of bees, a mine without a cargo bee, or home.
void Economy::beeTick() {
    const ColonyStructure* h = hub();
    auto find = [&](int id) -> const ColonyStructure* {
        for (const ColonyStructure& s : structures_)
            if (s.id == id)
                return &s;
        return nullptr;
    };
    ++beeClock_;
    for (Bee& b : bees_) {
        if (b.home) {
            if (b.dock > 0)
                --b.dock; // (a cargo bee waiting in the hub for its pods)
            continue;
        }
        if (b.dock > 0) {
            if (--b.dock == 0 && b.docking)
                b.home = true, b.docking = false; // in through the hub's dock
            continue;
        }
        if (b.docking)
            continue;
        if (b.flight == 0)
            continue;
        // Speeding up and slowing down along the way (the game's curve).
        const float f = float(++b.flown) / float(b.flight), e = f * f * (3 - 2 * f);
        b.x = b.fromX + (b.toX - b.fromX) * e, b.y = b.fromY + (b.toY - b.fromY) * e;
        if (b.flown < b.flight)
            continue;
        b.flight = 0;
        // Home: in through the hub's dock, in its turn.
        auto dockHome = [&] {
            b.dock = dockTurn(h->id, dockIn(*h, b.worker ? kWorkerSpeed : b.cargo ? kCargoSpeed : kBeeSpeed));
            b.docking = true;
        };
        if (b.cargo || b.worker) {
            if (!b.site && h && std::hypot(b.x - h->x, b.y - h->y) < 1)
                dockHome();
        } else if (b.site) {
            b.working = true;
        } else if (h && std::hypot(b.x - h->x, b.y - h->y) < 1) {
            dockHome();
        }
    }
    // A site gone (or done) lets its bees go.
    for (Bee& b : bees_)
        if (!b.cargo && !b.worker && b.site)
            if (const ColonyStructure* s = find(b.site); !s || s->progress >= 1)
                b.site = 0, b.working = false;
    // The construction bees: each site, first ordered first, takes up to
    // four, at its free corners.
    for (const ColonyStructure& s : structures_) {
        if (s.progress >= 1)
            continue;
        int at = 0;
        std::array<bool, kBeesASite> taken{};
        for (const Bee& b : bees_)
            if (!b.cargo && !b.worker && b.site == s.id)
                ++at, taken[size_t(b.spot)] = true;
        for (Bee& b : bees_) {
            if (at >= kBeesASite)
                break;
            if (b.cargo || b.worker || b.site)
                continue;
            b.site = s.id;
            b.spot = int(std::find(taken.begin(), taken.end(), false) - taken.begin());
            taken[size_t(b.spot)] = true;
            ++at;
            fly(b, s.x + (b.spot & 1 ? kBeeSpot : -kBeeSpot), s.y + (b.spot & 2 ? kBeeSpot : -kBeeSpot));
        }
    }
    podTick();
    // Idle ones home.
    for (Bee& b : bees_)
        if (!b.site && !b.home && !b.docking && b.flight == 0 && b.dock == 0 && h)
            fly(b, h->x, h->y);
}

bool Economy::spend(int structure, float power) {
    for (ColonyStructure& s : structures_)
        if (s.id == structure) {
            if (s.progress < 1 || s.stored < power)
                return false;
            s.stored -= power;
            return true;
        }
    return true; // (not the economy's)
}

bool Economy::razable(int structure) const {
    for (const ColonyStructure& s : structures_)
        if (s.id == structure)
            return s.progress >= 1 && !s.evacuating && s.type != "colony-hub" && s.type != "storage-facility" &&
                   buildingType(s.type) && buildingType(s.type)->upgradeOf.empty();
    return false;
}

int Economy::pendingBuilds() const {
    return int(std::count_if(structures_.begin(), structures_.end(), [](const ColonyStructure& s) { return s.progress < 1; }));
}

bool Economy::cancel(int structure) {
    const ColonyStructure* s = this->structure(structure);
    if (!s || s->progress >= 1)
        return false;
    if (!free_)
        if (const BuildingType* t = buildingType(s->type))
            deposit(unitsOf(t->costFor(side_)));
    for (Bee& b : bees_)
        if (b.site == structure)
            b.site = 0, b.working = false;
    remove(structure);
    return true;
}

// The game's raze: half the type's cost back at once (in whole units,
// rounded down; what was spent on upgrades isn't), then the crew leave.
bool Economy::raze(int structure) {
    if (!razable(structure))
        return false;
    for (ColonyStructure& s : structures_)
        if (s.id == structure) {
            Units back = unitsOf(buildingType(s.type)->costFor(side_));
            for (int64_t& u : back)
                u /= 2;
            deposit(back);
            s.evacuating = true, s.evacTicks = 0;
            // Its people walk out to the hub's pool; those still on their
            // way turn back.
            int leaving = s.crew;
            for (const auto& c : s.coming)
                leaving += c.second >= kRiding / 2 ? 0 : std::abs(c.first);
            s.coming.clear();
            // Those riding to it go to the hub instead; those it was to
            // send off walk out with the rest (their building asking again).
            std::vector<int> unsent;
            for (Ride& r : rides_) {
                if (r.from == s.id && !r.loaded)
                    unsent.push_back(r.id);
                else if (r.to == s.id)
                    r.listed = false, r.toPool = true, r.to = hub() ? hub()->id : -1;
            }
            const int id = s.id, housed = s.pool;
            const float x = s.x, y = s.y;
            s.crew = 0, s.target = 0, s.pool = 0;
            // Its jobs still in work go to another yard (its crew gone, they
            // couldn't finish here).
            handOffJobs(id, false);
            for (int r : unsent)
                leaving += endRide(r, false);
            // With transports, an unarmed APC comes for them and takes them
            // to the hub's pool; the building waits for it to go
            // (exe.md#demolishing). Else they walk.
            if (leaving > 0 && transports_ && hub())
                addRides(id, hub()->id, leaving, true, false);
            else if (leaving > 0)
                walking_.push_back({leaving, kCrewWalk});
            // Those housed in its pool (an accommodation block's) leave too:
            // for the hub's pool if it has room for them, else another
            // block's with room, else the hub's all the same; by transport,
            // or walking, as its crew.
            if (housed > 0) {
                const ColonyStructure* to = hub();
                if (!to || buildingType(to->type)->poolSlots - to->pool < housed)
                    for (const ColonyStructure& b : structures_)
                        if (b.type == "accommodation" && b.id != id && b.progress >= 1 && !b.evacuating &&
                            buildingType(b.type)->poolSlots - b.pool >= housed) {
                            to = &b;
                            break;
                        }
                if (transports_ && to)
                    addRides(id, to->id, housed, true, false);
                else if (ColonyStructure* b = to && to->type == "accommodation" ? structureById(to->id) : nullptr)
                    b->coming.push_back({-housed, std::hypot(double(b->x - x), double(b->y - y)) / kCrewSpeed});
                else
                    walking_.push_back({housed, kCrewWalk});
            }
            // Its bees fly off.
            if (auto d = depots_.find(s.id); d != depots_.end()) {
                for (int id : {d->second.worker, d->second.cargo})
                    if (Bee* b = beeById(id); b && b->site == s.id)
                        releaseBee(*b, s.id);
                d->second.worker = d->second.cargo = 0;
            }
        }
    return true;
}

// A bee let go by its building (the game's pods in transit there weren't
// traced; Reluva's): a worker bee's pod, out from its store or in to it,
// goes with the building, as the pods waiting on its yard do; a cargo
// bee's, bound for it from another yard, are set down where the bee is,
// off any yard, for whoever takes them next. Then it's free (home, or
// another building's).
void Economy::releaseBee(Bee& b, int structure) {
    for (int id : b.pods)
        for (auto it = pods_.begin(); it != pods_.end(); ++it)
            if (it->id == id) {
                if (!b.cargo) {
                    pods_.erase(it);
                    break;
                }
                it->state = Pod::Parked, it->inbound = false, it->pad = 0, it->slot = -1;
                it->x = b.x, it->y = b.y;
                if (it->dest == structure)
                    it->dest = 0;
                break;
            }
    b.pods.clear();
    b.site = 0, b.fetching = 0, b.inside = false, b.retries = 0;
}

// Evacuated: its people out, no bee of its still docked there, and a few
// ticks gone (the game asks each of its parts each tick whether it can
// let go).
bool Economy::evacuated(int structure) const {
    for (const ColonyStructure& s : structures_)
        if (s.id == structure) {
            if (!s.evacuating || s.crew > 0 || s.pool > 0 || !s.coming.empty() || s.evacTicks < kEvacTicks)
                return false;
            // A yard's vehicle still rolling out (exe.md#demolishing: its
            // jobs finish first).
            if (std::any_of(jobs_.begin(), jobs_.end(), [&](const VehicleJob& j) { return j.yard == s.id; }))
                return false;
            // Its people still to be taken out.
            if (std::any_of(rides_.begin(), rides_.end(), [&](const Ride& r) { return r.from == s.id && !r.loaded; }))
                return false;
            return std::none_of(bees_.begin(), bees_.end(), [&](const Bee& b) {
                return (b.worker || b.cargo) && b.flight == 0 && std::hypot(b.x - s.x, b.y - s.y) < 1;
            });
        }
    return false;
}

void Economy::syncState(save::Serializer& s) {
    s(stores_)(refined_)(capacity_)(capBank_)(freeWorkers_)(vehicles_)(structures_)(ore_)(dug_)(mineCarry_)(oreCycle_)(refineSlot_);
    s(tickClock_)(clock_)(lastPowerWarning_)(reserve_)(requested_)(met_)(consumption_)(pendingShort_)(jobs_);
    s(finished_)(officers_)(yardRunning_)(orbit_)(officersDown_)(life_)(lifeDraw_)(lifeProduced_)(lifeUsed_);
    s(lifeMet_)(dead_)(walking_)(transports_)(rides_)(nextRide_)(lastShortage_)(nextId_)(dockFree_);
    s(beeClock_)(free_)(beesModelled_)(bees_)(nextBee_)(nextPod_)(pods_)(depots_)(lostBees_);
}

} // namespace reluva::newworlds
