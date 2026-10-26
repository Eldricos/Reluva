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
// A material by a script's name for it ("dilithium", "polycomposite",
// "polycomp", "transalumin", ...); -1 if none.
int materialNamed(std::string_view name);
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
    std::array<bool, 3> sides{true, true, true}; // the sides that have it (a lab is one side's)
    // Power, in the game's units a tick: a generator's output at full
    // efficiency (per side), or a consumer's buffer (capacity, intake a
    // tick) and its standing use a tick.
    std::array<float, 3> output{};
    float bufferCap = 0, intake = 0, demand = 0;
    int capacity = 0;         // colony capacity it adds once built
    // Its efficiency's terms (exe.md#crew-and-efficiency): the officer
    // speciality it wants (0 none: the officer term is then whole) and
    // whether its crew counts (generators, hubs, pads, housing, farms and
    // defences work whole without).
    int speciality = 0;
    bool crewless = true;
    // People: the crew it needs to work at all, the crew it takes (its
    // slots), and the people it can house in its pool (hub 150,
    // accommodation 200).
    int required = 0, crewSlots = 0, poolSlots = 0;
    double buildSeconds = 40; // order to finished
    // The Klingons' and Romulans' where their models build faster or
    // slower (0: the same).
    double kliSeconds = 0, romSeconds = 0;
    double secondsFor(char side) const {
        return side == 'K' && kliSeconds > 0 ? kliSeconds : side == 'R' && romSeconds > 0 ? romSeconds : buildSeconds;
    }
    const Stock& costFor(char side) const;
    const std::vector<std::string>& needsFor(char side) const;
};
const std::vector<BuildingType>& buildingTypes();
const BuildingType* buildingType(std::string_view id);

// A vehicle the vehicle yard makes (exe.md#vehicle-production): the side's,
// by the game's type id, its cost, what must stand first (buildings,
// upgrades, the science station's labs) and its build value: the yard
// works value x 300 units of power into it, at its efficiency.
struct VehicleType {
    int type;
    char side;
    const char* panel; // "PHASER TANK"
    Stock cost;
    std::vector<std::string> needs;
    int value;
    bool bee() const;  // goes into the hub, not out on the map
};
const std::vector<VehicleType>& vehicleTypes();
const VehicleType* vehicleType(int type);

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
    float fill = 1;      // the buffer's fill, averaged (for efficiency)
    float supply = 1;    // the share of what it asked that it got, averaged (its panel's SUPPLY)
    float health = 1;    // its hit points' share, 0-1
    int crew = 0;        // at work in it
    int pool = 0;        // housed in it (hub, accommodation)
    int wanted = 0;      // crew asked for and not yet sent
    int target = -1;     // the crew the player wants in it (-1: all its slots)
    bool evacuating = false; // being razed: its people leaving
    int evacTicks = 0;       // ticks since
    bool beaming = false;    // its crew on their way down from orbit
    // People on their way to it: how many, and seconds still to walk
    // (kRiding: in, or waiting for, one of the colony's transports).
    std::vector<std::pair<int, double>> coming;
};

class Economy {
public:
    Economy(char side, Stock start, int capacity, ColonyMap map);

    // The stock as the panel shows it: what the colony's storage
    // facilities hold between them (exe.md#stores). Each keeps its own; the
    // mission's stock goes into the first, as do refunds and what the
    // script gives, and the processor sends its refined goods there every
    // 32 ticks. Costs are taken from them in turn; a vehicle's only from
    // the first storage facility the colony ever had (still, after it's
    // gone: then outside the stock). A storage facility lost takes what it
    // held with it, as a processor does its ore and goods. (Until the
    // colony has one, the mission's stock waits for it, counted.)
    Stock stock() const;
    Stock storeOf(int structure) const; // a storage facility's own
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
    float consumption() const { return consumption_; } // the share of what's made that was drawn
    float reserve() const { return float(reserve_ / kReserveCap); }
    const std::vector<ColonyStructure>& structures() const { return structures_; }
    // Raw ore waiting at the processor, in tonnes, by ore kind - 1.
    std::array<float, kOres> oreStock() const;

    // An AI side's colony (exe.md#ai-colonies): what it builds and the
    // vehicles its yards make cost it nothing, and its build orders don't
    // wait for what a building needs or for room in the colony.
    void setFree(bool free) { free_ = free; }
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
    // Stock given (or taken, negative) by the mission, in panel units;
    // never below nothing.
    void give(int material, int amount);
    // A building destroyed: it no longer counts, works or draws power.
    void remove(int id);
    // Demolishing (exe.md#demolishing): whether a building may be razed
    // (built, not the hub or a storage facility, not already evacuating);
    // the order: half its type's cost back at once (upgrades not), its
    // crew sent out. Once it's empty (evacuated()), it's destroyed.
    // A defensive turret's shot: power taken from its buffer, if it holds
    // that much (exe.md#sides-and-combat); false, and nothing taken, if not.
    bool spend(int structure, float power);
    bool razable(int structure) const;
    bool raze(int structure);
    // A building or upgrade still going up cancelled from the colony's
    // details (exe.md#demolishing: a yes / no, then gone; Reluva gives its
    // cost back, as an aborted build order's). False if it isn't a site.
    bool cancel(int structure);
    // Sites and upgrades not finished: the game takes no more orders at 10.
    int pendingBuilds() const;
    bool evacuated(int structure) const;

    // Vehicles: the side's that the menu offers now (their yard and what
    // they need standing), and an order for one (the cost taken at once;
    // false, and the colony told, if it can't be paid; false if the colony
    // is at its size limit). The yard's work is shown as 0-1.
    std::vector<const VehicleType*> vehicleMenu() const;
    bool vehicleAvailable(const VehicleType& v) const;
    bool orderVehicle(const VehicleType& v);
    struct VehicleJob {
        int type;
        int yard;          // the vehicle yard's structure id
        double work;       // power units still to put in
        double total;
        double rollout = 0; // seconds left rolling out, once done
    };
    const std::vector<VehicleJob>& vehicleJobs() const { return jobs_; }
    // Vehicles finished since last asked: type id and the yard's id.
    std::vector<std::pair<int, int>> takeVehicles();
    // Whether a vehicle yard is working (between its start and stop
    // efficiencies, as it was); its panel shows SUSPENDED when not.
    bool yardRunning(int structure) const {
        auto it = yardRunning_.find(structure);
        return it == yardRunning_.end() || it->second;
    }
    // A building's best officer skill at its speciality (0 none): the
    // officer term of its efficiency.
    void setOfficer(int structure, float skill);
    // People (exe.md#crew-and-efficiency): the side's roster of 150, of
    // whom the officers are some; those not yet on the planet are in
    // orbit. Call before the colony's buildings are added.
    void setOfficers(int n);
    int inOrbit() const { return orbit_; }
    int people() const;  // on the planet: pools, crews and officers
    int pool() const;    // in the hub's and accommodation's pools
    int crew() const;    // at work in the buildings
    // The life-support pool's share full (0-1): the hub keeps it up with
    // power, 16 units a person a tick; farms add to it.
    float lifeSupport() const { return float(life_ / kLifeCap); }
    float lifeSupportPower() const { return lifeDraw_; } // power a tick it took
    // The life-support pool as a farm's panel shows it (its producer
    // block): what was drawn over what the farms made last tick, the share
    // of the hub's buying back granted.
    float lifeConsumption() const { return lifeProduced_ > 0 ? float(std::min(1.0, lifeUsed_ / lifeProduced_)) : 0; }
    float lifeDemandMet() const { return lifeMet_; }
    // People who died with what they were in (the sickbay's DEAD).
    int dead() const { return dead_; }
    // One of the colony's plain crew killed (by disease): out of a pool
    // first, else a building's crew.
    void loseOne();
    // People walking back to the hub's pool from a building razed.
    int walking() const;
    // The crew the player wants in a building (the panel's assign
    // control, 0 to its slots): more are asked for, those over it go back
    // to the hub's pool.
    void setCrewTarget(int structure, int n);
    // A building's hit points' share (its health term), from the map.
    void setHealth(int structure, float share);
    // A building's efficiency, as the game's: 0.4 x (health + power fill
    // + officer) x crew. Health is a step table (whole from 75 %), the
    // power fill its buffer's (whole for generators and the hub).
    float efficiency(const ColonyStructure& s) const;

    // Bees (exe.md#bees), once set (an AI's colony, and one never told,
    // builds at each type's pace as before): the colony's construction
    // bees live in the hub and fly out to a site's four spots, up to four
    // to a site, the first sites ordered first; each at work puts a
    // quarter of the type's pace in (so one builds it four times slower
    // than four, as the training missions' times are with four). A site
    // done, its bees go on to one still short of them, or home. The cargo
    // bees shuttle between the working mines and the processor, docking
    // at each; a mine with none serving it sends nothing. Bees fly
    // straight, 150 units a tick, a flight at least 10 ticks.
    void setBees(int construction, int cargo, int worker = 0);

    // The colony's transports (exe.md#transports), once switched on (the
    // player's colony; an AI's people all walk): the unarmed APCs that live
    // in the hub ferry people in tens. A building's crew from an
    // accommodation block, or beamed down to the transporter pad, rides
    // from there; those over a lowered crew target ride back to a hub or
    // block with room. The hub's own pool walks, as before. Each ride waits
    // for a transport; the caller drives them and says when one has taken
    // its people on, set them down, or was lost with them.
    static constexpr double kRiding = 1e18;
    struct Ride {
        int id = 0;
        int from = 0, to = 0; // structures
        int people = 0;
        bool toPool = false;  // into its pool, else its crew
        bool listed = false;  // counted in to's coming (else on its own, as walking)
        bool loaded = false;
        double wait = 0;      // seconds before they're there to take (beaming down)
    };
    void setTransports(bool on) { transports_ = on; }
    // The colony's finished hub, if it has one.
    const ColonyStructure* hub() const;
    const std::vector<Ride>& rides() const { return rides_; }
    void rideLoaded(int ride);
    void rideDelivered(int ride);
    void rideLost(int ride);
    struct Bee {
        int id = 0;
        bool cargo = false;
        bool worker = false;  // a worker bee: its building's pods out to its pad
        bool home = true;     // in the hub (not drawn)
        float x = 0, y = 0;   // where it is
        float heading = 0;
        int site = 0;         // the site it builds, or the mine it serves (0 none)
        int spot = 0;         // its corner of the site (0-3)
        bool working = false; // at its spot, building
        bool loaded = false;  // a cargo bee on its way to the processor
        int dock = 0;         // ticks still docked (or waiting: it doesn't fly meanwhile)
        bool inside = false;  // a worker bee: docked in its building
        bool docking = false; // going into the hub (home when its dock is done)
        int fetching = 0;     // a worker bee: the pod on its yard it's on its way to take in
        int retries = 0;      // a cargo bee: checks spent waiting for a second pod
        float fromX = 0, fromY = 0, toX = 0, toY = 0;
        int flown = 0, flight = 0; // ticks: 0 flight when not flying
        std::vector<int> pods;     // the pods aboard (a worker's one, a cargo bee's two)
    };
    const std::vector<Bee>& bees() const { return bees_; }
    int beesAt(int site) const; // construction bees at work on a site

    // Cargo pods (exe.md#bees), with bees: a mine packs what it digs into
    // pods (80 t the Federation's, 160 t the Klingons' and Romulans'; its
    // stock split evenly over stock / size + 1 of them) once it holds a
    // pod's worth, and stops while four wait; every mine, processor and
    // storage facility has a worker bee of its own that flies its pods out
    // to a pad round it; each pod goes to the first processor (ore) or
    // storage facility (goods) that takes it, and a building with more
    // than one coming asks for a cargo bee, which brings two a trip and
    // puts their contents in its store. The processor packs its refined
    // goods into pods every 32 ticks. Without bees (the computer's
    // colonies) the ore and goods go straight on.
    struct Pod {
        int id = 0;
        std::array<int64_t, kMaterials> load{}; // ore by kind - 1, or materials
        bool refined = false;
        int origin = 0, dest = 0; // structures (dest 0: no taker yet)
        enum State { Queued, Parked, Aboard } state = Queued;
        float x = 0, y = 0;       // parked: where
        int pad = 0;              // the building whose yard it stands on (or is being set down on)
        int slot = -1;            // its place there (-1 none)
        bool inbound = false;     // aboard a worker bee taking it in to its building
    };
    const std::vector<Pod>& pods() const { return pods_; }

    // Docks (exe.md#docking): each building's from its model's dock
    // points (from its middle, world units): a vehicle flies in from
    // ARRIVAL to CAPTURE and LOCK, out to RELEASE and DEPARTURE, each leg
    // its distance over the vehicle's speed (at least 10 ticks; the lock
    // and release legs three times that). One vehicle at a time. Without
    // them, 20 ticks each way.
    struct DockPoints {
        bool known = false, vertical = false;
        std::array<float, 3> arrival{}, capture{}, lock{}, release{}, departure{};
    };
    void setDocks(std::function<DockPoints(const std::string& type)> docks) {
        docks_ = std::move(docks);
        dockCache_.clear();
    }
    int dockIn(const ColonyStructure& s, float speed) const;
    // (An unarmed APC stops at RELEASE: toDeparture false.)
    int dockOut(const ColonyStructure& s, float speed, bool toDeparture = true) const;
    const ColonyStructure* structure(int id) const;
    // A mine's, processor's or storage facility's yard (exe.md#bees): its
    // 16 places, an L along the outer edges of the quarter block its dock
    // lets out into.
    std::pair<float, float> padPlace(const ColonyStructure& s, int slot) const;
    // A mine's own store: the ore it has dug and not yet packed, tonnes by
    // ore kind - 1.
    std::array<float, kOres> mineStock(int structure) const;

    // Moves time on: construction, power, mining and refining. Returns what
    // happened, as the mission scripts' events (built <type>, power-low,
    // short-resources, mine-exhausted at its position).
    std::vector<ScriptEvent> advance(double seconds);

private:
    static constexpr double kReserveCap = 640000;
    static constexpr double kLifeCap = 655360;
    void requestCrew(ColonyStructure& s);
    void staff(ColonyStructure& s, std::vector<ScriptEvent>& out);
    void retryCrew();
    void tick(std::vector<ScriptEvent>& out);
    void produce(std::vector<ScriptEvent>& out);
    float crewShare(const ColonyStructure& s) const;
    int sideIndex() const;
    bool mineTake(ColonyStructure& mine, std::array<int64_t, kOres>& into); // one mining take; false when dry
    void beeTick();
    void podTick();
    void pack(int structure, std::array<int64_t, kMaterials>& stock, bool refined);
    bool takes(const ColonyStructure& s, const Pod& p) const;
    void unload(const Pod& p);
    Bee* beeById(int id);
    int64_t podSize() const { return side_ == 'F' ? 80000 : 160000; }
    // A cargo bee's bays: the Federation's two, the Klingons' and Romulans' one
    // (so every cargo bee carries 160 T a trip).
    int cargoBays() const { return side_ == 'F' ? 2 : 1; }
    void fly(Bee& b, float x, float y);

    // The stores, in the game's units.
    using Units = std::array<int64_t, kMaterials>;
    struct Store {
        int structure; // its storage facility (0: the mission's stock, before one stands)
        bool gone = false; // its facility destroyed (kept only as the yards' store)
        Units units{};
    };
    int64_t total(int m) const;
    bool pays(const Units& cost) const;
    void take(Units cost);     // in turn from the stores
    void deposit(const Units& u); // into the first standing store (none: lost)
    void addStore(int structure);
    Store* yardStore();       // the first storage facility's, where vehicles are paid from
    static Units unitsOf(const Stock& s);

    char side_;
    int powerClass(std::string_view type) const;
    std::vector<Store> stores_;
    Units refined_{}; // at the processor, waiting to go to storage
    int capacity_, vehicles_ = 0;
    ColonyMap map_;
    std::vector<ColonyStructure> structures_;
    std::array<int64_t, kOres> ore_{}; // raw ore at the processor, game units
    std::map<std::pair<int, int>, int> dug_; // blocks' amounts taken so far
    std::vector<std::pair<int, double>> mineCarry_; // per mine id: tonnes owed by the bees
    int oreCycle_ = 0, refineSlot_ = 0;
    double tickClock_ = 0, clock_ = 0, lastPowerWarning_ = -1e9;
    double reserve_ = kReserveCap;
    float requested_ = 0, met_ = 1, consumption_ = 0;
    std::vector<ScriptEvent> pendingShort_;
    std::vector<VehicleJob> jobs_;
    std::vector<std::pair<int, int>> finished_;
    std::map<int, float> officers_;
    std::map<int, bool> yardRunning_;
    int orbit_ = 150, officersDown_ = 0;
    double life_ = kLifeCap;
    float lifeDraw_ = 0; // power the hub put into life support last tick
    double lifeProduced_ = 0, lifeUsed_ = 0;
    float lifeMet_ = 1;
    int dead_ = 0;
    std::vector<std::pair<int, double>> walking_; // people on their way to the hub's pool, seconds left
    bool transports_ = false;
    std::vector<Ride> rides_;
    int nextRide_ = 1;
    void addRides(int from, int to, int people, bool toPool, bool listed, double wait = 0);
    const ColonyStructure* pad() const;
    ColonyStructure* structureById(int id);
    double lastCrewWarning_ = -1e9;
    int nextId_ = 1;
    std::function<DockPoints(const std::string&)> docks_;
    mutable std::map<std::string, DockPoints> dockCache_;
    const DockPoints& dockOf(const ColonyStructure& s) const;
    std::map<int, int> dockFree_; // each building's dock: the bee tick it's next free
    int beeClock_ = 0;
    // A vehicle's turn at a building's dock, taking so many ticks: how long
    // until it's through (its wait for the dock included).
    int dockTurn(int structure, int ticks);
    bool free_ = false;
    bool beesModelled_ = false;
    std::vector<Bee> bees_;
    int nextBee_ = 1, nextPod_ = 1;
    std::vector<Pod> pods_;
    // Each mine, processor and storage facility's worker bee (its id; 0
    // none), cargo bee, store of dug ore (a mine's) and whether a worker
    // bee has been asked of the yard.
    struct Depot {
        int worker = 0, cargo = 0;
        std::array<int64_t, kMaterials> dug{};
        bool asked = false;
    };
    std::map<int, Depot> depots_;
    int shipClock_ = 0;
};

} // namespace reluva::newworlds
