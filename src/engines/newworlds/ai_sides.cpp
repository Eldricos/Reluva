// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Reluva contributors

#include "engines/newworlds/ai_sides.h"

#include "engines/newworlds/levels.h"
#include "engines/newworlds/placement.h"
#include "engines/newworlds/start_data.h"

#include <algorithm>
#include <tuple>
#include <cmath>
#include <set>

namespace reluva::newworlds {

namespace {

// The natives' buildings that make units as a yard does: the Taubat hangar
// (115), K2's rebels' hangar (143), side 4's 151 and the Metar portal (201).
bool isFactory(int type) { return type == 115 || type == 143 || type == 151 || type == 201; }

// How long a native factory is busy with one unit (exe.md#ai-colonies):
// its build value in ticks at full speed, a tick to finish, then the roll
// out through its dock (two clear checks, three times the lock-to-release
// leg, the release-to-departure leg, each at least 10 ticks at the unit's
// dock speed), before it takes the next request. A Metar portal gives one
// every 8-15 s, a Taubat hangar one every 8.7 s.
double factorySeconds(int factory, int unit) {
    const CombatType* t = combatType(unit);
    const bool metar = t && t->race == 'M';
    const bool native = metar || (t && t->race == 'T') || (unit >= 139 && unit <= 141);
    const int build = metar ? 16 : native ? 64 : 640;
    const int speed = unit == 99 || unit == 203                                      ? 200
                      : unit == 48 || unit == 105 || unit == 129 || unit == 205 ? 150
                                                                                     : 100;
    const auto [lock, depart] = factory == 201   ? std::pair{7838, 5373}
                                : factory == 151 ? std::pair{3224, 4634}
                                                 : std::pair{2530, 3200};
    const int ticks = build + 1 + 2 + 3 * std::max(10, lock / speed) + std::max(10, depart / speed);
    return ticks / 20.0;
}

} // namespace

namespace {

constexpr float kBlock = 16384;
// Placeholders (not found in the executable): how far guards go after
// what comes near their spot, and how near a waypoint counts as reached.
constexpr float kArrived = 0.6f * kBlock;
// Units placed on one spot stand this far apart, and this far out from a
// building there.
constexpr float kSpacing = 2500;
constexpr float kHubClear = 12000;
// A placeholder: how long a building the economy doesn't know (defences,
// shield generators) takes to go up.
constexpr double kOtherBuildSeconds = 40;

std::pair<float, float> worldAt(int a, int b) { return {float(b) / 8 * kBlock, float(a) / 8 * kBlock}; }

} // namespace

bool isBee(const CombatType& t) { return std::string_view(t.name).find("bee") != std::string_view::npos; }

bool isMainHub(int type) {
    return type == 23 || type == 28 || type == 43 || type == 102 || type == 109 || type == 110;
}

bool joinsRallyGroup(const CombatType& t) {
    return !t.building && !isBee(t) && std::string_view(t.name) != "unarmed-apc";
}

std::pair<float, float> rallySpot(float hubX, float hubY, int n) {
    const float cx = std::floor(hubX / kBlock) * kBlock, cy = std::floor(hubY / kBlock) * kBlock;
    n %= 16;
    return {cx + 2560 + 4096 * float(n % 4), cy + 2560 + 4096 * float(n / 4)};
}

std::pair<float, float> parkingSpot(int n, float x, float y, bool building) {
    const float clear = building ? kHubClear : 0;
    if (n == 0 && clear == 0)
        return {x, y};
    const float ring = float(n / 8), angle = float(n % 8) * 0.785f + ring * 0.4f;
    const float r = clear + (ring + (clear > 0 ? 0 : 1)) * kSpacing;
    return {x + std::cos(angle) * r, y + std::sin(angle) * r};
}

namespace {
bool gCadet = false;
} // namespace

void setCadetMode(bool on) { gCadet = on; }
bool cadetMode() { return gCadet; }

int spawnObject(World& world, int slot, char race, const CombatType& type, float x, float y, const std::string& label,
                const std::string& model, bool player) {
    UnitKind kind{type.name, model.empty() ? type.model : model,
                  type.building ? 0 : kPaceSpeed * type.pace};
    const int id = world.addUnit(race, kind, x, y, 3.1416f / 2);
    Unit& u = *world.unit(id);
    u.slot = slot;
    u.type = type.id;
    u.name = type.name;
    u.label = label;
    u.building = type.building;
    const float tough = !player             ? 1.0f
                        : gCadet            ? 2.0f
                        : race == 'F'       ? 1.2f
                        : race == 'R'       ? 1.3f
                                            : 1.0f;
    u.hp = u.maxHp = std::floor(float(std::max(1, type.hp)) * tough); // (the game truncates)
    u.weapon = type.weapon;
    u.ai = !player;
    u.flies = std::string_view(type.name) == "bird-of-prey";
    u.cloaks = type.cloaks;
    return id;
}

const CombatType* typeNamed(std::string_view name, char race, bool buildingsOnly) {
    // The types with each name, by id (built once).
    static const std::map<std::string_view, std::vector<const CombatType*>, std::less<>> byName = [] {
        std::map<std::string_view, std::vector<const CombatType*>, std::less<>> out;
        for (int id = 1; id < 330; ++id)
            if (const CombatType* t = combatType(id))
                out[t->name].push_back(t);
        return out;
    }();
    const auto it = byName.find(name);
    if (it == byName.end())
        return nullptr;
    const CombatType* first = nullptr;
    for (const CombatType* t : it->second)
        if (t->building || !buildingsOnly) {
            if (t->race == race)
                return t;
            if (!first)
                first = t;
        }
    return first;
}

const VehicleType* orderOnlyVehicle(int type) {
    // Stock: dilithium, polycomp, thermoconc, trans al, nitrium, duranium.
    static const std::vector<VehicleType> kDevices = {
        {224, 'F', "ANOMALY DEVICE", Stock{5, 4, 0, 0, 0, 20}, {}, 128},
        {225, 'R', "ANOMALY DEVICE", Stock{5, 4, 0, 0, 0, 20}, {}, 128},
        {226, 'K', "ANOMALY DEVICE", Stock{5, 4, 0, 0, 0, 20}, {}, 128},
        {227, 'R', "ALIEN DEVICE", Stock{}, {}, 16},
    };
    for (const VehicleType& v : kDevices)
        if (v.type == type)
            return &v;
    return nullptr;
}

std::vector<std::string> scriptNames(const Unit& u, int playerSlot) {
    std::vector<std::string> out;
    const bool colony = u.side == 'F' || u.side == 'K' || u.side == 'R' || u.side == 'V';
    if (u.slot == playerSlot || !colony) {
        out.push_back(u.name);
    } else {
        const std::string race = raceName(u.side);
        out.push_back(race + "-" + u.name);
        if (u.name == "colony-hub")
            out.push_back(race + "-hub");
    }
    if (!u.label.empty() && u.label != out[0])
        out.push_back(u.label);
    return out;
}

AiSides::AiSides(std::string_view mission, int playerSlot, World& world)
    : mission_(mission), player_(playerSlot), world_(world) {
    for (const SideInfo* s : slotsOf(mission))
        races_[s->slot] = s->race;
}

char AiSides::raceOf(int slot) const {
    auto it = races_.find(slot);
    return it == races_.end() ? '?' : it->second;
}

std::vector<int> AiSides::slotsNamed(const std::string& side) const {
    std::vector<int> out;
    for (const auto& [slot, race] : races_)
        if (side == raceName(race))
            out.push_back(slot);
    if (out.empty())
        for (const auto& [slot, race] : races_)
            for (const SidePlacement* p : sidePlacements(mission_, slot))
                if (const CombatType* t = combatType(p->type); t && side == raceName(t->race)) {
                    out.push_back(slot);
                    break;
                }
    return out;
}

std::pair<float, float> AiSides::openSpot(float x, float y) const { return world_.openSpot(x, y); }

void AiSides::start() {
    for (const SideInfo* s : slotsOf(mission_))
        for (int to = 0; to < 7; ++to)
            world_.setStance(s->slot, to, s->posture[size_t(to)]);
    // The other colonies' economies, with the mission's stock (else the
    // game's default) and their officers.
    for (const auto& [slot, race] : races_)
        if (slot != player_ && slot <= 2 && (race == 'F' || race == 'K' || race == 'R')) {
            Stock stock{200, 1000, 1000, 1000, 5000, 5000};
            startStock(mission_, race, stock);
            Colony& c = colonies_[slot];
            c.economy = std::make_unique<Economy>(race, stock, 50, ground_);
            c.economy->setFree(true);
            c.economy->setOfficers(int(officers(mission_, slot).size()));
        }
    for (const auto& [slot, race] : races_) {
        if (slot == player_)
            continue;
        std::map<std::pair<int, int>, int> onSpot;
        std::set<std::pair<int, int>> built; // spots a building stands on
        for (const SidePlacement* p : sidePlacements(mission_, slot))
            if (const CombatType* t = combatType(p->type); t && t->building)
                built.insert({p->a, p->b});
        auto colony = colonies_.find(slot);
        // Where the side's fighting vehicles line up (exe.md#starting-
        // colonies): its hub's block, once the hub is placed; before that
        // (or with none) the map's middle.
        const float midX = float(world_.terrain().width) * kTile / 2, midY = float(world_.terrain().height) * kTile / 2;
        float hubX = midX, hubY = midY;
        for (const SidePlacement* p : sidePlacements(mission_, slot)) {
            const CombatType* t = combatType(p->type);
            if (t && t->building && isMainHub(t->id) && hubX == midX && hubY == midY)
                std::tie(hubX, hubY) = worldAt(p->a, p->b);
            // Without a model only what the scripts name, and shield
            // generators (their fields count), are placed (unseen). Bees
            // stay in the hub.
            // Unarmed APCs too, once the hub's placed (its transports).
            if (t && std::string_view(t->name) == "unarmed-apc" && !(hubX == midX && hubY == midY))
                continue;
            if (!t || p->forWhom == 1 || isBee(*t) ||
                (!*t->model && !*p->model && !*p->label && std::string_view(t->name) != "shield-generator"))
                continue;
            auto [x, y] = worldAt(p->a, p->b);
            if (t->building) // on its footprint's middle (exe.md#building-placement)
                std::tie(x, y) = snapped(footprintOf(t->id), x, y);
            if (!t->building && joinsRallyGroup(*t) && rallied_[slot] < 16) {
                // The first 16 on the rally group's places in the hub's
                // block, whatever spot the mission gives.
                std::tie(x, y) = rallySpot(hubX, hubY, rallied_[slot]++);
                std::tie(x, y) = openSpot(x, y);
            } else if (!t->building) {
                // The rest round their own spot (the game drives them in
                // from there), ring by ring (round a building standing
                // there).
                std::tie(x, y) = parkingSpot(onSpot[{p->a, p->b}]++, x, y, built.count({p->a, p->b}) > 0);
                std::tie(x, y) = openSpot(x, y);
            }
            const int id = spawnObject(world_, slot, race, *t, x, y, p->label, p->model);
            if (colony != colonies_.end() && t->building && buildingType(t->name))
                colony->second.structureOf[id] = colony->second.economy->addBuilt(t->name, x, y);
        }
        // Each building's best officer at its speciality.
        if (colony != colonies_.end()) {
            std::map<int, float> best;
            for (const Officer* o : officers(mission_, slot))
                for (const auto& [id, structure] : colony->second.structureOf) {
                    const Unit* u = world_.unit(id);
                    const BuildingType* b = u ? buildingType(u->name) : nullptr;
                    if (u && b && u->type == o->building && b->speciality == o->speciality)
                        best[structure] = std::max(best[structure], o->skill);
                }
            for (const auto& [structure, skill] : best)
                colony->second.economy->setOfficer(structure, skill);
        }
    }
    // Every order starts armed (exe.md#ai-colonies): the unnamed ones, the
    // named builds, patrols and sweeps (on their delay), and named orders
    // in mode 3, which go at once; a named guard or assault in another
    // mode waits for its message.
    for (const NpcOrder* o : npcOrders(mission_))
        if ((!*o->name || o->task == 9 || o->task <= 3 || o->mode == 3) && (o->slot != player_ || o->human))
            queue(*o);
}

bool AiSides::payShot(const Unit& turret, float power) {
    auto c = colonies_.find(turret.slot);
    if (c == colonies_.end())
        return true;
    auto it = c->second.structureOf.find(turret.id);
    return it == c->second.structureOf.end() || c->second.economy->spend(it->second, power);
}

const Economy* AiSides::colony(int slot) const {
    auto it = colonies_.find(slot);
    return it == colonies_.end() ? nullptr : it->second.economy.get();
}

std::pair<float, float> AiSides::wayPoint(const NpcOrder& o, int leg) const {
    if (o.ways == 0)
        return {-1, -1};
    const int i = std::clamp(leg, 0, o.ways - 1);
    return worldAt(o.way[i][0], o.way[i][1]);
}

void AiSides::queue(const NpcOrder& o) {
    running_.push_back(Running{&o, {}});
    if (ready(running_.back()))
        fire(running_.back());
}

// Whether an order may start now (exe.md#ai-colonies): its delay passed
// (patrols, sweeps and builds; guards and assaults have none); a sweep
// (task 2, but for modes 3, 5 and 6) only with an enemy within 50000 of
// its last waypoint; an assault only while its target side has anything
// left; task 10 never.
bool AiSides::ready(const Running& r) const {
    const NpcOrder& o = *r.order;
    if (o.task == 10)
        return false;
    const bool quick = o.mode == 3 || o.mode == 5 || o.mode == 6;
    if ((o.task <= 3 || o.task == 9) && clock_ < o.delay * kScriptSecond && !(quick && o.task <= 2) &&
        !(r.seen && o.task <= 2))
        return false;
    if (o.task == 2 && !quick && o.ways > 0) {
        const auto [lx, ly] = wayPoint(o, o.ways - 1);
        return std::any_of(world_.units().begin(), world_.units().end(), [&](const Unit& v) {
            return v.hp > 0 && v.built >= 1 && !v.cloaked && world_.stance(o.slot, v.slot) == 3 &&
                   std::abs(v.x - lx) <= 50000 && std::abs(v.y - ly) <= 50000;
        });
    }
    if (o.task >= 5 && o.task <= 8)
        return std::any_of(world_.units().begin(), world_.units().end(),
                           [&](const Unit& v) { return v.slot == o.task - 5 && v.hp > 0; });
    return true;
}

// Ready again for a new group: after a sweep's pass, or a group lost (up to
// three groups in all, but for modes 3, 5 and 6; an assault waits for a
// new message instead).
void AiSides::rearm(Running& r) {
    const NpcOrder& o = *r.order;
    const bool quick = o.mode == 3 || o.mode == 5 || o.mode == 6;
    const bool lost = r.units.empty();
    if (lost && (quick || ++r.groups >= 3 || (o.task >= 5 && o.task <= 8))) {
        r.done = true;
        return;
    }
    const int groups = r.groups;
    r = Running{r.order, {}};
    r.groups = groups;
}

// An order's time has come: a build tries its spot; a group gathers its
// units (beamed in, the side's own, or asked of its yards).
void AiSides::fire(Running& r) {
    r.started = true;
    if (r.order->task == 9 || r.order->task == 3) {
        build(r);
        return;
    }
    const NpcOrder& o = *r.order;
    const CombatType* t = combatType(o.type);
    if (!t || t->building) {
        r.done = true;
        return;
    }
    if (o.spawn[0] >= 0) {
        const auto [x, y] = worldAt(o.spawn[0], o.spawn[1]);
        const char race = raceOf(o.slot);
        for (int n = 0; n < o.count; ++n) {
            const float angle = float(n) * 1.1f;
            const auto [sx, sy] = openSpot(x + std::cos(angle) * kSpacing, y + std::sin(angle) * kSpacing);
            r.units.push_back(spawnObject(world_, o.slot, race == '?' ? t->race : race, *t, sx, sy, o.name));
            if (o.ways > 0) {
                const auto [wx, wy] = wayPoint(o, 0);
                world_.moveTo(r.units.back(), wx, wy);
            }
        }
    }
    gather(r);
}

// A group short of units: the side's idle ones of the type, nearest its
// first waypoint first; the rest asked of its vehicle yards (an AI
// colony's, free; never the player's). Once it has them all it sets off.
void AiSides::gather(Running& r) {
    const NpcOrder& o = *r.order;
    r.units.erase(std::remove_if(r.units.begin(), r.units.end(), [&](int id) { return !world_.unit(id); }),
                  r.units.end());
    if (r.gathered)
        return;
    std::set<int> busy;
    for (const Running& other : running_)
        if (!other.done)
            busy.insert(other.units.begin(), other.units.end());
    const auto [wx, wy] = wayPoint(o, 0);
    std::vector<std::pair<float, int>> idle;
    for (const Unit& u : world_.units())
        if (u.slot == o.slot && u.type == o.type && !u.building && !busy.count(u.id))
            idle.push_back({wx < 0 ? 0 : std::hypot(u.x - wx, u.y - wy), u.id});
    std::sort(idle.begin(), idle.end());
    // Each unit sets off on the first leg as soon as it's the group's; the
    // later legs wait for all of them.
    for (size_t i = 0; i < idle.size() && int(r.units.size()) + r.awaited < o.count; ++i) {
        r.units.push_back(idle[i].second);
        if (wx >= 0 && o.ways > 0) {
            const float angle = float(r.units.size()) * 2.4f, off = float(r.units.size() > 1) * kSpacing;
            world_.moveTo(idle[i].second, wx + std::cos(angle) * off, wy + std::sin(angle) * off);
        }
    }
    // Units already being made that no group is waiting for (a group gone
    // meanwhile) are this one's first; then new ones are asked for: of an
    // AI colony's yards, or of a native side's factories.
    for (int spare = outstanding(o.slot, o.type); spare > 0 && int(r.units.size()) + r.awaited < o.count; --spare)
        ++r.awaited;
    auto colony = colonies_.find(o.slot);
    const VehicleType* v = vehicleType(o.type);
    while (colony != colonies_.end() && v && int(r.units.size()) + r.awaited < o.count &&
           colony->second.economy->orderVehicle(*v))
        ++r.awaited;
    const bool factory = colony == colonies_.end() && std::any_of(world_.units().begin(), world_.units().end(), [&](const Unit& u) {
        return u.slot == o.slot && u.building && isFactory(u.type);
    });
    for (; factory && int(r.units.size()) + r.awaited < o.count; ++r.awaited)
        factoryQueue_[o.slot].push_back(o.type);
    // The player's side: a device only an order makes, of the player's
    // yards (paid for as the player's own).
    while (o.slot == player_ && playerYards_ && orderOnlyVehicle(o.type) && int(r.units.size()) + r.awaited < o.count &&
           playerYards_(o.type))
        ++r.awaited;
    if (int(r.units.size()) >= o.count) {
        r.gathered = true;
        steer(r);
    }
}

// A build order on its turn: once its spot is clear, the building goes up
// there as a site (an AI colony's in its economy). A named one keeps
// building it again whenever it's gone; the other builds once.
void AiSides::build(Running& r) {
    const NpcOrder& o = *r.order;
    const CombatType* t = combatType(o.type);
    if (!t || !t->building || o.ways == 0) {
        r.done = true;
        return;
    }
    if (r.site && world_.unit(r.site))
        return;
    // Its spot: the middle of its waypoint's tile, tested as the player's
    // hologram is (exe.md#building-placement); not clear, it waits for its
    // next turn (no other spot is tried).
    const auto [wx, wy] = wayPoint(o, 0);
    const float tx = (std::floor(wx / kTile) + 0.5f) * kTile, ty = (std::floor(wy / kTile) + 0.5f) * kTile;
    // Its own building already there (placed at the start): kept.
    if (const Unit* u = world_.unit(placementBlocker(world_, o.type, tx, ty)); u && u->slot == o.slot && u->type == o.type) {
        r.site = u->id;
        if (o.task == 3)
            r.done = true;
        return;
    }
    const auto seen = [&](int bx, int by) { return !seen_ || seen_(o.slot, bx, by); };
    if (placementTest(world_, o.type, tx, ty, o.slot, seen) != PlaceFail::None)
        return;
    const auto [x, y] = snapped(footprintOf(o.type), tx, ty);
    int structure = 0;
    auto colony = colonies_.find(o.slot);
    const BuildingType* b = buildingType(t->name);
    if (colony != colonies_.end() && b) {
        structure = colony->second.economy->order(t->name, x, y, int(x / kTile), int(y / kTile));
        if (!structure)
            return; // (a mining station off its deposit) tried again on its next turn
    }
    const char race = raceOf(o.slot);
    r.site = spawnObject(world_, o.slot, race == '?' ? t->race : race, *t, x, y, {});
    world_.unit(r.site)->built = 0;
    if (structure)
        colony->second.structureOf[r.site] = structure;
    else
        nativeSites_[r.site] = 0;
    if (o.task == 3)
        r.done = true;
}

// The AI colonies' economies moved on: sites going up, their buildings'
// health, the vehicles their yards finish (each to the order that asked
// for one of its type, oldest first).
void AiSides::colonies(double seconds) {
    factories(seconds);
    for (auto& [id, put] : nativeSites_) {
        put += seconds;
        if (Unit* u = world_.unit(id))
            u->built = float(std::min(1.0, put / kOtherBuildSeconds));
    }
    for (auto& [slot, c] : colonies_) {
        for (const auto& [id, structure] : c.structureOf)
            if (const Unit* u = world_.unit(id))
                c.economy->setHealth(structure, u->hp / u->maxHp);
        for (double left = seconds; left > 1e-9; left -= 0.25)
            c.economy->advance(std::min(left, 0.25));
        for (const ColonyStructure& s : c.economy->structures())
            for (const auto& [id, structure] : c.structureOf)
                if (structure == s.id)
                    if (Unit* u = world_.unit(id)) {
                        u->built = s.progress;
                        u->efficiency = c.economy->efficiency(s);
                    }
        for (const auto& [type, yard] : c.economy->takeVehicles()) {
            const CombatType* t = combatType(type);
            if (!t)
                continue;
            float x = 0, y = 0;
            for (const ColonyStructure& s : c.economy->structures())
                if (s.id == yard)
                    x = s.x, y = s.y - 1.2f * kTile;
            const auto [px, py] = openSpot(x, y);
            const int id = spawnObject(world_, slot, raceOf(slot), *t, px, py, {});
            // One no order asked for (a group gone meanwhile) parks at the
            // side's rally point, armed.
            if (!deliver(slot, type, id) && t->weapon.kind != WeaponKind::None)
                for (const ColonyStructure& s : c.economy->structures())
                    if (s.type == "colony-hub") {
                        const auto [rx, ry] = rallySpot(s.x, s.y, rallied_[slot]++);
                        world_.rally(id, rx, ry);
                        break;
                    }
        }
    }
}

// A unit a side's yard or factory finished: to the oldest group waiting
// for one of its type.
bool AiSides::deliver(int slot, int type, int id) {
    for (Running& r : running_)
        if (!r.done && r.awaited > 0 && r.order->slot == slot && r.order->type == type) {
            --r.awaited;
            r.units.push_back(id);
            return true;
        }
    return false;
}

// Units of a type a side has being made (or asked of its factories) that
// no group is waiting for any more.
int AiSides::outstanding(int slot, int type) const {
    int made = 0;
    if (auto c = colonies_.find(slot); c != colonies_.end()) {
        for (const Economy::VehicleJob& j : c->second.economy->vehicleJobs())
            made += j.type == type;
    } else {
        if (auto q = factoryQueue_.find(slot); q != factoryQueue_.end())
            made += int(std::count(q->second.begin(), q->second.end(), type));
        for (const FactoryJob& j : factoryJobs_)
            made += j.slot == slot && j.type == type;
    }
    for (const Running& r : running_)
        if (!r.done && r.order->slot == slot && r.order->type == type)
            made -= r.awaited;
    return std::max(0, made);
}

// The natives' factories: each idle one takes its side's oldest request,
// builds it and rolls it out (factorySeconds), the unit out at the end. A
// factory lost gives its unit back to the queue.
void AiSides::factories(double seconds) {
    for (auto it = factoryJobs_.begin(); it != factoryJobs_.end();) {
        const Unit* f = world_.unit(it->factory);
        if (!f || f->hp <= 0) {
            factoryQueue_[it->slot].push_front(it->type);
            it = factoryJobs_.erase(it);
            continue;
        }
        it->left -= seconds;
        if (it->left > 0) {
            ++it;
            continue;
        }
        if (const CombatType* t = combatType(it->type)) {
            const auto [px, py] = openSpot(f->x, f->y - 1.2f * kTile);
            const int id = spawnObject(world_, it->slot, raceOf(it->slot), *t, px, py, {});
            deliver(it->slot, it->type, id);
        }
        it = factoryJobs_.erase(it);
    }
    for (const Unit& f : world_.units()) {
        auto q = factoryQueue_.find(f.slot);
        if (q == factoryQueue_.end() || q->second.empty() || !f.building || f.built < 1 || f.hp <= 0 ||
            !isFactory(f.type) ||
            std::any_of(factoryJobs_.begin(), factoryJobs_.end(), [&](const FactoryJob& j) { return j.factory == f.id; }))
            continue;
        factoryJobs_.push_back({f.id, f.slot, q->second.front(), factorySeconds(f.type, q->second.front())});
        q->second.pop_front();
    }
}

bool AiSides::order(const std::string& side, const std::string& text) {
    // Several orders at once ("abort a b c", or "a b"): each in turn.
    std::string rest = text;
    const bool abort = rest.rfind("abort ", 0) == 0;
    if (abort)
        rest = rest.substr(6);
    if (rest.find(' ') != std::string::npos) {
        bool all = true;
        for (size_t at = 0; at < rest.size();) {
            const size_t end = std::min(rest.find(' ', at), rest.size());
            if (end > at)
                all = order(side, (abort ? "abort " : "") + rest.substr(at, end - at)) && all;
            at = end + 1;
        }
        return all;
    }
    std::string name = rest;
    const std::vector<int> slots = slotsNamed(side);
    const NpcOrder* found = nullptr;
    for (const NpcOrder* o : npcOrders(mission_))
        if (name == o->name && (!found || std::count(slots.begin(), slots.end(), o->slot)))
            found = o;
    if (!found)
        return false;
    // An abort ends a patrol's or sweep's group, or disarms a build; a guard
    // or an assault doesn't read its message: any is a trigger to it.
    if (abort && !(found->task == 4 || (found->task >= 5 && found->task <= 8))) {
        for (Running& r : running_)
            if (r.order == found)
                r.done = true;
        return true;
    }
    // An order already armed takes no second group from its message: a
    // patrol or sweep not yet gone goes now, without waiting out its delay;
    // a group out carries on; a build stays as it is.
    for (Running& r : running_)
        if (r.order == found && !r.done) {
            r.seen = true;
            if (!r.started && ready(r))
                fire(r);
            return true;
        }
    queue(*found);
    return true;
}

void AiSides::stance(const std::string& side, const std::string& verb) {
    const int p = verb == "hostile" ? 3 : verb == "allied" ? 1 : 2;
    for (int slot : slotsNamed(side))
        world_.setStance(slot, player_, p);
}

// An order's units each second (exe.md#ai-colonies): each to its leg's
// waypoint, unless it's fighting; when they're all there, the next leg. At
// the end a looping patrol (task 1) goes round again, a sweep (2) lets its
// units go and waits to sweep again, a patrol or sweep in mode 5 or 6
// becomes a guard there, and an assault (5-8) goes for its target side,
// nearest first, each unit dealing first with what attacks it. A guard
// (4) goes to its spot and holds it (mode 4: protecting all its side).
void AiSides::steer(Running& r) {
    const NpcOrder& o = *r.order;
    r.units.erase(std::remove_if(r.units.begin(), r.units.end(), [&](int id) { return !world_.unit(id); }),
                  r.units.end());
    // A group wiped out: another tried, up to three.
    if (r.units.empty()) {
        rearm(r);
        return;
    }
    if (o.ways == 0 || r.guarding)
        return;
    auto spread = [&](int n, float x, float y) {
        const float angle = float(n) * 2.4f, off = float(n > 1) * kSpacing;
        return std::pair{x + std::cos(angle) * off, y + std::sin(angle) * off};
    };
    // A guard holds its spot: mode 4 protecting every object of its side
    // (after their live attackers, anywhere), modes 2 and 3 firing at what
    // comes in range.
    if (o.task == 4) {
        const auto [wx, wy] = wayPoint(o, 0);
        int n = 0;
        for (int id : r.units) {
            const auto [tx, ty] = spread(++n, wx, wy);
            world_.moveTo(id, tx, ty);
            if (o.mode == 4)
                world_.protect({id});
        }
        r.guarding = true;
        return;
    }
    const bool patrol = o.task == 1 || o.task == 2;
    auto [wx, wy] = wayPoint(o, r.leg);
    float cx = 0, cy = 0;
    bool allThere = true;
    for (int id : r.units) {
        const Unit& u = *world_.unit(id);
        cx += u.x / float(r.units.size()), cy += u.y / float(r.units.size());
        // There, or stopped short (no way on): only those still driving
        // or fighting hold the group up.
        if (std::hypot(u.x - wx, u.y - wy) > kArrived * (1 + float(r.units.size()) / 4) && (u.moving() || u.firing))
            allThere = false;
    }
    // (A leg's first seconds don't count: its units are only setting off.)
    if (allThere && !r.hunting && r.legClock >= 2) {
        r.legClock = 0;
        if (r.leg + 1 < o.ways) {
            ++r.leg;
        } else if (o.slot == player_) {
            r.done = true; // the player's units, placed, are theirs again
            return;
        } else if (patrol && (o.mode == 5 || o.mode == 6)) {
            // A guard there: mode 5 of its side's objects within 30000 of
            // each unit as it gets there, mode 6 of itself.
            for (int id : r.units) {
                const Unit& u = *world_.unit(id);
                std::vector<int> them;
                if (o.mode == 6)
                    them = {id};
                else
                    for (const Unit& v : world_.units())
                        if (v.slot == o.slot && v.hp > 0 && std::abs(v.x - u.x) <= 30000 && std::abs(v.y - u.y) <= 30000)
                            them.push_back(v.id);
                world_.protect({id}, them);
            }
            r.guarding = true;
            return;
        } else if (o.task == 1) {
            if (o.ways == 1) { // one waypoint: there it stays
                r.guarding = true;
                return;
            }
            r.leg = 0;
        } else if (o.task == 2) {
            rearm(r); // its units let go; it sweeps again when it's ready
            return;
        } else {
            r.hunting = true;
        }
        std::tie(wx, wy) = wayPoint(o, r.leg);
    }
    // An assault's prey: the nearest object of its target side.
    const Unit* prey = nullptr;
    if (r.hunting) {
        float best = 1e30f;
        for (const Unit& v : world_.units())
            if (v.slot == o.task - 5 && v.hp > 0 && !v.cloaked && v.built >= 1)
                if (const float d = std::hypot(v.x - cx, v.y - cy); d < best)
                    best = d, prey = &v;
        if (!prey) { // nothing left of it: over
            r.done = true;
            return;
        }
    }
    int n = 0;
    for (int id : r.units) {
        Unit& u = *world_.unit(id);
        ++n;
        if (u.firing || (u.ordered && u.target))
            continue;
        if (r.hunting) {
            const Unit* a = world_.liveAttacker(u);
            world_.attack({id}, a ? a->id : prey->id);
            continue;
        }
        if (u.moving())
            continue;
        const auto [tx, ty] = spread(n, wx, wy);
        if (std::hypot(u.x - tx, u.y - ty) > kSpacing) {
            // No way there from here last time: none now either (the path
            // would come back empty again), so only what moveTo() does
            // besides.
            const std::array<float, 4> from{u.x, u.y, tx, ty};
            if (auto it = noWay_.find(id); it != noWay_.end() && it->second == from && !u.flies && !u.building) {
                u.ordered = false;
                u.target = 0;
                u.firing = false;
                continue;
            }
            world_.moveTo(id, tx, ty);
            if (!u.moving() && !u.flies && !u.building)
                noWay_[id] = from;
            else
                noWay_.erase(id);
        }
    }
}

void AiSides::advance(double seconds) {
    clock_ += seconds;
    // The game's ticks counted (whole ones; the rest carried), for "each
    // second" below: a clock's rounding never skips or doubles one.
    tickRest_ += seconds * 20;
    const int ticks = int(std::floor(tickRest_ + 1e-6));
    tickRest_ -= ticks;
    const int before = ticks_;
    ticks_ += ticks;
    colonies(seconds);
    for (Running& r : running_)
        if (r.started && !r.done)
            r.clock += seconds, r.legClock += seconds;
    // Each second: orders whose time has come fire; builds try their spot
    // again; groups still short gather; the rest are steered.
    if (ticks_ / 20 != before / 20)
        for (size_t i = 0; i < running_.size(); ++i) {
            Running& r = running_[i];
            if (r.done)
                continue;
            if (!r.started) {
                if (ready(r))
                    fire(r);
            } else if (r.order->task == 9 || r.order->task == 3) {
                build(r);
            } else if (!r.gathered) {
                gather(r);
            } else {
                steer(r);
            }
        }
    running_.erase(std::remove_if(running_.begin(), running_.end(), [](const Running& r) { return r.done; }),
                   running_.end());
}

std::vector<ScriptEvent> AiSides::observe(const std::vector<WorldEvent>& events) {
    std::vector<ScriptEvent> out;
    for (const WorldEvent& e : events) {
        if (e.kind != WorldEvent::Destroyed)
            continue;
        nativeSites_.erase(e.what.id);
        if (auto c = colonies_.find(e.what.slot); c != colonies_.end())
            if (auto it = c->second.structureOf.find(e.what.id); it != c->second.structureOf.end()) {
                c->second.economy->remove(it->second);
                c->second.structureOf.erase(it);
            }
        const int a = int(e.what.y / kBlock * 8), b = int(e.what.x / kBlock * 8);
        for (const std::string& n : scriptNames(e.what, player_))
            out.push_back({"destroyed", {n}, a, b});
        if (e.what.name == "colony-hub" && e.what.slot != player_) {
            out.push_back({"hub-destroyed", {raceName(e.what.side)}, a, b});
            // A joint mission's rival: its own mission fails with its hub.
            const std::vector<char> sides = missionSides(mission_);
            if (std::find(sides.begin(), sides.end(), e.what.side) != sides.end())
                out.push_back({"side-failed", {raceName(e.what.side)}, a, b});
        }
    }
    for (const WorldEvent& e : events)
        if (e.kind == WorldEvent::Destroyed)
            noWay_.erase(e.what.id);
    // Nothing the counts come from changed (the units and their names,
    // sides, types, whether up and whether held): the same counts, so
    // none to tell, and the same colony sizes.
    const std::vector<Unit>& units = world_.units();
    bool same = observedValid_ && observed_.size() == units.size();
    for (size_t i = 0; same && i < units.size(); ++i) {
        const Unit& u = units[i];
        const Observed& o = observed_[i];
        same = o.id == u.id && o.slot == u.slot && o.type == u.type && o.side == u.side &&
               o.building == u.building && o.built == (u.built >= 1) && o.held == (u.heldBy >= 0) &&
               o.name == u.name && o.label == u.label;
    }
    if (!same) {
        observed_.resize(units.size());
        for (size_t i = 0; i < units.size(); ++i) {
            const Unit& u = units[i];
            Observed& o = observed_[i];
            o.id = u.id, o.slot = u.slot, o.type = u.type, o.side = u.side;
            o.building = u.building, o.built = u.built >= 1, o.held = u.heldBy >= 0;
            o.name = u.name, o.label = u.label;
        }
        observedValid_ = true;
    }
    std::map<std::string, int> counts;
    std::map<int, int> size, total; // total: with the buildings other sides hold
    if (same) {
        size = observedSize_, total = observedTotal_;
    } else {
    for (const Unit& u : world_.units())
        if (!(u.slot == player_ && u.building) && u.built >= 1) // (sites aren't counted)
            for (const std::string& n : scriptNames(u, player_))
                ++counts[n];
    // Each side's groups, by class (exe.md#group-counts): its small and
    // large buildings and its armed vehicles, standing and not held by
    // another side; and its colony's size, small + 3 x large, as a share
    // of what it was at the start.
    for (const Unit& u : world_.units()) {
        const CombatType* t = combatType(u.type);
        if (t && u.built >= 1 && u.heldBy >= 0)
            total[u.slot] += t->mask & 0x2 ? 1 : t->mask & 0x4 ? 3 : 0;
        if (!t || u.built < 1 || u.heldBy >= 0)
            continue;
        const char race = raceOf(u.slot) == '?' ? u.side : raceOf(u.slot);
        const std::string side = u.slot == player_ ? std::string() : std::string(raceName(race)) + "-";
        if (t->mask & 0x2)
            ++counts[side + "small-buildings"], size[u.slot] += 1;
        else if (t->mask & 0x4)
            ++counts[side + "large-buildings"], size[u.slot] += 3;
        else if (t->mask & 0x100)
            ++counts[side + "fighting-units"];
        total[u.slot] += t->mask & 0x2 ? 1 : t->mask & 0x4 ? 3 : 0;
    }
    observedSize_ = size, observedTotal_ = total;
    }
    if (colonyBase_.empty())
        colonyBase_ = size, colonyBase_[-1] = 0; // (taken once)
    for (const auto& [slot, base] : colonyBase_) {
        if (slot < 0 || base <= 0)
            continue;
        // colony-size: as the game's size monitors (kind 8), what the
        // side itself holds; colony-total (kind 9) counts its buildings
        // other sides have captured too.
        const char race = raceOf(slot);
        for (const auto& [kind, now, told] : {std::tuple{"colony-size", size[slot], &colonyShare_},
                                              std::tuple{"colony-total", total[slot], &colonyTotal_}}) {
            const int share = int(now * 100 / base);
            if (auto it = told->find(slot); it != told->end() && it->second == share)
                continue;
            (*told)[slot] = share;
            if (slot == player_)
                out.push_back({kind, {std::to_string(share)}});
            else if (race != '?')
                out.push_back({kind, {raceName(race), std::to_string(share)}});
        }
    }
    if (same) // (counts_ is what they came to)
        return out;
    for (const auto& [n, c] : counts)
        if (auto it = counts_.find(n); it == counts_.end() || it->second != c)
            out.push_back({"count", {n, std::to_string(c)}});
    for (const auto& [n, c] : counts_)
        if (!counts.count(n))
            out.push_back({"count", {n, "0"}});
    counts_ = std::move(counts);
    return out;
}

std::vector<AiSides::OrderStatus> AiSides::status() const {
    std::vector<OrderStatus> out;
    for (const Running& r : running_) {
        if (r.done)
            continue;
        int blocker = 0;
        std::string refusal;
        if ((r.order->task == 3 || r.order->task == 9) && r.order->ways > 0 && !r.site) {
            const auto [wx, wy] = wayPoint(*r.order, 0);
            const float tx = (std::floor(wx / kTile) + 0.5f) * kTile, ty = (std::floor(wy / kTile) + 0.5f) * kTile;
            blocker = placementBlocker(world_, r.order->type, tx, ty);
            const int slot = r.order->slot;
            const auto seen = [&](int bx, int by) { return !seen_ || seen_(slot, bx, by); };
            switch (placementTest(world_, r.order->type, tx, ty, slot, seen)) {
            case PlaceFail::Edge: refusal = "at the map's edge"; break;
            case PlaceFail::Ground: refusal = "not ground it can stand on"; break;
            case PlaceFail::Unseen: refusal = "not seen by its side"; break;
            case PlaceFail::Taken: refusal = "taken"; break;
            case PlaceFail::Units: refusal = "units in the way"; break;
            case PlaceFail::Deposit: refusal = "no ore"; break;
            case PlaceFail::None: break;
            }
        }
        out.push_back({r.order, r.started, r.gathered, r.hunting, r.guarding, int(r.units.size()), r.awaited, r.leg,
                       r.gathered ? r.legClock : r.clock, r.site, blocker, refusal, r.units});
    }
    return out;
}

std::vector<std::string> AiSides::running() const {
    std::vector<std::string> out;
    for (const Running& r : running_)
        if (!r.done)
            out.push_back(r.order->name);
    return out;
}

void AiSides::syncState(save::Serializer& s) {
    // The running orders, each by its place in the mission's list.
    const std::vector<const NpcOrder*> orders = npcOrders(mission_);
    const uint32_t n = s.count(running_.size());
    if (s.loading())
        running_.assign(n, Running{});
    for (Running& r : running_) {
        save::syncIndex(s, r.order, orders);
        s(r.units)(r.leg)(r.clock)(r.legClock)(r.started)(r.gathered)(r.hunting)(r.done)(r.awaited)(r.site);
        s(r.groups)(r.guarding)(r.seen);
    }
    // The other colonies' economies, made as the mission starts.
    if (s.count(colonies_.size()) != colonies_.size() && s.loading())
        s.fail("other colonies than the mission's");
    for (auto& [slot, colony] : colonies_) {
        int k = slot;
        s(k);
        if (s.loading() && k != slot)
            s.fail("other colonies than the mission's");
        s(colony.structureOf);
        colony.economy->syncState(s);
    }
    s(factoryQueue_)(factoryJobs_);
    s(races_)(nativeSites_)(rallied_)(colonyBase_)(colonyShare_)(colonyTotal_)(counts_)(clock_)(ticks_)(tickRest_);
    if (s.loading()) { // (made again from what's loaded)
        observedValid_ = false;
        noWay_.clear();
    }
}

} // namespace reluva::newworlds
